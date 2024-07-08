#include "allocators.h"
#include <malloc.h>

static AllocatorDynamic_c _Allocator_c__heap = { 0 };
static AllocatorStaticArena_c _Allocator_c__static_arena = { 0 };

/*
 *                  HEAP ALLOCATOR
 */

static void*
_AllocatorHeap__alloc(size_t size)
{
    uassert(_Allocator_c__heap.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__heap.base.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _Allocator_c__heap.n_allocs++;

    return malloc(size);
}

static void*
_AllocatorHeap__aligned_alloc(size_t alignment, size_t size)
{
    uassert(_Allocator_c__heap.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__heap.base.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(size % alignment == 0 && "size must be rounded to align");

    _Allocator_c__heap.n_allocs++;

    return aligned_alloc(alignment, size);
}

static void*
_AllocatorHeap__realloc(void* ptr, size_t size)
{
    uassert(_Allocator_c__heap.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__heap.base.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _Allocator_c__heap.n_reallocs++;
    return realloc(ptr, size);
}

static void*
_AllocatorHeap__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    uassert(_Allocator_c__heap.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__heap.base.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(((size_t)ptr % alignment) == 0 && "aligned_realloc existing pointer unaligned");
    uassert(size % alignment == 0 && "size must be rounded to align");

    _Allocator_c__heap.n_reallocs++;

    // TODO: implement #ifdef MSVC it supports _aligned_realloc()

    void* result = NULL;

    // Check if we have available space for realloc'ing new size
    //
    size_t new_size = malloc_usable_size(ptr);
    // NOTE: malloc_usable_size() returns a value no less than the size of
    // the block of allocated memory pointed to by ptr. 

    if (new_size >= size) {
        // This should return extended memory
        result = realloc(ptr, size);
        if (result == NULL) {
            // memory error
            return NULL;
        }
        if (result != ptr || (size_t)result % alignment != 0) {
            // very rare case, when some thread acquired space or returned unaligned result
            // Pessimistic case
            void* aligned = aligned_alloc(alignment, size);
            memcpy(aligned, result, size);
            free(result);
            return aligned;
        }
    } else {
        // No space available, alloc new memory + copy + release old
        result = aligned_alloc(alignment, size);
        if (result == NULL) {
            return NULL;
        }
        memcpy(result, ptr, new_size);
        free(ptr);
    }

    return result;
}

static void*
_AllocatorHeap__free(void* ptr)
{
    uassert(_Allocator_c__heap.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__heap.base.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _Allocator_c__heap.n_free++;
    free(ptr);
    return NULL; // always NULL
}


/**
 * @brief  heap-based allocator (simple proxy for malloc/free/realloc)
 *
 * @return
 */
const Allocator_c*
AllocatorHeap_new(void)
{
    uassert(_Allocator_c__heap.base.magic == 0 && "Already initialized");

    _Allocator_c__heap.base = (Allocator_c){
        .magic = ALLOCATOR_HEAP_MAGIC,
        .alloc = _AllocatorHeap__alloc,
        .realloc = _AllocatorHeap__realloc,
        .free = _AllocatorHeap__free,
        .realloc_aligned = _AllocatorHeap__aligned_realloc,
        .alloc_aligned = _AllocatorHeap__aligned_alloc,
    };

    return &_Allocator_c__heap.base;
}


/*
 *                  STATIC ARENA ALLOCATOR
 */
static void*
_AllocatorStaticArena__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    (void)ptr;
    (void)size;
    (void)alignment;
    uassert(_Allocator_c__static_arena.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__static_arena.base.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}
static void*
_AllocatorStaticArena__realloc(void* ptr, size_t size)
{
    (void)ptr;
    (void)size;
    uassert(_Allocator_c__static_arena.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__static_arena.base.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}

static void*
_AllocatorStaticArena__free(void* ptr)
{
    (void)ptr;
    uassert(_Allocator_c__static_arena.base.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_c__static_arena.base.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");

    // Not supported by static arena allocator
    return NULL; // always NULL
}

static void*
_AllocatorStaticArena__aligned_alloc(size_t alignment, size_t size)
{
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");

    AllocatorStaticArena_c* a = &_Allocator_c__static_arena;

    if (size == 0) {
        uassert(size > 0 && "zero size");
        return NULL;
    }
    if (size % alignment != 0) {
        uassert(size >= alignment && "size not aligned, must be a rounded to alignment");
        return NULL;
    }

    size_t offset = ((size_t)a->next % alignment);

    alignment = (offset ? alignment - offset : 0);

    if ((char*)a->next + size + alignment > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    void* ptr = (char*)a->next + alignment;
    a->next = (char*)ptr + size;
    return ptr;
}
static void*
_AllocatorStaticArena__alloc(size_t size)
{
    uassert(_Allocator_c__static_arena.base.magic != 0 && "not initialized");

    AllocatorStaticArena_c* a = &_Allocator_c__static_arena;

    if (size == 0) {
        uassert(size > 0 && "zero size");
        return NULL;
    }

    void* ptr = a->next;
    if ((char*)a->next + size > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    u32 offset = (size % sizeof(size_t));
    a->next = (char*)a->next + size + (offset ? sizeof(size_t) - offset : 0);

    return ptr;
}


/**
 * @brief Static arena allocator (can be heap or stack arena)
 *
 * Static allocator should be created at the start of the application (maybe in main()),
 * and freed after app shutdown.
 *
 * Note: memory leaks are not caught by sanitizers, if you forget to call
 * AllocatorStaticArena_free() sanitizers will be silent.
 *
 * No realloc() supported by this arena!
 *
 * @param capacity arena capacity (min 1024, max for stack - beware stack overflow)
 * @param is_heap - true - for heap allocated arena, false - allocate stack
 * @return  allocator instance
 */
const Allocator_c*
AllocatorStaticArena_new(char* buffer, size_t capacity)
{
    uassert(_Allocator_c__static_arena.base.magic == 0 && "Already initialized");

    uassert(capacity >= 1024 && "capacity is too low");
    uassert(((capacity & (capacity - 1)) == 0) && "must be power of 2");

    AllocatorStaticArena_c* a = &_Allocator_c__static_arena;

    a->base = (Allocator_c){
        .magic = ALLOCATOR_STATIC_ARENA_MAGIC,
        .alloc = _AllocatorStaticArena__alloc,
        .alloc_aligned = _AllocatorStaticArena__aligned_alloc,
        .realloc = _AllocatorStaticArena__realloc,
        .realloc_aligned = _AllocatorStaticArena__aligned_realloc,
        .free = _AllocatorStaticArena__free,
    };

    a->mem = buffer;
    if (a->mem == NULL) {
        memset(a, 0, sizeof(*a));
        return NULL;
    }

    memset(a->mem, 0, capacity);

    a->max = (char*)a->mem + capacity;
    a->next = a->mem;
    // a->alignment = alignment;

    u32 offset = ((u64)a->next % sizeof(size_t));
    a->next = (char*)a->next + (offset ? sizeof(size_t) - offset : 0);

    uassert(((u64)a->next % sizeof(size_t) == 0) && "alloca/malloc() returned non word aligned ptr");

    return &a->base;
}


/**
 * @brief Frees Staic arena allocator
 */
const Allocator_c*
AllocatorStaticArena_free(void)
{
    AllocatorStaticArena_c* a = &_Allocator_c__static_arena;

    uassert(a->base.magic != 0 && "allocator is not initialized");
    uassert(a->base.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "wrong allocator type");

    memset(a, 0, sizeof(*a));
    return NULL;
}

const Allocator_c*
AllocatorHeap_free(void)
{
    AllocatorDynamic_c* a = &_Allocator_c__heap;

    uassert(a->base.magic != 0 && "allocator is not initialized");
    uassert(a->base.magic == ALLOCATOR_HEAP_MAGIC && "wrong allocator type");

    if (a->n_allocs != a->n_free) {
        uperrorf("AllocatorHeap: number allocations don't match number of free\n");
    }

    memset(a, 0, sizeof(*a));

    return NULL;
}
