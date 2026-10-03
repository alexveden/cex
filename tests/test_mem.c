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

static_assert(mem$MAX == (usize)PTRDIFF_MAX, "mem$MAX must be PTRDIFF_MAX");

test$case(test_mem_has_overflow)
{
    usize m = (usize)-1;

    // fits
    tassert(!mem$has_overflow((usize)10, (usize)0, (usize)0));
    tassert(!mem$has_overflow((usize)10, (usize)0, (usize)10));
    tassert(!mem$has_overflow((usize)10, (usize)3, (usize)7));
    tassert(!mem$has_overflow((usize)10, (usize)10, (usize)0));
    tassert(!mem$has_overflow((usize)0, (usize)0, (usize)0));
    tassert(!mem$has_overflow(mem$MAX, (usize)0, mem$MAX));

    // exceeds remaining room
    tassert(mem$has_overflow((usize)10, (usize)0, (usize)11));
    tassert(mem$has_overflow((usize)10, (usize)3, (usize)8));
    tassert(mem$has_overflow((usize)10, (usize)10, (usize)1));
    tassert(mem$has_overflow((usize)0, (usize)0, (usize)1));
    tassert(mem$has_overflow(mem$MAX, (usize)1, mem$MAX));

    // invalid start past the bound
    tassert(mem$has_overflow((usize)10, (usize)11, (usize)0));
    tassert(mem$has_overflow((usize)0, (usize)1, (usize)0));
    tassert(mem$has_overflow((usize)10, (usize)11, (usize)5));

    // out of domain: SIZE_MAX / negative
    tassert(mem$has_overflow(mem$MAX + 1, (usize)0, (usize)0));
    tassert(mem$has_overflow(m, (usize)0, (usize)0));
    tassert(mem$has_overflow((usize)10, m, (usize)0));
    tassert(mem$has_overflow((usize)10, (usize)0, m));
    tassert(mem$has_overflow(m, (usize)0, m));
    tassert(mem$has_overflow((usize)10, (i32)-1, (usize)0));

    // mixed argument types convert to usize
    tassert(mem$has_overflow((u32)10, (u32)3, (u32)8));
    tassert(mem$has_overflow((u8)10, (u8)3, (u8)8));
    tassert(mem$has_overflow((i32)10, (i32)3, (i32)8));
    return EOK;
}

test$case(test_mem_has_overflow_slice_sub)
{
    u8 buf[10] = {0};
    usize off = 0, n = 0;
    u8* sub = NULL;

    off = 3;
    n = 7;
    sub = !mem$has_overflow(sizeof(buf), off, n) ? buf + off : NULL;
    tassert(sub == &buf[3]);

    off = 3;
    n = 8;
    sub = !mem$has_overflow(sizeof(buf), off, n) ? buf + off : NULL;
    tassert(sub == NULL);

    off = 11;
    n = 0;
    sub = !mem$has_overflow(sizeof(buf), off, n) ? buf + off : NULL;
    tassert(sub == NULL);

    off = (usize)-1;
    n = 1;
    sub = !mem$has_overflow(sizeof(buf), off, n) ? buf + off : NULL;
    tassert(sub == NULL);
    return EOK;
}

test$case(test_mem_has_overflow_chunk_loop)
{
    u8 buf[16] = {0};
    usize off = 0, consumed = 0, chunk = 3;
    while (!mem$has_overflow(sizeof(buf), off, chunk)) {
        consumed += chunk;
        off += chunk;
    }
    tassert_eq(consumed, (usize)15);
    tassert(off <= sizeof(buf));
    return EOK;
}

test$case(test_mem_has_overflow_untrusted_length)
{
    usize cap = 1024, len = 0;
    tassert(!mem$has_overflow(cap, len, (usize)1024));
    tassert(mem$has_overflow(cap, len, (usize)1025));
    tassert(mem$has_overflow(cap, len, (usize)-1));
    tassert(mem$has_overflow(cap, len, (usize)(i64)-1));
    return EOK;
}

test$case(test_mem_has_overflow_domain_idiom)
{
    // has_overflow(mem$MAX, 0, x) == (x > mem$MAX)
    tassert(!mem$has_overflow(mem$MAX, (usize)0, mem$MAX));
    tassert(mem$has_overflow(mem$MAX, (usize)0, mem$MAX + 1));
    tassert(mem$has_overflow(mem$MAX, (usize)0, (usize)-1));
    return EOK;
}

test$case(test_mem_calc_overflow_growth)
{
    usize cap = 10, len = 7, add = 5;
    usize need = mem$calc_overflow(cap, len, add);
    tassert_eq(need, (usize)2);
    tassert(need <= mem$MAX);
    tassert_eq(cap + need, len + add);

    tassert_eq(mem$calc_overflow(cap, len, (usize)3), (usize)0);
    return EOK;
}

test$case(test_mem_calc_overflow_sentinel)
{
    tassert_eq(mem$calc_overflow(mem$MAX + 1, (usize)0, (usize)0), mem$MAX + 1);
    tassert_eq(mem$calc_overflow((usize)10, (usize)11, (usize)0), mem$MAX + 1);
    tassert_eq(mem$calc_overflow((usize)10, (usize)0, (usize)-1), mem$MAX + 1);
    tassert_eq(mem$calc_overflow(mem$MAX, mem$MAX, mem$MAX), mem$MAX);
    tassert(mem$calc_overflow((usize)10, (usize)7, (usize)5) < mem$MAX);
    return EOK;
}

static usize _cex_test_ho_evals = 0;

static usize
_cex_test_ho_arg(void)
{
    _cex_test_ho_evals++;
    return 3;
}

test$case(test_mem_has_overflow_evals_args_once)
{
    _cex_test_ho_evals = 0;
    (void)mem$has_overflow(_cex_test_ho_arg(), _cex_test_ho_arg(), _cex_test_ho_arg());
    tassert_eq(_cex_test_ho_evals, (usize)3);
    return EOK;
}

test$case(test_mem_calc_overflow_evals_args_once)
{
    _cex_test_ho_evals = 0;
    (void)mem$calc_overflow(_cex_test_ho_arg(), _cex_test_ho_arg(), _cex_test_ho_arg());
    tassert_eq(_cex_test_ho_evals, (usize)3);
    return EOK;
}

test$case(test_global_mem_allocator_replaceable)
{
    IAllocator saved_mem = mem$;
    AllocatorHeap_c* custom =
        mem$malloc(test$alloc, sizeof(AllocatorHeap_c), alignof(AllocatorHeap_c));
    memcpy(custom, &_cex__default_global__allocator_heap, sizeof(*custom));
    memset(&custom->stats, 0, sizeof(custom->stats));

    mem$ = &custom->alloc;
    tassert(mem$ == &custom->alloc);

    u8* p = mem$malloc(mem$, 100);
    tassert(p != NULL);
    tassert_eq(custom->stats.n_allocs, 1);

    mem$free(mem$, p);
    tassert_eq(custom->stats.n_free, 1);

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

test$case(test_mem_realloc_nulls_old_ptr_on_success)
{
    u8* p = mem$malloc(test$alloc, 32);
    tassert(p != NULL);

    u8* q = mem$realloc(test$alloc, p, 64);
    tassert(q != NULL);
    tassert(p == NULL);
    return EOK;
}

test$case(test_mem_realloc_nulls_old_ptr_on_oom)
{
    u8* p = mem$malloc(test$alloc, 32);
    tassert(p != NULL);

    test$alloc_set_oom_probability(1.0);
    u8* q = mem$realloc(test$alloc, p, 64);
    test$alloc_set_oom_probability(0.0);

    tassert(q == NULL);
    tassert(p == NULL);
    return EOK;
}

static int _cex_test_allocator_evals = 0;

static IAllocator
_cex_test_eval_allocator(void)
{
    _cex_test_allocator_evals++;
    return test$alloc;
}

test$case(test_mem_macros_eval_allocator_once)
{
    _cex_test_allocator_evals = 0;
    u8* p = mem$malloc(_cex_test_eval_allocator(), 32);
    tassert(p != NULL);
    tassert_eq(_cex_test_allocator_evals, 1);

    _cex_test_allocator_evals = 0;
    p = mem$realloc(_cex_test_eval_allocator(), p, 64);
    tassert(p != NULL);
    tassert_eq(_cex_test_allocator_evals, 1);

    _cex_test_allocator_evals = 0;
    u8* c = mem$calloc(_cex_test_eval_allocator(), 1, 32);
    tassert(c != NULL);
    tassert_eq(_cex_test_allocator_evals, 1);

    _cex_test_allocator_evals = 0;
    u64* n = mem$new(_cex_test_eval_allocator(), u64);
    tassert(n != NULL);
    tassert_eq(_cex_test_allocator_evals, 1);

    _cex_test_allocator_evals = 0;
    mem$free(_cex_test_eval_allocator(), c);
    tassert(c == NULL);
    tassert_eq(_cex_test_allocator_evals, 1);

    mem$free(test$alloc, p);
    mem$free(test$alloc, n);
    return EOK;
}

test$main();
