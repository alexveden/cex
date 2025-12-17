#pragma once

#if __has_include("cex.h")
#    include "cex.h"
#else
#error "./cex.h not found, check if exist or you should set compiler argument `-I.`"
#endif

#ifndef json$$struct
/// JSON Generator attribute, put it before your `typedef struct` to enable JSON code generation
#define json$$struct(...)
#endif

#ifndef json$$field
/// JSON field metadata attribute, used for adjusting json.gen. behavior for specific field
/// all field parameters are optional 
/// json$$field(.name = "json_name", .nullable = false, .optional = false, .skip = false )
/// .name = "json_name" = maps json specific name to C struct field
/// .nullable = allows strings or objects to be `null` in json, assigns NULL on deserialization
/// .optional = allows object keys absence in a JSON file
/// .skip = totally skip json serialization/deserialization of that field (initialize manually)
#define json$$field(...)
#endif

#ifndef CEX_MAX_JSON_DEPTH
#    define CEX_MAX_JSON_DEPTH 128
#endif

extern const struct _CEX_JsonError_struct
{
    Exc null_field;
    Exc missing_field;
    Exc unknown_field;
    Exc parsing;
    Exc wrong_type;
    Exc encoding;
} JsonError;

/// Creates new json reader container, kwargs... are optional see: jr_kw. json_reader is a
/// one pass, non-allocating parser.
#define json$rd_new(json_reader, content, len, kwargs...)                                          \
    json.rd.create((json_reader), (content), (len), &(json_rd_kw){ kwargs })

/// Gets last json reader error
#define json$rd_err(json_reader) (json_reader)->error

#define json$rd_is_type_compatible(json_reader, c_out_ptr_type)                                    \
    ((json_reader)->type == _Generic(                                                              \
                                (c_out_ptr_type),                                                  \
         u8*: JsonType__num,                                                                       \
         i8*: JsonType__num,                                                                       \
         i16*: JsonType__num,                                                                      \
         u16*: JsonType__num,                                                                      \
         i32*: JsonType__num,                                                                      \
         u32*: JsonType__num,                                                                      \
         i64*: JsonType__num,                                                                      \
         u64*: JsonType__num,                                                                      \
         f32*: JsonType__num,                                                                      \
         f64*: JsonType__num,                                                                      \
         _Bool*: JsonType__bool,                                                                   \
         str_s*: JsonType__str,                                                                    \
         const char**: JsonType__str,                                                              \
         char**: JsonType__str,                                                                    \
         void*: JsonType__null                                                                     \
                            ))

/*clang-format off*/

/// Returns ("format", err, args) text macro compatible with any printf() functions, used for
/// printing/formatting JSON debug message with reference to error type and line/col position of an
/// error.
/// Examples:
/// str.fmt(alloc, json$rd_err_fmt(&json_reader));
/// io.printf(json$rd_err_fmt(&json_reader));
/// io.fprintf(stderr, json$rd_err_fmt(&json_reader));
#define json$rd_err_fmt(json_reader)                                                               \
    "JSON %s(%s) at line: %d col: %d\n", (json_reader)->error ? "Parsing Error " : "",             \
        (json_reader)->error ? (json_reader)->error : "OK", (json_reader)->_impl.lexer.line + 1,   \
        (json_reader)->_impl.lexer.col

/*clang-format on*/

/// Executes (try_expression) (must return Exc or Exception type), and sets last json_reader.error
/// on fail, after this does `goto goto_on_fail_label`. Prints tracebacks in unit test suites.
#define json$rd_egoto(json_reader, try_expression, goto_on_fail_label)                             \
    if (!(json_reader)->error) {                                                                   \
        e$except_silent (err, try_expression) {                                                    \
            (json_reader)->error = err;                                                            \
            goto goto_on_fail_label;                                                               \
        }                                                                                          \
    }

#define json$rd_str_unescape(in_value, out_str_s_ptr, allocator)                                     \
    json.rd.str_unescape(in_value, out_str_s_ptr, allocator)

#define json$rd_str_unescape_inplace(in_value, out_buf, in_out_buf_size)                             \
    json.rd.str_unescape_inplace(in_value, out_buf, in_out_buf_size)

/// Gets next `json_type` scope as a string slice (str_s), you may use another json_reader instance
/// to parse result of this function. Sets json_reader.error + returns (str_s){0} on failure.
#define json$rd_get_scope_str_s(json_reader, json_type)                                            \
    json.rd.get_scope((json_reader), json_type)

/// Context specific iterator over json_reader scope:
/// json$rd_foreach(val, json_reader) - iterates over array scope items
/// json$rd_foreach(key, val, json_reader) - iterates over object scope key:value pairs
/// key and val - are typeof(str_s)
#define json$rd_foreach(...)                                                                       \
    _json$rd_foreach_impl(__VA_ARGS__, _json$rd_foreach_obj, _json$rd_foreach_arr)(__VA_ARGS__)

#define _json$rd_foreach_impl(_1, _2, _3, NAME, ...) NAME

#define _json$rd_foreach_arr(_val, json_reader)                                                    \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = json.rd.step_in((json_reader), JsonType__arr);                \
    }                                                                                              \
    for (str_s(_val) = { 0 };                                                                      \
         json.rd.next((json_reader)) ? ((_val) = ((json_reader)->val), 1) : 0;)

#define _json$rd_foreach_obj(_key, _val, json_reader)                                              \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = json.rd.step_in((json_reader), JsonType__obj);                \
    }                                                                                              \
    for (str_s(_key) = { 0 }, (_val) = { 0 };                                                      \
         json.rd.next((json_reader))                                                         \
             ? ((_key) = ((json_reader)->key), (_val) = ((json_reader)->val), 1)                   \
             : 0;)

typedef enum JsonType_e
{
    JsonType__eos = -2, // end of scope (after json.rd.step_in())
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


typedef struct json_rd_kw
{
    bool strict_mode;
} json_rd_kw;

typedef struct json_rd_c
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

} json_rd_c;

/// JSON Writer json$wr_new() keyword arguments
typedef struct json_wr_kw
{
    FILE* stream;
    sbuf_c* buf;
    u32 indent;
    bool simplified; // used in debug print: keys without "", serde.*.print() prepends type
} json_wr_kw;

/// JSON Writer container type
typedef struct json_wr_c
{
    FILE* stream;
    sbuf_c* buf;
    Exc error;
    u32 indent;
    u32 indent_width;
    u32 scope_depth;
    bool simplified;
    u8 scope_stack[CEX_MAX_JSON_DEPTH];
} json_wr_c;

/// Creates new instance of json writer, non allocating serializer, with support of exporting to
/// FILE* or backing by string buffer sbuf_c
#define json$wr_new(json_writer, kwargs...)                                                        \
    json.wr.create((json_writer), &(json_wr_kw){ kwargs })

/// Checks if json writer has no errors
#define json$wr_validate(json_writer) json.wr.validate((json_writer))

/// Writes a new key (must be in json$wr_scope(jw, JsonType__obj))
#define json$wr_key(key_name_string) json.wr.print_key(_json$wr_scope_var, key_name_string)

/// Writes a new value to the json scope (object or array), expects json compatible primitive
/// arguments. Use `json$rd_fmt` for customizable output.
#define json$wr_val(json_compatible_val)                                                           \
    ({                                                                                             \
        char* format = _Generic(                                                                   \
            (json_compatible_val),                                                                 \
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
            _Bool: "%B",                                                                           \
            str_s: "\"%S\"",                                                                       \
            const char*: "\"%s\"",                                                                 \
            char*: "\"%s\"",                                                                       \
            void*: "null"                                                                          \
        );                                                                                         \
        json.wr.print_val(_json$wr_scope_var, format, (json_compatible_val));                \
    })

#define _json$wr_scope_var _json_writer_macro_scope

/// Opens JSON scope, jsontype_arr_or_obj expects JsonType__obj or JsonType__arr
#define json$wr_scope(json_writer_ptr, jsontype_arr_or_obj)                                        \
    for (json_wr_c * _json$wr_scope_var                                                            \
             __attribute__((__cleanup__(cex_json__wr__print_scope_exit))) =                        \
             json.wr.print_scope_enter((json_writer_ptr), jsontype_arr_or_obj),              \
             *cex$tmpname(jsonbuf_sentinel) = _json$wr_scope_var;                                  \
         cex$tmpname(jsonbuf_sentinel) && _json$wr_scope_var != NULL;                              \
         cex$tmpname(jsonbuf_sentinel) = NULL)


/// Append any formatted string in the json$wr_scope, it's for low level printing
#define json$wr_fmt(format, ...) json.wr.print(_json$wr_scope_var, format, ##__VA_ARGS__)

// __cleanup__() attribute requires defined function
void cex_json__wr__print_scope_exit(json_wr_c** jwptr);

typedef struct json_gen_field_s
{
    str_s type;
    char* name;
    char* json_name;
    struct
    {
        bool is_ptr;
        bool is_nullable;
        bool is_string;
        bool is_array;
        bool is_hashmap;
        bool is_optional;
        bool is_skipped;
    } flags;
} json_gen_field_s;

typedef struct json_gen_type_s
{
    char* name;
    char* ns_name;
    arr$(json_gen_field_s*) fields;
    struct
    {
        bool is_struct;
    } flags;
} json_gen_type_s;

typedef struct json_gen_kw
{
    char* out_namespace;
    u32 buf_initial_capacity;
    char* workdir;
    char* out_dir;
} json_gen_kw;

typedef struct json_gen_c
{
    IAllocator allc;
    hm$(str_s, json_gen_type_s*) types;
    arr$(char*) includes;
    char* namespace;
    sbuf_c c_file_content;
    sbuf_c h_file_content;
    char* workdir;
    char* outdir;
    char* target;
    char* c_out_name;
    char* h_out_name;
} json_gen_c;

struct __cex_namespace__json {
    // Autogenerated by CEX
    // clang-format off


    struct {
        /// JSON Generator in can parse all json$$struct() inside source code and automatically
        Exception       (*create)(json_gen_c* self, IAllocator allc, json_gen_kw* kwargs);
        /// (low-level) Generates a content of a json serde engine and stores it in `self` sbuf.
        Exception       (*generate_full)(json_gen_c* self);
        /// (low-level) Parses the .h file and add all json$$struct() types into serialization pipeline
        Exception       (*process_file)(json_gen_c* self, char* path);
        /// Runs full JSON generation sequence: finds all header files in target directory, processes
        Exception       (*run)(json_gen_c* self);
    } gen;

    struct {
        /// Create new JSON reader (it doesn't allocate memory and read-only content view)
        Exception       (*create)(json_rd_c* it, char* content, usize content_len, json_rd_kw* kwargs);
        /// Get str_s of the next JSON scope, in case if you need to parse it later, depending on
        str_s           (*get_scope)(json_rd_c* it, JsonType_e scope_type);
        /// Get next item of current scope, set it.key, it.val, it.type fields
        bool            (*next)(json_rd_c* it);
        /// Skips next scope of the json data
        Exc             (*skip)(json_rd_c* it);
        /// Make step inside JSON object or array scope (cex_json__rd__next() starts emitting
        Exception       (*step_in)(json_rd_c* it, JsonType_e expected_type);
        /// Early step out from JSON scope (you must immediately break the loop/func after step out)
        Exception       (*step_out)(json_rd_c* it);
        /// Replaces escaped JSON string values (e.g. \uXXXX or \n \t) with real byte representation
        Exception       (*str_unescape)(str_s value_str, str_s* out_val, IAllocator allc);
        /// Replaces escaped JSON string values (e.g. \uXXXX or \n \t) with real byte representation
        Exception       (*str_unescape_inplace)(str_s value_str, char* out_buf, usize* in_out_size);
        /// Checks if json_rd_c object has any errors
        Exception       (*validate)(json_rd_c* jr);
    } rd;

    struct {
        Exception       (*create)(json_wr_c* jw, json_wr_kw* kwargs);
        void            (*print)(json_wr_c* jw, char* format,...);
        /// Print json "key": part, expected to be used with json.wr.print_val() at the next step.
        void            (*print_key)(json_wr_c* jw, char* key);
        /// Low-level alternative for json$wr_scope(jw, scope_type) macro. Enters json scope_type of
        json_wr_c*      (*print_scope_enter)(json_wr_c* jw, JsonType_e scope_type);
        /// Low-level alternative for json$wr_scope(jw, scope_type) macro. Exits json scope_type of
        void            (*print_scope_exit)(json_wr_c** jwptr);
        /// Writes JSON escaped string, supports unicode (\uXXXX) and binary escaping (\t\n\f, etc)
        void            (*print_str_escaped)(json_wr_c* jw, char* s, usize slen, bool add_quotes);
        /// Prints formatted raw data into json. IMPORTANT: it can wreck your formatting, use with
        void            (*print_val)(json_wr_c* jw, char* format,...);
        /// Validates json writer state
        Exception       (*validate)(json_wr_c* jw);
    } wr;

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__json json;

