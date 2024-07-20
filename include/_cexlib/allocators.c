#include "allocators.h"
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>

// struct Allocator_i;
#define ALLOCATOR_HEAP_MAGIC 0xFEED0001U
#define ALLOCATOR_STACK_MAGIC 0xFEED0002U
#define ALLOCATOR_STATIC_ARENA_MAGIC 0xFEED0003U

static void* allocator_heap__malloc(size_t size);
static void* allocator_heap__calloc(size_t nmemb, size_t size);
static void* allocator_heap__aligned_malloc(size_t alignment, size_t size);
static void* allocator_heap__realloc(void* ptr, size_t size);
static void* allocator_heap__aligned_realloc(void* ptr, size_t alignment, size_t size);
static void allocator_heap__free(void* ptr);
static FILE* allocator_heap__fopen(const char* filename, const char* mode);
static int allocator_heap__fclose(FILE* f);
static int allocator_heap__open(const char* pathname, int flags, unsigned int mode);
static int allocator_heap__close(int fd);

static void* allocator_staticarena__malloc(size_t size);
static void* allocator_staticarena__calloc(size_t nmemb, size_t size);
static void* allocator_staticarena__aligned_malloc(size_t alignment, size_t size);
static void* allocator_staticarena__realloc(void* ptr, size_t size);
static void* allocator_staticarena__aligned_realloc(void* ptr, size_t alignment, size_t size);
static void allocator_staticarena__free(void* ptr);
static FILE* allocator_staticarena__fopen(const char* filename, const char* mode);
static int allocator_staticarena__fclose(FILE* f);
static int allocator_staticarena__open(const char* pathname, int flags, unsigned int mode);
static int allocator_staticarena__close(int fd);

static allocator_heap_s
    allocator__heap_data = { .base = {
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
static allocator_staticarena_s
    allocator__staticarena_data = { .base = {
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
    uassert(allocator__heap_data.magic == 0 && "Already initialized");

    allocator__heap_data.magic = ALLOCATOR_HEAP_MAGIC;

#ifndef NDEBUG
    memset(&allocator__heap_data.stats, 0, sizeof(allocator__heap_data.stats));
#endif

    return &allocator__heap_data.base;
}

const Allocator_i*
allocators__heap__destroy(void)
{
    uassert(allocator__heap_data.magic != 0 && "Already destroyed");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    allocator__heap_data.magic = 0;

#ifndef NDEBUG
    allocator_heap_s* a = &allocator__heap_data;
    // NOTE: this message only shown if no DNDEBUG
    if (a->stats.n_allocs != a->stats.n_free) {
        utracef(
            "Allocator: Possible memory leaks/double free: memory allocator->allocs() [%d] != allocator->free() [%d] count! \n",
            a->stats.n_allocs,
            a->stats.n_free
        );
    }
    if (a->stats.n_fopen != a->stats.n_fclose) {
        utracef(
            "Allocator: Possible FILE* leaks: allocator->fopen() [%d] != allocator->fclose() [%d]!\n",
            a->stats.n_fopen,
            a->stats.n_fclose
        );
    }
    if (a->stats.n_open != a->stats.n_close) {
        utracef(
            "Allocator: Possible file descriptor leaks: allocator->open() [%d] != allocator->close() [%d]!\n",
            a->stats.n_open,
            a->stats.n_close
        );
    }

    memset(&allocator__heap_data.stats, 0, sizeof(allocator__heap_data.stats));
#endif

    return NULL;
}

static void*
allocator_heap__malloc(size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__heap_data.stats.n_allocs++;
#endif

    return malloc(size);
}

static void*
allocator_heap__calloc(size_t nmemb, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__heap_data.stats.n_allocs++;
#endif

    return calloc(nmemb, size);
}

static void*
allocator_heap__aligned_malloc(size_t alignment, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(size % alignment == 0 && "size must be rounded to align");

#ifndef NDEBUG
    allocator__heap_data.stats.n_allocs++;
#endif

    return aligned_alloc(alignment, size);
}

static void*
allocator_heap__realloc(void* ptr, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__heap_data.stats.n_reallocs++;
#endif

    return realloc(ptr, size);
}

static void*
allocator_heap__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(((size_t)ptr % alignment) == 0 && "aligned_realloc existing pointer unaligned");
    uassert(size % alignment == 0 && "size must be rounded to align");

#ifndef NDEBUG
    allocator__heap_data.stats.n_reallocs++;
#endif

    // TODO: implement #ifdef MSVC it supports _aligned_realloc()

    void* result = NULL;

    // Check if we have available space for realloc'ing new size
    //
#ifdef _WIN32
    size_t new_size = _msize(ptr);
#else
    // NOTE: malloc_usable_size() returns a value no less than the size of
    // the block of allocated memory pointed to by ptr.
    size_t new_size = malloc_usable_size(ptr);
#endif

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
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    if (ptr != NULL) {
        allocator__heap_data.stats.n_free++;
    }
#endif

    free(ptr);
}

static FILE*
allocator_heap__fopen(const char* filename, const char* mode)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    FILE* res = fopen(filename, mode);

#ifndef NDEBUG
    if (res != NULL) {
        allocator__heap_data.stats.n_fopen++;
    }
#endif

    return res;
}

static int
allocator_heap__open(const char* pathname, int flags, unsigned int mode)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);

#ifndef NDEBUG
    if (fd != -1) {
        allocator__heap_data.stats.n_open++;
    }
#endif

    return fd;
}

static int
allocator_heap__close(int fd)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    int ret = close(fd);

#ifndef NDEBUG
    if (ret != -1) {
        allocator__heap_data.stats.n_close++;
    }
#endif

    return ret;
}

static int
allocator_heap__fclose(FILE* f)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

#ifndef NDEBUG
    allocator__heap_data.stats.n_fclose++;
#endif

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
    uassert(allocator__staticarena_data.magic == 0 && "Already initialized");

    uassert(capacity >= 1024 && "capacity is too low");
    uassert(((capacity & (capacity - 1)) == 0) && "must be power of 2");

    allocator_staticarena_s* a = &allocator__staticarena_data;

    a->magic = ALLOCATOR_STATIC_ARENA_MAGIC;

    a->mem = buffer;
    if (a->mem == NULL) {
        memset(a, 0, sizeof(*a));
        return NULL;
    }

    memset(a->mem, 0, capacity);

    a->max = (char*)a->mem + capacity;
    a->next = a->mem;

    size_t offset = ((size_t)a->next % sizeof(size_t));
    a->next = (char*)a->next + (offset ? sizeof(size_t) - offset : 0);

    uassert(((size_t)a->next % sizeof(size_t) == 0) && "alloca/malloc() returned non word aligned ptr");

    return &a->base;
}

const Allocator_i*
allocators__staticarena__destroy(void)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");


    allocator_staticarena_s* a = &allocator__staticarena_data;
    a->magic = 0;
    a->mem = NULL;
    a->next = NULL;
    a->max = NULL;

#ifndef NDEBUG
    if (a->stats.n_allocs != a->stats.n_free) {
        utracef(
            "Allocator: Possible memory leaks/double free: memory allocator->allocs() [%d] != allocator->free() [%d] count! \n",
            a->stats.n_allocs,
            a->stats.n_free
        );
    }
    if (a->stats.n_fopen != a->stats.n_fclose) {
        utracef(
            "Allocator: Possible FILE* leaks: allocator->fopen() [%d] != allocator->fclose() [%d]!\n",
            a->stats.n_fopen,
            a->stats.n_fclose
        );
    }
    if (a->stats.n_open != a->stats.n_close) {
        utracef(
            "Allocator: Possible file descriptor leaks: allocator->open() [%d] != allocator->close() [%d]!\n",
            a->stats.n_open,
            a->stats.n_close
        );
    }

    memset(&allocator__staticarena_data.stats, 0, sizeof(allocator__staticarena_data.stats));
#endif

    return NULL;
}


static void*
allocator_staticarena__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    (void)ptr;
    (void)size;
    (void)alignment;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}
static void*
allocator_staticarena__realloc(void* ptr, size_t size)
{
    (void)ptr;
    (void)size;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}

static void
allocator_staticarena__free(void* ptr)
{
    (void)ptr;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");

#ifndef NDEBUG
    allocator_staticarena_s* a = &allocator__staticarena_data;
    if(ptr != NULL){
        a->stats.n_free++;
    }
#endif
}

static void*
allocator_staticarena__aligned_malloc(size_t alignment, size_t size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");

    allocator_staticarena_s* a = &allocator__staticarena_data;


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

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static void*
allocator_staticarena__malloc(size_t size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    allocator_staticarena_s* a = &allocator__staticarena_data;

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

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static void*
allocator_staticarena__calloc(size_t nmemb, size_t size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    allocator_staticarena_s* a = &allocator__staticarena_data;

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

    u32 offset = (alloc_size % sizeof(size_t));
    a->next = (char*)a->next + alloc_size + (offset ? sizeof(size_t) - offset : 0);

    memset(ptr, 0, alloc_size);

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static FILE*
allocator_staticarena__fopen(const char* filename, const char* mode)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    FILE* res = fopen(filename, mode);

#ifndef NDEBUG
    if (res != NULL) {
        allocator_staticarena_s* a = &allocator__staticarena_data;
        a->stats.n_fopen++;
    }
#endif

    return res;
}

static int
allocator_staticarena__fclose(FILE* f)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

#ifndef NDEBUG
    allocator_staticarena_s* a = &allocator__staticarena_data;
    a->stats.n_fclose++;
#endif

    return fclose(f);
}
static int
allocator_staticarena__open(const char* pathname, int flags, unsigned int mode)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);

#ifndef NDEBUG
    if (fd != -1) {
        allocator__staticarena_data.stats.n_open++;
    }
#endif
    return fd;
}

static int
allocator_staticarena__close(int fd)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    int ret = close(fd);

#ifndef NDEBUG
    if (ret != -1) {
        allocator__staticarena_data.stats.n_close++;
    }
#endif

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