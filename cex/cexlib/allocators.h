#pragma once
#include <cex/cex.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// struct Allocator_c;
#define ALLOCATOR_HEAP_MAGIC 0xFEED0001U
#define ALLOCATOR_STACK_MAGIC 0xFEED0002U
#define ALLOCATOR_STATIC_ARENA_MAGIC 0xFEED0003U



typedef struct
{
    alignas(64) Allocator_c base;
    u32 n_allocs;
    u32 n_reallocs;
    u32 n_free;
} AllocatorDynamic_c;
_Static_assert(sizeof(AllocatorDynamic_c) == 64, "size!");

typedef struct
{
    alignas(64) Allocator_c base;
    void* mem;
    void* next;
    void* max;
} AllocatorStaticArena_c;
_Static_assert(sizeof(AllocatorStaticArena_c) == 128, "size!");

const Allocator_c* AllocatorStaticArena_new(char* buffer, size_t capacity);
const Allocator_c* AllocatorStaticArena_free(void);

const Allocator_c* AllocatorHeap_new(void);
const Allocator_c* AllocatorHeap_free(void);
