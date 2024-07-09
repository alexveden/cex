#include "sview.h"


// static inline size_t _str__len(sview_c* s){
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
sview__isvalid(const sview_c* s)
{
    return s->buf != NULL;
}

static inline ssize_t
sview__index(sview_c* s, const char* c, u8 clen)
{
    ssize_t result = -1;

    if (!sview__isvalid(s)) {
        return -1;
    }

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) {
        split_by_idx[(u8)c[i]] = 1;
    }

    for (size_t i = 0; i < s->len; i++) {
        if (split_by_idx[(u8)s->buf[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

sview_c
sview_cstr(const char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) {
        return (sview_c){ 0 };
    }

    return (sview_c){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}

sview_c
sview_cbuf(char* s, size_t length)
{
    if (unlikely(s == NULL)) {
        return (sview_c){ 0 };
    }

    return (sview_c){
        .buf = s,
        .len = length,
    };
}

sview_c
sview_sub(sview_c s, ssize_t start, ssize_t end)
{
    if (unlikely(s.len == 0)) {
        if (start == 0 && end == 0) {
            return s;
        } else {
            return (sview_c){ 0 };
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
        return (sview_c){ 0 };
    }

    return (sview_c){
        .buf = s.buf + start,
        .len = (size_t)(end - start),
    };
}

Exception
sview_copy(sview_c s, char* dest, size_t destlen)
{
    uassert(dest != s.buf && "self copy is not allowed");

    if (unlikely(dest == NULL || destlen == 0)) {
        return Error.argument;
    }

    // If we fail next, it still writes empty string
    dest[0] = '\0';

    if (unlikely(!sview__isvalid(&s))) {
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
sview_length(sview_c s)
{
    return s.len;
}

bool
sview_is_valid(sview_c s)
{
    return sview__isvalid(&s);
}

char*
sview_iter(sview_c s, cex_iterator_s* iterator)
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
        if (unlikely(!sview__isvalid(&s) || s.len == 0)) {
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
sview_indexof(sview_c s, sview_c needle, size_t start, size_t end)
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
sview_contains(sview_c s, sview_c needle)
{
    return sview_indexof(s, needle, 0, 0) != -1;
}

bool
sview_starts_with(sview_c s, sview_c needle)
{
    return sview_indexof(s, needle, 0, needle.len) != -1;
}

bool
sview_ends_with(sview_c s, sview_c needle)
{
    if (needle.len > s.len) {
        return false;
    }

    return sview_indexof(s, needle, s.len - needle.len, 0) != -1;
}


static inline void
sview__strip_left(sview_c* s)
{
    char* cend = s->buf + s->len;

    while (s->buf < cend) {
        switch (*s->buf) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->buf++;
                s->len--;
                break;
            default:
                return;
        }
    }
}

static inline void
sview__strip_right(sview_c* s)
{
    while (s->len > 0) {
        switch (s->buf[s->len - 1]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->len--;
                break;
            default:
                return;
        }
    }
}

sview_c
sview_lstrip(sview_c s)
{
    if (s.buf == NULL) {
        return (sview_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (sview_c){
            .buf = "",
            .len = 0,
        };
    }

    sview_c result = (sview_c){
        .buf = s.buf,
        .len = s.len,
    };

    sview__strip_left(&result);
    return result;
}

sview_c
sview_rstrip(sview_c s)
{
    if (s.buf == NULL) {
        return (sview_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (sview_c){
            .buf = "",
            .len = 0,
        };
    }

    sview_c result = (sview_c){
        .buf = s.buf,
        .len = s.len,
    };

    sview__strip_right(&result);
    return result;
}

sview_c
sview_strip(sview_c s)
{
    if (s.buf == NULL) {
        return (sview_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (sview_c){
            .buf = "",
            .len = 0,
        };
    }

    sview_c result = (sview_c){
        .buf = s.buf,
        .len = s.len,
    };

    sview__strip_left(&result);
    sview__strip_right(&result);
    return result;
}

int
sview_cmp(sview_c self, sview_c other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    size_t min_len = (self.len > other.len) ? self.len : other.len;

    int cmp = memcmp(self.buf, other.buf, min_len);
    if (cmp == 0 && self.len != other.len) {
        if(self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }

    }
    return cmp;
}

int
sview_cmpc(sview_c self, const char* other)
{
    return sview_cmp(self, sview_cstr(other));
}

sview_c*
sview_iter_split(sview_c s, const char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
        size_t split_by_len;
        sview_c str;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) <= alignof(size_t), "cex_iterator_s _ctx misalign");

    if (unlikely(iterator->val == NULL)) {
        // First run handling
        if (unlikely(!sview__isvalid(&s))) {
            return NULL;
        }
        ctx->split_by_len = strlen(split_by);

        if (ctx->split_by_len == 0) {
            return NULL;
        }
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        ssize_t idx = sview__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) {
            idx = s.len;
        }
        ctx->cursor = idx;
        ctx->str = sview.sub(s, 0, idx);

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
            ctx->str = sview.cstr("");
            return iterator->val;
        }

        // Get remaining string after prev split_by char
        sview_c tok = sview.sub(s, ctx->cursor, 0);
        ssize_t idx = sview__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->str = tok;
            ctx->cursor = s.len;
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->str = sview.sub(tok, 0, idx);
            ctx->cursor += idx;
        }

        return iterator->val;
    }
}

const struct __module__sview sview = {
    // Autogenerated by CEX
    // clang-format off
    .cstr = sview_cstr,
    .cbuf = sview_cbuf,
    .sub = sview_sub,
    .copy = sview_copy,
    .length = sview_length,
    .is_valid = sview_is_valid,
    .iter = sview_iter,
    .indexof = sview_indexof,
    .contains = sview_contains,
    .starts_with = sview_starts_with,
    .ends_with = sview_ends_with,
    .lstrip = sview_lstrip,
    .rstrip = sview_rstrip,
    .strip = sview_strip,
    .cmp = sview_cmp,
    .cmpc = sview_cmpc,
    .iter_split = sview_iter_split,
    // clang-format on
};