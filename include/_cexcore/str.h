#pragma once
#include "cex.h"
#include <stdalign.h>
#include <stdint.h>


typedef struct
{
    // NOTE: len comes first which prevents bad casting of str_c to char*
    // stb_sprintf() has special case of you accidentally pass str_c to
    // %s format specifier (it will show -> (str_c->%S))
    size_t len;
    char* buf;
} str_c;

_Static_assert(alignof(str_c) == alignof(size_t), "align");
_Static_assert(sizeof(str_c) == sizeof(size_t)*2, "size");

static inline str_c
_str__propagate_inline_small_func(str_c s)
{
    // this function only for s$(<str_c>)
    return s;
}

// clang-format off
#define s$(string)                                             \
    _Generic(string,                                           \
        char*: str.cstr,                                     \
        const char*: str.cstr,                               \
        str_c: _str__propagate_inline_small_func           \
    )(string)
// clang-format on



struct __module__str
{
    // Autogenerated by CEX
    // clang-format off

str_c
(*cstr)(const char* ccharptr);

str_c
(*cbuf)(char* s, size_t length);

str_c
(*sub)(str_c s, ssize_t start, ssize_t end);

Exception
(*copy)(str_c s, char* dest, size_t destlen);

str_c
(*sprintf)(char* dest, size_t dest_len, const char* format, ...);

size_t
(*len)(str_c s);

bool
(*is_valid)(str_c s);

char*
(*iter)(str_c s, cex_iterator_s* iterator);

ssize_t
(*find)(str_c s, str_c needle, size_t start, size_t end);

ssize_t
(*rfind)(str_c s, str_c needle, size_t start, size_t end);

bool
(*contains)(str_c s, str_c needle);

bool
(*starts_with)(str_c s, str_c needle);

bool
(*ends_with)(str_c s, str_c needle);

str_c
(*remove_prefix)(str_c s, str_c prefix);

str_c
(*remove_suffix)(str_c s, str_c suffix);

str_c
(*lstrip)(str_c s);

str_c
(*rstrip)(str_c s);

str_c
(*strip)(str_c s);

int
(*cmp)(str_c self, str_c other);

int
(*cmpi)(str_c self, str_c other);

str_c*
(*iter_split)(str_c s, const char* split_by, cex_iterator_s* iterator);

Exception
(*to_f32)(str_c self, f32* num);

Exception
(*to_f64)(str_c self, f64* num);

Exception
(*to_i8)(str_c self, i8* num);

Exception
(*to_i16)(str_c self, i16* num);

Exception
(*to_i32)(str_c self, i32* num);

Exception
(*to_i64)(str_c self, i64* num);

Exception
(*to_u8)(str_c self, u8* num);

Exception
(*to_u16)(str_c self, u16* num);

Exception
(*to_u32)(str_c self, u32* num);

Exception
(*to_u64)(str_c self, u64* num);

    // clang-format on
};
extern const struct __module__str str; // CEX Autogen
