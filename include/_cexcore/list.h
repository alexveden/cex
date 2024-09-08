#pragma once
#include "cex.h"


/**
 * @brief Dynamic array (list) implementation
 */
typedef struct
{
    // NOTE: do not assign this pointer to local variables, is can become dangling after dlist
    // operations (e.g. after realloc()). So local pointers can be pointing to invalid area!
    void* arr;
    size_t len;
} list_c;

#define list$define(eltype)                                                                        \
    /* NOTE: shadow struct the same as list_c, only used for type safety. const  prevents user to  \
     * overwrite struct arr.arr pointer (elements are ok), and also arr.count */                   \
    struct  \
    {                                                                                              \
        eltype* const arr;                                                                         \
        const size_t len;                                                                          \
    }

#define list$new(list_c_ptr, capacity, allocator)                                                        \
    (list.create(                                                                                  \
        (list_c*)list_c_ptr,                                                                             \
        capacity,                                                                                  \
        sizeof(typeof(*(((list_c_ptr))->arr))),                                                          \
        alignof(typeof(*(((list_c_ptr))->arr))),                                                         \
        allocator                                                                                  \
    ))

#define list$new_static(list_c_ptr, buf, buf_len)                                                        \
    (list.create_static(                                                                           \
        (list_c*)list_c_ptr,                                                                             \
        buf,                                                                                       \
        buf_len,                                                                                   \
        sizeof(typeof(*(((list_c_ptr))->arr))),                                                          \
        alignof(typeof(*(((list_c_ptr))->arr)))                                                          \
    ))

typedef struct
{
    struct
    {
        u16 magic;
        u16 elsize;
        u16 elalign;
    } header;
    size_t count;
    size_t capacity;
    const Allocator_i* allocator;
    // NOTE: we must use packed struct, because elements of the list my have absolutely different
    // alignment, so the list_head_s takes place at (void*)1st_el - sizeof(list_head_s)
} __attribute__((packed)) list_head_s;

#define _CEX_LIST_BUF 32
// _Static_assert(sizeof(list_head_s) == 32, "size"); // platform dependent
_Static_assert(sizeof(list_head_s) <= _CEX_LIST_BUF, "size");
_Static_assert(alignof(list_head_s) == 1, "align");

struct __module__list
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*create)(list_c* self, size_t capacity, size_t elsize, size_t elalign, const Allocator_i* allocator);

Exception
(*create_static)(list_c* self, void* buf, size_t buf_len, size_t elsize, size_t elalign);

Exception
(*insert)(void* self, void* item, size_t index);

Exception
(*del)(void* self, size_t index);

void
(*sort)(void* self, int (*comp)(const void*, const void*));

Exception
(*append)(void* self, void* item);

void
(*clear)(void* self);

Exception
(*extend)(void* self, void* items, size_t nitems);

size_t
(*len)(void* self);

size_t
(*capacity)(void* self);

void*
(*destroy)(void* self);

void*
(*iter)(void* self, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__list list; // CEX Autogen
