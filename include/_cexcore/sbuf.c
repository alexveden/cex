#include "sbuf.h"
#include "_stb_sprintf.h"
#include "cex.h"
#include <stdarg.h>

struct _sbuf__sprintf_ctx
{
    sbuf_head_s* head;
    char* buf;
    Exc err;
    int count;
    int length;
    char tmp[STB_SPRINTF_MIN];
};

static inline sbuf_head_s*
sbuf__head(sbuf_c self)
{
    uassert(self != NULL);
    sbuf_head_s* head = (sbuf_head_s*)(self - sizeof(sbuf_head_s));

    uassert(head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->length <= head->capacity && "count > capacity");
    uassert(head->header.nullterm == 0 && "nullterm != 0");

    return head;
}
static inline size_t
sbuf__alloc_capacity(size_t capacity)
{
    uassert(capacity < INT32_MAX && "requested capacity exceeds 2gb, maybe overflow?");

    capacity += sizeof(sbuf_head_s) + 1; // also +1 for nullterm

    if (capacity >= 512) {
        return capacity * 1.2;
    } else {
        // Round up to closest pow*2 int
        u64 p = 4;
        while (p < capacity) {
            p *= 2;
        }
        return p;
    }
}
static inline Exception
sbuf__grow_buffer(sbuf_c* self, u32 length)
{
    sbuf_head_s* head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));

    if (unlikely(head->allocator == NULL)) {
        // sbuf is static, bad luck, overflow
        return Error.overflow;
    }

    u32 new_capacity = sbuf__alloc_capacity(length);
    head = head->allocator->realloc(head, new_capacity);
    if (unlikely(head == NULL)) {
        *self = NULL;
        return Error.memory;
    }

    head->capacity = new_capacity - sizeof(sbuf_head_s) - 1,
    *self = (char*)head + sizeof(sbuf_head_s);
    (*self)[head->capacity] = '\0';
    return Error.ok;
}

Exception
sbuf_create(sbuf_c* self, u32 capacity, const Allocator_i* allocator)
{
    uassert(self != NULL);
    uassert(capacity != 0);
    uassert(allocator != NULL);

    if (capacity < 512) {
        // NOTE: if buffer is too small, allocate more size based on sbuf__alloc_capacity() formula
        // otherwise use exact number from a user to build the initial buffer
        capacity = sbuf__alloc_capacity(capacity);
    }

    char* buf = allocator->malloc(capacity);

    if (buf == NULL) {
        return Error.memory;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = capacity - sizeof(sbuf_head_s) - 1,
        .allocator = allocator,
    };

    *self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (*self)[0] = '\0';
    (*self)[head->capacity] = '\0';

    return Error.ok;
}

Exception
sbuf_create_static(sbuf_c* self, char* buf, size_t buf_size)
{

    if (unlikely(self == NULL)) {
        uassert(self != NULL);
        return Error.argument;
    }

    if (unlikely(buf == NULL)) {
        uassert(buf != NULL);
        return Error.argument;
    }
    if (unlikely(buf_size == 0)) {
        uassert(buf_size > 0);
        return Error.argument;
    }
    if (unlikely(buf_size <= sizeof(sbuf_head_s) + 1)) {
        uassert(buf_size > sizeof(sbuf_head_s) + 1);
        return Error.overflow;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = buf_size - sizeof(sbuf_head_s) - 1,
        .allocator = NULL,
    };

    *self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (*self)[0] = '\0';
    (*self)[head->capacity] = '\0';

    return Error.ok;
}

Exception
sbuf_grow(sbuf_c* self, u32 capacity)
{
    sbuf_head_s* head = sbuf__head(*self);
    if (capacity <= head->capacity) {
        // capacity is enough, no need to grow
        return Error.ok;
    }
    return sbuf__grow_buffer(self, capacity);
}

void
sbuf_update_len(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    uassert((*self)[head->capacity] == '\0');

    head->length = strlen(*self);
}

Exception
sbuf_replace(sbuf_c* self, const str_c oldstr, const str_c newstr)
{
    uassert(self != NULL);
    uassert(oldstr.buf != newstr.buf && "old and new overlap");
    uassert(*self != newstr.buf && "self and new overlap");
    uassert(*self != oldstr.buf && "self and old overlap");

    sbuf_head_s* head = sbuf__head(*self);

    if (unlikely(!str.is_valid(oldstr) || !str.is_valid(newstr) || oldstr.len == 0)) {
        return Error.argument;
    }

    str_c s = str.cbuf(*self, head->length);

    if (unlikely(s.len == 0)) {
        return Error.ok;
    }

    u32 capacity = head->capacity;

    ssize_t idx = -1;
    while ((idx = str.find(s, oldstr, idx + 1, 0)) != -1) {
        // pointer to start of the found `old`

        char* f = &((*self)[idx]);

        if (oldstr.len == newstr.len) {
            // Tokens exact match just replace
            memcpy(f, newstr.buf, newstr.len);
        } else if (newstr.len < oldstr.len) {
            // Move remainder of a string to fill the gap
            memcpy(f, newstr.buf, newstr.len);
            memmove(f + newstr.len, f + oldstr.len, s.len - idx - oldstr.len);
            s.len -= (oldstr.len - newstr.len);
            if (newstr.len == 0) {
                // NOTE: Edge case: replacing all by empty string, reset index again
                idx--;
            }
        } else {
            // Try resize
            if (unlikely(s.len + (newstr.len - oldstr.len) > capacity - 1)) {
                except_silent(err, sbuf__grow_buffer(self, s.len + (newstr.len - oldstr.len)))
                {
                    return err;
                }
                // re-fetch head in case of realloc
                head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
                s.buf = *self;
                f = &((*self)[idx]);
            }
            // Move exceeding string to avoid overwriting
            memmove(f + newstr.len, f + oldstr.len, s.len - idx - oldstr.len + 1);
            memcpy(f, newstr.buf, newstr.len);
            s.len += (newstr.len - oldstr.len);
        }
    }

    head->length = s.len;
    // always null terminate
    (*self)[s.len] = '\0';

    return Error.ok;
}

Exception
sbuf_append(sbuf_c* self, str_c s)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    if (s.buf == NULL) {
        return Error.argument;
    }

    if (s.len == 0) {
        return Error.ok;
    }

    uassert(*self != s.buf && "buffer overlap");

    u64 length = head->length;
    u64 capacity = head->capacity;

    // Try resize
    if (length + s.len > capacity - 1) {
        except_silent(err, sbuf__grow_buffer(self, length + s.len))
        {
            return err;
        }
    }
    memcpy((*self + length), s.buf, s.len);
    length += s.len;

    // always null terminate
    (*self)[length] = '\0';

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    return Error.ok;
}

void
sbuf_clear(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    head->length = 0;
    (*self)[head->length] = '\0';
}

u32
sbuf_len(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->length;
}

u32
sbuf_capacity(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->capacity;
}

sbuf_c
sbuf_destroy(sbuf_c* self)
{
    uassert(self != NULL);

    if (*self != NULL) {
        sbuf_head_s* head = sbuf__head(*self);

        // NOTE: null-terminate string to avoid future usage,
        // it will appear as empty string if references anywhere else
        ((char*)head)[0] = '\0';
        (*self)[0] = '\0';
        *self = NULL;

        if (head->allocator != NULL) {
            // allocator is NULL for static sbuf
            head->allocator->free(head);
        }
        memset(self, 0, sizeof(*self));
    }
    return NULL;
}

static char*
sbuf__sprintf_callback(const char* buf, void* user, int len)
{
    struct _sbuf__sprintf_ctx* ctx = (struct _sbuf__sprintf_ctx*)user;
    sbuf_c sbuf = ((char*)ctx->head + sizeof(sbuf_head_s));
    uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

    if (unlikely(ctx->err != EOK)) {
        return ctx->tmp;
    }

    uassert((buf != ctx->buf) || (sbuf + ctx->length + len <= sbuf + ctx->count && "out of bounds"));

    if (unlikely(ctx->length + len > ctx->count)) {
        bool buf_is_tmp = buf != ctx->buf;

        if (len < 0 || ctx->length + len > INT32_MAX) {
            ctx->err = Error.integrity;
            return ctx->tmp;
        }

        // NOTE: sbuf likely changed after realloc
        except_silent(err, sbuf__grow_buffer(&sbuf, ctx->length + len + 1))
        {
            ctx->err = err;
            return ctx->tmp;
        }
        // re-fetch head in case of realloc
        ctx->head = (sbuf_head_s*)(sbuf - sizeof(sbuf_head_s));
        uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

        ctx->buf = sbuf + ctx->head->length;
        ctx->count = ctx->head->capacity;

        uassert(ctx->count >= ctx->length);

        if (!buf_is_tmp) {
            // If we use the same buffer for sprintf() prevent use-after-free issue
            // if ctx->buf was reallocated to  another pointer
            buf = ctx->buf;
        }
    }

    ctx->length += len;
    ctx->head->length += len;

    if (len > 0) {
        if (buf != ctx->buf) {
            // NOTE: copy data only if previously tmp buffer used
            memcpy(ctx->buf, buf, len);
        }
        ctx->buf += len;
    }

    // NOTE: if string buffer is small, uses stack-based tmp buffer (about 512bytes)
    // // and then copy into sbuf_s buffer when ready. When string grows, uses heap allocated
    // // sbuf_c directly without copy
    return ((ctx->count - ctx->length) >= STB_SPRINTF_MIN) ? ctx->buf : ctx->tmp;
}

Exception
sbuf_vsprintf(sbuf_c* self, const char* format, va_list va)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    struct _sbuf__sprintf_ctx ctx = {
        .head = head,
        .err = EOK,
        .buf = *self + head->length,
        .length = head->length,
        .count = head->capacity,
    };

    STB_SPRINTF_DECORATE(vsprintfcb)
    (sbuf__sprintf_callback, &ctx, sbuf__sprintf_callback(NULL, &ctx, 0), format, va);

    // re-fetch self in case of realloc in sbuf__sprintf_callback
    *self = ((char*)ctx.head + sizeof(sbuf_head_s));

    // always null terminate
    (*self)[ctx.head->length] = '\0';
    (*self)[ctx.head->capacity] = '\0';

    return ctx.err;
}

Exception
sbuf_sprintf(sbuf_c* self, const char* format, ...)
{

    va_list va;
    va_start(va, format);
    Exc result = sbuf_vsprintf(self, format, va);
    va_end(va);
    return result;
}

str_c
sbuf_to_str(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    return (str_c){
        .buf = *self,
        .len = head->length,
    };
}

bool
sbuf_isvalid(sbuf_c* self)
{
    if (self == NULL) {
        return false;
    }
    if (*self == NULL) {
        return false;
    }

    sbuf_head_s* head = (sbuf_head_s*)((char*)(*self) - sizeof(sbuf_head_s));

    if (head->header.magic != 0xf00e) {
        return false;
    }
    if (head->capacity == 0) {
        return false;
    }
    if (head->length > head->capacity) {
        return false;
    }
    if (head->header.nullterm != 0) {
        return false;
    }

    return true;
}


static inline ssize_t
sbuf__index(const char* self, size_t self_len, const char* c, u8 clen)
{
    ssize_t result = -1;

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) {
        split_by_idx[(u8)c[i]] = 1;
    }

    for (size_t i = 0; i < self_len; i++) {
        if (split_by_idx[(u8)self[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

str_c*
sbuf_iter_split(sbuf_c* self, const char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");
    uassert(self != NULL);

    // temporary struct based on _ctxbuffer
    struct 
    {
        str_c base_str;
        str_c str;
        size_t split_by_len;
        size_t cursor;
    }* ctx = (void*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");

    if (unlikely(iterator->val == NULL)) {
        ctx->split_by_len = strlen(split_by);
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        if (unlikely(ctx->split_by_len == 0)) {
            return NULL;
        }

        ctx->base_str = sbuf_to_str(self);

        ssize_t idx = sbuf__index(ctx->base_str.buf, ctx->base_str.len, split_by, ctx->split_by_len);
        if (idx < 0) {
            idx = ctx->base_str.len;
        }
        ctx->cursor = idx;

        ctx->str = str.sub(ctx->base_str, 0, idx);

        iterator->val = &ctx->str;
        iterator->idx.i = 0;
        return iterator->val;
    } else {
        uassert(iterator->val == &ctx->str);

        if (unlikely(ctx->cursor >= ctx->base_str.len)) {
            return NULL; // reached the end stops
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == ctx->base_str.len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            ctx->str = (str_c){ .buf = "", .len = 0 };
            iterator->idx.i++;
            return iterator->val;
        }

        // Get remaining string after prev split_by char
        str_c tok = str.sub(ctx->base_str, ctx->cursor, 0);
        ssize_t idx = sbuf__index(tok.buf, tok.len, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->str = tok;
            ctx->cursor = ctx->base_str.len;
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->str = str.sub(tok, 0, idx);
            ctx->cursor += idx;
        }

        return iterator->val;
    }
}

const struct __module__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off
    .create = sbuf_create,
    .create_static = sbuf_create_static,
    .grow = sbuf_grow,
    .update_len = sbuf_update_len,
    .replace = sbuf_replace,
    .append = sbuf_append,
    .clear = sbuf_clear,
    .len = sbuf_len,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    .vsprintf = sbuf_vsprintf,
    .sprintf = sbuf_sprintf,
    .to_str = sbuf_to_str,
    .isvalid = sbuf_isvalid,
    .iter_split = sbuf_iter_split,
    // clang-format on
};
