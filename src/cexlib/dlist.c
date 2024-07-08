#include "dlist.h"

static inline dlist_head_s*
_dlist__head(dlist_c* self)
{
    uassert(self != NULL);
    dlist_head_s* head = (dlist_head_s*)((char*)self->arr - sizeof(dlist_head_s));
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->count <= head->capacity && "count > capacity");

    return head;
}
static inline size_t
_dlist__alloc_capacity(size_t capacity)
{
    uassert(capacity > 0);

    if (capacity < 4) {
        return 4;
    } else if (capacity >= 1024) {
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

static inline void*
_dlist__elidx(dlist_head_s* head, size_t idx)
{
    // Memory alignment
    // |-----|head|--el1--|--el2--|--elN--|
    //  ^^^^ - head can moved to el1, because of el1 alignment
    void* result = (char*)head + sizeof(dlist_head_s) + (head->header.elsize * idx);

    uassert((size_t)result % head->header.elalign == 0 && "misaligned array index pointer");

    return result;
}

static inline dlist_head_s*
_dlist__realloc(dlist_head_s* head, size_t alloc_size)
{
    // Memory alignment
    // |-----|head|--el1--|--el2--|--elN--|
    //  ^^^^ - head can moved to el1, because of el1 alignment
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    size_t offset = 0;
    size_t align = alignof(dlist_head_s);
    if (head->header.elalign > sizeof(dlist_head_s)){
        align = head->header.elalign;
        offset = head->header.elalign - sizeof(dlist_head_s);
    }
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    void* mptr = (char*)head - offset;
    uassert((size_t)mptr % alignof(size_t) == 0 && "misaligned malloc pointer");
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    uassert(alloc_size % head->header.elalign == 0 && "misaligned size");

    void* result =  head->allocator->realloc_aligned(mptr, align, alloc_size);
    uassert((size_t)result % align == 0 && "misaligned after realloc");

    head = (dlist_head_s*)((char*)result + offset);
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    return head;
}

static inline size_t
_dlist__alloc_size(size_t capacity, size_t elsize, size_t elalign)
{
    uassert(capacity > 0 && "zero capacity");
    uassert(elsize > 0 && "zero element size");
    uassert(elalign > 0 && "zero element align");
    uassert(elsize % elalign == 0 && "element size has to be rounded to elalign");

    size_t result = (capacity * elsize) +
                    (elalign > sizeof(dlist_head_s) ? elalign : sizeof(dlist_head_s));
    uassert(result % elalign == 0 && "alloc_size is unaligned");
    return result;
}

Exception
_dlist_create(
    dlist_c* self,
    size_t capacity,
    size_t elsize,
    size_t elalign,
    const Allocator_c* allocator
)
{
    if (self == NULL) {
        uassert(self != NULL && "must not be NULL");
        return Error.argument;
    }

    if (allocator == NULL) {
        uassert(allocator != NULL && "allocator invalid");
        return Error.argument;
    }

    if (elsize == 0 || elsize > INT16_MAX) {
        uassert(elsize > 0 && "zero elsize");
        uassert(elsize < INT16_MAX && "element size if too high");
        return Error.argument;
    }
    if (elalign == 0 || elalign > 64 || (elalign & (elalign - 1)) != 0) {
        uassert(elalign <= 64 && "el align is too high");
        uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
        uassert(elsize > 0 && "zero elsize");
        uassert(elalign > 0 && "zero elalign");
        return Error.argument;
    }

    // Clear dlist pointer
    memset(self, 0, sizeof(dlist_c));

    capacity = _dlist__alloc_capacity(capacity);
    size_t alloc_size = _dlist__alloc_size(capacity, elsize, elalign);
    char* buf = allocator->alloc_aligned(elalign, alloc_size);

    if (buf == NULL) {
        return Error.memory;
    }

    if (elalign > sizeof(dlist_head_s)) {
        buf += elalign - sizeof(dlist_head_s);
    }

    dlist_head_s* head = (dlist_head_s*)buf;
    *head = (dlist_head_s){
        .header = {
            .magic = 0x1eed,
            .elsize = elsize,
            .elalign = elalign,
        }, 
        .count = 0, 
        .capacity = capacity,
        .allocator = allocator,
    };

    dlist_c* d = (dlist_c*)self;
    d->count = 0;
    d->arr = _dlist__elidx(head, 0);

    return Error.ok;
}

Exception
_dlist_append(void* self, void* item)
{
    dlist_c* d = (dlist_c*)self;
    dlist_head_s* head = _dlist__head(self);

    if (head->count == head->capacity) {
        size_t new_cap = _dlist__alloc_capacity(head->capacity + 1);
        size_t alloc_size = _dlist__alloc_size(new_cap, head->header.elsize, head->header.elalign);
        head = _dlist__realloc(head, alloc_size);
        uassert(head->header.magic == 0x1eed && "head missing after realloc");

        if (head == NULL) {
            d->count = 0;
            return Error.memory;
        }

        d->arr = _dlist__elidx(head, 0);
        head->capacity = new_cap;
    }
    memcpy(_dlist__elidx(head, d->count), item, head->header.elsize);
    head->count++;
    d->count = head->count;

    return Error.ok;
}

Exception
_dlist_extend(void* self, void* items, size_t nitems)
{
    dlist_c* d = (dlist_c*)self;
    dlist_head_s* head = _dlist__head(self);

    if (unlikely(items == NULL || nitems == 0)) {
        return Error.argument;
    }

    if (head->count + nitems > head->capacity) {
        size_t new_cap = _dlist__alloc_capacity(head->capacity + nitems);

        size_t alloc_size = _dlist__alloc_size(new_cap, head->header.elsize, head->header.elalign);
        head = _dlist__realloc(head, alloc_size);
        uassert(head->header.magic == 0x1eed && "head missing after realloc");

        if (d->arr == NULL) {
            d->count = 0;
            return Error.memory;
        }

        d->arr = _dlist__elidx(head, 0);
        head->capacity = new_cap;
    }
    memcpy(_dlist__elidx(head, d->count), items, head->header.elsize * nitems);
    head->count += nitems;
    d->count = head->count;

    return Error.ok;
}

size_t
_dlist_count(void* self)
{
    dlist_c* d = (dlist_c*)self;
    dlist_head_s* head = _dlist__head(self);
    d->count = head->count;
    return head->count;
}

void*
_dlist_destroy(void* self)
{
    dlist_c* d = (dlist_c*)self;

    if (self != NULL) {
        if (d->arr != NULL) {
            dlist_head_s* head = _dlist__head(self);
            size_t offset = 0;
            if (head->header.elalign > sizeof(dlist_head_s)){
                offset = head->header.elalign - sizeof(dlist_head_s);
            }
            void* mptr = (char*)head - offset;
            head->allocator->free(mptr);
            d->arr = NULL;
            d->count = 0;
        }
    }

    return NULL;
}

void*
_dlist_iter(void* self, cex_iterator_s* iterator)
{
    uassert(self != NULL && "self == NULL");
    uassert(iterator != NULL && "null iterator");

    dlist_c* d = (dlist_c*)self;
    dlist_head_s* head = _dlist__head(self);

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == 8, "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        ctx->cursor = 0;
    } else {
        ctx->cursor++;
    }

    if (ctx->cursor >= d->count) {
        return NULL;
    }

    iterator->idx.i = ctx->cursor;
    iterator->val = (char*)d->arr + ctx->cursor * head->header.elsize;
    return iterator->val;
}

const struct __module__dlist dlist = {
    // Autogenerated by CEX
    // clang-format off
    .create = _dlist_create,
    .append = _dlist_append,
    .extend = _dlist_extend,
    .count = _dlist_count,
    .destroy = _dlist_destroy,
    .iter = _dlist_iter,
    // clang-format on
};
