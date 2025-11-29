#include "json2.h"

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
cex_json__iter__create(json_iter_c* it, char* content, usize content_len, bool strict_mode)
{
    uassert(it != NULL);
    if (content == NULL) { return Error.argument; }
    *it = (json_iter_c){
        ._impl = {
            .strict_mode = strict_mode,
            .lexer = CexParser.create(content, content_len, false),
        },
    };
    if (it->_impl.lexer.content == it->_impl.lexer.content_end) { return Error.empty; }
    return EOK;
}

/**
 * @brief Make step inside JSON object or array scope (json.iter.next() starts emitting this scope)
 *
 * @param it
 * @param expected_type Expected scope type (for sanity checks)
 * @return
 */
Exception
cex_json__iter__step_in(json_iter_c* it, JsonType_e expected_type)
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
        it->_impl.scope_stack[it->_impl.scope_depth] = '{';
        it->_impl.scope_depth++;
    } else if (it->type == JsonType__arr) {
        uassert(it->_impl.curr_token == CexTkn__lbracket);
        it->_impl.scope_stack[it->_impl.scope_depth] = '[';
        it->_impl.scope_depth++;
    } else {
        // return json.iter.next(it);
        it->error = "Stepping in is only for objects or arrays";
        goto error;
    }
    // json.iter.next() is going to check if we step in or skipping whole block
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
 * After calling step out, next call of `json.iter.next()` will return outer scope item,
 * make sure that you also break the loop or exiting parsing function for current scope.
 *
 * @param it
 * @return
 */
Exception
cex_json__iter__step_out(json_iter_c* it)
{
    if (unlikely(it->_impl.scope_depth == 0)) {
        it->error = "Bad scope/level for step out";
        return it->error;
    }
    u32 scope_depth_initial = it->_impl.scope_depth - 1;
    while (it->_impl.scope_depth > scope_depth_initial && json.iter.next(it)) {}
    return it->error;
}

static Exc
_cex_json__iter__skip(json_iter_c* it)
{
    // Simulate full step-in/next sequence for all nested stuff (because it serves as syntax check)
    u32 scope_depth_initial = it->_impl.scope_depth;
    if (json.iter.step_in(it, it->type)) { return it->error; }

    while (it->_impl.scope_depth > scope_depth_initial) {
        if (!json.iter.next(it) && it->error) { break; }
        switch (it->type) {
            case JsonType__arr:
            case JsonType__obj:
                if (json.iter.step_in(it, it->type)) { return it->error; }
            default:
                break;
        }
    }
    return it->error;
}

/**
 * @brief Get next JSON item for a scope
 *
 * @param it
 * @return false - on end of file, error, or json.iter.step_in() scope
 */
bool
cex_json__iter__next(json_iter_c* it)
{
    if (unlikely(it->error != EOK)) { goto error; }
    it->key = (str_s){ 0 };

    if (unlikely(
            it->_impl.curr_token == CexTkn__lbrace || it->_impl.curr_token == CexTkn__lbracket
        )) {
        // User didn't step into object/array, skipping it
        if (_cex_json__iter__skip(it) != EOK) { goto error; }
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
        if (it->_impl.scope_stack[it->_impl.scope_depth - 1] == '{') {
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
                    } else if (t.type != CexTkn__string) {
                        goto error_unexpected;
                    }
                    fallthrough(); // we get another key: value
                }
                case CexTkn__string: {
                    // Getting key of a object
                    it->key = t.value;
                    t = $next_tok();
                    if (t.type != CexTkn__colon) { goto error_unexpected; }
                    t = $next_tok();
                    it->_impl.has_items = true;
                    goto parse_generic; // parsing value
                }
                default: {
                    goto parse_generic;
                }
            }
        } else if (it->_impl.scope_stack[it->_impl.scope_depth - 1] == '[') {
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
                it->_impl.scope_stack[it->_impl.scope_depth - 1] == '{') {
                if (it->_impl.strict_mode && it->_impl.prev_token == CexTkn__comma) {
                    it->error = "Ending comma in object";
                    goto error;
                }
                it->_impl.scope_stack[it->_impl.scope_depth - 1] = '\0';
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
                it->_impl.scope_stack[it->_impl.scope_depth - 1] == '[') {
                if (it->_impl.strict_mode && it->_impl.prev_token == CexTkn__comma) {
                    it->error = "Ending comma in array";
                    goto error;
                }
                it->_impl.scope_stack[it->_impl.scope_depth - 1] = '\0';
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

#define $print(format, ...) /* temp macro */                                                       \
    ({                                                                                             \
        if (jb->error == EOK) {                                                                    \
            Exc err = sbuf.appendf(&jb->buf, format, __VA_ARGS__);                                 \
            if (unlikely(err != EOK && jb->error == EOK)) { jb->error = err; }                     \
        }                                                                                          \
    })

#define $printva() /* temp macro! */                                                               \
    if (jb->error == EOK) {                                                                        \
        va_list va;                                                                                \
        va_start(va, format);                                                                      \
        Exc err = sbuf.appendfva(&jb->buf, format, va);                                            \
        if (unlikely(err != EOK && jb->error != EOK)) { jb->error = err; }                         \
        va_end(va);                                                                                \
    }


void
_cex__jsonbuf_indent(json_buf_c* jb)
{
    if (unlikely(jb->error != EOK)) { return; }
    for (u32 i = 0; i < jb->indent; i++) { $print(" ", ""); }
}

/**
 * @brief Create JSON buffer/builder container used with json$buf / json$fmt / json$kstr macros
 *
 * @param jb
 * @param capacity initial capacity of buffer (will be resized if not enough)
 * @param indent JSON indentation (0 - to produce minified version)
 * @param allc allocator for buffer
 * @return
 */
Exception
cex_json__buf__create(json_buf_c* jb, u32 capacity, u8 indent, IAllocator allc)
{
    e$assert(jb != NULL);
    e$assert(allc != NULL);

    *jb = (json_buf_c){
        .indent_width = indent,
        .buf = sbuf.create(capacity, allc),
    };
    return EOK;
}

/**
 * @brief Destroy JSON buffer instance (not necessary to call if initialized on tmem$ allocator)
 *
 * @param jb
 */
void
cex_json__buf__destroy(json_buf_c* jb)
{
    if (jb != NULL) {
        if (jb->buf != NULL) { sbuf.destroy(&jb->buf); }
        memset(jb, 0, sizeof(*jb));
    }
}

/**
 * @brief Get JSON buffer contents (NULL if any error occurred)
 *
 * @param jb
 * @return
 */
char*
cex_json__buf__get(json_buf_c* jb)
{
    if (jb->error != EOK) {
        return NULL;
    } else {
        return jb->buf;
    }
}

/**
 * @brief Check if there is any error in JSON buffer
 *
 * @param jb
 * @return
 */
Exception
cex_json__buf__validate(json_buf_c* jb)
{
    return jb->error;
}

void
_cex_json__buf__clear(json_buf_c* jb)
{
    uassert(jb);
    uassert(jb->buf != NULL && "uninitialized or already destroyed");

    sbuf.clear(&jb->buf);
    jb->indent = 0;
    jb->error = EOK;
    if (jb->scope_depth > 0) {
        jb->scope_depth = 0;
        memset(jb->scope_stack, 0, sizeof(jb->scope_stack));
    }
}

void
_cex_json__buf__print(json_buf_c* jb, char* format, ...)
{
    _cex__jsonbuf_indent(jb);
    $printva();
}

void
_cex_json__buf__print_item(json_buf_c* jb, char* format, ...)
{
    uassertf(
        jb->scope_depth > 0 && jb->scope_stack[jb->scope_depth - 1] == '[',
        "Expected to be in json array scope"
    );
    _cex__jsonbuf_indent(jb);
    $printva();
    $print(",%s", (jb->indent_width > 0) ? "\n" : " ");
}

void
_cex_json__buf__print_key(json_buf_c* jb, char* key, char* format, ...)
{
    uassertf(
        jb->scope_depth > 0 && jb->scope_stack[jb->scope_depth - 1] == '{',
        "Expected to be in json object scope"
    );
    _cex__jsonbuf_indent(jb);
    $print("\"%s\": ", key);
    $printva();
    $print(",%s", (jb->indent_width > 0) ? "\n" : " ");
}

json_buf_c*
_cex__jsonbuf_print_scope_enter(json_buf_c* jb, JsonType_e scope_type)
{
    usize slen = sbuf.len(&jb->buf);
    if (slen && jb->buf[slen - 1] == '\n') { _cex__jsonbuf_indent(jb); }
    if (scope_type == JsonType__obj) {
        $print("%c%s", '{', (jb->indent_width > 0) ? "\n" : "");
        if (jb->scope_depth <= sizeof(jb->scope_stack) - 1) {
            jb->scope_stack[jb->scope_depth] = '{';
            jb->scope_depth++;
        } else {
            jb->error = "Scope overflow";
        }
    } else if (scope_type == JsonType__arr) {
        $print("%c%s", '[', (jb->indent_width > 0) ? "\n" : "");
        if (jb->scope_depth <= sizeof(jb->scope_stack) - 1) {
            jb->scope_stack[jb->scope_depth] = '[';
            jb->scope_depth++;
        } else {
            jb->error = "Scope overflow";
        }
    } else {
        unreachable();
    }
    jb->indent += jb->indent_width;
    return jb;
}

void
_cex__jsonbuf_print_scope_exit(json_buf_c** jbptr)
{
    uassert(*jbptr != NULL);
    json_buf_c* jb = *jbptr;

    if (jb->indent >= jb->indent_width) { jb->indent -= jb->indent_width; }
    if (jb->scope_depth > 0) {
        // Removing last comma
        usize slen = sbuf.len(&jb->buf);
        if (slen > 0) {
            bool has_new_line = false;
            for (u32 i = slen; --i > 0;) {
                char c = jb->buf[i];
                switch (c) {
                    case '\n':
                        has_new_line = true;
                        break;
                    case ' ':
                    case '\t':
                    case '\r':
                        break; // skip whitespace
                    case ',':
                        sbuf.shrink(&jb->buf, i);
                        if (has_new_line) {
                            e$except_silent (err, sbuf.append(&jb->buf, "\n")) {
                                if (jb->error == EOK) { jb->error = err; }
                            }
                        }
                        goto loop_break;
                    default:
                        goto loop_break;
                }
            }
        }
    loop_break:
        _cex__jsonbuf_indent(jb);

        $print(
            "%c%s%s",
            (jb->scope_stack[jb->scope_depth - 1] == '[') ? ']' : '}',
            (jb->scope_depth > 1) ? "," : "",
            (jb->indent_width > 0) ? "\n" : ""
        );
        jb->scope_depth--;
    } else {
        jb->error = "Scope overflow";
    }
}

#undef $next_tok /* TEMP MACRO */
#undef $print
#undef $printva

const struct __cex_namespace__json json = {
    // Autogenerated by CEX
    // clang-format off


    .buf = {
        .create = cex_json__buf__create,
        .destroy = cex_json__buf__destroy,
        .get = cex_json__buf__get,
        .validate = cex_json__buf__validate,
    },

    .iter = {
        .create = cex_json__iter__create,
        .next = cex_json__iter__next,
        .step_in = cex_json__iter__step_in,
        .step_out = cex_json__iter__step_out,
    },

    // clang-format on
};
