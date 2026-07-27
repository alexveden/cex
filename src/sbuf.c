#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_str)
#include "_sprintf.h"
#include "all.h"
#include <stdarg.h>

// temp const: will be #undef at the bottom
#define SBUF_MAGIC 0xf00efeed

struct _sbuf__sprintf_ctx
{
    sbuf_head_s* head;
    char* buf;
    Exc err;
    usize count;
    usize length;
    char tmp[CEX_SPRINTF_MIN];
};

static inline sbuf_head_s*
_sbuf__head(sbuf_c self)
{
    if (unlikely(!self)) { return NULL; }
    sbuf_head_s* head = (sbuf_head_s*)(self - sizeof(sbuf_head_s));

    uassert(head->header.magic == SBUF_MAGIC && "not a sbuf_head_s / bad pointer");
    uassert(head->length <= head->capacity && "count > capacity");
    uassert(head->header.nullterm == 0 && "nullterm != 0");

    return head;
}

static inline void
_sbuf__set_error(sbuf_head_s* head, Exc err)
{
    if (!head) { return; }
    if (head->err) { return; }

    head->err = err;
    head->length = 0;
    head->capacity = 0;

    // nullterm string contents
    *((char*)head + sizeof(sbuf_head_s)) = '\0';
}

static inline usize
_sbuf__alloc_capacity(usize capacity)
{
    uassert(capacity < INT32_MAX && "requested capacity exceeds 2gb, maybe overflow?");

    capacity += sizeof(sbuf_head_s) + 1; // also +1 for nullterm

    if (capacity >= 512) {
        return capacity + capacity / 5;
    } else {
        // Round up to closest pow*2 int
        u64 p = 64;
        while (p < capacity) { p *= 2; }
        return p;
    }
}
static inline Exception
_sbuf__grow_buffer(sbuf_c* self, usize length)
{
    sbuf_head_s* head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));

    if (unlikely(head->allocator == NULL)) {
        // sbuf is static, bad luck, overflow
        _sbuf__set_error(head, Error.overflow);
        return Error.overflow;
    }

    usize new_capacity = _sbuf__alloc_capacity(length);
    head = mem$realloc(head->allocator, head, new_capacity);
    if (unlikely(head == NULL)) {
        *self = NULL;
        return Error.memory;
    }

    head->capacity = new_capacity - sizeof(sbuf_head_s) - 1,
    *self = (char*)head + sizeof(sbuf_head_s);
    (*self)[head->capacity] = '\0';
    return Error.ok;
}

/// Creates new dynamic string builder backed by allocator
static sbuf_c
cex_sbuf_create(usize capacity, IAllocator allocator)
{
    if (unlikely(allocator == NULL)) {
        uassert(allocator != NULL);
        return NULL;
    }

    if (capacity < 512) { capacity = _sbuf__alloc_capacity(capacity); }

    char* buf = mem$malloc(allocator, capacity);
    if (unlikely(buf == NULL)) { return NULL; }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { .magic = SBUF_MAGIC, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = capacity - sizeof(sbuf_head_s) - 1,
        .allocator = allocator,
    };

    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}


/// Creates dynamic string backed by static array
static sbuf_c
cex_sbuf_create_static(char* buf, usize buf_size)
{
    if (unlikely(buf == NULL)) {
        uassert(buf != NULL);
        return NULL;
    }
    if (unlikely(buf_size == 0)) {
        uassert(buf_size > 0);
        return NULL;
    }
    if (unlikely(buf_size <= sizeof(sbuf_head_s) + 1)) {
        uassert(buf_size > sizeof(sbuf_head_s) + 1);
        return NULL;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { SBUF_MAGIC, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = buf_size - sizeof(sbuf_head_s) - 1,
        .allocator = NULL,
    };
    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}


/// Sets the length of a string to any value, if new_length greater than capacity, re-allocates more
/// space, always null-terminating. Newly allocated space is not ZII'ed, you must fill it yourself.
static Exc
cex_sbuf_set_len(sbuf_c* self, usize new_length)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(!head)) { return Error.runtime; }
    if (unlikely(head->err)) { return head->err; }
    
    if (unlikely(head->capacity == 0 || new_length > head->capacity - 1)) {
        e$except_silent (err, _sbuf__grow_buffer(self, new_length)) { return err; }
        // re-fetch head in case of realloc
        head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
        uassert(head); // clang-tidy false positive, should never happen
    } 

    head->length = new_length;
    (*self)[head->length] = '\0';
    return EOK;
}

/// Clears string
static void
cex_sbuf_clear(sbuf_c* self)
{
    cex_sbuf_set_len(self, 0);
}

/// Returns string length from its metadata
static usize
cex_sbuf_len(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return 0; }
    return head->length;
}


/// Returns string capacity from its metadata
static usize
cex_sbuf_capacity(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return 0; }
    return head->capacity;
}

/// Destroys the string, deallocates the memory, or nullify static buffer.
static sbuf_c
cex_sbuf_destroy(sbuf_c* self)
{
    uassert(self != NULL);

    sbuf_head_s* head = _sbuf__head(*self);
    if (head != NULL) {

        // NOTE: null-terminate string to avoid future usage,
        // it will appear as empty string if references anywhere else
        ((char*)head)[0] = '\0';
        (*self)[0] = '\0';
        *self = NULL;

        if (head->allocator != NULL) {
            // allocator is NULL for static sbuf
            mem$free(head->allocator, head);
        } else {
            // static buffer
            memset(self, 0, sizeof(*self));
        }
    }
    return NULL;
}

static char*
_cex_sbuf_sprintf_callback(char* buf, void* user, u32 len)
{
    struct _sbuf__sprintf_ctx* ctx = (struct _sbuf__sprintf_ctx*)user;
    sbuf_c sbuf = ((char*)ctx->head + sizeof(sbuf_head_s));

    uassert(ctx->head->header.magic == SBUF_MAGIC && "not a sbuf_head_s / bad pointer");
    if (unlikely(ctx->err != EOK)) { return NULL; }
    uassert((buf != ctx->buf) || (sbuf + ctx->length + len <= sbuf + ctx->count && "out of bounds"));

    if (unlikely(ctx->length + len > ctx->count)) {
        bool buf_is_tmp = buf != ctx->buf;

        if (len > INT32_MAX || ctx->length + len > (u32)INT32_MAX) {
            ctx->err = Error.integrity;
            return NULL;
        }

        // sbuf likely changed after realloc
        e$except_silent (err, _sbuf__grow_buffer(&sbuf, ctx->length + len + 1)) {
            ctx->err = err;
            return NULL;
        }
        // re-fetch head in case of realloc
        ctx->head = (sbuf_head_s*)(sbuf - sizeof(sbuf_head_s));
        uassert(ctx->head->header.magic == SBUF_MAGIC && "not a sbuf_head_s / bad pointer");

        ctx->buf = sbuf + ctx->head->length;
        ctx->count = ctx->head->capacity;
        uassert(ctx->count >= ctx->length);
        if (!buf_is_tmp) { buf = ctx->buf; }
    }

    ctx->length += len;
    ctx->head->length += len;
    if (len > 0) {
        if (buf != ctx->buf) {
            memcpy(ctx->buf, buf, len); // copy data only if previously tmp buffer used
        }
        ctx->buf += len;
    }
    return ((ctx->count - ctx->length) >= CEX_SPRINTF_MIN) ? ctx->buf : ctx->tmp;
}

/// Append format va (using CEX formatting engine), always null-terminating
static Exc
cex_sbuf_appendfva(sbuf_c* self, char* format, va_list va)
{
    if (unlikely(self == NULL)) { return Error.argument; }
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return Error.runtime; }
    if (unlikely(head->err)) { return head->err; }

    struct _sbuf__sprintf_ctx ctx = {
        .head = head,
        .err = EOK,
        .buf = *self + head->length,
        .length = head->length,
        .count = head->capacity,
    };

    cexsp__vsprintfcb(
        _cex_sbuf_sprintf_callback,
        &ctx,
        _cex_sbuf_sprintf_callback(NULL, &ctx, 0),
        format,
        va
    );

    // re-fetch self in case of realloc in sbuf__sprintf_callback
    *self = ((char*)ctx.head + sizeof(sbuf_head_s));

    // always null terminate
    (*self)[ctx.head->length] = '\0';
    (*self)[ctx.head->capacity] = '\0';

    return ctx.err;
}


/// Append format (using CEX formatting engine)
static Exc
cex_sbuf_appendf(sbuf_c* self, char* format, ...)
{

    va_list va;
    va_start(va, format);
    Exc result = cex_sbuf_appendfva(self, format, va);
    va_end(va);
    return result;
}

/// Append string to the builder
static Exc
cex_sbuf_append(sbuf_c* self, char* s)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return Error.runtime; }

    if (unlikely(s == NULL)) {
        _sbuf__set_error(head, "sbuf.append s=NULL");
        return Error.argument;
    }
    if (head->err) { return head->err; }

    // `s` must not point into the sbuf's own buffer
    // (would cause use-after-free on realloc or memcpy overlap)
    if (unlikely(s >= *self && s < *self + head->capacity)) {
        return Error.argument;
    }

    usize length = head->length;
    usize capacity = head->capacity;
    usize slen = strlen(s);

    // Try resize
    if (length + slen > capacity - 1) {
        e$except_silent (err, _sbuf__grow_buffer(self, length + slen)) { return err; }
        uassert(*self); // clang-tidy false positive, should never happen
    }
    memcpy((*self + length), s, slen);
    length += slen;

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    // always null terminate / also at capacity
    (*self)[head->capacity] = '\0';
    (*self)[length] = '\0';

    return Error.ok;
}

/// Validate dynamic string state, with detailed Exception
static Exception
cex_sbuf_validate(sbuf_c* self)
{
    if (unlikely(self == NULL)) { return "NULL argument"; }
    if (unlikely(*self == NULL)) { return "Memory error or already free'd"; }

    sbuf_head_s* head = (sbuf_head_s*)((char*)(*self) - sizeof(sbuf_head_s));

    if (unlikely(head->err)) { return head->err; }
    if (unlikely(head->header.magic != SBUF_MAGIC)) { return "Bad magic or non sbuf_c* pointer type"; }
    if (unlikely(head->capacity == 0)) { return "Zero capacity"; }
    if (head->length > head->capacity) { return "Length > capacity"; }
    if (head->header.nullterm != 0) { return "Missing null term in header"; }

    return EOK;
}


/// Returns false if string invalid
static bool
cex_sbuf_isvalid(sbuf_c* self)
{
    if (cex_sbuf_validate(self)) {
        return false;
    } else {
        return true;
    }
}

#undef SBUF_MAGIC

CEX_NAMESPACE_DEF struct __cex_namespace__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off

    .append = cex_sbuf_append,
    .appendf = cex_sbuf_appendf,
    .appendfva = cex_sbuf_appendfva,
    .capacity = cex_sbuf_capacity,
    .clear = cex_sbuf_clear,
    .create = cex_sbuf_create,
    .create_static = cex_sbuf_create_static,
    .destroy = cex_sbuf_destroy,
    .isvalid = cex_sbuf_isvalid,
    .len = cex_sbuf_len,
    .set_len = cex_sbuf_set_len,
    .validate = cex_sbuf_validate,

    // clang-format on
};
#endif
