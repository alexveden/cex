#include <stdint.h>

#define cex$platform_oom_panic _arena_oom_panic
static void _arena_oom_panic(const char*, const char*, uint32_t, const char*, const char*);

#include "src/all.c"

static u32 _arena_oom_count = 0;
static const char* _arena_oom_msg = NULL;
static const char* _arena_oom_prefix = NULL;

static void
_arena_oom_panic(const char* prefix, const char* file, u32 line, const char* func, const char* msg)
{
    (void)file;
    (void)line;
    (void)func;
    _arena_oom_count++;
    _arena_oom_msg = msg;
    _arena_oom_prefix = prefix;
}

static void
_arena_oom_reset(void)
{
    _arena_oom_count = 0;
    _arena_oom_msg = NULL;
    _arena_oom_prefix = NULL;
}

#define _arena_oom_assert(expected_msg)                                                            \
    do {                                                                                           \
        tassert_eq(_arena_oom_count, 1);                                                           \
        tassert(strcmp(_arena_oom_prefix, "[MEMORY] ") == 0);                                      \
        tassert(strcmp(_arena_oom_msg, (expected_msg)) == 0);                                      \
    } while (0)

test$case(test_arena_oom_panic_create_invalid_page_size)
{
    _arena_oom_reset();
    uassert_disable();
    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 512 });
    uassert_enable();

    tassert(arena == NULL);
    _arena_oom_assert("invalid arena page size");
    return EOK;
}

test$case(test_arena_oom_panic_malloc_invalid_size)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    _arena_oom_reset();
    uassert_disable();
    void* p = mem$malloc(arena, 0);
    uassert_enable();

    tassert(p == NULL);
    _arena_oom_assert("invalid allocation size or alignment");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_oom_panic_calloc_invalid_nmemb)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    _arena_oom_reset();
    uassert_disable();
    void* p = arena->calloc(arena, CEX_ARENA_MAX_ALLOC + 1, 1, 8);
    uassert_enable();

    tassert(p == NULL);
    _arena_oom_assert("invalid element count");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_oom_panic_calloc_invalid_size)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    _arena_oom_reset();
    uassert_disable();
    void* p = arena->calloc(arena, 1, CEX_ARENA_MAX_ALLOC + 1, 8);
    uassert_enable();

    tassert(p == NULL);
    _arena_oom_assert("invalid element size");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_oom_panic_realloc_invalid_size)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    u8* p = mem$malloc(arena, 100);
    tassert(p != NULL);

    _arena_oom_reset();
    uassert_disable();
    u8* q = mem$realloc(arena, p, CEX_ARENA_MAX_ALLOC + 1);
    uassert_enable();

    tassert(q == NULL);
    _arena_oom_assert("invalid realloc size");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_oom_panic_request_page_too_large)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 1024, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    allocator_arena_rec_s rec = { 0 };
    _cex_arena_rec_set_size(&rec, CEX_ARENA_MAX_ALLOC - 1024);

    _arena_oom_reset();
    uassert_disable();
    allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(allc, rec, NULL);
    uassert_enable();

    tassert(page == NULL);
    _arena_oom_assert("arena page size is too large");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$main();
