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
cex_json__reader__create(json_reader_c* it, char* content, usize content_len, json_reader_kw* kwargs)
{
    uassert(it != NULL);
    if (content == NULL) { return Error.argument; }

    bool strict_mode = false;
    if (kwargs != NULL) { strict_mode = kwargs->strict_mode; }

    *it = (json_reader_c){
        ._impl = {
            .strict_mode = strict_mode,
            .lexer = CexParser.create(content, content_len, false),
        },
    };
    if (it->_impl.lexer.content == it->_impl.lexer.content_end) { return Error.empty; }
    json.reader.next(it);
    return EOK;
}

/**
 * @brief Make step inside JSON object or array scope (json.reader.next() starts emitting this scope)
 *
 * @param it
 * @param expected_type Expected scope type (for sanity checks)
 * @return
 */
Exception
cex_json__reader__step_in(json_reader_c* it, JsonType_e expected_type)
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
        // return json.reader.next(it);
        it->error = "Stepping in is only for objects or arrays";
        goto error;
    }
    // json.reader.next() is going to check if we step in or skipping whole block
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
 * After calling step out, next call of `json.reader.next()` will return outer scope item,
 * make sure that you also break the loop or exiting parsing function for current scope.
 *
 * @param it
 * @return
 */
Exception
cex_json__reader__step_out(json_reader_c* it)
{
    if (unlikely(it->_impl.scope_depth == 0)) {
        it->error = "Bad scope/level for step out";
        return it->error;
    }
    u32 scope_depth_initial = it->_impl.scope_depth - 1;
    while (it->_impl.scope_depth > scope_depth_initial && json.reader.next(it)) {}
    return it->error;
}

static Exc
_cex_json__reader__skip(json_reader_c* it)
{
    // Simulate full step-in/next sequence for all nested stuff (because it serves as syntax check)
    u32 scope_depth_initial = it->_impl.scope_depth;
    if (json.reader.step_in(it, it->type)) { return it->error; }

    while (it->_impl.scope_depth > scope_depth_initial) {
        if (!json.reader.next(it) && it->error) { break; }
        switch (it->type) {
            case JsonType__arr:
            case JsonType__obj:
                if (json.reader.step_in(it, it->type)) { return it->error; }
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
 * @return false - on end of file, error, or json.reader.step_in() scope
 */
bool
cex_json__reader__next(json_reader_c* it)
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
_cex_json_writer_indent(json_writer_c* jw, bool last_item)
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
 * @param jb
 * @param capacity initial capacity of buffer (will be resized if not enough)
 * @param indent JSON indentation (0 - to produce minified version)
 * @param allc allocator for buffer
 * @return
 */
Exception
_cex_json__writer__create(json_writer_c* jw, sbuf_c buf, FILE* stream, json_writer_kw* kwargs)
{
    e$assert(jw != NULL);

    if (buf == NULL && stream == NULL) { return "Empty buf and stream kwargs"; }
    if (buf != NULL && stream != NULL) { return "buf and stream kwargs are mutually exclusive"; }

    u32 indent = 0;
    if (kwargs) {
        indent = kwargs->indent;
    }

    *jw = (json_writer_c){
        .indent_width = indent,
        .buf = buf,
        .stream = stream,
    };

    return EOK;
}

/**
 * @brief Destroy JSON buffer instance (not necessary to call if initialized on tmem$ allocator)
 *
 * @param jb
 */
void
cex_json__writer__destroy(json_writer_c* jw)
{
    if (jw != NULL) {
        if (jw->buf != NULL) { sbuf.destroy(&jw->buf); }
        memset(jw, 0, sizeof(*jw));
    }
}

/**
 * @brief Get JSON buffer contents (NULL if any error occurred)
 *
 * @param jb
 * @return
 */
char*
cex_json__writer__get(json_writer_c* jw)
{
    if (jw->error != EOK) {
        return NULL;
    } else {
        return jw->buf;
    }
}

/**
 * @brief Check if there is any error in JSON buffer
 *
 * @param jb
 * @return
 */
Exception
cex_json__writer__validate(json_writer_c* jw)
{
    return jw->error;
}


void
_cex_json__writer__print(json_writer_c* jw, char* format, ...)
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
_cex_json__writer__print_item(json_writer_c* jw, char* format, ...)
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
_cex_json__writer__print_key(json_writer_c* jw, char* format, ...)
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

json_writer_c*
_cex_json_writer_print_scope_enter(json_writer_c* jw, JsonType_e scope_type, bool should_indent)
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
_cex_json_writer_print_scope_exit(json_writer_c** jbptr)
{
    uassert(*jbptr != NULL);
    json_writer_c* jw = *jbptr;

    if (jw->indent >= jw->indent_width) { jw->indent -= jw->indent_width; }
    if (jw->scope_depth > 0) {
        _cex_json_writer_indent(jw, true);

        $print("%c", (jw->scope_stack[jw->scope_depth - 1] & $scope_arr) ? ']' : '}');
        jw->scope_depth--;
    } else {
        jw->error = "Scope overflow";
    }
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


    .reader = {
        .create = cex_json__reader__create,
        .next = cex_json__reader__next,
        .step_in = cex_json__reader__step_in,
        .step_out = cex_json__reader__step_out,
    },

    .writer = {
        .create = NULL,
        .destroy = cex_json__writer__destroy,
        .get = cex_json__writer__get,
        .validate = cex_json__writer__validate,
    },

    // clang-format on
};
