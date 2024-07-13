#include "allocators.h"
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>

// struct Allocator_i;
#define ALLOCATOR_HEAP_MAGIC 0xFEED0001U
#define ALLOCATOR_STACK_MAGIC 0xFEED0002U
#define ALLOCATOR_STATIC_ARENA_MAGIC 0xFEED0003U

static void* allocator_heap__malloc(size_t size);
static void* allocator_heap__calloc(size_t nmemb,  size_t size);
static void* allocator_heap__aligned_malloc(size_t alignment, size_t size);
static void* allocator_heap__realloc(void* ptr, size_t size);
static void* allocator_heap__aligned_realloc(void* ptr, size_t alignment, size_t size);
static void allocator_heap__free(void* ptr);
static FILE* allocator_heap__fopen(const char* filename, const char* mode);
static int allocator_heap__fclose(FILE* f);
static int allocator_heap__open(const char* pathname, int flags, mode_t mode);
static int allocator_heap__close(int fd);

static void* allocator_staticarena__malloc(size_t size);
static void* allocator_staticarena__calloc(size_t nmemb,  size_t size);
static void* allocator_staticarena__aligned_malloc(size_t alignment, size_t size);
static void* allocator_staticarena__realloc(void* ptr, size_t size);
static void* allocator_staticarena__aligned_realloc(void* ptr, size_t alignment, size_t size);
static void allocator_staticarena__free(void* ptr);
static FILE* allocator_staticarena__fopen(const char* filename, const char* mode);
static int allocator_staticarena__fclose(FILE* f);
static int allocator_staticarena__open(const char* pathname, int flags, mode_t mode);
static int allocator_staticarena__close(int fd);

static allocator_heap_s
    _allocator_c__heap = { .base = {
                               .malloc = allocator_heap__malloc,
                               .malloc_aligned = allocator_heap__aligned_malloc,
                               .realloc = allocator_heap__realloc,
                               .realloc_aligned = allocator_heap__aligned_realloc,
                               .calloc = allocator_heap__calloc,
                               .free = allocator_heap__free,
                               .fopen = allocator_heap__fopen,
                               .fclose = allocator_heap__fclose,
                               .open = allocator_heap__open,
                               .close = allocator_heap__close,
                           } };
static AllocatorStaticArena_c
    _Allocator_i__static_arena = { .base = {
                                       .malloc = allocator_staticarena__malloc,
                                       .malloc_aligned = allocator_staticarena__aligned_malloc,
                                       .calloc = allocator_staticarena__calloc,
                                       .realloc = allocator_staticarena__realloc,
                                       .realloc_aligned = allocator_staticarena__aligned_realloc,
                                       .free = allocator_staticarena__free,
                                       .fopen = allocator_staticarena__fopen,
                                       .fclose = allocator_staticarena__fclose,
                                       .open = allocator_staticarena__open,
                                       .close = allocator_staticarena__close,
                                   } };

/*
 *                  HEAP ALLOCATOR
 */

/**
 * @brief  heap-based allocator (simple proxy for malloc/free/realloc)
 */
const Allocator_i*
allocators__heap__create(void)
{
    uassert(_allocator_c__heap.magic == 0 && "Already initialized");

    _allocator_c__heap.magic = ALLOCATOR_HEAP_MAGIC;

    memset(&_allocator_c__heap.stats, 0, sizeof(_allocator_c__heap.stats));

    return &_allocator_c__heap.base;
}

const Allocator_i*
allocators__heap__destroy(void)
{
    uassert(_allocator_c__heap.magic != 0 && "Already destroyed");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _allocator_c__heap.magic = 0;
    memset(&_allocator_c__heap.stats, 0, sizeof(_allocator_c__heap.stats));

    return NULL;
}

static void*
allocator_heap__malloc(size_t size)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _allocator_c__heap.stats.n_allocs++;

    return malloc(size);
}

static void* allocator_heap__calloc(size_t nmemb,  size_t size) {
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _allocator_c__heap.stats.n_allocs++;

    return calloc(nmemb, size);
}

static void*
allocator_heap__aligned_malloc(size_t alignment, size_t size)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(size % alignment == 0 && "size must be rounded to align");

    _allocator_c__heap.stats.n_allocs++;

    return aligned_alloc(alignment, size);
}

static void*
allocator_heap__realloc(void* ptr, size_t size)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _allocator_c__heap.stats.n_reallocs++;

    return realloc(ptr, size);
}

static void*
allocator_heap__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(((size_t)ptr % alignment) == 0 && "aligned_realloc existing pointer unaligned");
    uassert(size % alignment == 0 && "size must be rounded to align");

    _allocator_c__heap.stats.n_reallocs++;

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

static void
allocator_heap__free(void* ptr)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    _allocator_c__heap.stats.n_free++;
    free(ptr);
}

static FILE*
allocator_heap__fopen(const char* filename, const char* mode)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    FILE* res = fopen(filename, mode);
    if (res != NULL) {
        _allocator_c__heap.stats.n_fopen++;
    }
    return res;
}

static int
allocator_heap__open(const char* pathname, int flags, mode_t mode)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);
    if (fd != -1) {
        _allocator_c__heap.stats.n_open++;
    }
    return fd;
}

static int
allocator_heap__close(int fd)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    int ret = close(fd);
    if (ret != -1) {
        _allocator_c__heap.stats.n_close++;
    }

    return ret;
}

static int
allocator_heap__fclose(FILE* f)
{
    uassert(_allocator_c__heap.magic != 0 && "Allocator not initialized");
    uassert(_allocator_c__heap.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

    _allocator_c__heap.stats.n_fclose++;
    return fclose(f);
}


/*
 *                  STATIC ARENA ALLOCATOR
 */

/**
 * @brief Static arena allocator (can be heap or stack arena)
 *
 * Static allocator should be created at the start of the application (maybe in main()),
 * and freed after app shutdown.
 *
 * Note: memory leaks are not caught by sanitizers, if you forget to call
 * allocators.staticarena.destroy() sanitizers will be silent.
 *
 * No realloc() supported by this arena!
 *
 * @param buffer - pointer to memory buffer
 * @param capacity - capacity of a buffer (minimal requires is 1024)
 * @return  allocator instance
 */
const Allocator_i*
allocators__staticarena__create(char* buffer, size_t capacity)
{
    uassert(_Allocator_i__static_arena.magic == 0 && "Already initialized");

    uassert(capacity >= 1024 && "capacity is too low");
    uassert(((capacity & (capacity - 1)) == 0) && "must be power of 2");

    AllocatorStaticArena_c* a = &_Allocator_i__static_arena;

    a->magic = ALLOCATOR_STATIC_ARENA_MAGIC;

    a->mem = buffer;
    if (a->mem == NULL) {
        memset(a, 0, sizeof(*a));
        return NULL;
    }

    memset(a->mem, 0, capacity);

    a->max = (char*)a->mem + capacity;
    a->next = a->mem;

    u32 offset = ((u64)a->next % sizeof(size_t));
    a->next = (char*)a->next + (offset ? sizeof(size_t) - offset : 0);

    uassert(((u64)a->next % sizeof(size_t) == 0) && "alloca/malloc() returned non word aligned ptr");

    return &a->base;
}

const Allocator_i*
allocators__staticarena__destroy(void)
{
    uassert(_Allocator_i__static_arena.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_i__static_arena.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");


    AllocatorStaticArena_c* a = &_Allocator_i__static_arena;
    a->magic = 0;
    a->mem = NULL;
    a->next = NULL;
    a->max = NULL;
    memset(&_Allocator_i__static_arena.stats, 0, sizeof(_Allocator_i__static_arena.stats));

    

    return NULL;
}


static void*
allocator_staticarena__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    (void)ptr;
    (void)size;
    (void)alignment;
    uassert(_Allocator_i__static_arena.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_i__static_arena.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}
static void*
allocator_staticarena__realloc(void* ptr, size_t size)
{
    (void)ptr;
    (void)size;
    uassert(_Allocator_i__static_arena.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_i__static_arena.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}

static void
allocator_staticarena__free(void* ptr)
{
    (void)ptr;
    uassert(_Allocator_i__static_arena.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_i__static_arena.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
}

static void*
allocator_staticarena__aligned_malloc(size_t alignment, size_t size)
{
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");

    AllocatorStaticArena_c* a = &_Allocator_i__static_arena;

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
allocator_staticarena__malloc(size_t size)
{
    uassert(_Allocator_i__static_arena.magic != 0 && "not initialized");

    AllocatorStaticArena_c* a = &_Allocator_i__static_arena;

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

static void*
allocator_staticarena__calloc(size_t nmemb, size_t size)
{
    uassert(_Allocator_i__static_arena.magic != 0 && "not initialized");

    AllocatorStaticArena_c* a = &_Allocator_i__static_arena;

    size_t alloc_size = nmemb * size;
    if (nmemb != 0 && alloc_size / nmemb != size) {
        // overflow handling
        return NULL;
    }

    if (alloc_size == 0) {
        uassert(alloc_size > 0 && "zero size");
        return NULL;
    }

    void* ptr = a->next;
    if ((char*)a->next + alloc_size > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    u32 offset = (size % sizeof(size_t));
    a->next = (char*)a->next + size + (offset ? sizeof(size_t) - offset : 0);

    memset(ptr, 0, alloc_size);

    return ptr;
}

static FILE*
allocator_staticarena__fopen(const char* filename, const char* mode)
{
    uassert(_Allocator_i__static_arena.magic != 0 && "not initialized");
    uassert(filename != NULL);
    uassert(mode != NULL);

    AllocatorStaticArena_c* a = &_Allocator_i__static_arena;
    FILE* res = fopen(filename, mode);
    if (res != NULL) {
        a->stats.n_fopen++;
    }
    return res;
}

static int
allocator_staticarena__fclose(FILE* f)
{
    uassert(_Allocator_i__static_arena.magic != 0 && "not initialized");
    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

    AllocatorStaticArena_c* a = &_Allocator_i__static_arena;
    a->stats.n_fclose++;
    return fclose(f);
}
static int
allocator_staticarena__open(const char* pathname, int flags, mode_t mode)
{
    uassert(_Allocator_i__static_arena.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_i__static_arena.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);
    if (fd != -1) {
        _Allocator_i__static_arena.stats.n_open++;
    }
    return fd;
}

static int
allocator_staticarena__close(int fd)
{
    uassert(_Allocator_i__static_arena.magic != 0 && "Allocator not initialized");
    uassert(_Allocator_i__static_arena.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    int ret = close(fd);
    if (ret != -1) {
        _Allocator_i__static_arena.stats.n_close++;
    }

    return ret;
}

const struct __module__allocators allocators = {
    // Autogenerated by CEX
    // clang-format off

    .heap = {  // sub-module .heap >>>
        .create = allocators__heap__create,
        .destroy = allocators__heap__destroy,
    },  // sub-module .heap <<<

    .staticarena = {  // sub-module .staticarena >>>
        .create = allocators__staticarena__create,
        .destroy = allocators__staticarena__destroy,
    },  // sub-module .staticarena <<<
    // clang-format on
};
