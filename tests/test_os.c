#include "src/all.c"
#include <math.h>

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(test_timer)
{
    f64 t = os.timer();
    tassert(t > 0);
    tassert(!isnan(t));
    tassert(t < INFINITY);
    tassert(t > -INFINITY);
    // timer starts from first call of os.timer(), so numbers should be small
    tassert_le(t, 5);

    f64 t2 = os.timer();
    tassert(t2 >= t);
    tassert_le(t2, 5);

    os.sleep(0.1);
    t2 = os.timer();
    f64 tdiff = t2 - t;
    // NOTE: CI timings may be very slow, we estimate order of magnitude
    tassertf(tdiff > 0.1 && tdiff < 0.35, "%g", tdiff);

    t = t2;
    os.sleep(1.1);
    t2 = os.timer();
    tdiff = t2 - t;
    tassertf(tdiff > 1.1 && tdiff < 1.35, "%g", tdiff);

    return EOK;
}

test$case(test_cpu_count)
{

    tassert_ge(os.cpu_count(), 1);
    tassert_le(os.cpu_count(), 128);

    return EOK;
}

test$case(test_time_scope)
{
    os$time_scope()
    {
        os.sleep(0.1);
    }
    os$time_scope()
    {
        os.sleep(1.1);
    }
    os$time_scope() {}
    os$time_scope()
    {
        break; // early exit warning
    }

    return EOK;
}

test$case(test_hash)
{
    // --- NULL / empty ---
    tassert_eq(os.hash(NULL, 0, 10), 0);
    tassert_eq(os.hash("", 0, 10), 0);
    tassert_eq(os.hash("hello", 0, 0), 0);
    tassert_eq(os.hash("hello", 0, 42), 0);

    // --- basic smoke: delegates to _cexds__hash_bytes ---
    char cstr[] = { "hello" };
    u64 h0 = os.hash(cstr, sizeof(cstr), 0);
    tassert(h0 != 0);
    tassert_eq(_cexds__hash_bytes(cstr, sizeof(cstr), 0), h0);
    tassert_eq(h0, 6329348214770146015UL);

    return EOK;
}

test$main();
