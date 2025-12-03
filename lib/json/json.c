#include "json.h"

/* TEMP MACROS - for private implementation*/
#define $scope_obj (1 << 1)
#define $scope_arr (1 << 2)
#define $scope_has_items (1 << 3)
#define $scope_has_key (1 << 4)
#define $last_scope(jw)                                                                            \
    ((jw)->scope_depth && (jw)->scope_stack[(jw)->scope_depth - 1])                                \
        ? (jw)->scope_stack[(jw)->scope_depth - 1]                                                 \
        : 0

#define $print(format, ...) /* temp macro */                                                       \
    ({                                                                                             \
        if (jw->error == EOK) {                                                                    \
            if (jw->buf) {                                                                         \
                Exc err = sbuf.appendf(&jw->buf, format, __VA_ARGS__);                             \
                if (unlikely(err != EOK && jw->error == EOK)) { jw->error = err; }                 \
            } else if (jw->stream) {                                                               \
                io.fprintf(jw->stream, format, __VA_ARGS__);                                       \
            }                                                                                      \
        }                                                                                          \
    })

#define $printva() /* temp macro! */                                                               \
    if (jw->error == EOK) {                                                                        \
        va_list va;                                                                                \
        va_start(va, format);                                                                      \
        if (jw->buf) {                                                                             \
            Exc err = sbuf.appendfva(&jw->buf, format, va);                                        \
            if (unlikely(err != EOK && jw->error != EOK)) { jw->error = err; }                     \
        } else if (jw->stream) {                                                                   \
            int result = cexsp__vfprintf(jw->stream, format, va);                                  \
            if (result == -1) { jw->error = Error.io; }                                            \
        }                                                                                          \
        va_end(va);                                                                                \
    }

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
 * @brief Create new JSON reader (it doesn't allocate memory and uses content slicing)
 *
 * @param it self instance (typically allocated on stack)
 * @param content  JSON content
 * @param content_len JSON content length (if 0 length will be recalculated via strlen())
 * @param strict_mode true - stick to JSON spec, false - allowing comments, trailing commas, nan
 * @return
 */
Exception
_cex_json__reader__create(jr_c* it, char* content, usize content_len, jr_kw* kwargs)
{
    uassert(it != NULL);
    if (content == NULL) { return Error.argument; }

    bool strict_mode = false;
    if (kwargs != NULL) { strict_mode = kwargs->strict_mode; }

    *it = (jr_c){
        ._impl = {
            .strict_mode = strict_mode,
            .lexer = CexParser.create(content, content_len, false),
        },
    };
    if (it->_impl.lexer.content == it->_impl.lexer.content_end) { return Error.empty; }
    _cex_json__reader__next(it);
    return EOK;
}

/**
 * @brief Make step inside JSON object or array scope (_cex_json__reader__next() starts emitting this scope)
 *
 * @param it
 * @param expected_type Expected scope type (for sanity checks)
 * @return
 */
Exception
_cex_json__reader__step_in(jr_c* it, JsonType_e expected_type)
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
        // return _cex_json__reader__next(it);
        it->error = "Stepping in is only for objects or arrays";
        goto error;
    }
    // _cex_json__reader__next() is going to check if we step in or skipping whole block
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

// /**
//  * @brief Early step out from JSON scope (you must immediately break the loop/func after step out)
//  *
//  * After calling step out, next call of `_cex_json__reader__next()` will return outer scope item,
//  * make sure that you also break the loop or exiting parsing function for current scope.
//  *
//  * @param it
//  * @return
//  */
// Exception
// cex_json__reader__step_out(jr_c* it)
// {
//     if (unlikely(it->_impl.scope_depth == 0)) {
//         it->error = "Bad scope/level for step out";
//         return it->error;
//     }
//     u32 scope_depth_initial = it->_impl.scope_depth - 1;
//     while (it->_impl.scope_depth > scope_depth_initial && _cex_json__reader__next(it)) {}
//     return it->error;
// }

static Exc
_cex_json__reader__skip(jr_c* it)
{
    // Simulate full step-in/next sequence for all nested stuff (because it serves as syntax check)
    u32 scope_depth_initial = it->_impl.scope_depth;
    if (_cex_json__reader__step_in(it, it->type)) { return it->error; }

    while (it->_impl.scope_depth > scope_depth_initial) {
        if (!_cex_json__reader__next(it) && it->error) { break; }
        switch (it->type) {
            case JsonType__arr:
            case JsonType__obj:
                if (_cex_json__reader__step_in(it, it->type)) { return it->error; }
            default:
                break;
        }
    }
    return it->error;
}

str_s _cex_json__reader__get_scope(jr_c* it, JsonType_e scope_type) {
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

    if(_cex_json__reader__skip(it) == EOK){
        char* last_cur = it->_impl.lexer.cur;
        uassert(last_cur > cur);
        uassert(last_cur < it->_impl.lexer.content_end);
        uassert(it->type == JsonType__eos);
        if (last_cur > cur) {
            // NOTE: we must have at least something in result,
            // valid .buf with .len=0, may lead to full text parse
            // if the jr$new()
            result = (str_s){.buf = cur, .len = last_cur - cur};
        }
    }

    return result;
}

bool
_cex_json__reader__next(jr_c* it)
{
    if (unlikely(it->error != EOK)) { goto error; }
    it->key = (str_s){ 0 };

    if (unlikely(
            it->_impl.curr_token == CexTkn__lbrace || it->_impl.curr_token == CexTkn__lbracket
        )) {
        // User didn't step into object/array, skipping it
        if (_cex_json__reader__skip(it) != EOK) { goto error; }
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
            it->val = t.value;
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
_cex_json_writer_indent(jw_c* jw, bool last_item)
{
    if (unlikely(jw->error != EOK)) { return; }
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

/**
 * @brief Create JSON buffer/builder container used with json$buf / json$fmt / json$kstr macros
 *
 * @param jw
 * @param capacity initial capacity of buffer (will be resized if not enough)
 * @param indent JSON indentation (0 - to produce minified version)
 * @param allc allocator for buffer
 * @return
 */
Exception
_cex_json__writer__create(jw_c* jw, jw_kw* kwargs)
{
    uassert(jw != NULL);
    uassert(kwargs != NULL);

    if (kwargs->buf == NULL && kwargs->stream == NULL) { return "Empty buf and stream kwargs"; }
    if (kwargs->buf != NULL && kwargs->stream != NULL) {
        return "buf and stream kwargs are mutually exclusive";
    }

    *jw = (jw_c){
        .indent_width = kwargs->indent,
        .buf = kwargs->buf,
        .stream = kwargs->stream,
    };

    return EOK;
}

void
_cex_json__writer__print(jw_c* jw, char* format, ...)
{
    u8 last_scope = $last_scope(jw);
    if (!(last_scope & $scope_has_key)) { _cex_json_writer_indent(jw, false); }

    $printva();

    if (jw->scope_depth && jw->scope_stack[jw->scope_depth - 1]) {
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_items;
        jw->scope_stack[jw->scope_depth - 1] &= ~$scope_has_key;
    }
}

void
_cex_json__writer__print_item(jw_c* jw, char* format, ...)
{
    u8 last_scope = $last_scope(jw);
    // if (!(last_scope & $scope_has_key)) { _cex_json_writer_indent(jw, false); }

    if (!(last_scope & $scope_has_key)) {
        uassertf(!(last_scope & $scope_obj), "Writing jw$val() without setting jw$key() before");
        _cex_json_writer_indent(jw, false);
    }

    // if (!(last_scope & $scope_has_key) && (last_scope & $scope_has_items)) {
    // _cex_json_writer_indent(jw, false); }
    $printva();
    if (jw->scope_depth && jw->scope_stack[jw->scope_depth - 1]) {
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_items;
        jw->scope_stack[jw->scope_depth - 1] &= ~$scope_has_key;
    }
}

void
_cex_json__writer__print_key(jw_c* jw, char* format, ...)
{
    uassertf(
        jw->scope_depth > 0 && jw->scope_stack[jw->scope_depth - 1] & $scope_obj,
        "Expected to be in json object scope"
    );
    _cex_json_writer_indent(jw, false);
    $print("\"", "");
    $printva();
    $print("\": ", "");
    if (jw->scope_depth && jw->scope_stack[jw->scope_depth - 1]) {
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_items;
        jw->scope_stack[jw->scope_depth - 1] |= $scope_has_key;
    }
}

jw_c*
_cex_json__writer__print_scope_enter(jw_c* jw, JsonType_e scope_type, bool should_indent)
{
    (void)should_indent;
    u8 last_scope = $last_scope(jw);

    if (!(last_scope & $scope_has_key)) {
        uassertf(
            !(last_scope & $scope_obj),
            "Entering jw$scope() value without setting jw$key() before"
        );
        if (last_scope & $scope_has_items) { _cex_json_writer_indent(jw, false); }
    }

    if (scope_type == JsonType__obj) {
        $print("%c", '{');
        if (jw->scope_depth <= sizeof(jw->scope_stack) - 1) {
            jw->scope_stack[jw->scope_depth] = $scope_obj;
            jw->scope_depth++;
        } else {
            jw->error = "Scope overflow";
        }
    } else if (scope_type == JsonType__arr) {
        $print("%c", '[');
        if (jw->scope_depth <= sizeof(jw->scope_stack) - 1) {
            jw->scope_stack[jw->scope_depth] = $scope_arr;
            jw->scope_depth++;
        } else {
            jw->error = "Scope overflow";
        }
    } else {
        unreachable();
    }
    jw->indent += jw->indent_width;
    jw->scope_stack[jw->scope_depth - 1] &= ~$scope_has_key;
    return jw;
}

void
_cex_json__writer__print_scope_exit(jw_c** jwptr)
{
    uassert(*jwptr != NULL);
    jw_c* jw = *jwptr;

    if (jw->indent >= jw->indent_width) { jw->indent -= jw->indent_width; }
    if (jw->scope_depth > 0) {
        _cex_json_writer_indent(jw, true);

        $print("%c", (jw->scope_stack[jw->scope_depth - 1] & $scope_arr) ? ']' : '}');
        jw->scope_depth--;
    } else {
        jw->error = "Scope overflow";
    }
}

Exception
_cex_json__writer__validate(jw_c* jw)
{
    if (jw == NULL) { return Error.argument; }
    return jw->error;
}

#undef $next_tok /* TEMP MACRO */
#undef $print
#undef $printva
#undef $scope_obj
#undef $scope_arr
#undef $scope_has_items
#undef $scope_has_key
#undef $last_scope
