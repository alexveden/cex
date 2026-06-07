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
    tassert(mem$add_overflow((isize)9223372036854775807, (isize)1, &sr));

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
    tassert(mem$sub_overflow((isize)(-9223372036854775807 - 1), (isize)1, &sr));
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

test$main();
