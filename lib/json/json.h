#include "cex.h"

#ifndef CEX_MAX_JSON_DEPTH
#    define CEX_MAX_JSON_DEPTH 128
#endif

#define jr$new(json_reader, content, len, kwargs...)                                               \
    _cex_json__reader__create((json_reader), (content), (len), &(json_reader_kw){ kwargs })


#define jr$foreach(...) _jr$foreach_impl(__VA_ARGS__, _jr$foreach_obj, _jr$foreach_arr)(__VA_ARGS__)

#define _jr$foreach_impl(_1, _2, _3, NAME, ...) NAME

#define _jr$foreach_arr(_val, json_reader)                                                         \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = _cex_json__reader__step_in((json_reader), JsonType__arr);                  \
    }                                                                                              \
    for (str_s(_val) = { 0 };                                                                      \
         _cex_json__reader__next((json_reader)) ? ((_val) = ((json_reader)->val), 1) : 0;)

#define _jr$foreach_obj(_key, _val, json_reader)                                                   \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = _cex_json__reader__step_in((json_reader), JsonType__obj);                  \
    }                                                                                              \
    for (str_s(_key) = { 0 }, (_val) = { 0 };                                                      \
         _cex_json__reader__next((json_reader))                                                           \
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


typedef struct json_reader_kw
{
    bool strict_mode;
} json_reader_kw;

typedef struct json_reader_c
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

} json_reader_c;

typedef struct json_writer_kw
{
    FILE* stream;
    sbuf_c buf;
    u32 indent;
} json_writer_kw;

typedef struct json_writer_c
{
    FILE* stream;
    sbuf_c buf;
    Exc error;
    u32 indent;
    u32 indent_width;
    u32 scope_depth;
    u8 scope_stack[CEX_MAX_JSON_DEPTH];
} json_writer_c;

#define jw$new(json_writer, kwargs...)                                             \
    _cex_json__writer__create((json_writer), &(json_writer_kw){ kwargs })                            \

#define jw$validate(json_writer) _cex_json__writer__validate((json_writer))

#define jw$key(format, ...) _cex_json__writer__print_key(jw$scope_var, format, ##__VA_ARGS__)

#define jw$val(json_compatible_val)                                                                \
    ({                                                                                             \
        char* format = _Generic(                                                                   \
            json_compatible_val,                                                                   \
            u8: "%d",                                                                             \
            i8: "%d",                                                                             \
            i16: "%d",                                                                             \
            u16: "%d",                                                                             \
            i32: "%d",                                                                             \
            u32: "%u",                                                                             \
            i64: "%ld",                                                                             \
            u64: "%lu",                                                                             \
            f32: "%f",                                                                             \
            f64: "%f",                                                                             \
            char: "\"%c\"",                                                                             \
            _Bool: "%d",                                                                           \
            str_s: "\"%S\"",                                                                       \
            const char*: "\"%s\"",                                                                        \
            char*: "\"%s\""                                                                        \
        );                                                                                         \
        _cex_json__writer__print_item(jw$scope_var, format, json_compatible_val);                  \
    })

#define jw$scope_var _json_writer_macro_scope

/// Opens JSON buffer scope (json_writer_ptr data is cleared out)
#define jw$scope(json_writer_ptr, jsontype_arr_or_obj)                                             \
    for (json_writer_c * jw$scope_var                                                              \
             __attribute__((__cleanup__(_cex_json__writer__print_scope_exit))) =                     \
             _cex_json__writer__print_scope_enter((json_writer_ptr), jsontype_arr_or_obj, true),     \
             *cex$tmpname(jsonbuf_sentinel) = jw$scope_var;                                        \
         cex$tmpname(jsonbuf_sentinel) && jw$scope_var != NULL;                                    \
         cex$tmpname(jsonbuf_sentinel) = NULL)


/// Append any formatted string, it's for low level printing (jw$scope)
#define jw$fmt(format, ...) _cex_json__writer__print(jw$scope_var, format, ##__VA_ARGS__)

// clang-format off
Exception _cex_json__reader__create(json_reader_c* it, char* content, usize content_len, json_reader_kw* kwargs);
Exception _cex_json__reader__step_in(json_reader_c* it, JsonType_e expected_type);
bool _cex_json__reader__next(json_reader_c* it);


void _cex_json__writer__print(json_writer_c* jw, char* format, ...);
void _cex_json__writer__print_item(json_writer_c* jw, char* format, ...);
void _cex_json__writer__print_key(json_writer_c* jw, char* format, ...);
void _cex_json__writer__print_scope_exit(json_writer_c** jwptr);
json_writer_c* _cex_json__writer__print_scope_enter(json_writer_c* jw, JsonType_e scope_type, bool should_indent);
Exception _cex_json__writer__create(json_writer_c* jw, json_writer_kw* kwargs);
Exception _cex_json__writer__validate(json_writer_c* jw);

