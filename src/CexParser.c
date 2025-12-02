#if !defined(cex$enable_minimal)

#include "CexParser.h"
#include <ctype.h>
#include "str.h"

const char* CexTkn_str[] = {
#define X(name) cex$stringize(name),
    _CexTknList
#undef X
};

// NOTE: lx$ are the temporary macros (will be #undef at the end of this file)
#define lx$next(lx)                                                                                \
    ({                                                                                             \
        char res = '\0';                                                                           \
        if ((lx->cur < lx->content_end)) {                                                         \
            if (*lx->cur == '\n') { lx->line++; lx->col = 0;}                                                  \
            lx->col++;\
            res = *(lx->cur++);                                                                    \
        }                                                                                          \
        res;                                                                                       \
    })

#define lx$rewind(lx)                                                                              \
    ({                                                                                             \
        if (lx->cur > lx->content) {                                                               \
            lx->cur--;                                                                             \
            if (*lx->cur == '\n') { lx->line--; }                                                  \
        }                                                                                          \
    })

#define lx$peek(lx) ((lx->cur < lx->content_end) ? *lx->cur : '\0')

#define lx$peek_next(lx) ((lx->cur + 1 < lx->content_end) ? lx->cur[1] : '\0')

#define lx$skip_space(lx, c)                                                                       \
    while (c && isspace((c))) {                                                                    \
        lx$next(lx);                                                                               \
        (c) = lx$peek(lx);                                                                         \
    }

CexParser_c
CexParser_create(char* content, u32 content_len, bool fold_scopes)
{
    if (content_len == 0) { content_len = str.len(content); }
    CexParser_c lx = { .content = content,
                       .content_end = (content) ? content + content_len : NULL,
                       .cur = content,
                       .fold_scopes = fold_scopes };
    return lx;
}

void
CexParser_reset(CexParser_c* lx)
{
    uassert(lx != NULL);
    lx->cur = lx->content;
    lx->line = 0;
}

static cex_token_s
_CexParser__scan_ident(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__ident, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        if (!(isalnum(c) || c == '_' || c == '$')) {
            lx$rewind(lx);
            break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
_CexParser__scan_number(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__number, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '/':
            case '*':
            case '+':
            case '-':
            case ',':
            case ')':
            case ']':
            case '}':
                lx$rewind(lx);
                return t;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
_CexParser__scan_string(CexParser_c* lx)
{
    cex_token_s t = { .type = (*lx->cur == '"' ? CexTkn__string : CexTkn__char),
                      .value = { .buf = lx->cur + 1, .len = 0 } };
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\\': // escape char, unconditionally skip next
                lx$next(lx);
                t.value.len++;
                break;
            case '\'':
                if (t.type == CexTkn__char) { goto end; }
                break;
            case '"':
                if (t.type == CexTkn__string) { goto end; }
                break;
            default: {
                if (unlikely((u8)c < 0x20)) {
                    t.type = CexTkn__error;
                    t.value = (str_s){ 0 };
                    return t;
                }
            }
        }
        t.value.len++;
    }
end:
    if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
}

static cex_token_s
_CexParser__scan_comment(CexParser_c* lx)
{
    cex_token_s t = { .type = lx->cur[1] == '/' ? CexTkn__comment_single : CexTkn__comment_multi,
                      .value = { .buf = lx->cur, .len = 2 } };
    lx$next(lx);
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                if (t.type == CexTkn__comment_single) {
                    lx$rewind(lx);
                    goto end;
                }
                break;
            case '*':
                if (t.type == CexTkn__comment_multi) {
                    if (lx$peek(lx) == '/') {
                        lx$next(lx);
                        t.value.len += 2;
                        goto end;
                    }
                }
                break;
        }
        t.value.len++;
    }
end:
    if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
}

static cex_token_s
_CexParser__scan_preproc(CexParser_c* lx)
{
    lx$next(lx);

    if (lx->cur >= lx->content_end) { return (cex_token_s){ .type = CexTkn__error }; }

    char c = *lx->cur;
    lx$skip_space(lx, c);
    cex_token_s t = { .type = CexTkn__preproc, .value = { .buf = lx->cur, .len = 0 } };

    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                goto end;
            case '\\':
                // next line concat for #define
                t.value.len++;
                if (!lx$next(lx)) {
                    // Oops, we expected next symbol here, but got EOF, invalidate token then
                    t.value.len++;
                    goto end;
                }
                break;
        }
        t.value.len++;
    }
end:
    if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
}

static cex_token_s
_CexParser__scan_scope(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__unk, .value = { .buf = lx->cur, .len = 0 } };

    if (!lx->fold_scopes) {
        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__lparen;
                break;
            case '[':
                t.type = CexTkn__lbracket;
                break;
            case '{':
                t.type = CexTkn__lbrace;
                break;
            default:
                unreachable();
        }
        t.value.len = 1;
        lx$next(lx);
        if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
            t = (cex_token_s){ .type = CexTkn__error };
        }
        return t;
    } else {
        char scope_stack[128] = { 0 };
        u32 scope_depth = 0;

#define scope$push(c) /* temp macro! */                                                            \
    if (++scope_depth < sizeof(scope_stack)) { scope_stack[scope_depth - 1] = c; }
#define scope$pop_if(c) /* temp macro! */                                                          \
    if (scope_depth > 0 && scope_depth <= sizeof(scope_stack) &&                                   \
        scope_stack[scope_depth - 1] == c) {                                                       \
        scope_stack[--scope_depth] = '\0';                                                         \
    }

        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__paren_block;
                break;
            case '[':
                t.type = CexTkn__bracket_block;
                break;
            case '{':
                t.type = CexTkn__brace_block;
                break;
            default:
                unreachable();
        }

        while ((c = lx$peek(lx))) {
            switch (c) {
                case '{':
                    scope$push(c);
                    break;
                case '[':
                    scope$push(c);
                    break;
                case '(':
                    scope$push(c);
                    break;
                case '}':
                    scope$pop_if('{');
                    break;
                case ']':
                    scope$pop_if('[');
                    break;
                case ')':
                    scope$pop_if('(');
                    break;
                case '"':
                case '\'': {
                    auto s = _CexParser__scan_string(lx);
                    t.value.len += s.value.len + 2;
                    continue;
                }
                case '/': {
                    if (lx$peek_next(lx) == '/' || lx$peek_next(lx) == '*') {
                        auto s = _CexParser__scan_comment(lx);
                        t.value.len += s.value.len;
                        continue;
                    }
                    break;
                }
                case '#': {
                    char* ppstart = lx->cur;
                    auto s = _CexParser__scan_preproc(lx);
                    if (s.value.buf) {
                        t.value.len += s.value.len + (s.value.buf - ppstart) + 1;
                    } else {
                        goto end;
                    }
                    continue;
                }
            }
            if (lx$next(lx)) { t.value.len++; }

            if (scope_depth == 0) { goto end; }
        }

#undef scope$push
#undef scope$pop_if

end:
    if (unlikely(scope_depth != 0 || t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
    }
}

cex_token_s
CexParser_next_token(CexParser_c* lx)
{

#define tok$new(tok_type)                                                                          \
    ({                                                                                             \
        lx$next(lx);                                                                               \
        (cex_token_s){ .type = tok_type, .value = { .buf = lx->cur - 1, .len = 1 } };              \
    })

    char c;
    while ((c = lx$peek(lx))) {
        lx$skip_space(lx, c);
        if (!c) { break; }

        if (isalpha(c) || c == '_' || c == '$') { return _CexParser__scan_ident(lx); }
        if (isdigit(c)) { return _CexParser__scan_number(lx); }

        switch (c) {
            case '\'':
            case '"':
                return _CexParser__scan_string(lx);
            case '/':
                if (lx$peek_next(lx) == '/' || lx$peek_next(lx) == '*') {
                    return _CexParser__scan_comment(lx);
                } else {
                    break;
                }
                break;
            case '*':
                return tok$new(CexTkn__star);
            case '.':
                return tok$new(CexTkn__dot);
            case ',':
                return tok$new(CexTkn__comma);
            case ';':
                return tok$new(CexTkn__eos);
            case ':':
                return tok$new(CexTkn__colon);
            case '-':
                return tok$new(CexTkn__minus);
            case '+':
                return tok$new(CexTkn__plus);
            case '?':
                return tok$new(CexTkn__question);
            case '=': {
                if (lx->cur > lx->content) {
                    switch (lx->cur[-1]) {
                        case '=':
                        case '*':
                        case '/':
                        case '~':
                        case '!':
                        case '+':
                        case '-':
                        case '>':
                        case '<':
                            goto unkn;
                        default:
                            break;
                    }
                }
                switch (lx$peek_next(lx)) {
                    case '=':
                        goto unkn;
                    default:
                        break;
                }

                return tok$new(CexTkn__eq);
            }
            case '{':
            case '(':
            case '[':
                return _CexParser__scan_scope(lx);
            case '}':
                return tok$new(CexTkn__rbrace);
            case ')':
                return tok$new(CexTkn__rparen);
            case ']':
                return tok$new(CexTkn__rbracket);
            case '#':
                return _CexParser__scan_preproc(lx);
            default:
                break;
        }


    unkn:
        return tok$new(CexTkn__unk);
    }

#undef tok$new
    return (cex_token_s){ 0 }; // EOF
}

cex_token_s
CexParser_next_entity(CexParser_c* lx, arr$(cex_token_s) * children)
{
    uassert(lx->fold_scopes && "CexParser must be with fold_scopes=true");
    uassert(children != NULL && "non initialized arr$");
    uassert(*children != NULL && "non initialized arr$");
    cex_token_s result = { 0 };

#ifdef CEX_TEST
    log$trace("New entity check...\n");
#endif
    arr$clear(*children);
    cex_token_s t;
    bool has_cex_namespace = false;
    u32 i = 0;
    (void)i;
    while ((t = CexParser_next_token(lx)).type) {
#ifdef CEX_TEST
        log$trace("%02d: %-15s %S\n", i, CexTkn_str[t.type], t.value);
#endif
        if (unlikely(t.type == CexTkn__error)) { goto error; }

        if (unlikely(!result.type)) {
            result.type = CexTkn__global_misc;
            result.value = t.value;
            if (t.type == CexTkn__preproc) {
                result.value.buf--;
                result.value.len++;
            }
        } else {
            // Extend token text
            result.value.len = t.value.buf - result.value.buf + t.value.len;
            uassert(result.value.len < 1024 * 1024 && "token diff it too high, bad pointers?");
        }
        arr$push(*children, t);
        switch (t.type) {
            case CexTkn__preproc: {
                if (str.slice.starts_with(t.value, str$s("define "))) {
                    CexParser_c _lx = CexParser.create(t.value.buf, t.value.len, true);
                    cex_token_s _t = CexParser.next_token(&_lx);

                    _t = CexParser.next_token(&_lx);
                    if (unlikely(_t.type != CexTkn__ident)) {
                        log$trace("Expected ident at %S line: %d\n", t.value, lx->line);
                        goto error;
                    }
                    result.type = CexTkn__macro_const;

                    _t = CexParser.next_token(&_lx);
                    if (_t.type == CexTkn__paren_block) { result.type = CexTkn__macro_func; }
                } else {
                    result.type = CexTkn__preproc;
                }
                goto end;
            }
            case CexTkn__paren_block: {
                if (result.type == CexTkn__var_decl) {
                    if (!str.slice.match(t.value, "\\(\\(*\\)\\)")) {
                        result.type = CexTkn__func_decl; // Check if not __attribute__(())
                    }
                } else {
                    if (i > 0 && children[0][i - 1].type == CexTkn__ident) {
                        if (!str.slice.match(children[0][i - 1].value, "__attribute__")) {
                            result.type = CexTkn__func_decl;
                        }
                    }
                }
                break;
            }
            case CexTkn__brace_block: {
                if (result.type == CexTkn__func_decl) {
                    result.type = CexTkn__func_def;
                    goto end;
                }
                break;
            }
            case CexTkn__eq: {
                if (has_cex_namespace) {
                    result.type = CexTkn__cex_module_def;
                } else {
                    result.type = CexTkn__var_def;
                }
                break;
            }

            case CexTkn__ident: {
                if (str.slice.match(t.value, "(typedef|struct|enum|union)")) {
                    if (result.type != CexTkn__var_decl) { result.type = CexTkn__typedef; }
                } else if (str.slice.match(t.value, "CEX_NAMESPACE") || str.slice.match(t.value, "extern")) {
                    result.type = CexTkn__var_decl;
                } else if (str.slice.match(t.value, "__cex_namespace__*")) {
                    has_cex_namespace = true;
                    if (result.type == CexTkn__var_decl) {
                        result.type = CexTkn__cex_module_decl;
                    } else if (result.type == CexTkn__typedef) {
                        result.type = CexTkn__cex_module_struct;
                    }
                }
                break;
            }
            case CexTkn__eos: {
                goto end;
            }
            default: {
            }
        }
        i++;
        uassert(arr$len(*children) == i);
    }
end:
    return result;

error:
    arr$clear(*children);
    result = (cex_token_s){ .type = CexTkn__error };
    goto end;
}

void
CexParser_decl_free(cex_decl_s* decl, IAllocator alloc)
{
    (void)alloc; // maybe used in the future
    if (decl) {
        sbuf.destroy(&decl->args);
        sbuf.destroy(&decl->ret_type);
        mem$free(alloc, decl);
    }
}

cex_decl_s*
CexParser_decl_parse(
    CexParser_c* lx,
    cex_token_s decl_token,
    arr$(cex_token_s) children,
    char* ignore_keywords_pattern,
    IAllocator alloc
)
{
    (void)children;
    switch (decl_token.type) {
        case CexTkn__func_decl:
        case CexTkn__func_def:
        case CexTkn__macro_func:
        case CexTkn__macro_const:
        case CexTkn__typedef:
        case CexTkn__cex_module_struct:
        case CexTkn__cex_module_def:
            break;
        default:
            return NULL;
    }
    cex_decl_s* result = mem$new(alloc, cex_decl_s);
    result->args = sbuf.create(128, alloc);
    result->ret_type = sbuf.create(128, alloc);
    result->type = decl_token.type;

    // CexParser line is at the end of token, find, the beginning
    result->line = lx->line;

    char* ignore_pattern =
        "(__attribute__|static|inline|__asm__|extern|volatile|restrict|register|__declspec|noreturn|_Noreturn)";


    cex_token_s prev_t = { 0 };
    bool prev_skipped = false;
    isize name_idx = -1;
    isize args_idx = -1;
    isize idx = -1;
    isize prev_idx = -1;

    for$each (it, children) {
        idx++;
        switch (it.type) {
            case CexTkn__ident: {
                if (str.slice.eq(it.value, str$s("static"))) { result->is_static = true; }
                if (str.slice.eq(it.value, str$s("inline"))) { result->is_inline = true; }
                if (str.slice.match(it.value, ignore_pattern)) {
                    prev_skipped = true;
                    continue;
                }
                if (ignore_keywords_pattern && str.slice.match(it.value, ignore_keywords_pattern)) {
                    prev_skipped = true;
                    continue;
                }
                if (decl_token.type == CexTkn__typedef) {
                    if (str.slice.match(prev_t.value, "(struct|enum|union)")) {
                        name_idx = idx;
                        result->name = it.value;
                    }
                }
                if (decl_token.type == CexTkn__func_decl) {
                    // Function pointer typedef
                    if (str.slice.eq(it.value, str$s("typedef"))) {
                        result->type = CexTkn__typedef;
                    }
                } else if (decl_token.type == CexTkn__cex_module_struct ||
                           decl_token.type == CexTkn__cex_module_def) {
                    str_s ns_prefix = str$s("__cex_namespace__");
                    if (str.slice.starts_with(it.value, ns_prefix)) {
                        result->name = str.slice.sub(it.value, ns_prefix.len, 0);
                    }
                }
                prev_skipped = false;
                break;
            }
            case CexTkn__preproc: {
                if (decl_token.type == CexTkn__macro_func ||
                    decl_token.type == CexTkn__macro_const) {
                    args_idx = -1;
                    uassert(it.value.len > 0);
                    CexParser_c _lx = CexParser.create(it.value.buf, it.value.len, true);

                    cex_token_s _t = CexParser.next_token(&_lx);
                    uassert(str.slice.starts_with(_t.value, str$s("define")));

                    _t = CexParser.next_token(&_lx);
                    if (_t.type != CexTkn__ident) {
                        log$trace("Expected ident at %S\n", it.value);
                        goto fail;
                    }
                    result->name = _t.value;
                    char* prev_end = _t.value.buf + _t.value.len;
                    _t = CexParser.next_token(&_lx);
                    if (_t.type == CexTkn__paren_block) {
                        auto _args = _t.value.len > 2 ? str.slice.sub(_t.value, 1, -1) : str$s("");
                        e$goto(sbuf.appendf(&result->args, "%S", _args), fail);
                        prev_end = _t.value.buf + _t.value.len;
                        _t = CexParser.next_token(&_lx);
                    } // Macro body
                    if (_t.type) {
                        isize decl_len = decl_token.value.len - (prev_end - decl_token.value.buf);
                        uassert(decl_len > 0);
                        result->body = (str_s){
                            .buf = prev_end,
                            .len = decl_len,
                        };
                    }
                }

                break;
            }
            case CexTkn__comment_multi:
            case CexTkn__comment_single: {
                if (result->name.buf == NULL && sbuf.len(&result->ret_type) == 0 &&
                    sbuf.len(&result->args) == 0) {
                    str_s cmt = str.slice.strip(it.value);
                    // Use only doxygen style comments for docs
                    if (str.slice.match(cmt, "(/**|/*!)*")) {
                        result->docs = cmt;
                    } else if (str.slice.starts_with(cmt, str$s("///"))) {
                        if (prev_t.type == CexTkn__comment_single && result->docs.buf) {
                            // Trying extend previous /// comment
                            result->docs.len = (cmt.buf - result->docs.buf) + cmt.len;
                        } else {
                            result->docs = cmt;
                        }
                    }
                }
                break;
            }
            case CexTkn__bracket_block: {
                if (str.slice.starts_with(it.value, str$s("[["))) {
                    // Clang/C23 attribute
                    continue;
                }
                fallthrough();
            }
            case CexTkn__brace_block: {
                result->body = it.value;
                fallthrough();
            }
            case CexTkn__eos: {
                if (prev_t.type == CexTkn__paren_block) { args_idx = prev_idx; }
                if (decl_token.type == CexTkn__typedef && prev_t.type == CexTkn__ident &&
                    !str.slice.match(prev_t.value, "(struct|enum|union)")) {
                    if (name_idx < 0) { name_idx = idx; }
                    result->name = prev_t.value;
                }
                break;
            }
            case CexTkn__paren_block: {
                if (prev_skipped) {
                    prev_skipped = false;
                    continue;
                }

                if (prev_t.type == CexTkn__ident) {
                    if (result->name.buf == NULL) {
                        if (str.slice.match(it.value, "\\(\\**\\)")) {
#if defined(CEX_TEST)
                            // this looks a function returning function pointer,
                            // we intentionally don't support this, use typedef func ptr
                            log$error(
                                "Skipping function (returns raw fn pointer, use typedef): \n%S\n",
                                decl_token.value
                            );
#endif
                            goto fail;
                        }
                    }
                    // NOTE: name_idx may change until last {} or ;
                    // because in C is common to use MACRO(foo) which may look as args block
                    // we'll check it after the end of entity block
                    result->name = prev_t.value;
                    name_idx = idx;
                }

                break;
            }
            default:
                break;
        }

        prev_skipped = false;
        prev_t = it;
        prev_idx = idx;
    }

#define $append_fmt(buf, tok) /* temp macro, formats return type and arguments */                  \
    switch ((tok).type) {                                                                          \
        case CexTkn__eof:                                                                          \
        case CexTkn__comma:                                                                        \
        case CexTkn__star:                                                                         \
        case CexTkn__dot:                                                                          \
        case CexTkn__rbracket:                                                                     \
        case CexTkn__lbracket:                                                                     \
        case CexTkn__lparen:                                                                       \
        case CexTkn__rparen:                                                                       \
        case CexTkn__paren_block:                                                                  \
        case CexTkn__bracket_block:                                                                \
            break;                                                                                 \
        default:                                                                                   \
            if (sbuf.len(&(buf)) > 0) { e$goto(sbuf.append(&(buf), " "), fail); }                  \
    }                                                                                              \
    e$goto(sbuf.appendf(&(buf), "%S", (tok).value), fail);
    //  <<<<<  #define $append_fmt

    // Exclude items with starting _ or special functions or other stuff
    if (str.slice.match(result->name, "(_Static_assert|static_assert)")) {
        goto fail;
    }

    if (name_idx > 0) {
        // NOTE: parsing return type
        prev_skipped = false;
        for$each (it, children, name_idx - 1) {
            switch (it.type) {
                case CexTkn__ident: {
                    if (str.slice.match(it.value, ignore_pattern)) {
                        prev_skipped = true;
                        // GCC attribute
                        continue;
                    }
                    if (ignore_keywords_pattern &&
                        str.slice.match(it.value, ignore_keywords_pattern)) {
                        prev_skipped = true;
                        // GCC attribute
                        continue;
                    }
                    $append_fmt(result->ret_type, it);
                    break;
                }
                case CexTkn__bracket_block: {
                    if (str.slice.starts_with(it.value, str$s("[["))) {
                        // Clang/C23 attribute
                        continue;
                    }
                    break;
                }
                case CexTkn__brace_block:
                case CexTkn__comment_multi:
                case CexTkn__comment_single:
                    continue;
                case CexTkn__paren_block: {
                    if (prev_skipped) {
                        prev_skipped = false;
                        continue;
                    }
                    fallthrough();
                }
                default:
                    $append_fmt(result->ret_type, it);
            }
            prev_skipped = false;
        }
    }

    // NOTE: parsing arguments of a function
    if (args_idx >= 0) {
        prev_t = children[args_idx];
        str_s clean_paren_block = str.slice.sub(str.slice.strip(prev_t.value), 1, -1);
        CexParser_c lx = CexParser_create(clean_paren_block.buf, clean_paren_block.len, true);
        cex_token_s t = { 0 };
        bool skip_next = false;
        while ((t = CexParser_next_token(&lx)).type) {
#ifdef CEX_TEST
            log$trace("arg token: type: %s '%S'\n", CexTkn_str[t.type], t.value);
#endif
            switch (t.type) {
                case CexTkn__ident: {
                    if (str.slice.match(t.value, ignore_pattern)) {
                        skip_next = true;
                        continue;
                    }
                    if (ignore_keywords_pattern &&
                        str.slice.match(t.value, ignore_keywords_pattern)) {
                        skip_next = true;
                        continue;
                    }
                    break;
                }
                case CexTkn__paren_block: {
                    if (skip_next) {
                        skip_next = false;
                        continue;
                    }
                    break;
                }
                case CexTkn__comment_multi:
                case CexTkn__comment_single:
                    continue;
                case CexTkn__bracket_block: {
                    if (str.slice.starts_with(t.value, str$s("[["))) { continue; }
                    fallthrough();
                }
                default: {
                    break;
                }
            }
            $append_fmt(result->args, t);
            skip_next = false;
        }
    }

    if (decl_token.value.len > 0 && result->name.buf) {
        char* cur = lx->cur - 1;
        while (cur > result->name.buf) {
            if (*cur == '\n') {
                if (result->line > 0) { result->line--; }
            }
            cur--;
        }
    }
    if (!result->name.buf) {
        log$trace(
            "Decl without name of type %s, at line: %d\n",
            CexTkn_str[result->type],
            result->line
        );
        goto fail;
    }
#undef $append_fmt
    return result;

fail:
    CexParser_decl_free(result, alloc);
    return NULL;
}


#undef lx$next
#undef lx$peek
#undef lx$skip_space

const struct __cex_namespace__CexParser CexParser = {
    // Autogenerated by CEX
    // clang-format off

    .create = CexParser_create,
    .decl_free = CexParser_decl_free,
    .decl_parse = CexParser_decl_parse,
    .next_entity = CexParser_next_entity,
    .next_token = CexParser_next_token,
    .reset = CexParser_reset,

    // clang-format on
};

#endif
