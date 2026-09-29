#include "src/all.c"

static i32
ci_coverage_add(i32 a, i32 b)
{
    return a + b;
}

static i32
ci_coverage_classify(i32 n)
{
    if (n < 0) { return -1; }
    if (n == 0) { return 0; }
    return 1;
}

test$case(ci_coverage_math)
{
    tassert_eq(ci_coverage_add(2, 3), 5);
    tassert_eq(ci_coverage_add(-2, 3), 1);
    return EOK;
}

test$case(ci_coverage_branches)
{
    tassert_eq(ci_coverage_classify(-5), -1);
    tassert_eq(ci_coverage_classify(0), 0);
    tassert_eq(ci_coverage_classify(7), 1);
    return EOK;
}

test$main();
