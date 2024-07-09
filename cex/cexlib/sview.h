#pragma once
#include <cex/cex.h>
#include <stdalign.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    char*  buf;
    size_t len;
} sview_c;

_Static_assert(sizeof(sview_c) == 16, "size");
_Static_assert(alignof(sview_c) == 8, "size");

struct __module__sview
{
    // Autogenerated by CEX
    // clang-format off

sview_c
(*cstr)(const char* ccharptr);

sview_c
(*cbuf)(char* s, size_t length);

sview_c
(*sub)(sview_c s, ssize_t start, ssize_t end);

Exception
(*copy)(sview_c s, char* dest, size_t destlen);

size_t
(*len)(sview_c s);

bool
(*is_valid)(sview_c s);

char*
(*iter)(sview_c s, cex_iterator_s* iterator);

ssize_t
(*indexof)(sview_c s, sview_c needle, size_t start, size_t end);

bool
(*contains)(sview_c s, sview_c needle);

bool
(*starts_with)(sview_c s, sview_c needle);

bool
(*ends_with)(sview_c s, sview_c needle);

sview_c
(*lstrip)(sview_c s);

sview_c
(*rstrip)(sview_c s);

sview_c
(*strip)(sview_c s);

int
(*cmp)(sview_c self, sview_c other);

int
(*cmpc)(sview_c self, const char* other);

sview_c*
(*iter_split)(sview_c s, const char* split_by, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__sview sview; // CEX Autogen
