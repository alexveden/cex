#include "cex.h"

#ifndef CEX_MAX_JSON_DEPTH
#    define CEX_MAX_JSON_DEPTH 128
#endif

/// JSON Reader Namespace
#define __jr$

/// Creates new json reader container, kwargs... are optional see: jr_kw. json_reader is a
/// one pass, non-allocating parser.
#define jr$new(json_reader, content, len, kwargs...)                                               \
    _cex_json__reader__create((json_reader), (content), (len), &(jr_kw){ kwargs })

/// Gets last json reader error
#define jr$err(json_reader) (json_reader)->error

/*clang-format off*/

/// Returns ("format", err, args) text macro compatible with any printf() functions, used for
/// printing/formatting JSON debug message with reference to error type and line/col position of an
/// error.
/// Examples:
/// str.fmt(alloc, jr$err_fmt(&json_reader));
/// io.printf(jr$err_fmt(&json_reader));
/// io.fprintf(stderr, jr$err_fmt(&json_reader));
#define jr$err_fmt(json_reader)                                                                    \
    "JSON %s(%s) at line: %d col: %d\n", (json_reader)->error ? "Parsing Error " : "",             \
        (json_reader)->error ? (json_reader)->error : "OK", (json_reader)->_impl.lexer.line + 1,   \
        (json_reader)->_impl.lexer.col

/*clang-format on*/

/// Executes (try_expression) (must return Exc or Exception type), and sets last json_reader.error
/// on fail, after this does `goto goto_on_fail_label`. Prints tracebacks in unit test suites.
#define jr$egoto(json_reader, try_expression, goto_on_fail_label)                                  \
    if (!(json_reader)->error) {                                                                   \
        e$except_silent (err, try_expression) {                                                    \
            (json_reader)->error = err;                                                            \
            goto goto_on_fail_label;                                                               \
        }                                                                                          \
    }

/// Gets next `json_type` scope as a string slice (str_s), you may use another json_reade instance
/// to parse result of this function. Sets json_reader.error + returns (str_s){0} on failure.
#define jr$get_scope_str_s(json_reader, json_type)                                                 \
    _cex_json__reader__get_scope((json_reader), json_type)

/// Context specific iterator over json_reader scope:
/// jr$foreach(val, json_reader) - iterates over array scope items
/// jr$foreach(key, val, json_reader) - iterates over object scope key:value pairs
/// key and val - are typeof(str_s)
#define jr$foreach(...) _jr$foreach_impl(__VA_ARGS__, _jr$foreach_obj, _jr$foreach_arr)(__VA_ARGS__)

#define _jr$foreach_impl(_1, _2, _3, NAME, ...) NAME

#define _jr$foreach_arr(_val, json_reader)                                                         \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = _cex_json__reader__step_in((json_reader), JsonType__arr);           \
    }                                                                                              \
    for (str_s(_val) = { 0 };                                                                      \
         _cex_json__reader__next((json_reader)) ? ((_val) = ((json_reader)->val), 1) : 0;)

#define _jr$foreach_obj(_key, _val, json_reader)                                                   \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = _cex_json__reader__step_in((json_reader), JsonType__obj);           \
    }                                                                                              \
    for (str_s(_key) = { 0 }, (_val) = { 0 };                                                      \
         _cex_json__reader__next((json_reader))                                                    \
             ? ((_key) = ((json_reader)->key), (_val) = ((json_reader)->val), 1)                   \
             : 0;)

#define str$eq(str_s_slice, compare_to_literal)                                                    \
    ((str_s_slice).buf && (str_s_slice).len == sizeof(compare_to_literal) - 1 &&                   \
     memcmp((str_s_slice).buf, compare_to_literal, sizeof(compare_to_literal) - 1) == 0)

typedef enum JsonType_e
{
    JsonType__eos = -2, // end of scope (after _cex_json__reader__step_in())
    JsonType__err = -1, // data/integrity error
    JsonType__eof = 0,  // end of file reached
    JsonType__str,      // quoted string
    JsonType__obj,      // {key: value} object
    JsonType__arr,      // [ ... ] array
    JsonType__num,      // floating point num
    JsonType__bool,     // boolean
    JsonType__null,     // null (obj/arr)
    // -----------
    JsonType__cnt,
} JsonType_e;


typedef struct jr_kw
{
    bool strict_mode;
} jr_kw;

typedef struct jr_c
{
    str_s val;       // string value of the json item
    str_s key;       // associated key of the val (if inside object)
    JsonType_e type; // current json item type (also error, EOF, end-of-scope indication)
    Exc error;       // last iterator error

    struct
    {
        CexParser_c lexer;   // JSON lexer
        bool strict_mode;    // enforces JSON compliant spec
        bool has_items;      // flag is set when at least one item processed in scope
        CexTkn_e prev_token; // JSON previous token
        CexTkn_e curr_token; // JSON current token
        u32 scope_depth;
        u8 scope_stack[CEX_MAX_JSON_DEPTH];
    } _impl;

} jr_c;

/// JSON Writer jw$new() keyword arguments 
typedef struct jw_kw
{
    FILE* stream;
    sbuf_c buf;
    u32 indent;
} jw_kw;

/// JSON Writer container type
typedef struct jw_c
{
    FILE* stream;
    sbuf_c buf;
    Exc error;
    u32 indent;
    u32 indent_width;
    u32 scope_depth;
    u8 scope_stack[CEX_MAX_JSON_DEPTH];
} jw_c;

/// JSON Writer Namespace
#define __jw$

/// Creates new instance of json writer, non allocating serializer, with support of exporting to
/// FILE* or backing by string buffer sbuf_c
#define jw$new(json_writer, kwargs...)                                                             \
    _cex_json__writer__create((json_writer), &(jw_kw){ kwargs })

/// Checks if json writer has no errors
#define jw$validate(json_writer) _cex_json__writer__validate((json_writer))

/// Writes a new key (must be in jw$scope(jw, JsonType__obj))
#define jw$key(format, ...) _cex_json__writer__print_key(_jw$scope_var, format, ##__VA_ARGS__)

/// Writes a new value to the json scope (object or array), expects json compatible primitive
/// arguments. Use `jr$fmt` for customizable output.
#define jw$val(json_compatible_val)                                                                \
    ({                                                                                             \
        char* format = _Generic(                                                                   \
            json_compatible_val,                                                                   \
            u8: "%d",                                                                              \
            i8: "%d",                                                                              \
            i16: "%d",                                                                             \
            u16: "%d",                                                                             \
            i32: "%d",                                                                             \
            u32: "%u",                                                                             \
            i64: "%ld",                                                                            \
            u64: "%lu",                                                                            \
            f32: "%f",                                                                             \
            f64: "%f",                                                                             \
            char: "\"%c\"",                                                                        \
            _Bool: "%d",                                                                           \
            str_s: "\"%S\"",                                                                       \
            const char*: "\"%s\"",                                                                 \
            char*: "\"%s\""                                                                        \
        );                                                                                         \
        _cex_json__writer__print_item(_jw$scope_var, format, json_compatible_val);                  \
    })

#define _jw$scope_var _json_writer_macro_scope

/// Opens JSON scope, jsontype_arr_or_obj expects JsonType__obj or JsonType__arr
#define jw$scope(json_writer_ptr, jsontype_arr_or_obj)                                             \
    for (jw_c * _jw$scope_var                                                              \
             __attribute__((__cleanup__(_cex_json__writer__print_scope_exit))) =                   \
             _cex_json__writer__print_scope_enter((json_writer_ptr), jsontype_arr_or_obj, true),   \
             *cex$tmpname(jsonbuf_sentinel) = _jw$scope_var;                                        \
         cex$tmpname(jsonbuf_sentinel) && _jw$scope_var != NULL;                                    \
         cex$tmpname(jsonbuf_sentinel) = NULL)


/// Append any formatted string in the jw$scope, it's for low level printing
#define jw$fmt(format, ...) _cex_json__writer__print(_jw$scope_var, format, ##__VA_ARGS__)

// clang-format off
Exception _cex_json__reader__create(jr_c* it, char* content, usize content_len, jr_kw* kwargs);
Exception _cex_json__reader__step_in(jr_c* it, JsonType_e expected_type);
bool _cex_json__reader__next(jr_c* it);
str_s _cex_json__reader__get_scope(jr_c* it, JsonType_e scope_type);


void _cex_json__writer__print(jw_c* jw, char* format, ...);
void _cex_json__writer__print_item(jw_c* jw, char* format, ...);
void _cex_json__writer__print_key(jw_c* jw, char* format, ...);
void _cex_json__writer__print_scope_exit(jw_c** jwptr);
jw_c* _cex_json__writer__print_scope_enter(jw_c* jw, JsonType_e scope_type, bool should_indent);
Exception _cex_json__writer__create(jw_c* jw, jw_kw* kwargs);
Exception _cex_json__writer__validate(jw_c* jw);

