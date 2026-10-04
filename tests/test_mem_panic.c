#include <stdint.h>
#include <stdlib.h>

#define cex$platform_mem_panic _mem_panic
static void _mem_panic(const char*, const char*, uint32_t, const char*, const char*);

static int _mem_fail_malloc = 0;
static int _mem_fail_calloc = 0;
static int _mem_fail_realloc = 0;

static void*
_mem_malloc(size_t size)
{
    return _mem_fail_malloc ? NULL : malloc(size);
}
static void*
_mem_calloc(size_t nmemb, size_t size)
{
    return _mem_fail_calloc ? NULL : calloc(nmemb, size);
}
static void*
_mem_realloc(void* ptr, size_t size)
{
    return _mem_fail_realloc ? NULL : realloc(ptr, size);
}

#define cex$platform_malloc  _mem_malloc
#define cex$platform_calloc  _mem_calloc
#define cex$platform_realloc _mem_realloc

#include "src/all.c"

static u32 _mem_panic_count = 0;
static const char* _mem_panic_msg = NULL;
static const char* _mem_panic_prefix = NULL;

static void
_mem_panic(const char* prefix, const char* file, u32 line, const char* func, const char* msg)
{
    (void)file;
    (void)line;
    (void)func;
    _mem_panic_count++;
    _mem_panic_msg = msg;
    _mem_panic_prefix = prefix;
}

static void
_mem_panic_reset(void)
{
    _mem_panic_count = 0;
    _mem_panic_msg = NULL;
    _mem_panic_prefix = NULL;
}

#define _mem_panic_assert(expected_msg)                                                            \
    do {                                                                                           \
        tassert_eq(_mem_panic_count, 1);                                                           \
        tassert(strcmp(_mem_panic_prefix, "[MEMORY] ") == 0);                                      \
        tassert(strcmp(_mem_panic_msg, (expected_msg)) == 0);                                      \
    } while (0)

test$case(test_heap_mem_panic_on_invalid_malloc)
{
    _mem_panic_reset();
    uassert_disable();
    u8* p = mem$malloc(mem$, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("allocation size is zero");
    return EOK;
}

test$case(test_heap_mem_panic_size_too_large)
{
    _mem_panic_reset();
    uassert_disable();
    u8* p = mem$->malloc(mem$, mem$MAX, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("allocation size is too large");
    return EOK;
}

test$case(test_heap_mem_panic_alignment_too_large)
{
    _mem_panic_reset();
    uassert_disable();
    u8* p = mem$->malloc(mem$, 100, 128);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("allocation alignment is too large");
    return EOK;
}

test$case(test_heap_mem_panic_size_exceeds_48bit)
{
#if mem$platform() == 64
    _mem_panic_reset();
    uassert_disable();
    u8* p = mem$->malloc(mem$, 0x1000000000000ULL, 8);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("allocation size exceeds 48 bits");
#endif
    return EOK;
}

test$case(test_heap_mem_panic_size_not_aligned)
{
    _mem_panic_reset();
    uassert_disable();
    u8* p = mem$->malloc(mem$, 100, 16);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("requested size is not aligned");
    return EOK;
}

test$case(test_heap_mem_panic_on_calloc_zero)
{
    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->calloc(mem$, 0, 8, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("element count is zero or too high");
    return EOK;
}

test$case(test_heap_mem_panic_calloc_nmemb_too_high)
{
    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->calloc(mem$, mem$MAX, 1, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("element count is zero or too high");
    return EOK;
}

test$case(test_heap_mem_panic_calloc_size_zero)
{
    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->calloc(mem$, 1, 0, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("element size is zero or too high");
    return EOK;
}

test$case(test_heap_mem_panic_calloc_size_too_high)
{
    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->calloc(mem$, 1, mem$MAX, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("element size is zero or too high");
    return EOK;
}

test$case(test_heap_mem_panic_on_calloc_overflow)
{
    // both operands pass the individual checks, but nmemb * size wraps
    usize big = (usize)1 << (mem$platform() / 2);

    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->calloc(mem$, big, big, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("allocation size overflow");
    return EOK;
}

test$case(test_heap_mem_panic_on_realloc_null)
{
    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->realloc(mem$, NULL, 16, 0);
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("realloc of NULL");
    return EOK;
}

test$case(test_heap_mem_panic_realloc_alignment_mismatch)
{
    u8* p = mem$malloc(mem$, 32);
    tassert(p != NULL);

    _mem_panic_reset();
    uassert_disable();
    void* q = mem$->realloc(mem$, p, 64, 16);
    uassert_enable();

    tassert(q == NULL);
    _mem_panic_assert("realloc alignment mismatch");
    return EOK;
}

test$case(test_heap_mem_panic_malloc_oom)
{
    _mem_panic_reset();
    uassert_disable();
    _mem_fail_malloc = 1;
    u8* p = mem$->malloc(mem$, 100, 0);
    _mem_fail_malloc = 0;
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("out of memory");
    return EOK;
}

test$case(test_heap_mem_panic_calloc_oom)
{
    _mem_panic_reset();
    uassert_disable();
    _mem_fail_calloc = 1;
    void* p = mem$->calloc(mem$, 1, 100, 0);
    _mem_fail_calloc = 0;
    uassert_enable();

    tassert(p == NULL);
    _mem_panic_assert("out of memory");
    return EOK;
}

test$case(test_heap_mem_panic_realloc_oom)
{
    u8* p = mem$malloc(mem$, 100);
    tassert(p != NULL);

    _mem_panic_reset();
    uassert_disable();
    _mem_fail_realloc = 1;
    void* q = mem$->realloc(mem$, p, 100, 0);
    _mem_fail_realloc = 0;
    uassert_enable();

    tassert(q == NULL);
    _mem_panic_assert("out of memory");
    return EOK;
}

test$case(test_heap_mem_panic_realloc_oom_fallback_malloc)
{
    // alignment above max_align_t makes realloc fall back to malloc + memcpy
    u8* p = mem$malloc(mem$, 128, 64);
    tassert(p != NULL);

    _mem_panic_reset();
    uassert_disable();
    _mem_fail_malloc = 1;
    void* q = mem$->realloc(mem$, p, 128, 64);
    _mem_fail_malloc = 0;
    uassert_enable();

    tassert(q == NULL);
    _mem_panic_assert("out of memory");
    return EOK;
}

test$main();
