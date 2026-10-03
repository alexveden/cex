#include <stdint.h>

#define cex$platform_mem_panic _mem_panic
static void _mem_panic(const char*, const char*, uint32_t, const char*, const char*);

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

test$case(test_heap_mem_panic_on_invalid_malloc)
{
    _mem_panic_reset();
    uassert_disable();
    u8* p = mem$malloc(mem$, 0);
    uassert_enable();

    tassert(p == NULL);
    tassert_eq(_mem_panic_count, 1);
    tassert(strcmp(_mem_panic_prefix, "[MEMORY] ") == 0);
    tassert(strcmp(_mem_panic_msg, "allocation size is zero") == 0);
    return EOK;
}

test$case(test_heap_mem_panic_on_calloc_zero)
{
    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->calloc(mem$, 0, 8, 0);
    uassert_enable();

    tassert(p == NULL);
    tassert_eq(_mem_panic_count, 1);
    tassert(strcmp(_mem_panic_msg, "element count is zero or too high") == 0);
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
    tassert_eq(_mem_panic_count, 1);
    tassert(strcmp(_mem_panic_prefix, "[MEMORY] ") == 0);
    tassert(strcmp(_mem_panic_msg, "allocation size overflow") == 0);
    return EOK;
}

test$case(test_heap_mem_panic_on_realloc_null)
{
    _mem_panic_reset();
    uassert_disable();
    void* p = mem$->realloc(mem$, NULL, 16, 0);
    uassert_enable();

    tassert(p == NULL);
    tassert_eq(_mem_panic_count, 1);
    tassert(strcmp(_mem_panic_msg, "realloc of NULL") == 0);
    return EOK;
}

test$main();
