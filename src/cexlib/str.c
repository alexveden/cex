#include "str.h"
#include "include/cex.h"
#include <include/misc/utils.h>


// static inline size_t _str__len(str_c* s){
//     if (unlikely(s->_len == 0)) {
//         if (unlikely(s->_buf == NULL))
//             return 0;
//         if(unlikely(s->_buf[0] == '\0'))
//             return 0;
//
//         s->_len =
//     }
//     return s->_len;
// }
static inline bool
_str__isvalid(const str_c* s)
{
    return s->buf != NULL;
}

static inline ssize_t
_str__index(str_c* s, char c)
{
    ssize_t result = -1;

    if (_str__isvalid(s)) {
        for (size_t i = 0; i < s->len; i++) {
            if (s->buf[i] == c) {
                result = i;
                break;
            }
        }
    }

    return result;
}

str_c
_str_cstr(const char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}

str_c
_str_cbuf(char* s, size_t length)
{
    if (unlikely(s == NULL)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = s,
        .len = length,
    };
}

str_c
_str_sub(str_c s, ssize_t start, ssize_t end)
{
    if (unlikely(s.len == 0)) {
        if (start == 0 && end == 0) {
            return s;
        } else {
            return (str_c){ 0 };
        }
    }

    if (start < 0) {
        // negative start uses offset from the end (-1 - last char)
        start += s.len;
    }

    if (end == 0) {
        // if end is zero, uses remainder
        end = s.len;
    } else if (end < 0) {
        // negative end, uses offset from the end
        end += s.len;
    }

    if (unlikely((size_t)start >= s.len || end > (ssize_t)s.len || end < start)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = s.buf + start,
        .len = (size_t)(end - start),
    };
}

Exception
_str_copy(str_c s, char* dest, size_t destlen)
{
    uassert(dest != s.buf && "self copy is not allowed");

    if (unlikely(dest == NULL || destlen == 0)) {
        return Error.argument;
    }

    // If we fail next, it still writes empty string
    dest[0] = '\0';

    if (unlikely(!_str__isvalid(&s))) {
        return Error.check;
    }

    size_t slen = s.len;
    size_t len_to_copy = MIN(destlen - 1, slen);
    memcpy(dest, s.buf, len_to_copy);

    // Null terminate end of buffer
    dest[len_to_copy] = '\0';

    if (unlikely(slen >= destlen)) {
        return Error.overflow;
    }

    return Error.ok;
}

size_t
_str_length(str_c s)
{
    return s.len;
}

bool
_str_is_valid(str_c s)
{
    return _str__isvalid(&s);
}

char*
_str_iter(str_c s, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == 8, "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        // Iterator is not initialized set to 1st item
        if (unlikely(!_str__isvalid(&s) || s.len == 0)) {
            return NULL;
        }
        iterator->val = s.buf;
        iterator->idx.i = 0;
        return &s.buf[0];
    }

    if (ctx->cursor >= s.len - 1) {
        return NULL;
    }

    ctx->cursor++;
    iterator->idx.i++;
    iterator->val = &s.buf[iterator->idx.i];
    return iterator->val;
}

ssize_t
_str_indexof(str_c s, str_c needle, size_t start, size_t end)
{
    if (needle.len == 0 || needle.len > s.len) {
        return -1;
    }
    if (start >= s.len) {
        return -1;
    }

    if (end == 0 || end > s.len) {
        end = s.len;
    }

    for (size_t i = start; i < end - needle.len + 1; i++) {
        // check the 1st letter
        if (s.buf[i] == needle.buf[0]) {
            // check whole word
            if (memcmp(&s.buf[i], needle.buf, needle.len) == 0) {
                return i;
            }
        }
    }

    return -1;
}

bool
_str_contains(str_c s, str_c needle)
{
    return _str_indexof(s, needle, 0, 0) != -1;
}

bool
_str_starts_with(str_c s, str_c needle)
{
    return _str_indexof(s, needle, 0, needle.len) != -1;
}

bool
_str_ends_with(str_c s, str_c needle)
{
    if(needle.len > s.len) {
        return false;
    }

    return _str_indexof(s, needle, s.len - needle.len, 0) != -1;
}

str_c*
_str_iter_split(str_c s, const char split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
        str_c str;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) <= alignof(size_t), "cex_iterator_s _ctx misalign");

    if (unlikely(iterator->val == NULL)) {
        // First run handling
        if (unlikely(!_str__isvalid(&s))) {
            return NULL;
        }
        ssize_t idx = _str__index(&s, split_by);
        if (idx < 0) {
            idx = s.len;
        }
        ctx->cursor = idx;
        ctx->str = str.sub(s, 0, idx);

        iterator->val = &ctx->str;
        iterator->idx.i = 0;
        return iterator->val;
    } else {
        uassert(iterator->val == &ctx->str);

        if (ctx->cursor >= s.len) {
            return NULL; // reached the end stops
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == s.len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            ctx->str = str.cstr("");
            return iterator->val;
        }

        // Get remaining string after prev split_by char
        str_c tok = str.sub(s, ctx->cursor, 0);

        ssize_t idx = _str__index(&tok, split_by);
        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->str = tok;
            ctx->cursor = s.len;
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->str = str.sub(tok, 0, idx);
            ctx->cursor += idx + 1;
        }

        return iterator->val;
    }
}

const struct __module__str str = {
    // Autogenerated by CEX
    // clang-format off
    .cstr = _str_cstr,
    .cbuf = _str_cbuf,
    .sub = _str_sub,
    .copy = _str_copy,
    .length = _str_length,
    .is_valid = _str_is_valid,
    .iter = _str_iter,
    .indexof = _str_indexof,
    .contains = _str_contains,
    .starts_with = _str_starts_with,
    .ends_with = _str_ends_with,
    .iter_split = _str_iter_split,
    // clang-format on
};
