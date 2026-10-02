#include "src/all.c"

test$case(test_fuzz_create_dget)
{
    u8 data[] = { 1, 2, 3, 4, 5 };
    u8 out[5] = { 0 };
    u8 one = 0;

    cex_fuzz_s fz = fuzz.create(data, sizeof(data));
    tassert(fuzz.dget(&fz, out, sizeof(out)));
    tassert(memcmp(out, data, sizeof(out)) == 0);
    tassert(!fuzz.dget(&fz, &one, sizeof(one)));

    // not enough bytes for the requested size
    cex_fuzz_s partial = fuzz.create(data, 3);
    tassert(fuzz.dget(&partial, &one, 1));
    tassert(fuzz.dget(&partial, &one, 1));
    tassert(!fuzz.dget(&partial, &one, 2));

    // empty input
    cex_fuzz_s empty = fuzz.create(data, 0);
    tassert(!fuzz.dget(&empty, &one, 1));

    return EOK;
}

test$case(test_fuzz_dprob)
{
    u8 low[] = { 0 };
    u8 high[] = { 255 };
    u8 mid[] = { 128 };

    cex_fuzz_s fz = fuzz.create(low, sizeof(low));
    tassert(fuzz.dprob(&fz, 0.5));
    tassert(!fuzz.dprob(&fz, 0.5)); // exhausted

    fz = fuzz.create(high, sizeof(high));
    tassert(!fuzz.dprob(&fz, 0.5)); // 255/255 = 1.0 > 0.5

    fz = fuzz.create(mid, sizeof(mid));
    tassert(fuzz.dprob(&fz, 0.6)); // 128/255 ~ 0.502 <= 0.6

    fz = fuzz.create(mid, sizeof(mid));
    tassert(!fuzz.dprob(&fz, 0.4)); // 128/255 ~ 0.502 > 0.4

    return EOK;
}

test$case(test_fuzz_corpus_dir)
{
    char* dir = fuzz.corpus_dir("some/path/fuzz_thing.c");
    tassert(dir != NULL);
    tassert_eq(dir, "some/path/fuzz_thing_corpus");

    // cached: a different name returns the same buffer
    char* cached = fuzz.corpus_dir("other.c");
    tassert_eq_ptr(dir, cached);
    tassert_eq(cached, "some/path/fuzz_thing_corpus");

    return EOK;
}

test$main();
