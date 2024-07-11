#include "sbuf.h"
#include "_stb_sprintf.c"
#include "_stb_sprintf.h"
#include "cex/cex.h"
#include <stdarg.h>

struct _sbuf__sprintf_ctx
{
    sbuf_head_s* head;
    Exc err;
    stbsp__context c;
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
sbuf_create(sbuf_c* self, u32 capacity, const Allocator_c* allocator)
{
    uassert(self != NULL);
    uassert(capacity != 0);
    uassert(allocator != NULL);

    if (capacity < 512) {
        // NOTE: if buffer is too small, allocate more size based on sbuf__alloc_capacity() formula
        // otherwise use exact number from a user to build the initial buffer
        capacity = sbuf__alloc_capacity(capacity);
    }

    char* buf = allocator->alloc(capacity);

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

Exception
sbuf_append_c(sbuf_c* self, char* s)
{
    if (s == NULL) {
        return Error.argument;
    }

    sbuf_head_s* head = sbuf__head(*self);

    u32 length = head->length;
    u32 capacity = head->capacity;
    uassert(capacity > 0);


    while (*s != '\0') {

        // Try resize
        if (length > capacity - 1) {
            except(err, sbuf__grow_buffer(self, length + 1))
            {
                return err;
            }
        }

        (*self)[length] = *s;
        length++;
        s++;
    }
    // always null terminate
    (*self)[length] = '\0';

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    return Error.ok;
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
                except(err, sbuf__grow_buffer(self, s.len + (newstr.len - oldstr.len)))
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
        except(err, sbuf__grow_buffer(self, length + s.len))
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
    sbuf_head_s* head = sbuf__head(*self);
    head->length = 0;
    (*self)[head->length] = '\0';
}

u32
sbuf_len(const sbuf_c self)
{
    sbuf_head_s* head = sbuf__head(self);
    return head->length;
}

u32
sbuf_capacity(const sbuf_c self)
{
    sbuf_head_s* head = sbuf__head(self);
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
        return ctx->c.tmp;
    }

    uassert(
        (buf != ctx->c.buf) ||
        (sbuf + ctx->c.length + len <= sbuf + ctx->c.count && "out of bounds")
    );

    if (unlikely(ctx->c.length + len > ctx->c.count)) {
        bool buf_is_tmp = buf != ctx->c.buf;

        if (len < 0 || ctx->c.length + len > INT32_MAX) {
            ctx->err = Error.integrity;
            return ctx->c.tmp;
        }

        // NOTE: sbuf likely changed after realloc
        except(err, sbuf__grow_buffer(&sbuf, ctx->c.length + len + 1))
        {
            ctx->err = err;
            return ctx->c.tmp;
        }
        // re-fetch head in case of realloc
        ctx->head = (sbuf_head_s*)(sbuf - sizeof(sbuf_head_s));
        uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

        ctx->c.buf = sbuf + ctx->head->length;
        ctx->c.count = ctx->head->capacity;

        uassert(ctx->c.count >= ctx->c.length);

        if (!buf_is_tmp) {
            // If we use the same buffer for sprintf() prevent use-after-free issue
            // if ctx->c.buf was reallocated to  another pointer
            buf = ctx->c.buf;
        }
    }

    ctx->c.length += len;
    ctx->head->length += len;

    if (len > 0) {
        if (buf != ctx->c.buf) {
            // NOTE: copy data only if previously c.tmp buffer used
            memcpy(ctx->c.buf, buf, len);
        }
        ctx->c.buf += len;
    }

    // NOTE: if string buffer is small, uses stack-based c.tmp buffer (about 512bytes)
    // // and then copy into sbuf_s buffer when ready. When string grows, uses heap allocated
    // // sbuf_c directly without copy
    return ((ctx->c.count - ctx->c.length) >= STB_SPRINTF_MIN) ? ctx->c.buf : ctx->c.tmp;
}

Exception
sbuf_sprintf(sbuf_c* self, const char* format, ...)
{

    sbuf_head_s* head = sbuf__head(*self);
    // utracef("head s: %s, len: %d\n", *self, head->length);

    struct _sbuf__sprintf_ctx ctx = {
        .head = head,
        .err = EOK,
        .c = { .buf = *self + head->length, .length = head->length, .count = head->capacity }
    };

    va_list va;
    va_start(va, format);

    STB_SPRINTF_DECORATE(vsprintfcb)
    (sbuf__sprintf_callback, &ctx, sbuf__sprintf_callback(NULL, &ctx, 0), format, va);

    // re-fetch self in case of realloc in sbuf__sprintf_callback
    *self = ((char*)ctx.head + sizeof(sbuf_head_s));

    // always null terminate
    (*self)[ctx.head->length] = '\0';
    (*self)[ctx.head->capacity] = '\0';

    va_end(va);

    return ctx.err;
}

str_c
sbuf_toview(sbuf_c* self)
{
    sbuf_head_s* head = sbuf__head(*self);

    return (str_c){
        .buf = *self,
        .len = head->length,
    };
}


const struct __module__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off
    .create = sbuf_create,
    .create_static = sbuf_create_static,
    .grow = sbuf_grow,
    .append_c = sbuf_append_c,
    .replace = sbuf_replace,
    .append = sbuf_append,
    .clear = sbuf_clear,
    .len = sbuf_len,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    ._sprintf_callback = sbuf__sprintf_callback,
    .sprintf = sbuf_sprintf,
    .toview = sbuf_toview,
    // clang-format on
};