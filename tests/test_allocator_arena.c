#include "src/all.c"

#define alloc_cmp(alloc_size, align, exp_size, exp_padding, exp_align)                              \
    ({                                                                                             \
        allocator_arena_rec_s _r = _cex_alloc_estimate_alloc_size((alloc_size), (align));           \
        u64 _gs = _cex_arena_rec_get_size(&_r);                                                    \
        u8 _gp = _r.ptr_padding;                                                                   \
        u8 _ga = _cex_arena_rec_get_align(&_r);                                                    \
        int _ok = 1;                                                                               \
        if (_gs != (u64)(exp_size) || _gp != (u8)(exp_padding) || _ga != (u8)(exp_align)) {        \
            printf("Mismatch: size=%lu padding=%u align=%u (expected %lu %u %u)\n",                \
                   (unsigned long)_gs, (unsigned)_gp, (unsigned)_ga,                               \
                   (unsigned long)(u64)(exp_size), (unsigned)(u8)(exp_padding),                     \
                   (unsigned)(u8)(exp_align));                                                      \
            _ok = 0;                                                                               \
        }                                                                                          \
        _ok;                                                                                       \
    })

test$case(test_allocator_arena_alloc_size)
{
    tassert_eq(sizeof(allocator_arena_rec_s), 8);

    tassert(alloc_cmp(1, 0, 1, 7, 8));
    tassert(alloc_cmp(5, 0, 5, 3, 8));
    tassert(alloc_cmp(8, 0, 8, 8, 8));
    tassert(alloc_cmp(16, 0, 16, 8, 8));
    tassert(alloc_cmp(100, 0, 100, 4, 8));

    tassert(alloc_cmp(8, 8, 8, 8, 8));
    tassert(alloc_cmp(16, 8, 16, 8, 8));

    tassert(alloc_cmp(16, 16, 16, 8, 16));
    tassert(alloc_cmp(32, 16, 32, 8, 16));
    tassert(alloc_cmp(48, 16, 48, 8, 16));
    tassert(alloc_cmp(64, 16, 64, 8, 16));

    tassert(alloc_cmp(64, 64, 64, 8, 64));
    tassert(alloc_cmp(128, 64, 128, 8, 64));
    tassert(alloc_cmp(192, 64, 192, 8, 64));
    tassert(alloc_cmp(256, 64, 256, 8, 64));

    return EOK;
}

test$case(test_allocator_arena_40bit_size)
{
    // size just above 32-bit boundary
    allocator_arena_rec_s r;

#if mem$platform() > 32
    // (UINT32_MAX + 1) = 0x100000000, 8-aligned → padding = 8
    r = _cex_alloc_estimate_alloc_size((u64)UINT32_MAX + 1, 0);
    tassert_eq(_cex_arena_rec_get_size(&r), (u64)UINT32_MAX + 1);
    tassert_eq(r.ptr_padding, 8);
    tassert_eq(_cex_arena_rec_get_align(&r), 8);

    // (UINT32_MAX + 2) = 0x100000001, not 8-aligned → padding = 7
    r = _cex_alloc_estimate_alloc_size((u64)UINT32_MAX + 2, 0);
    tassert_eq(_cex_arena_rec_get_size(&r), (u64)UINT32_MAX + 2);
    tassert_eq(r.ptr_padding, 7);
    tassert_eq(_cex_arena_rec_get_align(&r), 8);

    // (UINT32_MAX + 9) = 0x100000008, 8-aligned → padding = 8
    r = _cex_alloc_estimate_alloc_size((u64)UINT32_MAX + 9, 0);
    tassert_eq(_cex_arena_rec_get_size(&r), (u64)UINT32_MAX + 9);
    tassert_eq(r.ptr_padding, 8);
    tassert_eq(_cex_arena_rec_get_align(&r), 8);

    // (UINT32_MAX + 17) = 0x100000010, 16-aligned → padding = 8
    r = _cex_alloc_estimate_alloc_size((u64)UINT32_MAX + 17, 16);
    tassert_eq(_cex_arena_rec_get_size(&r), (u64)UINT32_MAX + 17);
    tassert_eq(r.ptr_padding, 8);
    tassert_eq(_cex_arena_rec_get_align(&r), 16);

    r = _cex_alloc_estimate_alloc_size((1ULL << 40) - 1000, 8);
    tassert_eq(_cex_arena_rec_get_size(&r), (1ULL << 40) - 1000);
    tassert_eq(r.ptr_padding, 8);
    tassert_eq(_cex_arena_rec_get_align(&r), 8);
#endif

    // round-trip: set/get
    u64 sizes[] = { 0, 1, UINT32_MAX, (u64)UINT32_MAX + 1, (1ULL << 40) - 1000 };
    for$each (s, sizes) {
        _cex_arena_rec_set_size(&r, s);
        tassert_eq(_cex_arena_rec_get_size(&r), s);
    }

    return EOK;
}

test$case(test_allocator_arena_oversized)
{
    uassert_disable();

    // 1. Estimator returns zero struct for too-large sizes
    allocator_arena_rec_s r;
    r = _cex_alloc_estimate_alloc_size(CEX_ARENA_MAX_ALLOC + 1, 0);
    tassert_eq(r.size_low, 0);
    tassert_eq(r.size_high, 0);

#if mem$platform() > 32
    r = _cex_alloc_estimate_alloc_size((1ULL << 50), 0);
    tassert_eq(r.size_low, 0);
    tassert_eq(r.size_high, 0);
#endif

    // 2. malloc returns NULL for too-large size
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    void* p = mem$malloc(arena, CEX_ARENA_MAX_ALLOC + 1);
    tassert(p == NULL);

    // 3. calloc returns NULL for too-large nmemb or size
    p = arena->calloc(arena, CEX_ARENA_MAX_ALLOC + 1, 1, 8);
    tassert(p == NULL);
    p = arena->calloc(arena, 1, CEX_ARENA_MAX_ALLOC + 1, 8);
    tassert(p == NULL);

    // 4. realloc returns NULL for too-large size
    u8* p2 = mem$malloc(arena, 100);
    tassert(p2 != NULL);
    u8* p3 = mem$realloc(arena, p2, CEX_ARENA_MAX_ALLOC + 1);
    tassert(p3 == NULL);

    AllocatorArena_destroy(arena);
    uassert_enable();
    return EOK;
}


test$case(test_allocator_arena_create_destroy)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });
    tassert(arena != NULL);
    tassert(arena != tmem$);
    tassert(arena != mem$);
    tassert(mem$aligned_pointer(arena, alignof(AllocatorArena_c)) == arena);
    tassert_eq(arena->scope_depth(arena), 1);

    u8* p = mem$malloc(arena, 100);
    tassert(p != NULL);


    tassert_eq(mem$->scope_depth(mem$), 1);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, tal)
    {
        (void)tal;
        tassert_eq(allc->scope_depth, 2);
        tassert_eq(arena->scope_depth(arena), 2);
        tassert_eq(mem$->scope_depth(mem$), 1);

        mem$scope(arena, tal)
        {
            (void)tal;
            tassert_eq(allc->scope_depth, 3);
            tassert_eq(arena->scope_depth(arena), 3);
            tassert_eq(mem$->scope_depth(mem$), 1);
            mem$scope(arena, tal)
            {
                (void)tal;
                tassert_eq(allc->scope_depth, 4);
                tassert_eq(mem$->scope_depth(mem$), 1);
                tassert_eq(arena->scope_depth(arena), 4);
            }
            tassert_eq(allc->scope_depth, 3);
        }
        tassert_eq(allc->scope_depth, 2);
    }
    tassert_eq(allc->scope_depth, 1);
    tassert_eq(arena->scope_depth(arena), 1);

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_malloc)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });
    tassert(arena != NULL);

    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        u8* p = mem$malloc(_, 100);
        tassert(p != NULL);
        // tassert(mem$asan_enabled());
        // p[-2] = 1;
        // tassert_eq(p[100], 1); //   GOOD ASAN poison!
        // tassert_eq(p[-3], 0xf7);   //GOOD ASAN poison!


        // NOTE: includes size + alignment offset + padding + allocator_arena_rec_s
        tassert_eq(allc->stats.bytes_alloc, 112);
        tassert_eq(allc->used, 112);

        tassert_eq(allc->scope_depth, 2);
        tassert_eq(allc->scope_stack[0], 0);
        tassert_eq(allc->scope_stack[1], 0);
        tassert_eq(allc->scope_stack[2], 0);
        tassert_eq(allc->last_page->used_start, 0);
        tassert_eq(allc->last_page->cursor, 112);
        tassert(allc->last_page->last_alloc == p);

        mem$scope(arena, _)
        {
            tassert_eq(allc->scope_depth, 3);
            tassert_eq(allc->scope_stack[0], 0);
            tassert_eq(allc->scope_stack[1], 0);
            tassert_eq(allc->scope_stack[2], 112);

            char* p2 = mem$malloc(_, 4);
            tassert(p2 != NULL);
            tassert_eq(allc->stats.bytes_alloc, 112 + 16);
            tassert_eq(allc->used, 112 + 16);
            tassert_eq(allc->last_page->cursor, 112 + 16);
            tassert(allc->last_page->last_alloc == p2);

            mem$scope(arena, _)
            {
                char* p3 = mem$malloc(_, 4);
                tassert(p3 != NULL);
                tassert_eq(allc->stats.bytes_alloc, 112 + 16 + 16);
                tassert_eq(allc->used, 112 + 16 + 16);
                tassert_eq(allc->last_page->cursor, 112 + 16 + 16);

                tassert_eq(allc->scope_depth, 4);
                tassert_eq(allc->scope_stack[0], 0);
                tassert_eq(allc->scope_stack[1], 0);
                tassert_eq(allc->scope_stack[2], 112);
                tassert_eq(allc->scope_stack[3], 112 + 16);
                tassert(allc->last_page->last_alloc == p3);
                AllocatorArena_sanitize(arena);
            }
            // Unwinding arena
            tassert_eq(allc->used, 112 + 16);
            tassert_eq(allc->last_page->cursor, 112 + 16);
        }
        // Unwinding arena
        tassert_eq(allc->used, 112);
        tassert_eq(allc->last_page->cursor, 112);
    }
    // Unwinding arena
    tassert_eq(allc->used, 0);
    tassert_eq(allc->last_page->cursor, 0);

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_malloc_pointer_alignment)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 * 100 });
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        char* _p = mem$malloc(_, 100);
        tassert(_p != NULL);
        allocator_arena_page_s* page = allc->last_page;
        tassert(page != NULL);

        u32 align[] = { 8, 16, 32, 64 };

        for (int i = 0; i < 100; i++) {
            char* p = mem$malloc(arena, i % 32 + 1);
            tassert(p != NULL);
            tassert(mem$aligned_pointer(p, 8) == p);

            for$each (alignment, align) {
                tassert(page == allc->last_page && "this test case expected to use one page");
                tassert(alignment >= 8);
                tassert(alignment <= 64);
                tassert(mem$is_power_of2(alignment));
                usize alloc_size = alignment * (i % 4 + 1);
                char* ptr_algn = arena->calloc(arena, 1, alloc_size, alignment);
                for (u32 j = 0; j < alloc_size; j++) { tassert(ptr_algn[j] == 0); }
                memset(ptr_algn, 0xAA, alloc_size);
                tassert(ptr_algn != NULL);
                // ensure returned pointers are aligned

                uassert(((usize)(ptr_algn) & ((alignment)-1)) == 0);
                allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(ptr_algn);
                tassert_eq(_cex_arena_rec_get_align(rec), alignment);
                tassert_eq(_cex_arena_rec_get_size(rec), alloc_size);
                tassert(!_cex_arena_rec_is_free(rec));

                if (i % 2 == 0) {
                    tassert(arena->free(arena, ptr_algn) == NULL);
                    tassert(mem$asan_poison_check(ptr_algn, alloc_size));
                    tassert(_cex_arena_rec_is_free(rec));
                } else {
                    usize alloc_size2 = alignment * (i % 4 + 2);
                    tassert(alloc_size2 > alloc_size);

                    char* ptr_algn2 = arena->realloc(arena, ptr_algn, alloc_size2, alignment);
                    tassert(ptr_algn2);
                    tassert_eq(_cex_arena_rec_get_align(rec), alignment);
                    tassert_eq(_cex_arena_rec_get_size(rec), alloc_size2);
                    tassert(!_cex_arena_rec_is_free(rec));
                }

                AllocatorArena_sanitize(arena);
            }

            char* p2 = mem$realloc(arena, p, i % 32 + 100);
            tassert(p2 != NULL);
            tassert(mem$aligned_pointer(p2, 8) == p2);
            AllocatorArena_sanitize(arena);
        }
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_scope_sanitization)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        (void)_;
        u32 align[] = { 8, 16, 32, 64 };


        for (int i = 0; i < 10; i++) {
            mem$scope(arena, _)
            {
                char* p = mem$malloc(_, i % 32 + 1);
                tassert(p != NULL);
                tassert(mem$aligned_pointer(p, 8) == p);

                for$each (alignment, align) {
                    tassert(alignment >= 8);
                    tassert(alignment <= 64);
                    tassert(mem$is_power_of2(alignment));
                    usize alloc_size = alignment * (i % 4 + 1);
                    char* p = arena->malloc(arena, alloc_size, alignment);
                    memset(p, 0xAA, alloc_size);
                    tassert(p != NULL);
                    // ensure returned pointers are aligned

                    uassert(((usize)(p) & ((alignment)-1)) == 0);
                    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
                    tassert_eq(_cex_arena_rec_get_align(rec), alignment);
                    tassert_eq(_cex_arena_rec_get_size(rec), alloc_size);
                    tassert(!_cex_arena_rec_is_free(rec));

                    tassert(arena->free(arena, p) == NULL);
                    tassert(mem$asan_poison_check(p, alloc_size));
                    tassert(_cex_arena_rec_is_free(rec));

                    AllocatorArena_sanitize(arena);
                }
            }
            AllocatorArena_sanitize(arena);
        }
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_realloc)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });
    tassert(arena != NULL);

    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        char* p = mem$malloc(_, 100);
        tassert(p != NULL);
        memset(p, 0xAA, 100);
        // NOTE: includes size + alignment offset + padding + allocator_arena_rec_s
        tassert_eq(allc->stats.bytes_alloc, 112);
        tassert_eq(allc->used, 112);
        AllocatorArena_sanitize(arena);

        char* p2 = mem$malloc(arena, 100);
        tassert(p2 != NULL);
        memset(p2, 0xAA, 100);
        tassert_eq(allc->stats.bytes_alloc, 112 + 112);
        tassert_eq(allc->used, 112 + 112);
        AllocatorArena_sanitize(arena);

        char* p3 = mem$realloc(arena, p, 200);
        tassert(p3 != NULL);
        tassert(p3 != p);
        memset(p3, 0xAA, 100);
        tassert_eq(allc->stats.bytes_alloc, 112 + 112 + 216);
        tassert_eq(allc->used, 112 + 112 + 216);
        AllocatorArena_sanitize(arena);

        allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
        tassert_eq(_cex_arena_rec_get_align(rec), 8);
        tassert_eq(_cex_arena_rec_get_size(rec), 100);
        tassert(_cex_arena_rec_is_free(rec));

        rec = _cex_alloc_arena__get_rec(p3);
        tassert_eq(_cex_arena_rec_get_align(rec), 8);
        tassert_eq(_cex_arena_rec_get_size(rec), 200);
        tassert(!_cex_arena_rec_is_free(rec));
        tassert(allc->last_page->last_alloc == p3);

        // Extending last pointer!
        char* p4 = mem$realloc(arena, p3, 300);
        tassert(p4 != NULL);
        tassert(p3 == p4);
        memset(p3, 0xAA, 300);
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_page_size)
{
    usize page_size = 1024;
    for (u32 i = 0; i < 10000; i += 10) {
        usize est_page_size = _cex_alloc_estimate_page_size(page_size, i);
        usize base_page_size = mem$aligned_round(
            page_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
            alignof(allocator_arena_page_s)
        );

        tassert(est_page_size >= page_size);
        if (i > 0.7 * base_page_size) {
            if (i > 1024 * 1024) {
                uassert(est_page_size <= i * 1.2 + CEX_ARENA_MAX_ALIGN * 3);
            } else {
                uassert(est_page_size <= i * 2 + CEX_ARENA_MAX_ALIGN * 3);
            }
        } else {
            uassert(est_page_size == base_page_size);
        }
        tassert(est_page_size % alignof(allocator_arena_page_s) == 0);
    }
    return EOK;
}

test$case(test_allocator_arena_multiple_pages)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 1024 });
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;
    tassert(arena != NULL);
    allocator_arena_page_s* page = NULL;

    mem$scope(arena, _)
    {
        char* p = mem$malloc(_, 1000);
        tassert(p != NULL);

        page = allc->last_page;
        tassert(page != NULL);
        tassert(page->prev_page == NULL);
        tassert_eq(allc->used, 1016);
        tassert_eq(page->cursor, 1016);


        // next allocated on next page
        char* p2 = mem$malloc(arena, 1200);
        tassert(p2 != NULL);
        tassert(page != NULL);
        tassert(allc->last_page != NULL);
        tassert(allc->last_page != page);
        tassert(page->prev_page == NULL);
        tassert(allc->last_page->prev_page == page);
        tassert_eq(allc->used, 2232);
    }

    tassert(allc->last_page != NULL);
    tassert(allc->last_page == page); // replaced by first!
    tassert(allc->last_page->prev_page == NULL);
    tassert(allc->last_page->cursor == 0);
    tassert_eq(allc->used, 0);
    tassert_eq(allc->stats.pages_created, 2);
    tassert_eq(allc->stats.pages_free, 1); // last page should be still active

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_realloc_shrink)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });
    tassert(arena != NULL);

    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        char* p = mem$malloc(_, 100);
        tassert(p != NULL);
        memset(p, 0xAA, 100);
        // NOTE: includes size + alignment offset + padding + allocator_arena_rec_s
        tassert_eq(allc->stats.bytes_alloc, 112);
        tassert_eq(allc->used, 112);
        AllocatorArena_sanitize(arena);

        allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
        tassert_eq(_cex_arena_rec_get_align(rec), 8);
        u64 rsize = _cex_arena_rec_get_size(rec);
        tassert_eq(rsize, 100);
        tassert_eq(rec->ptr_padding, 4);
        tassert(!_cex_arena_rec_is_free(rec));
        tassert(mem$asan_poison_check(p + rsize, rec->ptr_padding));

        // same size just ignored
        char* p2 = mem$realloc(arena, p, 100);
        tassert(p2 != NULL);
        tassert(p2 == p);
        tassert_eq(allc->stats.bytes_alloc, 112);
        tassert_eq(allc->used, 112);
        tassert_eq(_cex_arena_rec_get_align(rec), 8);
        rsize = _cex_arena_rec_get_size(rec);
        tassert_eq(rsize, 100);
        tassert_eq(rec->ptr_padding, 4);
        tassert(!_cex_arena_rec_is_free(rec));
        tassert(mem$asan_poison_check(p + rsize, rec->ptr_padding));
        AllocatorArena_sanitize(arena);

        char* p3 = mem$realloc(arena, p, 50);
        tassert(p3 != NULL);
        tassert(p3 == p);
        tassert(p3 == p2);
        tassert_eq(allc->stats.bytes_alloc, 112);
        tassert_eq(allc->used, 112);
        // shrink is no-op — verify tail NOT poisoned after shrink
        // (regression: old code poisoned bytes [50,104) which caused ASAN use-after-poison
        //  on subsequent realloc growth that memcpy's rec->size bytes from old_ptr)
        rsize = _cex_arena_rec_get_size(rec);
        tassert(!mem$asan_poison_check(p + 50, rsize - 50 + rec->ptr_padding));
        tassert_eq(rsize, 100);
        tassert_eq(rec->ptr_padding, 4);
        tassert(!_cex_arena_rec_is_free(rec));
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_realloc_shrink_then_grow)
{
    // Regression: realloc shrink → grow must NOT trigger ASAN use-after-poison.
    // Old code poisoned the tail during shrink; a subsequent realloc growth
    // path would memcpy rec->size bytes from old_ptr, reading stale poison.
    // This test exercises both growth paths: malloc+copy (non-last-alloc)
    // and in-place (last-alloc), with disable_scopes=true (matching fuzzer).

    // --- Scenario A: non-last-alloc → malloc+copy growth path ---
    {
        IAllocator arena = AllocatorArena.create(
            &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
        );
        tassert(arena != NULL);

        usize N = 100;
        u8* p = mem$malloc(arena, N);
        tassert(p != NULL);
        memset(p, 0xAB, N);

        // blocker so p is NOT last_alloc → realloc growth forces malloc+copy
        u8* blocker = mem$malloc(arena, 16);
        tassert(blocker != NULL);

        // shrink (no-op after fix)
        usize M = 50;
        u8* shrunk = mem$realloc(arena, p, M);
        tassert(shrunk == p);

        // verify first M bytes preserved
        for (u32 i = 0; i < M; i++) { tassert_eq(p[i], 0xAB); }

        // grow — triggers malloc+copy (p is not last_alloc)
        usize N2 = 150;
        u8* grown = mem$realloc(arena, p, N2);
        tassert(grown != NULL);
        tassert(grown != p); // must have moved

        // verify first M bytes preserved via memcpy
        for (u32 i = 0; i < M; i++) { tassert_eq(grown[i], 0xAB); }

        // verify entire new allocation writable (no ASAN poison)
        memset(grown, 0xBA, N2);
        for (u32 i = 0; i < N2; i++) { tassert_eq(grown[i], 0xBA); }

        AllocatorArena_sanitize(arena);
        AllocatorArena_destroy(arena);
    }

    // --- Scenario B: last-alloc → in-place growth path ---
    {
        IAllocator arena = AllocatorArena.create(
            &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
        );
        tassert(arena != NULL);

        usize N = 100;
        u8* p = mem$malloc(arena, N);
        tassert(p != NULL);
        memset(p, 0xCD, N); // p IS last_alloc

        // shrink (no-op after fix)
        usize M = 50;
        u8* shrunk = mem$realloc(arena, p, M);
        tassert(shrunk == p);

        // verify first M bytes preserved
        for (u32 i = 0; i < M; i++) { tassert_eq(p[i], 0xCD); }

        // grow in-place (p IS last_alloc)
        usize N2 = 150;
        u8* grown = mem$realloc(arena, p, N2);
        tassert(grown == p); // same pointer, in-place extension

        // KEY REGRESSION: read bytes [M, N) that old code poisoned during shrink.
        // In old code, in-place growth unpoisoned only [N, N2), leaving [M, N)
        // poisoned → ASAN use-after-poison on any read in that range.
        // After fix, shrink does nothing — all bytes accessible.
        for (u32 i = M; i < N; i++) { tassert_eq(p[i], 0xCD); }

        // verify entire extended allocation writable
        memset(grown, 0xDC, N2);
        for (u32 i = 0; i < N2; i++) { tassert_eq(grown[i], 0xDC); }

        AllocatorArena_sanitize(arena);
        AllocatorArena_destroy(arena);
    }

    return EOK;
}

test$case(test_allocator_arena_malloc_mem_pattern)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        u8* p = mem$malloc(_, 100);
        tassert(p != NULL);
        for (u32 i = 0; i < 100; i++) { tassert(p[i] == 0xf7); }
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_pointer_lifetime)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        u8* p = mem$malloc(_, 100);
        allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
        tassert(p != NULL);
        tassert((void*)rec > (void*)allc->last_page);
        tassert(
            (void*)rec <
            (void*)allc->last_page + sizeof(allocator_arena_page_s) + allc->last_page->capacity
        );
        tassert_eq(_cex_arena_rec_get_size(rec), 100);
        mem$scope(arena, _)
        {
            u8* p2 = mem$malloc(_, 100);
            tassert(p2 != NULL);
            allocator_arena_rec_s* rec2 = _cex_alloc_arena__get_rec(p2);
            tassert((void*)rec2 > (void*)allc->last_page);
            tassert(
                (void*)rec2 <
                (void*)allc->last_page + sizeof(allocator_arena_page_s) + allc->last_page->capacity
            );
            tassert_eq(_cex_arena_rec_get_size(rec2), 100);
            tassert(_cex_allocator_arena__check_pointer_valid(allc, p2));
            uassert_disable();
            tassert(!_cex_allocator_arena__check_pointer_valid(allc, p));
        }
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_realloc_last_pointer)
{

    IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 10024 });
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        for (u32 z = 0; z < 10; z++) {
            u8* p = mem$malloc(arena, 1);
            tassert(p != NULL);
            *p = 0;
            for (u32 i = 1; i < 200; i++) {
                u8* new_p = mem$realloc(_, p, i + 1);
                tassert(new_p != NULL);
                tassert(new_p == p);
                tassert_eq(p[i - 1], i - 1);
                p[i] = i;
            }
            for (u32 i = 0; i < 200; i++) { tassert_eq(p[i], i); }
        }
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_mem_scope_exit_oversized_page)
{

    mem$arena(1024, arena)
    {
        // allocate some memory
        u8* p = mem$malloc(arena, 128);
        allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
        tassert_eq(_cex_arena_rec_get_size(rec), 128);
        memset(p, 0xAA, 128);
        (void)p;
        mem$scope(arena, _)
        {
            // allocate some with reallocating page
            p = mem$malloc(_, 4096);
            memset(p, 0xBB, 4096);
            memset(p, 0xab, 1);
            (void)p;
            allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
            tassert_eq(_cex_arena_rec_get_size(rec), 4096);
        }
    }
    return EOK;
}

test$case(test_mem_arena)
{

    mem$arena(4096, arena)
    {
        u8* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        AllocatorArena_c* allc = (AllocatorArena_c*)arena;
        mem$scope(arena, tal)
        {
            tassert_eq(allc->scope_depth, 2);
            u8* p = mem$malloc(tal, 100);
            tassert(p != NULL);
        }
    }
    return EOK;
}

test$case(test_mem_arena_with_return)
{

    mem$arena(4096, arena)
    {
        u8* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        AllocatorArena_c* allc = (AllocatorArena_c*)arena;
        mem$scope(arena, tal)
        {
            tassert_eq(allc->scope_depth, 2);
            u8* p = mem$malloc(tal, 10040);
            tassert(p != NULL);
            return EOK;
        }
    }
    return EOK;
}

test$case(test_mem_arena_nested_cleanup_assert)
{

    mem$arena(4096, arena)
    {
        AllocatorArena_c* allc = (AllocatorArena_c*)arena;

        u8* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        memset(p, 0xaa, 100);

        // This needs extra page
        u8* p2 = mem$malloc(arena, 10040);
        tassert(p2 != NULL);
        memset(p2, 0xbb, 10040);

        mem$scope(arena, tal)
        {
            tassert_eq(allc->scope_depth, 2);
            tassert_eq(allc->stats.pages_created, 2);

            u8* p3 = mem$malloc(tal, 100);
            tassert(p3 != NULL);
            memset(p3, 0xcc, 100);

            tassert_eq(allc->stats.pages_created, 2);
            tassert_eq(allc->scope_depth, 2);
        }

        tassert(p2 != NULL);
        for$each (c, p2, 10040) { tassert(c == 0xbb); }
        for$each (c, p, 100) { tassert(c == 0xaa); }
    }
    return EOK;
}

test$case(test_mem_arena_kw_ptr)
{
    mem$arena(&(AllocatorArena_kw){ .page_size = 4096 }, arena)
    {
        u8* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        memset(p, 0xAB, 100);
        AllocatorArena_c* allc = (AllocatorArena_c*)arena;
        tassert_eq(allc->scope_depth, 1);
        tassert_eq(allc->page_size, 4096);
        tassert(!allc->disable_scopes);
    }
    return EOK;
}

test$case(test_mem_arena_kw_val)
{
    mem$arena((&(AllocatorArena_kw){ .page_size = 8192, .disable_scopes = true }), arena)
    {
        u8* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        memset(p, 0xCD, 100);
        AllocatorArena_c* allc = (AllocatorArena_c*)arena;
        tassert_eq(allc->page_size, 8192);
        tassert(allc->disable_scopes);
    }
    return EOK;
}

test$case(test_mem_arena_kw_disable_scopes)
{
    AllocatorArena_kw kw = { .page_size = 4096, .disable_scopes = true };
    mem$arena(&kw, arena)
    {
        u8* p = mem$malloc(arena, 128);
        tassert(p != NULL);
        memset(p, 0xEF, 128);
        AllocatorArena_c* allc = (AllocatorArena_c*)arena;
        tassert_eq(allc->scope_depth, 0);
        tassert(allc->disable_scopes);
    }
    return EOK;
}

test$case(test_allocator_arena_disable_scopes_create_destroy)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    // scope_depth stays at 0 — no implicit scope_enter
    tassert_eq(allc->scope_depth, 0);
    tassert_eq(arena->scope_depth(arena), 0);

    // Allocation works without mem$scope()
    u8* p = mem$malloc(arena, 100);
    tassert(p != NULL);
    memset(p, 0xAB, 100);
    for (u32 i = 0; i < 100; i++) { tassert(p[i] == 0xAB); }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_disable_scopes_malloc_free)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    u8* p = mem$malloc(arena, 256);
    tassert(p != NULL);
    memset(p, 0xCD, 256);
    for (u32 i = 0; i < 256; i++) { tassert(p[i] == 0xCD); }
    usize used_after_alloc = allc->used;
    tassert(used_after_alloc > 0);

    // free and verify poisoning
    tassert(arena->free(arena, p) == NULL);
    tassert(mem$asan_poison_check(p, 256));
    tassert_eq(allc->used, used_after_alloc); // used counter unchanged (free is a no-op for arena)

    // allocate again — same memory reused or new
    u8* p2 = mem$malloc(arena, 128);
    tassert(p2 != NULL);
    tassert(allc->stats.bytes_alloc > 0);

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_disable_scopes_realloc)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    // alloc + realloc larger (realloc not last pointer → malloc+copy)
    u8* p = mem$malloc(arena, 16);
    u8* p_guard = mem$malloc(arena, 16); // p isn't last_alloc
    tassert(p != NULL && p_guard != NULL);

    u8* p2 = mem$realloc(arena, p, 128);
    tassert(p2 != NULL);
    tassert(p2 != p); // new pointer
    memset(p2, 0xEF, 128);

    // realloc shrink
    u8* p3 = mem$realloc(arena, p2, 32);
    tassert(p3 == p2);
    for (u32 i = 0; i < 32; i++) { tassert(p3[i] == 0xEF); }

    // in-place growth on a fresh last-alloc
    u8* p4 = mem$malloc(arena, 33);
    tassert(p4 != NULL);
    memset(p4, 0xAA, 33);
    for (usize sz = 33; sz < 200; sz++) {
        u8* np = mem$realloc(arena, p4, sz + 1);
        tassert(np == p4);
        np[sz] = (u8)sz;
    }
    for (u32 i = 0; i < 33; i++) { tassert(p4[i] == 0xAA); }
    for (usize sz = 33; sz < 200; sz++) { tassert(p4[sz] == (u8)sz); }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_disable_scopes_scope_noop)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    tassert_eq(allc->scope_depth, 0);

    // scope_enter is a no-op
    arena->scope_enter(arena);
    tassert_eq(allc->scope_depth, 0);

    // scope_exit is a no-op
    arena->scope_exit(arena);
    tassert_eq(allc->scope_depth, 0);

    // mem$scope wrapper also a no-op (doesn't crash, doesn't free)
    u8* p = mem$malloc(arena, 64);
    tassert(p != NULL);
    memset(p, 0xFF, 64);

    u8* q = NULL;

    mem$scope(arena, _)
    {
        q = mem$malloc(_, 64);
        tassert(q != NULL);
        memset(q, 0xFE, 64);
        tassert_eq(allc->scope_depth, 0);
    }

    // memory still valid after scope_exit (no-op)
    tassert_eq(allc->scope_depth, 0);
    for (u32 i = 0; i < 64; i++) { tassert(p[i] == 0xFF); }

    // q is still available after scope exit
    for (u32 i = 0; i < 64; i++) { tassert(q[i] == 0xFE); }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_disable_scopes_multiple_pages)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 1024, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    // Allocate beyond single-page capacity to exercise multi-page paths.
    // Page auto-sizing may fit several allocs per page — we just verify
    // that allocs succeed, pages are created, and data survives.
    u8* p1 = mem$malloc(arena, 800);
    tassert(p1 != NULL);
    u8* p2 = mem$malloc(arena, 800);
    tassert(p2 != NULL);
    u8* p3 = mem$malloc(arena, 800);
    tassert(p3 != NULL);

    tassert(allc->stats.pages_created >= 1);

    memset(p1, 0x11, 800);
    memset(p2, 0x22, 800);
    memset(p3, 0x33, 800);
    for (u32 i = 0; i < 800; i++) {
        tassert(p1[i] == 0x11);
        tassert(p2[i] == 0x22);
        tassert(p3[i] == 0x33);
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_disable_scopes_alignment)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);

    u32 alignments[] = { 8, 16, 32, 64 };
    for$each (align, alignments) {
        usize sz = align * 2;
        u8* p = mem$malloc(arena, sz, align);
        tassert(p != NULL);
        tassert(((usize)p & (align - 1)) == 0);
        memset(p, 0xAA, sz);

        // calloc also works
        u8* q = arena->calloc(arena, 1, sz, align);
        tassert(q != NULL);
        tassert(((usize)q & (align - 1)) == 0);
        for (u32 i = 0; i < sz; i++) { tassert(q[i] == 0); }
        memset(q, 0xBB, sz);
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_disable_scopes_sanitize)
{
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    tassert(AllocatorArena_sanitize(arena));

    // mix of alloc, free, realloc
    u8* a = mem$malloc(arena, 100);
    u8* b = mem$malloc(arena, 200);
    u8* c = mem$malloc(arena, 300);
    tassert(a && b && c);
    memset(a, 1, 100);
    memset(b, 2, 200);
    memset(c, 3, 300);
    tassert(AllocatorArena_sanitize(arena));

    arena->free(arena, b);
    tassert(AllocatorArena_sanitize(arena));

    u8* d = mem$realloc(arena, a, 150);
    tassert(d != NULL);
    tassert(AllocatorArena_sanitize(arena));

    u8* e = mem$malloc(arena, 50);
    tassert(e != NULL);
    tassert(AllocatorArena_sanitize(arena));

    // destroy at non-zero scope_depth is fine with disable_scopes
    tassert_eq(allc->scope_depth, 0);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_create_zii_page_size)
{
    // page_size = 0 (ZII) → should default to CEX_ALLOCATOR_TEMP_PAGE_SIZE
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 0 }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;
    tassert_eq(allc->page_size, CEX_ALLOCATOR_TEMP_PAGE_SIZE);
    tassert(!allc->disable_scopes);
    tassert_eq(allc->scope_depth, 1);

    u8* p = mem$malloc(arena, 100);
    tassert(p != NULL);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_create_zii_page_size_with_disable_scopes)
{
    // page_size = 0 (ZII), disable_scopes = true → page_size should default
    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 0, .disable_scopes = true }
    );
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;
    tassert_eq(allc->page_size, CEX_ALLOCATOR_TEMP_PAGE_SIZE);
    tassert(allc->disable_scopes);
    tassert_eq(allc->scope_depth, 0);

    u8* p = mem$malloc(arena, 100);
    tassert(p != NULL);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_create_null_kwargs)
{
    // NULL kwargs → all fields should use defaults
    IAllocator arena = AllocatorArena.create(NULL);
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;
    tassert_eq(allc->page_size, CEX_ALLOCATOR_TEMP_PAGE_SIZE);
    tassert(!allc->disable_scopes);
    tassert_eq(allc->scope_depth, 1);

    u8* p = mem$malloc(arena, 100);
    tassert(p != NULL);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$main();
