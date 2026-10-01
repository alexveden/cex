#include "src/all.c"

test$case(test_is_power_of2)
{
    static_assert(!mem$is_power_of2(0), "fail");
    static_assert(mem$is_power_of2(1), "fail");
    static_assert(mem$is_power_of2(2), "fail");
    static_assert(mem$is_power_of2(64), "fail");
    static_assert(mem$is_power_of2(128), "fail");
    return EOK;
}

test$case(test_aligned_size)
{
    tassert_eq(4, mem$aligned_round(3, 4));
    tassert_eq(64, mem$aligned_round(3, 64));
    tassert_eq(0, mem$aligned_round(0, 64));
    tassert_eq(64, mem$aligned_round(1, 64));
    tassert_eq(63, mem$aligned_round(63, 1));

    // alignas(64) char buf[127] = {0};
    char* buf = mem$malloc(mem$, 128, 64);

    log$debug("Initial buf addr: %p buf %%64: %zu\n", buf, ((usize)buf) % (usize)64L);

    tassert(((usize)&buf[0]) % 64 == 0);
    tassert(((usize)buf) % 64 == 0);
    tassert(((usize)mem$aligned_pointer(&buf[0], 64)) % 64 == 0);

    tassert(((usize)&buf[0]) % 64 == 0);
    tassert(((usize)buf) % 64 == 0);
    tassert(((usize)mem$aligned_pointer(buf, 64)) % 64 == 0);
    log$debug("After buf addr: %p add2: %p\n", buf, &buf[0]);

    tassertf(
        buf == mem$aligned_pointer(buf, 64),
        "buf addr: %p aligned addr: %p ptr_diff: %zd",
        buf,
        mem$aligned_pointer(buf, 64),
        (char*)buf - (char*)mem$aligned_pointer(buf, 64)
    );
    tassert(&buf[64] == mem$aligned_pointer(&buf[1], 64));
    mem$free(mem$, buf);

    return EOK;
}

test$case(test_mem_add_overflow)
{
    usize r;
    tassert(!mem$add_overflow((usize)10, (usize)20, &r));
    tassert_eq(r, 30);
    tassert(mem$add_overflow((usize)-1, (usize)1, &r));
    tassert_eq(r, 0);

    isize sr;
#if mem$platform() > 32
    tassert(mem$add_overflow((isize)9223372036854775807, (isize)1, &sr));
#else
    tassert(mem$add_overflow((isize)2147483647, (isize)1, &sr));
#endif

    u32 ur;
    tassert(!mem$add_overflow((u32)4000000000, (u32)1, &ur));
    tassert_eq(ur, (u32)4000000001);
    return EOK;
}

test$case(test_mem_sub_overflow)
{
    usize r;
    tassert(!mem$sub_overflow((usize)20, (usize)10, &r));
    tassert_eq(r, 10);
    tassert(mem$sub_overflow((usize)0, (usize)1, &r));
    tassert_eq(r, (usize)-1);

    isize sr;
#if mem$platform() > 32
    tassert(mem$sub_overflow((isize)(-9223372036854775807 - 1), (isize)1, &sr));
#else
    tassert(mem$sub_overflow((isize)(-2147483647 - 1), (isize)1, &sr));
#endif
    return EOK;
}

test$case(test_mem_mul_overflow)
{
    usize r;
    tassert(!mem$mul_overflow((usize)100, (usize)200, &r));
    tassert_eq(r, 20000);
    tassert(mem$mul_overflow((usize)-1, (usize)2, &r));
    tassert(!mem$mul_overflow((usize)((usize)-1 / 2), (usize)2, &r));
    tassert(mem$mul_overflow((usize)((usize)-1 / 2 + 1), (usize)2, &r));

    u32 ur;
    tassert(!mem$mul_overflow((u32)50000, (u32)50000, &ur));
    tassert_eq(ur, (u32)2500000000);
    tassert(mem$mul_overflow((u32)100000, (u32)100000, &ur));
    return EOK;
}

test$case(test_global_mem_allocator_replaceable)
{
    IAllocator saved_mem = mem$;
    AllocatorHeap_c custom = _cex__default_global__allocator_heap;
    memset(&custom.stats, 0, sizeof(custom.stats));

    mem$ = &custom.alloc;
    tassert(mem$ == &custom.alloc);

    u8* p = mem$malloc(mem$, 100);
    tassert(p != NULL);
    tassert_eq(custom.stats.n_allocs, 1);

    mem$free(mem$, p);
    tassert_eq(custom.stats.n_free, 1);

    mem$ = saved_mem;
    tassert(mem$ == &_cex__default_global__allocator_heap.alloc);
    return EOK;
}

test$case(test_global_mem_allocator_routes_to_test_alloc_oom)
{
    // route every mem$ allocation through the per-case arena, and make it fail on demand
    mem$ = test$alloc;
    test$alloc_set_oom_probability(1.0);

    // any function allocating via mem$ now exercises its OOM path
    tassert(mem$malloc(mem$, 64) == NULL);

    // no manual restore needed - the runner restores mem$ after the case
    return EOK;
}

test$main();
