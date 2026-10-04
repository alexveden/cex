#include <stdint.h>

#define cex$platform_mem_panic _arena_mem_panic
static void _arena_mem_panic(const char*, const char*, uint32_t, const char*, const char*);

#include "src/all.c"

static u32 _arena_mem_count = 0;
static const char* _arena_mem_msg = NULL;
static const char* _arena_mem_prefix = NULL;

static void
_arena_mem_panic(const char* prefix, const char* file, u32 line, const char* func, const char* msg)
{
    (void)file;
    (void)line;
    (void)func;
    _arena_mem_count++;
    _arena_mem_msg = msg;
    _arena_mem_prefix = prefix;
}

static void
_arena_mem_reset(void)
{
    _arena_mem_count = 0;
    _arena_mem_msg = NULL;
    _arena_mem_prefix = NULL;
}

#define _arena_mem_assert(expected_msg)                                                            \
    do {                                                                                           \
        tassert_eq(_arena_mem_count, 1);                                                           \
        tassert(strcmp(_arena_mem_prefix, "[MEMORY] ") == 0);                                      \
        tassert(strcmp(_arena_mem_msg, (expected_msg)) == 0);                                      \
    } while (0)

test$case(test_arena_mem_panic_create_invalid_page_size)
{
    _arena_mem_reset();
    uassert_disable();
    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 512 });
    uassert_enable();

    tassert(arena == NULL);
    _arena_mem_assert("arena page size is too small or too large");
    return EOK;
}

test$case(test_arena_mem_panic_malloc_invalid_size)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    _arena_mem_reset();
    uassert_disable();
    void* p = mem$malloc(arena, 0);
    uassert_enable();

    tassert(p == NULL);
    _arena_mem_assert("allocation size is zero");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_mem_panic_calloc_invalid_nmemb)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    _arena_mem_reset();
    uassert_disable();
    void* p = arena->calloc(arena, CEX_ARENA_MAX_ALLOC + 1, 1, 8);
    uassert_enable();

    tassert(p == NULL);
    _arena_mem_assert("element count is too large");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_mem_panic_calloc_invalid_size)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    _arena_mem_reset();
    uassert_disable();
    void* p = arena->calloc(arena, 1, CEX_ARENA_MAX_ALLOC + 1, 8);
    uassert_enable();

    tassert(p == NULL);
    _arena_mem_assert("element size is too large");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_mem_panic_calloc_overflow)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    // both operands pass the individual CEX_ARENA_MAX_ALLOC checks, but the product wraps
    usize big = (usize)1 << (mem$platform() / 2);

    _arena_mem_reset();
    uassert_disable();
    void* p = arena->calloc(arena, big, big, 8);
    uassert_enable();

    tassert(p == NULL);
    _arena_mem_assert("allocation size overflow");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_mem_panic_negative_size_rejected)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    // a negative value cast to usize exceeds PTRDIFF_MAX and must be rejected as OOM
    _arena_mem_reset();
    uassert_disable();
    void* p = mem$malloc(arena, (usize)-1);
    uassert_enable();
    tassert(p == NULL);
    _arena_mem_assert("allocation size is too large");

    _arena_mem_reset();
    uassert_disable();
    p = arena->calloc(arena, (usize)-1, 1, 8);
    uassert_enable();
    tassert(p == NULL);
    _arena_mem_assert("element count is too large");

    _arena_mem_reset();
    uassert_disable();
    p = arena->calloc(arena, 1, (usize)-1, 8);
    uassert_enable();
    tassert(p == NULL);
    _arena_mem_assert("element size is too large");

    u8* q = mem$malloc(arena, 100);
    tassert(q != NULL);

    _arena_mem_reset();
    uassert_disable();
    p = mem$realloc(arena, q, (usize)-1);
    uassert_enable();
    tassert(p == NULL);
    _arena_mem_assert("realloc size is too large");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_mem_panic_realloc_invalid_size)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    u8* p = mem$malloc(arena, 100);
    tassert(p != NULL);

    _arena_mem_reset();
    uassert_disable();
    u8* q = mem$realloc(arena, p, CEX_ARENA_MAX_ALLOC + 1);
    uassert_enable();

    tassert(q == NULL);
    _arena_mem_assert("realloc size is too large");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_mem_panic_alignment_too_large)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    _arena_mem_reset();
    uassert_disable();
    void* p = arena->malloc(arena, 100, 128);
    uassert_enable();

    tassert(p == NULL);
    _arena_mem_assert("allocation alignment is too large");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_arena_mem_panic_size_not_aligned)
{
    _arena_mem_reset();
    uassert_disable();
    allocator_arena_rec_s rec = _cex_alloc_estimate_alloc_size(17, 16);
    uassert_enable();

    tassert(rec.size_low == 0 && rec.size_high == 0);
    _arena_mem_assert("requested size is not aligned");
    return EOK;
}

test$case(test_arena_mem_panic_request_page_too_large)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 1024, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    allocator_arena_rec_s rec = { 0 };
    _cex_arena_rec_set_size(&rec, CEX_ARENA_MAX_ALLOC - 1024);

    _arena_mem_reset();
    uassert_disable();
    allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(allc, rec, NULL);
    uassert_enable();

    tassert(page == NULL);
    _arena_mem_assert("arena page size is zero or too large");

    AllocatorArena_destroy(arena);
    return EOK;
}

test$main();
