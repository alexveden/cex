#include "json.h"

const struct _CEX_JsonError_struct JsonError = {
    .parsing = "ParsingErrorJSON",
    .missing_field = "MissingFieldErrorJSON",
    .null_field = "NullFieldErrorJSON",
    .unknown_field = "UnknownFieldErrorJSON",
    .wrong_type = "WrongTypeErrorJSON",
    .encoding = "EncodingErrorJSON",
};

/* TEMP MACROS - for private implementation*/
#define $scope_obj (1 << 1)
#define $scope_arr (1 << 2)
#define $scope_has_items (1 << 3)
#define $scope_has_key (1 << 4)
#define $scope_null (1 << 5)

#define $last_scope(jw)                                                                            \
    ((jw)->scope_depth && (jw)->scope_stack[(jw)->scope_depth - 1])                                \
        ? (jw)->scope_stack[(jw)->scope_depth - 1]                                                 \
        : 0

#define $print(format, ...) /* temp macro */                                                       \
    ({                                                                                             \
        if (jw->buf) {                                                                             \
            Exc err = sbuf.appendf(jw->buf, format, ##__VA_ARGS__);                                \
            if (unlikely(err != EOK && jw->error == EOK)) { jw->error = err; }                     \
        } else if (jw->stream) {                                                                   \
            Exc err = io.fprintf(jw->stream, format, ##__VA_ARGS__);                               \
            if (unlikely(err != EOK && jw->error == EOK)) { jw->error = err; }                     \
        }                                                                                          \
    })

#define $printva() /* temp macro! */                                                               \
    va_list va;                                                                                    \
    va_start(va, format);                                                                          \
    if (jw->buf) {                                                                                 \
        Exc err = sbuf.appendfva(jw->buf, format, va);                                             \
        if (unlikely(err != EOK && jw->error != EOK)) { jw->error = err; }                         \
    } else if (jw->stream) {                                                                       \
        int result = cexsp__vfprintf(jw->stream, format, va);                                      \
        if (result == -1) { jw->error = Error.io; }                                                \
    }                                                                                              \
    va_end(va);

#define $next_tok() /* TEMP MACRO */                                                               \
    ({                                                                                             \
        cex_token_s _tok = CexParser.next_token(&it->_impl.lexer);                                 \
        if (!it->_impl.strict_mode) {                                                              \
            while (_tok.type == CexTkn__comment_single || _tok.type == CexTkn__comment_multi) {    \
                _tok = CexParser.next_token(&it->_impl.lexer);                                     \
            }                                                                                      \
        }                                                                                          \
        it->_impl.prev_token = it->_impl.curr_token;                                               \
        it->_impl.curr_token = _tok.type;                                                          \
        _tok;                                                                                      \
    })


/**
 * @brief Create new JSON reader (it doesn't allocate memory and read-only content view)
 *
 * @param it self instance (typically allocated on stack)
 * @param content  JSON content
 * @param content_len JSON content length (if 0 length will be recalculated via strlen())
 * @param kwargs - optional kwargs, can be NULL
 * @return
 */
static Exception
cex_json__rd__create(json_rd_c* it, char* content, usize content_len, json_rd_kw* kwargs)
{
    uassert(it != NULL);
    if (content == NULL) { return Error.argument; }

    bool strict_mode = false;
    if (kwargs != NULL) { strict_mode = kwargs->strict_mode; }

    *it = (json_rd_c){
        ._impl = {
            .strict_mode = strict_mode,
            .lexer = CexParser.create(content, content_len, false),
        },
    };
    if (it->_impl.lexer.content == it->_impl.lexer.content_end) { return Error.null_or_empty; }
    json.rd.next(it);
    return EOK;
}

/**
 * @brief Make step inside JSON object or array scope (cex_json__rd__next() starts emitting
 * this scope)
 *
 * @param it
 * @param expected_type Expected scope type (for sanity checks)
 * @return
 */
static Exception
cex_json__rd__step_in(json_rd_c* it, JsonType_e expected_type)
{
    if (unlikely(it->error != EOK)) { goto error; }
    if (unlikely(it->_impl.scope_depth >= sizeof(it->_impl.scope_stack) - 1)) {
        it->error = "JSON Scope nesting overflow";
        goto error;
    }
    if (unlikely(expected_type <= 0 || it->type != expected_type)) {
        it->error = "Unexpected type for stepping in";
        goto error;
    }

    if (it->type == JsonType__obj) {
        uassert(it->_impl.curr_token == CexTkn__lbrace);
        it->_impl.scope_stack[it->_impl.scope_depth] = $scope_obj;
        it->_impl.scope_depth++;
    } else if (it->type == JsonType__arr) {
        uassert(it->_impl.curr_token == CexTkn__lbracket);
        it->_impl.scope_stack[it->_impl.scope_depth] = $scope_arr;
        it->_impl.scope_depth++;
    } else {
        // return cex_json__rd__next(it);
        it->error = "Stepping in is only for objects or arrays";
        goto error;
    }
    // cex_json__rd__next() is going to check if we step in or skipping whole block
    it->_impl.prev_token = it->_impl.curr_token;
    it->_impl.curr_token = CexTkn__unk;
    it->_impl.has_items = false;
    return it->error;

error:
    it->type = JsonType__err;
    it->val = (str_s){ 0 };
    it->key = (str_s){ 0 };
    return it->error;
}

/**
 * @brief Early step out from JSON scope (you must immediately break the loop/func after step out)
 *
 * After calling step out, next call of `json.rd.next()` will return outer scope item,
 * make sure that you also break the loop or exiting parsing function for current scope.
 *
 * @param it
 * @return
 */
Exception
cex_json__rd__step_out(json_rd_c* it)
{
    if (unlikely(it->_impl.scope_depth == 0)) {
        it->error = "Bad scope/level for step out";
        return it->error;
    }
    u32 scope_depth_initial = it->_impl.scope_depth - 1;
    while (it->_impl.scope_depth > scope_depth_initial && json.rd.next(it)) {}
    return it->error;
}


/**
 * @brief Replaces escaped JSON string values (e.g. \uXXXX or \n \t) with real byte representation
 * supports unicode
 *
 * @param value_str input value (typically key of an object or string item ) (read-only)
 * @param out_buf output buffer, replaces its content
 * @param in_out_size (in) initial size of a buffer (must be at least value_str.len + 1 size), (out)
 * size of bytes written in out_buf
 * @return
 */
Exception
cex_json__rd__str_unescape_inplace(str_s value_str, char* out_buf, usize* in_out_size)
{
    uassert(out_buf);
    uassert(in_out_size);

    if (unlikely(!value_str.buf)) { return Error.null_or_empty; }
    if (unlikely(*in_out_size <= value_str.len)) {
        uassert(*in_out_size >= value_str.len + 1);
        return Error.assert;
    }

    u8* input = (u8*)value_str.buf;
    usize len = value_str.len;
    usize i = 0, j = 0;

    // clang-format off
    static const u8 hex_lut[256] = {
        ['0'] = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        ['A'] = 11, 12, 13, 14, 15, 16,
        ['a'] = 11, 12, 13, 14, 15, 16,
    };
    // clang-format on

    while (i < len) {
        if (input[i] != '\\') {
            // Normal character
            out_buf[j++] = input[i++];
        } else {
            // backslash (\) char
            i++; // Skip backslash

            if (i >= len) {
                out_buf[j++] = '\\';
                break;
            }

            // Handle standard JSON escapes
            switch (input[i]) {
                case '"':
                    out_buf[j++] = '"';
                    i++;
                    break;
                case '\\':
                    out_buf[j++] = '\\';
                    i++;
                    break;
                case '/':
                    out_buf[j++] = '/';
                    i++;
                    break;
                case 'b':
                    out_buf[j++] = '\b';
                    i++;
                    break;
                case 'f':
                    out_buf[j++] = '\f';
                    i++;
                    break;
                case 'n':
                    out_buf[j++] = '\n';
                    i++;
                    break;
                case 'r':
                    out_buf[j++] = '\r';
                    i++;
                    break;
                case 't':
                    out_buf[j++] = '\t';
                    i++;
                    break;

                case 'u': {
                    // Unicode escape: \uXXXX
                    i++; // Skip 'u'

                    // Parse 4 hex digits
                    if (unlikely(i + 3 >= len)) { goto fail; }

                    // Convert hex to Unicode code point
                    u32 codepoint = 0;
                    for (int _i = 0; _i < 4; _i++) {
                        uint8_t nibble = hex_lut[(u8)input[i++]];
                        if (unlikely(nibble == 0x0)) { goto fail; }
                        codepoint = (codepoint << 4) | (nibble - 1);
                    }

                    // Handle UTF-8 encoding
                    if (codepoint <= 0x7F) {
                        // 1 byte UTF-8
                        out_buf[j++] = (char)codepoint;
                    } else if (codepoint <= 0x7FF) {
                        // 2 bytes UTF-8
                        out_buf[j++] = 0xC0 | (codepoint >> 6);
                        out_buf[j++] = 0x80 | (codepoint & 0x3F);
                    } else if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                        // high is a first unicode value \uXXXX
                        // low is a second part value unicode \uYYYY
                        u32 high = codepoint;
                        u32 low = 0;
                        if (unlikely(i + 5 >= len)) { goto fail; }
                        if (unlikely(input[i] != '\\')) { goto fail; }
                        i++;
                        if (unlikely(input[i] != 'u')) { goto fail; }
                        i++;
                        for (int _i = 0; _i < 4; _i++) {
                            uint8_t nibble = hex_lut[(u8)input[i++]];
                            if (unlikely(nibble == 0x00)) { goto fail; }
                            low = (low << 4) | (nibble - 1);
                        }
                        if (unlikely(!(low >= 0xDC00 && low <= 0xDFFF))) { goto fail; }

                        codepoint = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                        out_buf[j++] = (u8)(0xF0 | (codepoint >> 18));
                        out_buf[j++] = (u8)(0x80 | ((codepoint >> 12) & 0x3F));
                        out_buf[j++] = (u8)(0x80 | ((codepoint >> 6) & 0x3F));
                        out_buf[j++] = (u8)(0x80 | (codepoint & 0x3F));
                    } else {
                        // 3 bytes UTF-8 (most common for \uXXXX)
                        out_buf[j++] = 0xE0 | (codepoint >> 12);
                        out_buf[j++] = 0x80 | ((codepoint >> 6) & 0x3F);
                        out_buf[j++] = 0x80 | (codepoint & 0x3F);
                    }
                    break;
                }

                default:
                    // Unknown escape, copy both characters
                    out_buf[j++] = '\\';
                    out_buf[j++] = input[i++];
                    break;
            }
        }
    }

    out_buf[j] = '\0';
    *in_out_size = j;

    return EOK;

fail:
    out_buf[j] = '\0';
    *in_out_size = 0;
    return JsonError.encoding;
}

/**
 * @brief Replaces escaped JSON string values (e.g. \uXXXX or \n \t) with real byte representation
 * supports unicode.
 *
 * @param value_str input value (typically key of an object or string item ) (read-only)
 * @param out_val Allocated str_s, out_val.buf - is allocated, make sure mem$free(allc, out_val.buf)
 * @param allc Allocator for the result
 * @return
 */
static Exception
cex_json__rd__str_unescape(str_s value_str, str_s* out_val, IAllocator allc)
{

    if (unlikely(!out_val)) { return Error.argument; }
    char* output = mem$malloc(allc, value_str.len + 1);
    if (unlikely(!output)) { return Error.memory; }

    usize output_size = value_str.len + 1;
    e$except_silent (err, cex_json__rd__str_unescape_inplace(value_str, output, &output_size)) {
        mem$free(allc, output);
        return err;
    }
    *out_val = (str_s){ .buf = output, .len = output_size };

    return EOK;
}

/**
 * @brief Skips next scope of the json data
 *
 * @param it
 * @return
 */
static Exc
cex_json__rd__skip(json_rd_c* it)
{
    // Simulate full step-in/next sequence for all nested stuff (because it serves as syntax check)
    u32 scope_depth_initial = it->_impl.scope_depth;
    if (json.rd.step_in(it, it->type)) { return it->error; }

    while (it->_impl.scope_depth > scope_depth_initial) {
        if (!json.rd.next(it) && it->error) { break; }
        switch (it->type) {
            case JsonType__arr:
            case JsonType__obj:
                if (cex_json__rd__step_in(it, it->type)) { return it->error; }
            default:
                break;
        }
    }
    return it->error;
}

/**
 * @brief Checks if json_rd_c object has any errors
 *
 * @param jr
 * @return
 */
static Exception
cex_json__rd__validate(json_rd_c* jr)
{
    if (jr == NULL) { return Error.argument; }
    return jr->error;
}

/**
 * @brief Get str_s of the next JSON scope, in case if you need to parse it later, depending on
 future JSON data
 *
 * @param it
 * @param scope_type Expected scope type
 * @return slice of json scope including `[]` and `{}`, on error - (str_s){ 0 };

 */
static str_s
cex_json__rd__get_scope(json_rd_c* it, JsonType_e scope_type)
{
    uassert(scope_type == JsonType__arr || scope_type == JsonType__obj);

    str_s result = { 0 };
    if (unlikely(it->error != EOK)) { return result; }

    if (unlikely(it->type != scope_type)) {
        if (scope_type == JsonType__arr) {
            it->error = "Expected array scope";
        } else {
            it->error = "Expected object scope";
        }
        return result;
    }

    uassert(it->_impl.curr_token == CexTkn__lbrace || it->_impl.curr_token == CexTkn__lbracket);

    char* cur = it->_impl.lexer.cur - 1;
    uassert(cur >= it->_impl.lexer.content);

    if (cex_json__rd__skip(it) == EOK) {
        char* last_cur = it->_impl.lexer.cur;
        uassert(last_cur > cur);
        uassert(last_cur < it->_impl.lexer.content_end);
        uassert(it->type == JsonType__eos);
        if (last_cur > cur) {
            // NOTE: we must have at least something in result,
            // valid .buf with .len=0, may lead to full text parse
            // if the json$rd_new()
            result = (str_s){ .buf = cur, .len = last_cur - cur };
        }
    }

    return result;
}

/**
 * @brief Get next item of current scope, set it.key, it.val, it.type fields
 *
 * @param it json reader object
 * @return true if next item is available, false - on error, end of scope, or file
 */
static bool
cex_json__rd__next(json_rd_c* it)
{
    if (unlikely(it->error != EOK)) { goto error; }
    it->key = (str_s){ 0 };

    if (unlikely(
            it->_impl.curr_token == CexTkn__lbrace || it->_impl.curr_token == CexTkn__lbracket
        )) {
        // User didn't step into object/array, skipping it
        if (cex_json__rd__skip(it) != EOK) { goto error; }
    }

    cex_token_s t = $next_tok();
    if (it->_impl.scope_depth == 0 && t.type != CexTkn__eof) {
        if (it->_impl.has_items) {
            // Additional items after end of previous scope
            goto error_unexpected;
        } else {
            it->_impl.has_items = true;
        }
    }
    if (it->_impl.scope_depth > 0) {
        if (it->_impl.scope_stack[it->_impl.scope_depth - 1] == $scope_obj) {
            // OBJECT: {"foo": "bar"}
            switch (t.type) {
                case CexTkn__comma: {
                    // Comma from previous item
                    if (it->_impl.prev_token == CexTkn__lbrace ||
                        it->_impl.prev_token == CexTkn__colon || !it->_impl.has_items) {
                        goto error_unexpected;
                    }
                    t = $next_tok();
                    if (t.type == CexTkn__rbrace) {
                        goto parse_generic;
                    } else if (t.type != CexTkn__string && t.type != CexTkn__ident &&
                               t.type != CexTkn__char) {
                        goto error_unexpected;
                    }
                    fallthrough(); // we get another key: value
                }
                case CexTkn__char:
                case CexTkn__ident: {
                    // NOTE: it->_impl.curr_token != CexTkn__string because of fallthrough()
                    if (t.type != CexTkn__string && it->_impl.strict_mode &&
                        (it->_impl.prev_token == CexTkn__comma ||
                         it->_impl.prev_token == CexTkn__unk)) {
                        it->error = "Keys without double quotes (strict mode)";
                        goto error;
                    }
                    fallthrough();
                }
                case CexTkn__string: {
                    // Getting key of a object
                    it->key = t.value;
                    t = $next_tok();
                    if (t.type != CexTkn__colon) { goto error_unexpected; }
                    t = $next_tok();
                    if (t.type == CexTkn__rbrace) { goto error_unexpected; }
                    it->_impl.has_items = true;
                    goto parse_generic; // parsing value
                }
                default: {
                    goto parse_generic;
                }
            }
        } else if (it->_impl.scope_stack[it->_impl.scope_depth - 1] == $scope_arr) {
            // ARRAY: ["foo", "bar"]
            switch (t.type) {
                case CexTkn__rbracket: {
                    goto parse_generic;
                }
                case CexTkn__comma: {
                    if (!it->_impl.has_items) { goto error_unexpected; }
                    t = $next_tok();
                    goto parse_generic;
                }
                default: {
                    if (it->_impl.has_items && it->_impl.prev_token != CexTkn__comma) {
                        goto error_unexpected;
                    }
                    it->_impl.has_items = true;
                    goto parse_generic;
                }
            }
        } else {
            it->error = "Unexpected scope char";
            goto error;
        }
    }

parse_generic:
    // Parsing generic value
    switch (t.type) {
        case CexTkn__lbrace: {
            it->type = JsonType__obj;
            it->val = (str_s){ 0 };
            goto end;
        }
        case CexTkn__lbracket: {
            it->type = JsonType__arr;
            it->val = (str_s){ 0 };
            goto end;
        }
        case CexTkn__rbrace: {
            if (it->_impl.scope_depth > 0 &&
                it->_impl.scope_stack[it->_impl.scope_depth - 1] & $scope_obj) {
                if (it->_impl.strict_mode && it->_impl.prev_token == CexTkn__comma) {
                    it->error = "Ending comma in object";
                    goto error;
                }
                it->_impl.scope_stack[it->_impl.scope_depth - 1] = 0;
                it->_impl.scope_depth--;
                it->type = JsonType__eos;
                it->val = (str_s){ 0 };
                if (it->_impl.scope_depth == 0) { it->_impl.has_items = true; }
                goto end;
            } else {
                goto error_unexpected;
            }
        }
        case CexTkn__rbracket: {
            if (it->_impl.scope_depth > 0 &&
                it->_impl.scope_stack[it->_impl.scope_depth - 1] & $scope_arr) {
                if (it->_impl.strict_mode && it->_impl.prev_token == CexTkn__comma) {
                    it->error = "Ending comma in array";
                    goto error;
                }
                it->_impl.scope_stack[it->_impl.scope_depth - 1] = 0;
                it->_impl.scope_depth--;
                it->type = JsonType__eos;
                it->val = (str_s){ 0 };
                if (it->_impl.scope_depth == 0) { it->_impl.has_items = true; }
                goto end;
            } else {
                goto error_unexpected;
            }
        }
        case CexTkn__comment_multi:
        case CexTkn__comment_single: {
            uassert(it->_impl.strict_mode);
            goto error_unexpected;
        }
        case CexTkn__string: {
            it->type = JsonType__str;
            it->val = t.value;
            goto end;
        }
        case CexTkn__plus:
            if (it->_impl.strict_mode) { goto error_unexpected; }
            fallthrough();
        case CexTkn__minus: {
            char* sign = t.value.buf;
            t = $next_tok();
            if (t.type != CexTkn__number) {
                // Handling -inf/+Inf
                if (!(!it->_impl.strict_mode && t.type == CexTkn__ident &&
                      str.slice.eqi(t.value, str$s("inf")))) {
                    goto error_unexpected;
                }
            }
            if (t.value.buf - sign != 1) { goto error_unexpected; }   // no whitespace allowed!
            t.value = (str_s){ .buf = sign, .len = t.value.len + 1 }; // extend including sign
        }
            fallthrough();
        case CexTkn__number: {
            it->type = JsonType__num;
            it->val = t.value;
            goto end;
        }
        case CexTkn__eof: {
            if (it->_impl.scope_depth == 0) {
                it->type = JsonType__eof;
                it->val = (str_s){ 0 };
                goto end;
            } else {
                goto error_unexpected;
            }
        }
        case CexTkn__ident: {
            if (str.slice.eq(t.value, str$s("null"))) {
                it->type = JsonType__null;
            } else if (str.slice.eq(t.value, str$s("true")) ||
                       str.slice.eq(t.value, str$s("false"))) {
                it->type = JsonType__bool;
            } else {
                if (!it->_impl.strict_mode) {
                    if (!(str.slice.eqi(t.value, str$s("nan")) ||
                          str.slice.eqi(t.value, str$s("inf")))) {
                        goto error_unexpected;
                    }
                    it->type = JsonType__num;
                } else {
                    goto error_unexpected;
                }
            }
            if (it->type != JsonType__null) {
                it->val = t.value;
            } else {
                it->val = (str_s){ .buf = NULL, .len = 1 };
            }
            goto end;
        }
        default: {
            goto error_unexpected;
        }
    }
    unreachable();

end:
    return it->type > 0;

error_unexpected:
    it->error = "Unexpected token";
error:
    it->type = JsonType__err;
    it->key = (str_s){ 0 };
    it->val = (str_s){ 0 };
    goto end;
}

void
_cex_json_writer_indent(json_wr_c* jw, bool last_item)
{
    // if (unlikely(jw->error != EOK)) { return; }
    if (jw->scope_depth && jw->scope_stack[jw->scope_depth - 1] & $scope_has_items) {
        if (!last_item) { $print(", ", ""); }
        if (jw->indent_width) { $print("\n", ""); }
    } else {
        if (!last_item) {
            if (jw->indent_width && jw->scope_depth) { $print("\n", ""); }
        } else {
            // skipping indent for empty obj/arr -> {} or []
            return;
        }
    }
    for (u32 i = 0; i < jw->indent; i++) { $print(" ", ""); }
}

Exception
cex_json__wr__create(json_wr_c* jw, json_wr_kw* kwargs)
{
    uassert(jw != NULL);
    uassert(kwargs != NULL);

    if (kwargs->buf == NULL && kwargs->stream == NULL) { return "Empty buf and stream kwargs"; }
    if (kwargs->buf != NULL && kwargs->stream != NULL) {
        return "buf and stream kwargs are mutually exclusive";
    }

    *jw = (json_wr_c){
        .indent_width = kwargs->indent,
        .buf = kwargs->buf,
        .stream = kwargs->stream,
        .simplified = kwargs->simplified,
    };

    return EOK;
}

static void
cex_json__wr__print(json_wr_c* jw, char* format, ...)
{
    u8 last_scope = $last_scope(jw);
    if (!(last_scope & $scope_has_key)) { _cex_json_writer_indent(jw, false); }

    $printva();

    if (jw->scope_depth && jw->scope_stack[jw->scope_depth - 1]) {
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_items;
        jw->scope_stack[jw->scope_depth - 1] &= ~$scope_has_key;
    }
}

/**
 * @brief Writes JSON escaped string, supports unicode (\uXXXX) and binary escaping (\t\n\f, etc)
 *
 * @param jw
 * @param s string
 * @param slen length of `s` argument
 * @param add_quotes add `"` around string content  `"`
 */
static void
cex_json__wr__print_str_escaped(json_wr_c* jw, char* s, usize slen, bool add_quotes)
{
    (void)jw;
    (void)s;
    (void)slen;

    char buf[256];
    usize cnt = 0;

    if (add_quotes) {
        buf[0] = '"';
        cnt++;
    }

    const char hex_digits[] = "0123456789ABCDEF";
    bool has_escape = false;

// TODO: consider big-endianess
#define $tohex(_byte32)                                                                            \
    buf[cnt++] = '\\';                                                                             \
    buf[cnt++] = 'u';                                                                              \
    buf[cnt++] = hex_digits[((_byte32) >> 12) & 0x0F]; /* 4th hex digit */                         \
    buf[cnt++] = hex_digits[((_byte32) >> 8) & 0x0F];  /* 3rd hex digit */                         \
    buf[cnt++] = hex_digits[((_byte32) >> 4) & 0x0F];  /* 2nd hex digit */                         \
    buf[cnt++] = hex_digits[(_byte32) & 0x0F];         /* 1st hex digit */

    for (usize i = 0; i < slen; i++) {
        u32 c = s[i];
        if (unlikely(cnt > sizeof(buf) - 20)) {
            buf[cnt++] = '\0';
            if (jw->buf) {
                Exc err = sbuf.append(jw->buf, buf);
                if (unlikely(err != EOK && jw->error == EOK)) { jw->error = err; }
            } else if (jw->stream) {
                if (unlikely(fputs(buf, jw->stream) < 0 && jw->error == EOK)) {
                    jw->error = Error.io;
                }
            }
            buf[0] = '\0';
            cnt = 0;
        }

        switch (c) {
            case '"':
                buf[cnt++] = '\\';
                buf[cnt++] = '"';
                has_escape = true;
                break;
            case '\\':
                buf[cnt++] = '\\';
                buf[cnt++] = '\\';
                has_escape = true;
                break;
            case '\b':
                buf[cnt++] = '\\';
                buf[cnt++] = 'b';
                has_escape = true;
                break;
            case '\f':
                buf[cnt++] = '\\';
                buf[cnt++] = 'f';
                has_escape = true;
                break;
            case '\n':
                buf[cnt++] = '\\';
                buf[cnt++] = 'n';
                has_escape = true;
                break;
            case '\r':
                buf[cnt++] = '\\';
                buf[cnt++] = 'r';
                has_escape = true;
                break;
            case '\t':
                buf[cnt++] = '\\';
                buf[cnt++] = 't';
                has_escape = true;
                break;
            case '/':
                // Forward slash escape is optional but safe
                buf[cnt++] = '\\';
                buf[cnt++] = '/';
                has_escape = true;
                break;

            default:
                if (unlikely(c < 0x20 || c == 0x7F)) {
                    has_escape = true;
                    $tohex(c);
                } else if (unlikely(c >= 0x80)) {
                    has_escape = true;
                    // Non-ASCII: encode as UTF-8 or \uXXXX
                    // For simplicity, we'll encode all non-ASCII as \uXXXX
                    // This is inefficient but safe
                    if ((c & 0xC0) == 0xC0) { // UTF-8 start byte
                        // Try to extract full UTF-8 codepoint
                        u32 codepoint = 0;
                        u32 seq_len = 0;

                        // Determine sequence length
                        if ((c & 0xF8) == 0xF0) {
                            seq_len = 4;
                        } else if ((c & 0xF0) == 0xE0) {
                            seq_len = 3;
                        } else if ((c & 0xE0) == 0xC0) {
                            seq_len = 2;
                        }

                        if (seq_len > 0) {
                            // Extract codepoint from UTF-8
                            codepoint = c & (0xFF >> (seq_len + 1));
                            if (i + seq_len <= slen) {
                                for (u32 k = 1; k < seq_len && s[i + k]; k++) {
                                    u8 next = s[i + k];
                                    if ((next & 0xC0) != 0x80) {
                                        break; // Invalid
                                    }
                                    codepoint = (codepoint << 6) | (next & 0x3F);
                                }

                                // Write \uXXXX or \uXXXX\uXXXX for > 0xFFFF
                                if (codepoint <= 0xFFFF) {
                                    $tohex(codepoint);
                                    i += seq_len - 1; // Skip continuation bytes
                                } else {
                                    // UTF-16 surrogate pair for > 0xFFFF
                                    codepoint -= 0x10000;
                                    u32 high = 0xD800 | (codepoint >> 10);
                                    u32 low = 0xDC00 | (codepoint & 0x3FF);
                                    $tohex(high);
                                    $tohex(low);
                                    i += seq_len - 1;
                                }
                            } else {
                                if (!jw->error) { jw->error = JsonError.encoding; }
                            }
                        } else {
                            // Invalid UTF-8, escape as raw byte
                            if (!jw->error) { jw->error = JsonError.encoding; }
                            $tohex(c);
                        }
                    } else if ((c & 0xC0) == 0x80) {
                        // UTF-8 continuation byte without start - invalid
                        if (!jw->error) { jw->error = JsonError.encoding; }
                        $tohex(c);
                    } else {
                        // Single byte > 0x7F but < 0xC0 (shouldn't happen in valid UTF-8)
                        if (!jw->error) { jw->error = JsonError.encoding; }
                        buf[cnt++] = c;
                    }
                } else {
                    // Regular ASCII printable character
                    buf[cnt++] = c;
                }
                break;
        }
    }

    if (add_quotes) {
        buf[cnt++] = '"';
    } else {
        if (has_escape && !jw->error) { jw->error = JsonError.encoding; }
    }

    buf[cnt++] = '\0';

    if (jw->buf) {
        Exc err = sbuf.append(jw->buf, buf);
        if (unlikely(err != EOK && jw->error == EOK)) { jw->error = err; }
    } else if (jw->stream) {
        if (unlikely(fputs(buf, jw->stream) < 0 && jw->error == EOK)) { jw->error = Error.io; }
    }

#undef $tohex
}

/**
 * @brief Prints formatted raw data into json. IMPORTANT: it can wreck your formatting, use with
 * care, this call does not escape strings.
 *
 *
 * @param jw
 * @param format
 */
static void
cex_json__wr__print_val(json_wr_c* jw, char* format, ...)
{
    u8 last_scope = $last_scope(jw);

    uassertf(
        !(last_scope & $scope_null) || !(last_scope & $scope_has_items),
        "Only one json$wr_val() is allowed in null scope"
    );
    uassert(format);

    if (!(last_scope & $scope_has_key)) {
        uassertf(
            !(last_scope & $scope_obj),
            "Writing json$wr_val() without setting json$wr_key() before"
        );
        _cex_json_writer_indent(jw, false);
    }

    if (format[0] == '"') {
        if (format[2] == 's') {
            va_list va = { 0 };
            va_start(va, format);
            char* s = va_arg(va, char*); // NOLINT
            if (s == NULL) {
                $print("null");
            } else {
                cex_json__wr__print_str_escaped(jw, s, str.len(s), true);
            }
            va_end(va);
        } else if (format[2] == 'S') {
            va_list va = { 0 };
            va_start(va, format);
            str_s s = va_arg(va, str_s); // NOLINT
            if (s.buf == NULL) {
                $print("null");
            } else {
                cex_json__wr__print_str_escaped(jw, s.buf, s.len, true);
            }
            va_end(va);
        } else {
            unreachable();
        }
    } else if (format[1] == 'B') {
        va_list va = { 0 };
        va_start(va, format);
        bool v = va_arg(va, int); // NOLINT

        if (v) {
            $print("true");
        } else {
            $print("false");
        }

        va_end(va);
    } else {
        $printva();
    }

    if (jw->scope_depth && jw->scope_stack[jw->scope_depth - 1]) {
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_items;
        jw->scope_stack[jw->scope_depth - 1] &= ~$scope_has_key;
    }
}

/**
 * @brief Print json "key": part, expected to be used with json.wr.print_val() at the next step.
 * Escapes unicode/binary content in a key.
 *
 * @param jw
 * @param key
 */
static void
cex_json__wr__print_key(json_wr_c* jw, char* key)
{
    uassertf(
        jw->scope_depth > 0 && jw->scope_stack[jw->scope_depth - 1] & $scope_obj,
        "Expected to be in json object scope"
    );
    _cex_json_writer_indent(jw, false);
    if (unlikely(key == NULL)) {
        $print("null");
        if (!jw->error) { jw->error = JsonError.null_field; }
    } else {
        cex_json__wr__print_str_escaped(jw, key, strlen(key), !jw->simplified);
    }

    $print(": ");
    if (jw->scope_depth && jw->scope_stack[jw->scope_depth - 1]) {
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_items;
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_key;
    }
}

/**
 * @brief Low-level alternative for json$wr_scope(jw, scope_type) macro. Enters json scope_type of
 * [] or {}, adds indent if necessary, pair with json.wr.print_scope_exit().
 *
 * @param jw
 * @param scope_type JsonType__obj or JsonType__arr
 * @return
 */
static json_wr_c*
cex_json__wr__print_scope_enter(json_wr_c* jw, JsonType_e scope_type)
{
    u8 last_scope = $last_scope(jw);

    if (!(last_scope & $scope_has_key)) {
        uassertf(
            !(last_scope & $scope_obj),
            "Entering json$wr_scope() value without setting json$wr_key() before"
        );
        if (last_scope & $scope_has_items) { _cex_json_writer_indent(jw, false); }
    }
    u8 scope = 0;

    if (scope_type == JsonType__obj) {
        $print("%c", '{');
        scope = $scope_obj;
    } else if (scope_type == JsonType__arr) {
        $print("%c", '[');
        scope = $scope_arr;
    } else if (scope_type == JsonType__null) {
        scope = $scope_null;
    } else {
        unreachable();
    }

    if (jw->scope_depth <= sizeof(jw->scope_stack) - 1) {
        jw->scope_stack[jw->scope_depth] = scope;
        jw->scope_depth++;
    } else {
        jw->error = "Scope overflow";
    }

    jw->indent += jw->indent_width;
    if (scope == $scope_null) {
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_key;
    } else {
        jw->scope_stack[jw->scope_depth - 1] &= ~$scope_has_key;
    }

    return jw;
}

/**
 * @brief Low-level alternative for json$wr_scope(jw, scope_type) macro. Exits json scope_type of
 * [] or {}, de-dents if necessary, pair with json.wr.print_scope_enter().
 *
 * @param jwptr, typically result of json.wr.print_scope_enter()
 */
void
cex_json__wr__print_scope_exit(json_wr_c** jwptr)
{
    uassert(*jwptr != NULL);
    json_wr_c* jw = *jwptr;

    if (jw->indent >= jw->indent_width) { jw->indent -= jw->indent_width; }
    if (jw->scope_depth > 0) {
        u8 scope = jw->scope_stack[jw->scope_depth - 1];

        if (!(scope & $scope_null)) {
            _cex_json_writer_indent(jw, true);
            $print("%c", (scope & $scope_arr) ? ']' : '}');
        }
        jw->scope_depth--;
    } else {
        jw->error = "Scope overflow";
    }
}

/**
 * @brief Validates json writer state
 *
 * @param jw
 * @return
 */
static Exception
cex_json__wr__validate(json_wr_c* jw)
{
    if (jw == NULL) { return Error.argument; }
    return jw->error;
}

/**
 * @brief JSON Generator in can parse all json$$struct() inside source code and automatically
 * generate serialization/deserialization/print code for making your structures compatible with
 * JSON.
 *
 * @param self
 * @param allc
 * @param kwargs, optional arguments, can be NULL
 * @return
 */
Exception
cex_json__gen__create(json_gen_c* self, IAllocator allc, json_gen_kw* kwargs)
{
    uassert(self);
    uassert(allc->meta.is_arena && "Expected arena allocator");
    u32 def_initial_capacity = 100 * 1024;
    char* def_namespace = "serde";
    char* def_workdir = ".";
    char* def_outdir = NULL;

    if (kwargs) {
        if (kwargs->out_namespace) { def_namespace = kwargs->out_namespace; }
        if (kwargs->buf_initial_capacity) { def_initial_capacity = kwargs->buf_initial_capacity; }
        if (kwargs->workdir) { def_workdir = kwargs->workdir; }
        if (kwargs->out_dir) { def_outdir = kwargs->out_dir; }
    }
    if (!def_outdir) { def_outdir = def_workdir; }

    auto fstats = os.fs.stat(def_workdir);
    if (!fstats.is_valid) {
        return e$raise(Error.not_found, "Working directory not exists: `%s`", def_workdir);
    }
    if (!fstats.is_directory) {
        return e$raise(Error.argument, "Expected directory, got file? `%s`", def_workdir);
    }

    *self = (json_gen_c){
        .allc = allc,
        .types = hm$new(self->types, allc, .capacity = 256),
        .includes = arr$new(self->includes, allc, .capacity = 16),
        .namespace = def_namespace,
        .c_file_content = sbuf.create(def_initial_capacity, allc),
        .h_file_content = sbuf.create(def_initial_capacity, allc),
        .target = os$path_join(allc, def_workdir, "*.h"),
        .c_out_name = os$path_join(allc, def_outdir, str.fmt(allc, "%s.c", def_namespace)),
        .h_out_name = os$path_join(allc, def_outdir, str.fmt(allc, "%s.h", def_namespace)),
    };

    return EOK;
}


Exception
_cex_json__gen___process_field_attr(
    json_gen_c* self,
    CexParser_c* lx,
    json_gen_field_s* field,
    cex_token_s t
)
{
    uassert(t.type == CexTkn__ident);

    (void)lx;
    (void)field;
    (void)t;
    if (str.slice.eq(t.value, str$s("json$$field"))) {
        str_s attr_name = t.value;
        log$info("Processing %S\n", t.value);
        while ((t = CexParser.next_token(lx)).type) {
            if (t.type == CexTkn__dot) {
                t = CexParser.next_token(lx);
                if (t.type != CexTkn__ident) {
                    return e$raise(Error.integrity, "cex$$attr expected identifier after dot");
                }
                str_s kw = t.value;

                t = CexParser.next_token(lx);
                if (t.type != CexTkn__eq) {
                    return e$raise(Error.integrity, "cex$$attr expected `=` after `.%S`", kw);
                }

                t = CexParser.next_token(lx);
                if (str$eq(kw, "nullable")) {
                    if (str$eq(t.value, "true")) {
                        field->flags.is_nullable = true;
                    } else if (str$eq(t.value, "false")) {
                        field->flags.is_nullable = false;
                    } else {
                        return e$raise(
                            Error.integrity,
                            "Expected .nulllable = true|false in %S, got `%S`",
                            attr_name,
                            t.value
                        );
                    }
                } else if (str$eq(kw, "optional")) {
                    if (str$eq(t.value, "true")) {
                        field->flags.is_optional = true;
                    } else if (str$eq(t.value, "false")) {
                        field->flags.is_optional = false;
                    } else {
                        return e$raise(
                            Error.integrity,
                            "Expected .optional = true|false in %S, got `%S`",
                            attr_name,
                            t.value
                        );
                    }
                } else if (str$eq(kw, "skip")) {
                    if (str$eq(t.value, "true")) {
                        field->flags.is_skipped = true;
                    } else if (str$eq(t.value, "false")) {
                        field->flags.is_skipped = false;
                    } else {
                        return e$raise(
                            Error.integrity,
                            "Expected .skip = true|false in %S, got `%S`",
                            attr_name,
                            t.value
                        );
                    }
                } else if (str$eq(kw, "name")) {
                    if (t.type == CexTkn__string && t.value.len > 0) {
                        e$except_null (field->json_name = str.slice.clone(t.value, self->allc)) {
                            return Error.memory;
                        }
                    } else {
                        return e$raise(
                            Error.integrity,
                            "Expected .name = \"string_name\" in %S, got `%S`",
                            attr_name,
                            t.value
                        );
                    }
                } else {
                    return e$raise(
                        Error.integrity,
                        "Unknown param_field: .%S in `.%S`",
                        kw,
                        attr_name
                    );
                }
            } else if (t.type == CexTkn__rparen) {
                t = CexParser.next_token(lx);
                e$assertf(t.type == CexTkn__eos, "Missing semicolon after cex$$attr field");

                for (t = CexParser.next_token(lx);
                     t.type == CexTkn__comment_single || t.type == CexTkn__comment_multi;
                     t = CexParser.next_token(lx)) {}

                e$assertf(t.type == CexTkn__ident, "Expected identifier after cex$$attr field");
                break;
            } else if (t.type == CexTkn__comma || t.type == CexTkn__lparen) {
                continue;
            } else {
                return e$raise(
                    Error.integrity,
                    "Unexpected token (%s) in %S",
                    CexTkn_str[t.type],
                    t.value
                );
            }
        }
    }


    cex_token_s prev_t = t;
    field->type = str.sstr(str.slice.clone(t.value, self->allc));
    if (str$eq(field->type, "sbuf_c") || str$eq(field->type, "str_s")) {
        field->flags.is_string = true;
    }

    while ((t = CexParser.next_token(lx)).type) {
        if (t.type == CexTkn__error) { return Error.integrity; }

        switch (t.type) {
            case CexTkn__eos: {
                e$assert(prev_t.type == CexTkn__ident);
                e$except_null (field->name = str.slice.clone(prev_t.value, self->allc)) {
                    return Error.memory;
                }
                goto end;
            } break;
            case CexTkn__star: {
                field->flags.is_ptr = true;
                if (str$eq(field->type, "char")) { field->flags.is_string = true; }
            } break;
            case CexTkn__ident:
                break;
            default: {
                e$assertf(false, "Unsupported token: %s\n", CexTkn_str[t.type]);
            }
        }


        prev_t = t;
    }

end:
    if (!field->json_name) { field->json_name = field->name; }
    log$info("New field: name=%s, type=%S\n", field->name, field->type);

    return EOK;
}

Exception
_cex_json__gen__codegen_serialize_field(json_gen_c* self, cex_codegen_s* cg$var, json_gen_field_s* f)
{
    (void)self;
    (void)f;
    e$assert(f->type.buf && f->type.len != 0);
    cg$pf("json$wr_key(\"%s\");", f->json_name);

    json_gen_type_s* field_type = hm$get(self->types, f->type);
    if (field_type) {
        // Project Type
        if (!f->flags.is_nullable) {
            cg$if ("unlikely(!item->%s)", f->name) { cg$pf("jw->error = JsonError.null_field;"); }
        } else {
            cg$pf("// field `%s` is nullable json$$field(.nullable = true)", f->name);
        }
        cg$scope ("e$except_silent (err, %s.%s.serialize(jw, %sitem->%s)) ",
                  self->namespace,
                  field_type->name,
                  f->flags.is_ptr ? "" : "&",
                  f->name) {
            cg$pn("jw->error = err;");
        }
    } else if (f->flags.is_string) {
        if (!f->flags.is_nullable) {
            cg$if ("unlikely(!item->%s%s)", f->name, (str$eq(f->type, "str_s") ? ".buf" : "")) {
                cg$pf("jw->error = JsonError.null_field;");
            }
        } else {
            cg$pf("// field `%s` is nullable json$$field(.nullable = true)", f->name);
        }
        cg$pf("json$wr_val(item->%s);", f->name);
    } else {
        // Primitive type
        cg$pf("json$wr_val(item->%s);", f->name);
    }
    cg$pn("");

    return EOK;
}

Exception
_cex_json__gen__codegen_deserialize_field(
    json_gen_c* self,
    cex_codegen_s* cg$var,
    json_gen_field_s* f,
    u32* out_field_idx
)
{
    (void)self;
    (void)f;
    e$assert(f->type.buf && f->type.len != 0);

    cg$elseif ("str$eq(k, \"%s\")", f->json_name) {
        if (!f->flags.is_optional) {
            cg$pf("fields_mask |= (1 << %d);", *out_field_idx);
            *out_field_idx += 1;
        } else {
            cg$pf("// fields_mask check skipped, field is json$$field(.optional = true)");
        }
        json_gen_type_s* field_type = hm$get(self->types, f->type);
        if (field_type) {
            // Project registered type
            // We need preallocate pointer to a new type!
            cg$if ("jr->type != JsonType__null") {
                cg$if ("jr->type != JsonType__obj") {
                    cg$pn("json$rd_egoto(jr, JsonError.wrong_type, fail);");
                }
                if (f->flags.is_ptr) {
                    cg$pf("out_item->%s = mem$new(allc, %S);", f->name, f->type);
                    cg$if ("!out_item->%s", f->name) { cg$pn("return Error.memory;"); }
                    cg$pf(
                        "json$rd_egoto(jr, %s.%s.deserialize(jr, out_item->%s, allc), fail);",
                        self->namespace,
                        field_type->name,
                        f->name
                    );
                } else {
                    cg$pf(
                        "json$rd_egoto(jr, %s.%s.deserialize(jr, &out_item->%s, allc), fail);",
                        self->namespace,
                        field_type->name,
                        f->name
                    );
                }
            }
            cg$else () {
                if (f->flags.is_ptr) {
                    if (!f->flags.is_nullable) {
                        cg$pf("json$rd_egoto(jr, JsonError.null_field, fail);");
                    } else {
                        if (!f->flags.is_nullable) {
                            cg$pf("json$rd_egoto(jr, JsonError.null_field, fail);");
                        } else {
                            cg$pf(
                                "// field `%s` is nullable json$$field(.nullable = true)",
                                f->name
                            );
                            cg$pf("out_item->%s = NULL;", f->name);
                        }
                    }
                } else {
                    cg$pf("json$rd_egoto(jr, JsonError.null_field, fail);");
                }
            }
        } else if (f->flags.is_string) {
            cg$if ("jr->type != JsonType__str && jr->type != JsonType__null") {
                cg$pn("json$rd_egoto(jr, JsonError.wrong_type, fail);");
            }

            if (str$eq(f->type, "char")) {
                uassert(f->flags.is_ptr);
                cg$if ("unlikely(!v.buf)") {
                    if (!f->flags.is_nullable) {
                        cg$pf("json$rd_egoto(jr, JsonError.null_field, fail);");
                    } else {
                        cg$pf("// field `%s` is nullable json$$field(.nullable = true)", f->name);
                        cg$pf("out_item->%s = NULL;", f->name);
                    }
                }
                cg$else () {
                    cg$pn("str_s out_s = {0};");
                    cg$pn("json$rd_egoto(jr, json$rd_str_unescape(v, &out_s, allc), fail);");
                    cg$pf("out_item->%s = out_s.buf;", f->name);
                }

            } else if (str$eq(f->type, "sbuf_c")) {
                cg$if ("unlikely(!v.buf)") {
                    if (!f->flags.is_nullable) {
                        cg$pf("json$rd_egoto(jr, JsonError.null_field, fail);");
                    } else {
                        cg$pf("// field `%s` is nullable json$$field(.nullable = true)", f->name);
                        cg$pf("out_item->%s = NULL;", f->name);
                    }
                }
                cg$else () {
                    cg$pf(
                        "out_item->%s = sbuf.create(v.len + sizeof(sbuf_head_s) + 1, allc);",
                        f->name
                    );
                    cg$if ("unlikely(!out_item->%s)", f->name) { cg$pn("return Error.memory;"); }
                    cg$pf("usize out_buf_len = v.len + 1;", f->name);
                    cg$pf(
                        "json$rd_egoto(jr, json$rd_str_unescape_inplace(v, out_item->%s, &out_buf_len), fail);",
                        f->name
                    );
                    cg$pf(
                        "json$rd_egoto(jr, sbuf.set_len(&out_item->%s, out_buf_len), fail);",
                        f->name
                    );
                }

            } else if (str$eq(f->type, "str_s")) {
                cg$if ("unlikely(!v.buf)") {
                    if (!f->flags.is_nullable) {
                        cg$pf("json$rd_egoto(jr, JsonError.null_field, fail);");
                    } else {
                        cg$pf("// field `%s` is nullable json$$field(.nullable = true)", f->name);
                        cg$pf("out_item->%s = (str_s){0};", f->name);
                    }
                }
                cg$else () {
                    cg$pf(
                        "json$rd_egoto(jr, json$rd_str_unescape(v, &out_item->%s, allc), fail);",
                        f->name
                    );
                }

            } else {
                uassertf(false, "field type, not implemented yet: type=%S\n", f->type);
            }


        } else {
            // Primitive type
            cg$if ("!json$rd_is_type_compatible(jr, &out_item->%s)", f->name) {
                cg$pn("json$rd_egoto(jr, JsonError.wrong_type, fail);");
            }
            cg$pf("json$rd_egoto(jr, str$convert(v, &out_item->%s), fail);", f->name);
        }
        // cg$pn("");
    }

    return cg$var->error;
}

Exception
_cex_json__gen__codegen_destroy_field(json_gen_c* self, cex_codegen_s* cg$var, json_gen_field_s* f)
{
    (void)self;
    (void)f;
    e$assert(f->type.buf && f->type.len != 0);

    json_gen_type_s* field_type = hm$get(self->types, f->type);
    if (field_type) {
        if (f->flags.is_ptr) {
            cg$pf("%s.%s.destroy(item->%s, allc);", self->namespace, field_type->name, f->name);
            cg$pf("mem$free(allc, item->%s);", f->name);
        } else {
            cg$pf("%s.%s.destroy(&item->%s, allc);", self->namespace, field_type->name, f->name);
        }
    } else if (f->flags.is_string) {
        if (str$eq(f->type, "char")) {
            cg$pf("mem$free(allc, item->%s);", f->name);
        } else if (str$eq(f->type, "sbuf_c")) {
            cg$pf("sbuf.destroy(&item->%s);", f->name);
        } else if (str$eq(f->type, "str_s")) {
            cg$pf("mem$free(allc, item->%s.buf);", f->name);
        } else {
            uassertf(false, "field type, not implemented yet: type=%S\n", f->type);
        }
    } else {
        // Primitive type do nothing
    }

    return cg$var->error;
}

Exception
_cex_json__gen__generate_type(json_gen_c* self, cex_codegen_s* cg$var, json_gen_type_s* t)
{
    cg$pn("");
    cg$pn("//");
    cg$pf("// Autogenerated serde for %s type", t->name);
    cg$pn("//");


    //
    // serialize codegen
    //
    cg$func ("Exception %s__%s__serialize(json_wr_c* jw, %s* item) ",
             self->namespace,
             t->ns_name,
             t->name) {
        cg$if ("!item") {
            cg$scope ("json$wr_scope(jw, JsonType__null)") { cg$pn("json$wr_val(NULL);"); }
            cg$pn("return EOK;");
        }

        cg$scope ("json$wr_scope(jw, JsonType__obj)") {
            for$each (it, t->fields) {
                if (!it->flags.is_skipped) {
                    e$ret(_cex_json__gen__codegen_serialize_field(self, cg$var, it));
                } else {
                    cg$pf("// field `%s` is skipped json$$field(.skip = true)", it->name);
                }
            }
        }

        cg$pn("return jw->error;");
    }

    //
    // print codegen
    //
    cg$func ("Exc %s__%s__print(%s* item, json_wr_kw* json_writer_kwargs) ",
             self->namespace,
             t->ns_name,
             t->name) {
        cg$pn("json_wr_c jw;");
        cg$pn("json_wr_kw kwargs = {.stream = stdout, .indent = 0, .simplified = true};");
        cg$if ("json_writer_kwargs") {
            cg$pn("kwargs = *json_writer_kwargs;");
            cg$if ("!kwargs.stream && !kwargs.buf") { cg$pn("kwargs.stream = stdout;"); }
        }
        cg$pn("e$ret(json.wr.create(&jw, &kwargs));");
        cg$if ("kwargs.simplified") {
            cg$pf("json.wr.print_val(&jw, \"%s(\");", t->name);
            cg$pf("Exc err = %s.%s.serialize(&jw, item);", self->namespace, t->ns_name);
            cg$pf(
                "json.wr.print_val(&jw, \"%%s%%s%%s)\\n\", (err) ? \" [error: \": \"\", (err) ? err : \"\", (err) ? \"]\": \"\" );",
                t->name
            );
            cg$pn("return err;");
        }
        cg$else () { cg$pf("return %s.%s.serialize(&jw, item);", self->namespace, t->ns_name); }
    }

    //
    // deserialize codegen
    //
    cg$func ("Exception %s__%s__deserialize(json_rd_c* jr, %s* out_item, IAllocator allc) ",
             self->namespace,
             t->ns_name,
             t->name) {
        cg$pn("uassert(jr != NULL);");
        cg$pn("uassert(out_item != NULL);");

        cg$pn("u64 fields_mask = 0;");
        u32 nfields = 0;

        cg$scope ("json$rd_foreach(k, v, jr) ") {
            cg$if ("!k.buf") {
                cg$pn("jr->error = JsonError.parsing;");
                cg$pn("goto fail;");
            }
            for$each (it, t->fields) {
                if (!it->flags.is_skipped) {
                    e$ret(_cex_json__gen__codegen_deserialize_field(self, cg$var, it, &nfields));
                } else {
                    cg$pf("// field `%s` is skipped json$$field(.skip = true)\n       ", it->name);
                }
            }
            cg$else () {
                cg$pn("jr->error = JsonError.unknown_field;");
                cg$pn("goto fail;");
            }
        }
        if (nfields >= 64) {
            return e$raise(
                Error.overflow,
                "Struct [%s] has more than 64 fields, try to split it into sub-types",
                t->name
            );
        }
        cg$if ("fields_mask != ((1 << %d) - 1)", nfields) {
            cg$if ("!jr->error") { cg$pn("jr->error = JsonError.missing_field;"); }
            cg$pn("goto fail;");
        }

        cg$if ("!jr->error") { cg$pn("return EOK;"); }

        cg$dedent();
        cg$pn("fail: ");
        cg$indent();
        cg$pf("%s.%s.destroy(out_item, allc);", self->namespace, t->ns_name);
        cg$pn("return jr->error;");
    }

    //
    // destroy codegen
    //
    cg$func ("void %s__%s__destroy(%s* item, IAllocator allc) ", self->namespace, t->ns_name, t->name) {
        cg$pn("uassert(allc != NULL);");
        cg$if ("item") {
            for$each (it, t->fields) {
                e$ret(_cex_json__gen__codegen_destroy_field(self, cg$var, it));
            }
            cg$pn("memset(item, 0, sizeof(*item));");
        }
    }

    return cg$var->error;
}

/**
 * @brief (low-level) Generates a content of a json serde engine and stores it in `self` sbuf.
 *
 * @param self
 * @return
 */
Exception
cex_json__gen__generate_full(json_gen_c* self)
{
    cg$init_scope(&self->h_file_content)
    {
        cg$pf("// Autogenerated serde engine by CEX");
        cg$pn("// DO NOT EDIT");
        cg$pn("//");
        cg$pn("#include \"cex.h\"");
        cg$pn("#include \"cexstd/json/json.h\"");
        for$each (it, self->includes) { cg$pf("#include \"%s\"", it); }

        cg$pn("");
        cg$pf("/// Generic `%s` type printer using json", self->namespace);
        cg$pf("/// Example: ");
        cg$pf(
            "/// %s$print(any_supported_type_pointer, .indent = 0, .simplified = true ); ",
            self->namespace
        );
        cg$pf("#define %s$print(item, kwargs...) \\", self->namespace);
        cg$pf("    _Generic((item), \\");

        for (u32 i = 0; i < arr$len(self->types); i++) {
            if (i > 0) { cg$pa(", \\\n"); }
            cg$pf(
                "        %s*: %s.%s.print",
                self->types[i].value->name,
                self->namespace,
                self->types[i].value->ns_name
            );
        }
        cg$pa("\\\n");
        cg$pf("    )(item, &(json_wr_kw) { kwargs })");
        cg$pn("");

        if (cg$var->error) { return cg$var->error; }
    }

    cg$init_scope(&self->c_file_content)
    {
        cg$pf("// Autogenerated serde engine by CEX");
        cg$pn("// DO NOT EDIT");
        cg$pn("//");
        cg$pf("#include \"%s.h\"", str.fmt(self->allc, self->namespace));

        for$each (it, self->types, arr$len(self->types)) {
            log$info("Type: %s #%d fields\n", it.value->name, arr$len(it.value->fields));
            e$ret(_cex_json__gen__generate_type(self, cg$var, it.value));
        }
        if (cg$var->error) { return cg$var->error; }
    }


    return EOK;
}

Exception
_cex_json__gen__process_decl(json_gen_c* self, CexParser_c* lx, cex_decl_s* d, bool* has_serde)
{
    uassert(self);
    uassert(lx);
    uassert(d);
    uassert(has_serde);

    *has_serde = false;

    if (d->type == CexTkn__typedef && d->attr_count > 0) {
        for$each (attr, d->attr, d->attr_count) {
            if (!str.slice.starts_with(attr, str$s("json$$struct"))) { continue; }
            log$info("Got item: type=%s Name: %S Attr: %S\n", CexTkn_str[d->type], d->name, attr);

            json_gen_type_s* stype = mem$new(self->allc, json_gen_type_s);
            if (!stype) { return Error.memory; }

            stype->fields = arr$new(stype->fields, self->allc);
            if (!stype->fields) { return Error.memory; }

            stype->name = str.slice.clone(d->name, self->allc);
            if (!stype->name) { return Error.memory; }

            if(d->name.len > 2 && d->name.buf[d->name.len - 2] == '_') {
                // Strip _s suffixes
                stype->ns_name = str.slice.clone(str.slice.sub(d->name, 0, -2), self->allc);
            } else {
                stype->ns_name = str.slice.clone(d->name, self->allc);
            }
            if (!stype->ns_name) { return Error.memory; }

            // TODO: parse json$$struct here for params

            CexParser_c sp = CexParser.create(d->body.buf, d->body.len, false);
            cex_token_s t = CexParser.next_token(&sp);
            e$assert(t.type == CexTkn__lbrace && "Expected { scope start");

            log$info("Type breakdown\n");
            while ((t = CexParser.next_token(&sp)).type) {
                if (t.type == CexTkn__error) {
                    log$error(CexParser$err_fmt(&sp, NULL));
                    return Error.integrity;
                }
                // log$info("Tok: type=%s Name: %S\n", CexTkn_str[t.type], t.value);
                if (t.type == CexTkn__ident) {
                    json_gen_field_s* field = mem$new(self->allc, json_gen_field_s);
                    if (!field) { return Error.memory; }
                    e$goto(_cex_json__gen___process_field_attr(self, &sp, field, t), fail);
                    arr$push(stype->fields, field);
                } else if (t.type == CexTkn__comment_multi || t.type == CexTkn__comment_single) {
                    continue;
                } else if (t.type == CexTkn__rbrace) {
                    t = CexParser.next_token(&sp);
                    if (t.type != CexTkn__eof) { goto fail; }
                    break;
                }
            }

            if (hm$getp(self->types, str.sstr(stype->name))) {
                return e$raise(Error.exists, "Duplicate json$$struct type name: %s", stype->name);
            }

            if (!hm$set(self->types, str.sstr(stype->name), stype)) { return Error.memory; }
            // hm$set(self->types, stype->name, stype);

            *has_serde = true;
            break;
        }
    }
    return EOK;

fail:
    log$error("Error processing json$$struct\n%s %S\n%S\n", d->ret_type, d->name, d->body);
    return Error.integrity;
}

/**
 * @brief (low-level) Parses the .h file and add all json$$struct() types into serialization pipeline
 *
 * @param self
 * @param path
 * @return
 */
Exception
cex_json__gen__process_file(json_gen_c* self, char* path)
{
    uassert(self);
    uassert(path);
    mem$arena(256 * 1024, _)
    {
        char* code = io.file.load(path, _);
        if (!code) { return e$raise(Error.io, "Error reading file: %s", path); }

        arr$(cex_token_s) items = arr$new(items, _);
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        bool has_serializable = false;
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__error) {
                log$error(CexParser$err_fmt(&lx, NULL));
                return lx.error;
            }

            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            bool has_serde = false;
            e$ret(_cex_json__gen__process_decl(self, &lx, d, &has_serde));
            if (has_serde) { has_serializable = true; }
        }

        if (has_serializable) {
            uassert(str.ends_with(path, ".h"));
            arr$push(self->includes, path);
        }
    }

    return EOK;
}

/**
 * @brief Runs full JSON generation sequence: finds all header files in target directory, processes
 * all types with `json$$struct()` attributes, saves result to the out dir and makes a namespace.
 *
 * @param self
 * @return
 */
Exception
cex_json__gen__run(json_gen_c* self)
{
    uassert(sbuf.len(&self->c_file_content) == 0 && "Already processed");

    for$each (src_fn, os.fs.find(self->target, true, self->allc)) {
        io.printf("file: %s\n", src_fn);
        e$ret(json.gen.process_file(self, src_fn));
    }

    if (arr$len(self->types) == 0) {
        log$info(
            "CexSerdeGen: no json$$struct types found in workdir: `%s`, skipping...",
            self->workdir
        );
        return EOK;
    }

    e$ret(json.gen.generate_full(self));

    // Check if output file is exists and it's a really CexSerdeGen generated
    mem$scope(tmem$, _)
    {
        if (os.path.exists(self->c_out_name)) {
            char* content = io.file.load(self->c_out_name, _);
            if (!str.starts_with(content, "// Autogenerated serde engine by CEX")) {
                return e$raise(
                    Error.integrity,
                    "File `%s` was not generated by CexSerdeGen, exiting",
                    self->c_out_name
                );
            }
        }
        if (os.path.exists(self->h_out_name)) {
            char* content = io.file.load(self->h_out_name, _);
            if (!str.starts_with(content, "// Autogenerated serde engine by CEX")) {
                return e$raise(
                    Error.integrity,
                    "File `%s` was not generated by CexSerdeGen, exiting",
                    self->h_out_name
                );
            }
        }
    }

    e$ret(io.file.save(self->c_out_name, self->c_file_content));
    e$ret(io.file.save(self->h_out_name, self->h_file_content));

    // This is only available when json.gen. is called inside build system
#if defined(CEX_BUILD)
    char* argv[] = { "process", self->c_out_name };
    e$ret(cexy.cmd.process(arr$len(argv), argv, NULL));
#endif

    return EOK;
}


#undef $next_tok /* TEMP MACRO */
#undef $print
#undef $printva
#undef $scope_obj
#undef $scope_arr
#undef $scope_has_items
#undef $scope_has_key
#undef $last_scope

const struct __cex_namespace__json json = {
    // Autogenerated by CEX
    // clang-format off


    .gen = {
        .create = cex_json__gen__create,
        .generate_full = cex_json__gen__generate_full,
        .process_file = cex_json__gen__process_file,
        .run = cex_json__gen__run,
    },

    .rd = {
        .create = cex_json__rd__create,
        .get_scope = cex_json__rd__get_scope,
        .next = cex_json__rd__next,
        .skip = cex_json__rd__skip,
        .step_in = cex_json__rd__step_in,
        .step_out = cex_json__rd__step_out,
        .str_unescape = cex_json__rd__str_unescape,
        .str_unescape_inplace = cex_json__rd__str_unescape_inplace,
        .validate = cex_json__rd__validate,
    },

    .wr = {
        .create = cex_json__wr__create,
        .print = cex_json__wr__print,
        .print_key = cex_json__wr__print_key,
        .print_scope_enter = cex_json__wr__print_scope_enter,
        .print_scope_exit = cex_json__wr__print_scope_exit,
        .print_str_escaped = cex_json__wr__print_str_escaped,
        .print_val = cex_json__wr__print_val,
        .validate = cex_json__wr__validate,
    },

    // clang-format on
};
