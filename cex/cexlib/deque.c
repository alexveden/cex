#include "deque.h"


static inline deque_head_s*
_deque__head(deque_c self)
{
    uassert(self != NULL);
    deque_head_s* head = (deque_head_s*)self;
    uassert(head->header.magic == 0xdef0 && "not a deque / bad pointer magic");
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->header.elalign > 0 && "header.elalign == 0 or memory corruption");
    uassert(head->header.elsize > 0 && "header.elsize == 0 or memory corruption");
    uassert(head->header.elsize < INT16_MAX && "element size is too high");
    uassert(head->header.eloffset > 0 && "header.eloffset == 0 or memory corruption");
    uassert(head->idx_tail >= head->idx_head && "idx head > idx tail");

    return head;
}

static inline size_t
_deque__alloc_capacity(size_t capacity)
{
    if (capacity < 16) {
        return 16;
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
_deque__get_byindex(deque_head_s* head, size_t index)
{
    size_t _idx = head->idx_head + index;
    // _idx = _idx % head->capacity;
    _idx = _idx & (head->capacity - 1);
    return ((char*)head) + head->header.eloffset + head->header.elsize * _idx;
}

static inline size_t
_deque__alloc_size(size_t capacity, size_t elsize, size_t elalign)
{
    uassert(capacity > 0 && "zero capacity");
    uassert(elsize > 0 && "zero element size");
    uassert(elalign > 0 && "zero element align");
    uassert((capacity & (capacity - 1)) == 0 && "capacity must be power of 2");
    uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
    uassert(elalign <= alignof(deque_head_s) && "elalign exceeds alignof(deque_head_s)");

    size_t result = (capacity * elsize) +
                    (elalign > sizeof(deque_head_s) ? elalign : sizeof(deque_head_s));

    uassert(result % elalign == 0 && "alloc_size is unaligned to elalign");
    uassert(result % alignof(deque_head_s) == 0 && "alloc_size is unaligned to deque_head_s");
    return result;
}

Exception
_deque_validate(deque_c *self)
{
    if (self == NULL) {
        return Error.argument;
    }
    if (*self == NULL) {
        return Error.argument;
    }

    deque_head_s* head = (deque_head_s*)*self;
    if ((size_t)head % alignof(deque_head_s) != 0) {
        return Error.memory;
    }

    if (head->header.magic != 0xdef0) {
        return Error.integrity;
    }
    if (head->capacity == 0) {
        return Error.integrity;
    }
    if (head->max_capacity > 0 && head->capacity > head->max_capacity) {
        return Error.integrity;
    }
    if (head->header.elalign == 0) {
        return Error.integrity;
    }
    if (head->header.elalign > 64) {
        return Error.integrity;
    }
    if (head->header.eloffset != sizeof(deque_head_s)) {
        return Error.integrity;
    }
    if (head->idx_tail < head->idx_head) {
        return Error.integrity;
    }

    return Error.ok;
}

Exception
_deque_create(
    deque_c* self,
    size_t max_capacity,
    bool rewrite_overflowed,
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

    if (elsize == 0 || elsize >= INT16_MAX) {
        uassert(elsize > 0 && "zero elsize");
        uassert(elsize < INT16_MAX && "element size if too high");
        return Error.argument;
    }
    if (elalign == 0 || elalign > 64 || (elalign & (elalign - 1)) != 0) {
        uassert(elalign > 0 && "zero elalign");
        uassert(elalign <= 64 && "el align is too high");
        uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
        return Error.argument;
    }

    if (max_capacity > 0 && (max_capacity & (max_capacity - 1)) != 0) {
        uassert(false && "max_capacity must be power of 2");
        return Error.argument;
    }
    if (max_capacity == 0 && rewrite_overflowed) {
        uassert(false && "max_capacity must be set when rewrite_overflowed is enabled");
        return Error.argument;
    }

    size_t capacity = _deque__alloc_capacity(max_capacity);
    size_t alloc_size = _deque__alloc_size(capacity, elsize, elalign);

    _Static_assert(alignof(deque_head_s) == 64, "align");
    deque_head_s* que = allocator->alloc_aligned(alignof(deque_head_s), alloc_size);

    if (que == NULL) {
        return Error.memory;
    }
    if (((size_t)que) % alignof(deque_head_s) != 0) {
        uassert(false && "memory pointer address must be aligned to 64 bytes");
        return Error.integrity;
    }

    *que = (deque_head_s){
        .header = {
            .magic = 0xdef0,
            .elsize = elsize,
            .elalign = elalign,
            .eloffset = sizeof(deque_head_s),
            .rewrite_overflowed = rewrite_overflowed,
        }, 
        .capacity = capacity,
        .max_capacity = max_capacity,
        .idx_head = 0,
        .idx_tail = 0,
        .allocator = allocator,
    };

    *self = (void*)que;

    return Error.ok;
}

Exception
_deque_create_static(
    deque_c* self,
    void* buf,
    size_t buf_len,
    bool rewrite_overflowed,
    size_t elsize,
    size_t elalign
)
{
    if (self == NULL) {
        uassert(self != NULL && "must not be NULL");
        return Error.argument;
    }

    if (elsize == 0 || elsize >= INT16_MAX) {
        uassert(elsize > 0 && "zero elsize");
        uassert(elsize < INT16_MAX && "element size if too high");
        return Error.argument;
    }
    if (elalign == 0 || elalign > 64 || (elalign & (elalign - 1)) != 0) {
        uassert(elalign > 0 && "zero elalign");
        uassert(elalign <= 64 && "el align is too high");
        uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
        return Error.argument;
    }
    if (buf_len < sizeof(deque_head_s) + 16 * elsize) {
        uassert(
            buf_len > sizeof(deque_head_s) + 16 * elsize &&
            "deque_c static buffer must hold at least 16 elements"
        );
        return Error.overflow;
    }

    if (((size_t)buf) % alignof(deque_head_s) != 0) {
        uassert(false && "memory buffer address must be aligned to 64 bytes");
        return Error.integrity;
    }

    // buffer size might not contain exact pow of 2 number, just round it down
    size_t max_capacity = (buf_len - sizeof(deque_head_s)) / elsize / 2;

    size_t capacity = _deque__alloc_capacity(max_capacity);
    size_t alloc_size = _deque__alloc_size(capacity, elsize, elalign);
    if (alloc_size > buf_len) {
        uassert(alloc_size > buf_len && "deque_c expected allocation exceeds buf_len");
        return Error.overflow;
    }

    deque_head_s* que = (deque_head_s*)buf;

    *que = (deque_head_s){
        .header = {
            .magic = 0xdef0,
            .elsize = elsize,
            .elalign = elalign,
            .eloffset = sizeof(deque_head_s),
            .rewrite_overflowed = rewrite_overflowed,
        }, 
        .capacity = capacity,
        .max_capacity = capacity,
        .idx_head = 0,
        .idx_tail = 0,
        .allocator = NULL,
    };

    *self = (void*)que;

    return Error.ok;
}

Exception
_deque_append(deque_c* self, const void* item)
{
    if (item == NULL) {
        return Error.argument;
    }

    deque_head_s* head = _deque__head(*self);
    if (head->idx_head == head->idx_tail) {
        // No records, it's safe to reset
        // We can reset from any place if deque is empty, this will save allocations
        head->idx_head = 0;
        head->idx_tail = 0;
    } else if (unlikely(head->idx_tail == head->capacity)) {
        size_t new_cap = _deque__alloc_capacity(head->capacity + 1);

        // We've reached the end of the buffer, let's decide to wrap or expand
        if (head->allocator == NULL || (head->max_capacity > 0 && new_cap > head->max_capacity)) {
            // This is static buffer, wrap of fail
            if (head->max_capacity > 0 && head->idx_tail - head->idx_head == head->capacity) {
                if (head->header.rewrite_overflowed) {
                    head->idx_head++; // let it rewrite
                } else {
                    return Error.overflow;
                }
            }
        } else {
            size_t alloc_size = _deque__alloc_size(
                new_cap,
                head->header.elsize,
                head->header.elalign
            );
            head = head->allocator->realloc_aligned(head, alignof(deque_head_s), alloc_size);
            if (head == NULL) {
                return Error.memory;
            }
            uassert(head->header.magic == 0xdef0 && "head missing after realloc");
            uassert((size_t)head % alignof(deque_head_s) == 0 && "misaligned after realloc");
            head->capacity = new_cap;
            *self = (void*)head;
        }
    } else if (unlikely(head->idx_tail - head->idx_head == head->capacity)) {
        if (head->header.rewrite_overflowed) {
            head->idx_head++; // let it rewrite
        } else {
            return Error.overflow;
        }
    }

    // size_t idx = head->idx_tail % head->capacity;
    size_t idx = head->idx_tail & (head->capacity - 1);
    void* elptr = (char*)(*self) + head->header.eloffset + head->header.elsize * idx;

    memcpy(elptr, item, head->header.elsize);

    head->idx_tail++;

    return Error.ok;
}

Exception
_deque_enqueue(deque_c* self, const void* item)
{
    return _deque_append(self, item);
}

Exception
_deque_push(deque_c* self, const void* item)
{
    return _deque_append(self, item);
}

void*
_deque_dequeue(deque_c* self)
{
    deque_head_s* head = _deque__head(*self);

    if (head->idx_tail <= head->idx_head) {
        return NULL;
    }

    // size_t idx = head->idx_head % head->capacity;
    size_t idx = head->idx_head & (head->capacity - 1);
    void* elptr = ((char*)*self) + head->header.eloffset + head->header.elsize * idx;

    head->idx_head++;
    return elptr;
}

void*
_deque_pop(deque_c* self)
{
    deque_head_s* head = _deque__head(*self);

    if (head->idx_tail <= head->idx_head) {
        return NULL;
    }

    // size_t idx = (head->idx_tail - 1) % head->capacity;
    size_t idx = (head->idx_tail - 1) & (head->capacity - 1);
    void* elptr = ((char*)*self) + head->header.eloffset + head->header.elsize * idx;

    head->idx_tail--;
    return elptr;
}

void*
_deque_get(deque_c* self, size_t index)
{
    deque_head_s* head = _deque__head(*self);

    if (unlikely(head->idx_head + index >= head->idx_tail)) {
        // out of bounds or empty
        return NULL;
    }

    return _deque__get_byindex(head, index);
}

size_t
_deque_count(const deque_c* self)
{
    deque_head_s* head = _deque__head(*self);
    return head->idx_tail - head->idx_head;
}

void
_deque_clear(deque_c* self)
{
    deque_head_s* head = _deque__head(*self);
    head->idx_head = 0;
    head->idx_tail = 0;
}

void*
_deque_destroy(deque_c* self)
{
    if (self != NULL) {
        deque_head_s* head = _deque__head(*self);
        if (head->allocator != NULL) {
            head->allocator->free(head);
        }
        *self = NULL;
    }

    return NULL;
}

void*
_deque_iter_get(deque_c* self, i32 direction, cex_iterator_s* iterator)
{
    uassert(self != NULL && "self == NULL");
    uassert(iterator != NULL && "null iterator");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
        i32 direction;
        size_t max_count;
        size_t cnt;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == alignof(size_t), "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        // iterator first run/initialization
        deque_head_s* head = _deque__head(*self);

        size_t cnt = head->idx_tail - head->idx_head;
        if (unlikely(cnt == 0)) {
            // Empty deque, early exit
            return NULL;
        }

        uassert((direction == 1 || direction == -1) && "direction must be 1 or -1");
        if (direction >= 0) {
            // let's act defensive here and assign 1/-1 to direction if assert disabled
            ctx->direction = 1;
            ctx->cursor = 0;
        } else {
            ctx->direction = -1;
            ctx->cursor = cnt - 1;
        }
        ctx->max_count = cnt;
        ctx->cnt = 0;
    } else {
        ctx->cursor += ctx->direction;
        ctx->cnt++;
        if (ctx->cnt == ctx->max_count) {
            return NULL;
        }
    }

    iterator->idx.i = ctx->cursor;
    iterator->val = _deque__get_byindex((deque_head_s*)*self, ctx->cursor);
    return iterator->val;
}

void*
_deque_iter_fetch(deque_c* self, i32 direction, cex_iterator_s* iterator)
{
    uassert(self != NULL && "self == NULL");
    uassert(iterator != NULL && "null iterator");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t capacity;
        size_t max_count;
        size_t cnt;
        i32 direction;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == alignof(size_t), "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        // iterator first run/initialization
        deque_head_s* head = _deque__head(*self);

        size_t cnt = head->idx_tail - head->idx_head;
        if (unlikely(cnt == 0)) {
            // Empty deque, early exit
            return NULL;
        }

        uassert((direction == 1 || direction == -1) && "direction must be 1 or -1");
        if (direction >= 0) {
            // let's act defensive here and assign 1/-1 to direction if assert disabled
            ctx->direction = 1;
        } else {
            ctx->direction = -1;
        }
        ctx->capacity = head->capacity;
        ctx->max_count = cnt;
        ctx->cnt = 0;
    } else {
        ctx->cnt++;

        if (unlikely(ctx->cnt == ctx->max_count)) {
            uassert(deque.count(self) == 0 && "que size was changed during iterator");
            return NULL;
        }
    }

    if (ctx->direction > 0) {
        // que always return index = 0
        iterator->idx.i = 0;
        iterator->val = _deque_dequeue(self);
    } else {
        // stack always return index = dequeue.capacity
        iterator->idx.i = ctx->capacity;
        iterator->val = _deque_pop(self);
    }

    return iterator->val;
}
const struct __module__deque deque = {
    // Autogenerated by CEX
    // clang-format off
    .validate = _deque_validate,
    .create = _deque_create,
    .create_static = _deque_create_static,
    .append = _deque_append,
    .enqueue = _deque_enqueue,
    .push = _deque_push,
    .dequeue = _deque_dequeue,
    .pop = _deque_pop,
    .get = _deque_get,
    .count = _deque_count,
    .clear = _deque_clear,
    .destroy = _deque_destroy,
    .iter_get = _deque_iter_get,
    .iter_fetch = _deque_iter_fetch,
    // clang-format on
};