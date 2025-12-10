#include "src/all.c"
#include <cexstd/random/Random.c>

struct foo
{
    i32 arr[10];
} bar = {
    .arr = { 1, 2, 3 },
};

test$case(random_seed)
{
    Random_c rnd = { 0 };
    Random_c rnd2 = { 0 };

    Random.seed(&rnd, 0);
    tassert(rnd.state[0] > 0);
    tassert(rnd.state[1] > 0);

    Random.seed(&rnd2, 0);
    tassert(rnd2.state[0] > 0);
    tassert(rnd2.state[1] > 0);
    tassert(rnd2.state[0] == rnd.state[0]);
    tassert(rnd2.state[1] == rnd.state[1]);

    u32 rnd_array[1000] = { 0 };

    for (u32 i = 0; i < arr$len(rnd_array); i++) {
        u32 r1 = Random.next(&rnd);
        u32 r2 = Random.next(&rnd2);
        tassert(r1 > 0);
        tassert(r2 > 0);
        tassert_eq(r1, r2);
        rnd_array[i] = r1;
        if (i > 0) { tassert(r1 != rnd_array[i - 1]); }
    }

    Random.seed(&rnd, 0);
    for (u32 i = 0; i < arr$len(rnd_array); i++) {
        u32 r1 = Random.next(&rnd);
        tassert(r1 > 0);
        tassert_eq(rnd_array[i], r1);
    }
    return EOK;
}
test$case(random_f32)
{
    Random_c rnd = { 0 };

    u64 seed = time(NULL);
    Random.seed(&rnd, seed);


    for (u32 i = 0; i < 100000; i++) {
        f32 r1 = Random.f32(&rnd);
        tassert(r1 >= 0.0f && r1 <= 1.0f);
    }

    return EOK;
}

test$case(random_i32)
{
    Random_c rnd = { 0 };

    Random.seed(&rnd, 999777);

    u32 cnt[201] = { 0 };

    for (u32 i = 0; i < 100000; i++) {
        i32 r1 = Random.i32(&rnd, -100, 100);
        tassert(r1 >= -100 && r1 <= 100);

        cnt[r1 + 100]++;
    }

    for (u32 i = 0; i < arr$len(cnt); i++) {
        tassertf(cnt[i] > 0, "range value not hit at: %d\n", -100 + i);
    }

    return EOK;
}

test$case(random_range)
{
    Random_c rnd = { 0 };

    Random.seed(&rnd, 999777);

    u32 cnt[100] = { 0 };

    for (u32 i = 0; i < 100000; i++) {
        usize r1 = Random.range(&rnd, 0, 100);
        tassert(r1 < 100);
        cnt[r1]++;
    }

    for (u32 i = 0; i < arr$len(cnt); i++) {
        tassertf(cnt[i] > 0, "range value not hit at: %d\n", i);
    }

    return EOK;
}

test$case(random_prob)
{
    Random_c rnd = { 0 };

    Random.seed(&rnd, 999777);

    u32 is_passed = 0;
    for (u32 i = 0; i < 100000; i++) {
        if (Random.prob(&rnd, 1.0)) { is_passed++; }
    }
    tassert_eq(is_passed, 100000);

    is_passed = 0;
    for (u32 i = 0; i < 100000; i++) {
        if (Random.prob(&rnd, 0.0)) { is_passed++; }
    }
    tassert_eq(is_passed, 0);

    Random.seed(&rnd, 999777);
    is_passed = 0;
    for (u32 i = 0; i < 1000000; i++) {
        if (Random.prob(&rnd, 0.5)) { is_passed++; }
    }
    tassert_eq(is_passed, 500165); // <-- almost equal


    Random.seed(&rnd, 999777);
    is_passed = 0;
    for (u32 i = 0; i < 100000; i++) {
        if (Random.prob(&rnd, 0.05)) { is_passed++; }
    }
    tassert_eq(is_passed, 5039); // <-- almost equal

    return EOK;
}

test$case(random_buf)
{
    Random_c rnd = { 0 };

    Random.seed(&rnd, 0);

    u32 b1 = 0;
    Random.buf(&rnd, &b1, sizeof(b1));

    Random.seed(&rnd, 0);
    u32 r2 = Random.next(&rnd);

    tassert_eq(b1, r2);

    u32 b2[10] = { 0 };
    static_assert(sizeof(b2) == 10 * sizeof(u32), "size");
    Random.buf(&rnd, b2, 0);
    for (u32 i = 0; i < arr$len(b2); i++) { tassert(b2[i] == 0); }

    Random.seed(&rnd, 0);
    Random.buf(&rnd, b2, sizeof(b2));

    Random.seed(&rnd, 0);
    for (u32 i = 0; i < arr$len(b2); i++) {
        tassert(b2[i] != 0);
        tassertf(b2[i] == Random.next(&rnd), "buf[%d]", i);
    }

    alignas(2) u8 b3[43] = { 0 };

    uassert(((usize)b3 + 1) % 4 != 0 && "expected unaligned");

    Random.seed(&rnd, 0);
    Random.buf(&rnd, b3 + 1, sizeof(b3) - 1); // Unaligned!

    Random.seed(&rnd, 0);
    for (u32 i = 0; i < 40; i += 4) {
        u32 r = 0;
        memcpy(&r, &b3[i + 1], sizeof(u32));
        tassertf(r == Random.next(&rnd), "buf[%d]", i);
    }


    u32 cnt[sizeof(b3)] = { 0 };

    Random.seed(&rnd, 0);
    for (u32 i = 0; i < 10000; i++) {
        memset(b3, 0, sizeof(b3));
        Random.buf(&rnd, b3, sizeof(b3));
        for$eachp(it, b3, sizeof(b3))
        {
            if (*it != 0) { cnt[it - b3]++; }
        }
    }
    for$each (it, cnt, arr$len(cnt)) {
        // every byte changed
        tassert(it > 0);
        // every byte changed at least 50% of time
        tassert(it > 5000);
    }
    return EOK;
}

test$main();
