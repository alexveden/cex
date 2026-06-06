#include "src/all.c"

static int
cmp_u32_desc(const void* a, const void* b)
{
    u32 va = *(const u32*)a;
    u32 vb = *(const u32*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

test$case(os_random_auto_seed)
{
    u32 r1 = os.random.next();
    tassert(r1 > 0);

    u32 r2 = os.random.next();
    tassert(r2 > 0);
    tassert(r2 != r1);

    f32 f = os.random.f32();
    tassert(f >= 0.0f && f <= 1.0f);

    usize rr = os.random.range(0, 100);
    tassert(rr < 100);

    u8 buf[16];
    os.random.buf(buf, sizeof(buf));
    bool has_nonzero = false;
    for (u32 i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) { has_nonzero = true; break; }
    }
    tassert(has_nonzero);

    return EOK;
}

test$case(os_random_seed)
{
    os.random.seed(0);

    usize n = 1000;
    u32 rnd_array[1000] = { 0 };
    for (u32 i = 0; i < n; i++) {
        u32 r = os.random.next();
        tassert(r > 0);
        rnd_array[i] = r;
        if (i > 0) { tassert(r != rnd_array[i - 1]); }
    }

    os.random.seed(0);
    for (u32 i = 0; i < n; i++) {
        tassert_eq(os.random.next(), rnd_array[i]);
    }

    os.random.seed(42);
    u32 s42_first = os.random.next();
    tassert(s42_first > 0);

    os.random.seed(0);
    for (u32 i = 0; i < n; i++) {
        tassert_eq(os.random.next(), rnd_array[i]);
    }

    return EOK;
}

test$case(os_random_i32)
{
    os.random.seed(999777);

    for (u32 i = 0; i < 100000; i++) {
        i32 r = os.random.i32(-100, 100);
        tassert(r >= -100 && r < 100);
    }

    return EOK;
}

test$case(os_random_f32)
{
    os.random.seed(12345);

    for (u32 i = 0; i < 100000; i++) {
        f32 r = os.random.f32();
        tassertf(r >= 0.0f && r < 1.0f, "f32 out of range: %f", r);
    }

    return EOK;
}

test$case(os_random_range)
{
    os.random.seed(999777);

    u32 cnt[100] = { 0 };

    for (u32 i = 0; i < 100000; i++) {
        usize r = os.random.range(0, 100);
        tassert(r < 100);
        cnt[r]++;
    }

    for (u32 i = 0; i < arr$len(cnt); i++) {
        tassertf(cnt[i] > 0, "range value not hit at: %d", i);
    }

    return EOK;
}

test$case(os_random_buf)
{
    os.random.seed(0);

    u32 b1 = 0;
    os.random.buf(&b1, sizeof(b1));

    os.random.seed(0);
    u32 r2 = os.random.next();
    tassert_eq(b1, r2);

    u32 b2[10] = { 0 };
    os.random.buf(b2, 0);
    for (u32 i = 0; i < arr$len(b2); i++) { tassert(b2[i] == 0); }

    os.random.seed(0);
    os.random.buf(b2, sizeof(b2));

    os.random.seed(0);
    for (u32 i = 0; i < arr$len(b2); i++) {
        tassert(b2[i] != 0);
        tassertf(b2[i] == os.random.next(), "buf[%d]", i);
    }

    alignas(2) u8 b3[43] = { 0 };
    uassert(((usize)b3 + 1) % 4 != 0 && "expected unaligned");

    os.random.seed(0);
    os.random.buf(b3 + 1, sizeof(b3) - 1);

    os.random.seed(0);
    for (u32 i = 0; i < 40; i += 4) {
        u32 r = 0;
        memcpy(&r, &b3[i + 1], sizeof(u32));
        tassertf(r == os.random.next(), "buf[%d]", i);
    }

    u32 cnt[sizeof(b3)] = { 0 };
    os.random.seed(0);
    for (u32 i = 0; i < 10000; i++) {
        memset(b3, 0, sizeof(b3));
        os.random.buf(b3, sizeof(b3));
        for$eachp(it, b3, sizeof(b3))
        {
            if (*it != 0) { cnt[it - b3]++; }
        }
    }
    for$each (it, cnt, arr$len(cnt)) {
        tassert(it > 0);
        tassert(it > 5000);
    }

    return EOK;
}

test$case(os_random_chi_square)
{
    os.random.seed(424242);

    int bins[100] = { 0 };
    int N = 100000;

    for (int i = 0; i < N; i++) {
        usize v = os.random.range(0, 100);
        bins[v]++;
        tassert(v < 100);
        tassert(v >= 0);
    }

    f64 expected = (f64)N / 100.0;
    f64 chi2 = 0;
    for (int i = 0; i < 100; i++) {
        f64 diff = (f64)bins[i] - expected;
        chi2 += (diff * diff) / expected;
    }

    tassertf(chi2 < 150.0, "chi-square=%.2f (expected < 150)", chi2);
    return EOK;
}

test$case(os_random_ticks)
{
    os.random.seed(42);
    tassert_eq(os.random.ticks(), 0);

    os.random.next();
    tassert_eq(os.random.ticks(), 1);

    os.random.next();
    tassert_eq(os.random.ticks(), 2);

    os.random.f32();
    tassert_eq(os.random.ticks(), 3);

    os.random.range(0, 10);
    tassert_eq(os.random.ticks(), 4);

    u8 buf[20];
    os.random.buf(buf, sizeof(buf));
    tassert_eq(os.random.ticks(), 9);

    os.random.seed(42);
    tassert_eq(os.random.ticks(), 0);

    return EOK;
}

test$main();
