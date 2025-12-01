#include "cex.h"

#ifndef CEX_MAX_JSON_DEPTH
#    define CEX_MAX_JSON_DEPTH 128
#endif

#define jr$new(json_reader, content, len, kwargs...)                                               \
    ({                                                                                             \
        json_reader_kw _kwargs = { kwargs };                                                       \
        (json_reader)->error = json.reader.create((json_reader), (content), (len), &_kwargs);      \
    })


#define jr$foreach(...) _jr$foreach_impl(__VA_ARGS__, _jr$foreach_obj, _jr$foreach_arr)(__VA_ARGS__)

#define _jr$foreach_impl(_1, _2, _3, NAME, ...) NAME

#define _jr$foreach_arr(_val, json_reader)                                                         \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = json.reader.step_in((json_reader), JsonType__arr);                  \
    }                                                                                              \
    for (str_s(_val) = { 0 };                                                                      \
         json.reader.next((json_reader)) ? ((_val) = ((json_reader)->val), 1) : 0;)

#define _jr$foreach_obj(_key, _val, json_reader)                                                   \
    if ((json_reader)->error == EOK) {                                                             \
        (json_reader)->error = json.reader.step_in((json_reader), JsonType__obj);                  \
    }                                                                                              \
    for (str_s(_key) = { 0 }, (_val) = { 0 };                                                      \
         json.reader.next((json_reader))                                                           \
             ? ((_key) = ((json_reader)->key), (_val) = ((json_reader)->val), 1)                   \
             : 0;)

#define str$eq(str_s_slice, compare_to_literal)                                                    \
    ((str_s_slice).buf && (str_s_slice).len == sizeof(compare_to_literal) - 1 &&                   \
     memcmp((str_s_slice).buf, compare_to_literal, sizeof(compare_to_literal) - 1) == 0)

typedef enum JsonType_e
{
    JsonType__eos = -2, // end of scope (after json.reader.step_in())
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
    u32 has_scope_items;
    u32 scope_depth;
    u8 scope_stack[CEX_MAX_JSON_DEPTH];
} json_writer_c;

#define jw$new(json_writer, sbuf_or_stream, kwargs...)                                             \
    ({                                                                                             \
        json_writer_kw _kwargs = { kwargs };                                                       \
        _Generic(                                                                                  \
            (sbuf_or_stream),                                                                      \
            sbuf_c: _cex_json__writer__set_buf((&_kwargs), (sbuf_or_stream)),                      \
            FILE*: _cex_json__writer__set_stream((&_kwargs), (sbuf_or_stream))                     \
        );                                                                                         \
        (json_writer)->error = json.writer.create((json_writer), &_kwargs);                        \
    })

#define _jw$buf_var _json_writer_macro_scope

/// Opens JSON buffer scope (json_writer_ptr data is cleared out)
#define jw$buf(json_writer_ptr, jsontype_arr_or_obj)                                               \
    for (json_writer_c * _jw$buf_var                                                               \
             __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) =                        \
             _cex__jsonbuf_print_scope_enter((json_writer_ptr), jsontype_arr_or_obj, true),        \
             *cex$tmpname(jsonbuf_sentinel) = _jw$buf_var;                                         \
         cex$tmpname(jsonbuf_sentinel) && _jw$buf_var != NULL;                                     \
         cex$tmpname(jsonbuf_sentinel) = NULL)


/// Add new key: {...} scope into (jw$buf)
#define jw$kobj_scope(key)                                                                         \
    _cex_json__writer__print(_jw$buf_var, "\"%s\": ", key);                                        \
    for (json_writer_c * cex$tmpname(jsonbuf_scope)                                                \
                             __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) =        \
             _cex__jsonbuf_print_scope_enter(_jw$buf_var, JsonType__obj, false),                   \
                             *cex$tmpname(jsonbuf_sentinel) = cex$tmpname(jsonbuf_scope);          \
         cex$tmpname(jsonbuf_sentinel) && cex$tmpname(jsonbuf_scope) != NULL;                      \
         cex$tmpname(jsonbuf_sentinel) = NULL)

/// Add new key: [...] scope into (jw$buf)
#define jw$karr_scope(key)                                                                         \
    _cex_json__writer__print(_jw$buf_var, "\"%s\": ", key);                                        \
    for (json_writer_c * cex$tmpname(jsonbuf_scope)                                                \
                             __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) =        \
             _cex__jsonbuf_print_scope_enter(_jw$buf_var, JsonType__arr, false),                    \
                             *cex$tmpname(jsonbuf_sentinel) = cex$tmpname(jsonbuf_scope);          \
         cex$tmpname(jsonbuf_sentinel) && cex$tmpname(jsonbuf_scope) != NULL;                      \
         cex$tmpname(jsonbuf_sentinel) = NULL)

/// Add new {...} scope into (jw$buf)
#define jw$obj_scope()                                                                             \
    for (json_writer_c * cex$tmpname(jsonbuf_scope)                                                \
                             __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) =        \
             _cex__jsonbuf_print_scope_enter(_jw$buf_var, JsonType__obj, true),                    \
                             *cex$tmpname(jsonbuf_sentinel) = cex$tmpname(jsonbuf_scope);          \
         cex$tmpname(jsonbuf_sentinel) && cex$tmpname(jsonbuf_scope) != NULL;                      \
         cex$tmpname(jsonbuf_sentinel) = NULL)

/// Add new [...] scope into (jw$buf)
#define jw$arr_scope()                                                                             \
    for (json_writer_c * cex$tmpname(jsonbuf_scope)                                                \
                             __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) =        \
             _cex__jsonbuf_print_scope_enter(_jw$buf_var, JsonType__arr, true),                    \
                             *cex$tmpname(jsonbuf_sentinel) = cex$tmpname(jsonbuf_scope);          \
         cex$tmpname(jsonbuf_sentinel) && cex$tmpname(jsonbuf_scope) != NULL;                      \
         cex$tmpname(jsonbuf_sentinel) = NULL)

/// Append any formatted string, it's for low level printing (jw$buf)
#define jw$fmt(format, ...) _cex_json__writer__print(_jw$buf_var, format, __VA_ARGS__)

/// Append string item into array scope (jw$buf)
#define jw$str(format, ...)                                                                        \
    _cex_json__writer__print_item(_jw$buf_var, "\"" format "\"", __VA_ARGS__)

/// Append value item into array scope (jw$buf)
#define jw$val(format, ...) _cex_json__writer__print_item(_jw$buf_var, format "", __VA_ARGS__)

/// Append `"<key>": "<format>"` (with your own format) into object scope (jw$buf)
#define jw$kstr(key, format, ...)                                                                  \
    _cex_json__writer__print_key(_jw$buf_var, (key), "\"" format "\"", __VA_ARGS__)

/// Append `"<key>": <format>` into object scope (jw$buf)
#define jw$kval(key, format, ...)                                                                  \
    _cex_json__writer__print_key(_jw$buf_var, (key), format, __VA_ARGS__)

void _cex_json__writer__print(json_writer_c* jb, char* format, ...);
void _cex_json__writer__print_item(json_writer_c* jb, char* format, ...);
void _cex_json__writer__print_key(json_writer_c* jb, char* key, char* format, ...);
json_writer_c* _cex__jsonbuf_print_scope_enter(json_writer_c* jb, JsonType_e scope_type, bool should_indent);
void _cex__jsonbuf_print_scope_exit(json_writer_c** jbptr);
void _cex_json__writer__set_buf(json_writer_kw* jb, void* sbuf);
void _cex_json__writer__set_stream(json_writer_kw* jb, void* stream);


/**
Low level JSON reader/writer namespace

Making own JSON buffer:

```c
json_writer_c jb;
e$ret(json.buf.create(&jb, 1024, 0, mem$));
jw$buf(&jb, JsonType__obj)
{
    jw$kstr("foo2", "%d", 1);
    jw$kobj_scope("foo3") {
        jw$kval("bar", "%s", "3");
    }
    jw$karr_scope("foo3") {
        jw$val("%d", 8);
        jw$str("%d", 9);
    }
}
>> json.buf.get(&jb) ->
>> {"foo2": "1", "foo3": {"bar": 3},"foo3": [8, "9"]}


Reading JSON buffer:
```c
    struct Foo
    {
        struct { u32 baz; u32 fuzz; } foo;
        u32 next;
        u32 baz;
    } data = { 0 };
    str_s content = str$s(
        "{ \"foo\" : {\"baz\": 3, \"fuzz\": 8, \"oops\": 0}, \"next\": 7, \"baz\": 17 }"
    );
    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, 0, false));
    if (json.reader.next(&js)) { e$ret(json.reader.step_in(&js, JsonType__obj)); }
    while (json.reader.next(&js)) {
        jr$case_invalid (&js) {}
        jr$case (&js, "foo") {
            e$ret(json.reader.step_in(&js, JsonType__obj));
            while (json.reader.next(&js)) {
                jr$case_invalid (&js) {}
                jr$case (&js, "fuzz") { e$ret(str$convert(js.val, &data.foo.fuzz)); }
                jr$case (&js, "baz") { e$ret(str$convert(js.val, &data.foo.baz)); }
                jr$case_default(&js)
                {
                    tassert_eq(js.key, str$s("oops"));
                }
            }
        }
        jr$case (&js, "next") { e$ret(str$convert(js.val, &data.next)); }
        jr$case (&js, "baz") { e$ret(str$convert(js.val, &data.baz)); }
    }
    e$assert(js.error == EOK && "No parsing errors");

```

*/
struct __cex_namespace__json
{
    // Autogenerated by CEX
    // clang-format off


    struct {
        /// Create new JSON reader (it doesn't allocate memory and uses content slicing)
        Exception       (*create)(json_reader_c* it, char* content, usize content_len, json_reader_kw* kwargs);
        /// Get next JSON item for a scope
        bool            (*next)(json_reader_c* it);
        /// Make step inside JSON object or array scope (json.reader.next() starts emitting this scope)
        Exception       (*step_in)(json_reader_c* it, JsonType_e expected_type);
        /// Early step out from JSON scope (you must immediately break the loop/func after step out)
        Exception       (*step_out)(json_reader_c* it);
    } reader;

    struct {
        /// Create JSON buffer/builder container used with json$buf / json$fmt / json$kstr macros
        Exception       (*create)(json_writer_c* jw, json_writer_kw* kwargs);
        /// Destroy JSON buffer instance (not necessary to call if initialized on tmem$ allocator)
        void            (*destroy)(json_writer_c* jw);
        /// Get JSON buffer contents (NULL if any error occurred)
        char*           (*get)(json_writer_c* jw);
        /// Check if there is any error in JSON buffer
        Exception       (*validate)(json_writer_c* jw);
    } writer;

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__json json;
