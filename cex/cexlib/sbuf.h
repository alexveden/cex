#pragma once
#include "sview.h"
#include <cex/cex.h>
#include <cex/cexlib/allocators.h>

/**
 * @brief Dynamic array (list) implementation
 */
typedef struct
{
    struct
    {
        u32 magic : 16;   // used for sanity checks
        u32 elsize : 8;   // maybe multibyte strings in the future?
        u32 nullterm : 8; // always zero to prevent usage of direct buffer
    } header;
    u32 length;
    u32 capacity;
    const Allocator_c* allocator;
} __attribute__((packed)) sbuf_head_s;

_Static_assert(sizeof(sbuf_head_s) == 20, "size");
_Static_assert(alignof(sbuf_head_s) == 1, "align");
_Static_assert(alignof(sbuf_head_s) == alignof(char), "align");

typedef char* sbuf_c;

struct __module__sbuf
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*create)(sbuf_c* self, u32 capacity, const Allocator_c* allocator);

Exception
(*create_static)(sbuf_c* self, char* buf, size_t buf_size);

Exception
(*grow)(sbuf_c* self, u32 capacity);

Exception
(*append_c)(sbuf_c* self, char* s);

Exception
(*replace)(sbuf_c* self, const sview_c old, const sview_c new);

Exception
(*append)(sbuf_c* self, sview_c s);

void
(*clear)(sbuf_c* self);

u32
(*len)(const sbuf_c self);

u32
(*capacity)(const sbuf_c self);

sbuf_c
(*destroy)(sbuf_c* self);

    // clang-format on
};
extern const struct __module__sbuf sbuf; // CEX Autogen
