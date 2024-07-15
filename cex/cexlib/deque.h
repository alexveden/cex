#pragma once
#include "cex.h"
#include "allocators.h"

typedef struct
{
    alignas(64) struct
    {
        size_t magic : 16;             // deque magic number for sanity checks
        size_t elsize : 16;            // element size
        size_t elalign : 8;            // align of element type
        size_t eloffset : 8;           // offset in bytes to the 1st element array
        size_t rewrite_overflowed : 8; // allow rewriting old unread values if que overflowed
    } header;
    size_t idx_head;
    size_t idx_tail;
    size_t max_capacity; // maximum capacity when deque stops growing (must be pow of 2!)
    size_t capacity;
    const Allocator_i* allocator; // can be NULL for static deque
} deque_head_s;
_Static_assert(sizeof(deque_head_s) == 64, "size");
_Static_assert(alignof(deque_head_s) == 64, "align");

struct _deque_c
{
    deque_head_s _head;
    // NOTE: data is hidden, it makes no sense to access it directly
};
typedef struct _deque_c* deque_c;


#define deque$new(self, eltype, max_capacity, rewrite_overflowed, allocator)                       \
    (deque.create(                                                                                 \
        self,                                                                                      \
        max_capacity,                                                                              \
        rewrite_overflowed,                                                                        \
        sizeof(eltype),                                                                            \
        alignof(eltype),                                                                           \
        allocator                                                                                  \
    ))

#define deque$new_static(self, eltype, buf, buf_len, rewrite_overflowed)                           \
    (deque.create_static(self, buf, buf_len, rewrite_overflowed, sizeof(eltype), alignof(eltype)))

struct __module__deque
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*validate)(deque_c *self);

Exception
(*create)(deque_c* self, size_t max_capacity, bool rewrite_overflowed, size_t elsize, size_t elalign, const Allocator_i* allocator);

Exception
(*create_static)(deque_c* self, void* buf, size_t buf_len, bool rewrite_overflowed, size_t elsize, size_t elalign);

Exception
(*append)(deque_c* self, const void* item);

Exception
(*enqueue)(deque_c* self, const void* item);

Exception
(*push)(deque_c* self, const void* item);

void*
(*dequeue)(deque_c* self);

void*
(*pop)(deque_c* self);

void*
(*get)(deque_c* self, size_t index);

size_t
(*len)(const deque_c* self);

void
(*clear)(deque_c* self);

void*
(*destroy)(deque_c* self);

void*
(*iter_get)(deque_c* self, i32 direction, cex_iterator_s* iterator);

void*
(*iter_fetch)(deque_c* self, i32 direction, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__deque deque; // CEX Autogen
