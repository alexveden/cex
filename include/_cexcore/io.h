#pragma once
#include "str.h"
#include "cex.h"
#include <stdio.h>


typedef struct io_c
{
    FILE* _fh;
    size_t _fsize;
    char* _fbuf;
    size_t _fbuf_size;
    const Allocator_i* _allocator;
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
(*fopen)(io_c* self, const char* filename, const char* mode, const Allocator_i* allocator);

Exception
(*fattach)(io_c* self, FILE* fh, const Allocator_i* allocator);

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
(*read)(io_c* self, void* obj_buffer, size_t obj_el_size, size_t* obj_count);

Exception
(*readall)(io_c* self, str_c* s);

Exception
(*readline)(io_c* self, str_c* s);

Exception
(*fprintf)(io_c* self, const char* format, ...);

void
(*printf)(const char* format, ...);

Exception
(*write)(io_c* self, void* obj_buffer, size_t obj_el_size, size_t obj_count);

void
(*close)(io_c* self);

    // clang-format on
};
extern const struct __module__io io; // CEX Autogen
