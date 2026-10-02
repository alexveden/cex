#include <stdint.h>

#define cex$platform_oom_panic _oom_panic
static void _oom_panic(const char*, const char*, uint32_t, const char*, const char*);

#include "src/all.c"

static u32 _oom_panic_count = 0;
static const char* _oom_panic_msg = NULL;
static const char* _oom_panic_prefix = NULL;

static void
_oom_panic(const char* prefix, const char* file, u32 line, const char* func, const char* msg)
{
    (void)file;
    (void)line;
    (void)func;
    _oom_panic_count++;
    _oom_panic_msg = msg;
    _oom_panic_prefix = prefix;
}

static void
_oom_panic_reset(void)
{
    _oom_panic_count = 0;
    _oom_panic_msg = NULL;
    _oom_panic_prefix = NULL;
}

test$case(test_heap_oom_panic_on_invalid_malloc)
{
    _oom_panic_reset();
    uassert_disable();
    u8* p = mem$malloc(mem$, 0);
    uassert_enable();

    tassert(p == NULL);
    tassert_eq(_oom_panic_count, 1);
    tassert(strcmp(_oom_panic_prefix, "[MEMORY] ") == 0);
    tassert(strcmp(_oom_panic_msg, "allocation size is zero") == 0);
    return EOK;
}

test$case(test_heap_oom_panic_on_calloc_zero)
{
    _oom_panic_reset();
    uassert_disable();
    void* p = mem$->calloc(mem$, 0, 8, 0);
    uassert_enable();

    tassert(p == NULL);
    tassert_eq(_oom_panic_count, 1);
    tassert(strcmp(_oom_panic_msg, "element count is zero or too high") == 0);
    return EOK;
}

test$case(test_heap_oom_panic_on_realloc_null)
{
    _oom_panic_reset();
    uassert_disable();
    void* p = mem$->realloc(mem$, NULL, 16, 0);
    uassert_enable();

    tassert(p == NULL);
    tassert_eq(_oom_panic_count, 1);
    tassert(strcmp(_oom_panic_msg, "realloc of NULL") == 0);
    return EOK;
}

test$main();
