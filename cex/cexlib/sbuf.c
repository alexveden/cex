#include "sbuf.h"
#include "cex/cex.h"
#include "stb_sprintf.h"
#include <stdarg.h>

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

    capacity = sbuf__alloc_capacity(capacity);

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
sbuf_replace(sbuf_c* self, const sview_c old, const sview_c new)
{
    uassert(self != NULL);
    uassert(old.buf != new.buf && "old and new overlap");
    uassert(*self != new.buf && "self and new overlap");
    uassert(*self != old.buf && "self and old overlap");

    sbuf_head_s* head = sbuf__head(*self);

    if (unlikely(!sview.is_valid(old) || !sview.is_valid(new) || old.len == 0)) {
        return Error.argument;
    }

    sview_c s = sview.cbuf(*self, head->length);

    if (unlikely(s.len == 0)) {
        return Error.ok;
    }

    u32 capacity = head->capacity;

    ssize_t idx = -1;
    while ((idx = sview.indexof(s, old, idx + 1, 0)) != -1) {
        // pointer to start of the found `old`

        char* f = &((*self)[idx]);

        if (old.len == new.len) {
            // Tokens exact match just replace
            memcpy(f, new.buf, new.len);
        } else if (new.len < old.len) {
            // Move remainder of a string to fill the gap
            memcpy(f, new.buf, new.len);
            memmove(f + new.len, f + old.len, s.len - idx - old.len);
            s.len -= (old.len - new.len);
            if (new.len == 0) {
                // NOTE: Edge case: replacing all by empty string, reset index again
                idx--;
            }
        } else {
            // Try resize
            if (unlikely(s.len + (new.len - old.len) > capacity - 1)) {
                except(err, sbuf__grow_buffer(self, s.len + (new.len - old.len)))
                {
                    return err;
                }
                // re-fetch head in case of realloc
                head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
                s.buf = *self;
                f = &((*self)[idx]);
            }
            // Move exceeding string to avoid overwriting
            memmove(f + new.len, f + old.len, s.len - idx - old.len + 1);
            memcpy(f, new.buf, new.len);
            s.len += (new.len - old.len);
        }
    }

    head->length = s.len;
    // always null terminate
    (*self)[s.len] = '\0';

    return Error.ok;
}

Exception
sbuf_append(sbuf_c* self, sview_c s)
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

Exception
sbuf_sprintf(sbuf_c* self, const char* format, ...)
{
    sbuf_head_s* head = sbuf__head(*self);

    // TODO: implement!
    va_list va;
    va_start(va, format);
    int result = stbsp_vsnprintf((*self + head->length), head->capacity, format, va) ;
    uassert(result >= 0);
    head->length += result;
    va_end(va);
    return Error.ok;
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
    .sprintf = sbuf_sprintf,
    // clang-format on
};