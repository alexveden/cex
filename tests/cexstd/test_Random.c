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

static int
cmp_u32_desc(const void* a, const void* b)
{
    u32 va = *(const u32*)a;
    u32 vb = *(const u32*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int
cmp_f32_desc(const void* a, const void* b)
{
    f32 va = *(const f32*)a;
    f32 vb = *(const f32*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

test$case(random_chi_square)
{
    Random_c rnd = { 0 };
    Random.seed(&rnd, 424242);

    int bins[100] = { 0 };
    int N = 100000;

    for (int i = 0; i < N; i++) {
        usize v = Random.range(&rnd, 0, 100);
        bins[v]++;
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

test$case(random_ks_f32)
{
    Random_c rnd = { 0 };
    Random.seed(&rnd, 12345);

    int N = 100000;
    f32* vals = mem$malloc(mem$, (usize)N * sizeof(f32));
    uassert(vals != NULL);

    for (int i = 0; i < N; i++) {
        vals[i] = Random.f32(&rnd);
    }

    qsort(vals, (usize)N, sizeof(f32), cmp_f32_desc);

    f64 D = 0;
    for (int i = 0; i < N; i++) {
        f64 ecdf = (f64)(i + 1) / (f64)N;
        f64 cdf = (f64)vals[i];
        f64 dev = fabs(ecdf - cdf);
        if (dev > D) D = dev;
    }

    mem$free(mem$, vals);

    tassertf(D < 0.006, "KS D=%.6f (expected < 0.006)", D);
    return EOK;
}

test$case(random_monte_carlo_pi)
{
    Random_c rnd = { 0 };
    Random.seed(&rnd, 9999);

    int N = 1000000;
    int inside = 0;

    for (int i = 0; i < N; i++) {
        f32 x = Random.f32(&rnd);
        f32 y = Random.f32(&rnd);
        if (x * x + y * y <= 1.0f) inside++;
    }

    f64 pi_est = 4.0 * (f64)inside / (f64)N;
    f64 pi_true = 3.14159265358979323846;
    f64 err = fabs(pi_est - pi_true);
    tassertf(err < 0.005, "pi=%.6f err=%.6f (expected < 0.005)", pi_est, err);
    return EOK;
}

test$case(random_runs_test)
{
    Random_c rnd = { 0 };
    Random.seed(&rnd, 77777);

    int N = 100000;
    f32 prev = Random.f32(&rnd);
    int runs = 1;
    int sign = 0;

    for (int i = 1; i < N; i++) {
        f32 cur = Random.f32(&rnd);
        int dir = (cur > prev) ? 1 : (cur < prev) ? -1 : 0;
        if (dir != 0) {
            if (sign == 0 || dir != sign) {
                runs++;
                sign = dir;
            }
        }
        prev = cur;
    }

    f64 exp_runs = (2.0 * (f64)N - 1.0) / 3.0;
    f64 var_runs = (16.0 * (f64)N - 29.0) / 90.0;
    f64 diff = (f64)runs - exp_runs;
    f64 z2 = (diff * diff) / var_runs;

    tassertf(z2 < 9.0, "runs z^2=%.4f (expected < 9.0)", z2);
    return EOK;
}

test$case(random_birthday_spacing)
{
    Random_c rnd = { 0 };
    Random.seed(&rnd, 112233);

    int n = 512;
    u32 m = 1 << 24;
    u32* vals = mem$malloc(mem$, (usize)n * sizeof(u32));
    uassert(vals != NULL);

    for (int i = 0; i < n; i++) {
        vals[i] = Random.next(&rnd) & (m - 1);
    }

    qsort(vals, (usize)n, sizeof(u32), cmp_u32_desc);

    u32* spacings = mem$malloc(mem$, (usize)n * sizeof(u32));
    uassert(spacings != NULL);

    for (int i = 0; i < n - 1; i++) {
        spacings[i] = vals[i + 1] - vals[i];
    }
    spacings[n - 1] = (m - vals[n - 1]) + vals[0];

    qsort(spacings, (usize)n, sizeof(u32), cmp_u32_desc);

    int collisions = 0;
    for (int i = 0; i < n - 1; i++) {
        if (spacings[i] == spacings[i + 1]) collisions++;
    }

    mem$free(mem$, vals);
    mem$free(mem$, spacings);

    tassertf(collisions < 12, "birthday collisions=%d (expected < 12)", collisions);
    return EOK;
}

static Random_c g_rnd_bench;

test$setup_suite()
{
    Random.seed(&g_rnd_bench, 42);
    return EOK;
}

test$teardown_suite()
{
    return EOK;
}

test$bench(bench_random_next)
{
    Random.next(&g_rnd_bench);
    return EOK;
}

test$bench(bench_random_f32)
{
    Random.f32(&g_rnd_bench);
    return EOK;
}

test$bench(bench_random_buf)
{
    u8 buf[1024];
    Random.buf(&g_rnd_bench, buf, sizeof(buf));
    return EOK;
}

test$main();
