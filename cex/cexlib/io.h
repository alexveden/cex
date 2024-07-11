#pragma once
#include "cex/cexlib/str.h"
#include <cex/cex.h>
#include <stdio.h>


typedef struct io_c
{
    FILE* _fh;
    size_t _fsize;
    char* _fbuf;
    size_t _fbuf_size;
    const Allocator_c* _allocator;
    struct
    {
        u32 is_attached : 1;

    } _flags;
} io_c;


struct __module__io
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*fopen)(io_c* self, const char* filename, const char* mode, const Allocator_c* allocator);

Exception
(*fattach)(io_c* self, FILE* fh, const Allocator_c* allocator);

int
(*fileno)(io_c* self);

bool
(*isatty)(io_c* self);

Exception
(*flush)(io_c* self);

Exception
(*seek)(io_c* self, long offset, int whence);

void
(*rewind)(io_c* self);

Exception
(*tell)(io_c* self, size_t* size);

size_t
(*size)(io_c* self);

Exception
(*read)(io_c* self, void* restrict obj_buffer, size_t obj_el_size, size_t* obj_count);

Exception
(*readall)(io_c* self, str_c* s);

Exception
(*readline)(io_c* self, str_c* s);

void
(*close)(io_c* self);

    // clang-format on
};
extern const struct __module__io io; // CEX Autogen
