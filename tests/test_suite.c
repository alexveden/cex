#include "src/all.c"
#include <math.h>
test$setup_case()
{
    tassert(test$alloc != NULL);

    AllocatorArena_c* test_arena = (AllocatorArena_c*)test$alloc;
    tassert_eq(test_arena->test_oom_probability, 0.0);

    u8* p2 = mem$malloc(test$alloc, 10);
    tassert(p2);

    return EOK;
}
test$teardown_case()
{
    tassert(test$alloc != NULL);

    AllocatorArena_c* test_arena = (AllocatorArena_c*)test$alloc;
    tassert_eq(test_arena->test_oom_probability, 0.0);
     
    u8* p2 = mem$malloc(test$alloc, 10);
    tassert(p2);

    return EOK;
}
test$setup_suite()
{
    // test$alloc is lives only for cases
    tassert(test$alloc == NULL);
    return EOK;
}
test$teardown_suite()
{
    // test$alloc is lives only for cases
    tassert(test$alloc == NULL);
    return EOK;
}

test$case(test_tassert_i8)
{
    i8 a = 0;
    i8 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT8_MAX;
    b = INT8_MIN;
    tassert_ne(a, b);

    a = INT8_MAX;
    b = INT8_MAX;
    tassert_eq(a, b);

    a = INT8_MIN;
    b = INT8_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u8)
{
    u8 a = 0;
    u8 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT8_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT8_MAX;
    b = UINT8_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_i16)
{
    i16 a = 0;
    i16 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT16_MAX;
    b = INT16_MIN;
    tassert_ne(a, b);

    a = INT16_MAX;
    b = INT16_MAX;
    tassert_eq(a, b);

    a = INT16_MIN;
    b = INT16_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u16)
{
    u16 a = 0;
    u16 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT16_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT16_MAX;
    b = UINT16_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_i32)
{
    i32 a = 0;
    i32 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT32_MAX;
    b = INT32_MIN;
    tassert_ne(a, b);

    a = INT32_MAX;
    b = INT32_MAX;
    tassert_eq(a, b);

    a = INT32_MIN;
    b = INT32_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u32)
{
    u32 a = 0;
    u32 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT32_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT32_MAX;
    b = UINT32_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_i64)
{
    i64 a = 0;
    i64 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT64_MAX;
    b = INT64_MIN;
    tassert_ne(a, b);

    a = INT64_MAX;
    b = INT64_MAX;
    tassert_eq(a, b);

    a = INT64_MIN;
    b = INT64_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u64)
{
    u64 a = 0;
    u64 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT64_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT64_MAX;
    b = UINT64_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_f32)
{
    f32 nan = 0.0f / 0.0f;
    f32 a = 0.0f;
    f32 b = 0.0f;

    a = 0.0f;
    b = 0.0f;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = 1.0f;
    b = 0.0f;
    tassert_ne(a, b);

    a = 0.0f;
    b = 1.0f;
    tassert_ne(a, b);

    a = 0.0f;
    b = 0.0f;
    tassert_ge(a, b);

    a = 1.0f;
    b = 0.0f;
    tassert_ge(a, b);

    a = 1.0f;
    b = 0.0f;
    tassert_gt(a, b);

    a = 0.0f;
    b = 0.0f;
    tassert_le(a, b);

    a = 0.0f;
    b = 1.0f;
    tassert_le(a, b);

    a = 0.0f;
    b = 1.0f;
    tassert_lt(a, b);

    a = nan;
    b = nan;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = nan;
    b = 0;
    tassert_ne(a, b);

    a = 1.0f / 3.0f;
    b = 1.0f / 3.0f;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VALF;
    b = HUGE_VALF;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = -HUGE_VALF;
    b = -HUGE_VALF;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VALF;
    b = HUGE_VALF;
    tassert_ge(a, b);

    a = -HUGE_VALF;
    b = HUGE_VALF;
    tassert_lt(a, b);

    a = HUGE_VALF;
    b = -HUGE_VALF;
    tassert_gt(a, b);

    a = -HUGE_VALF;
    b = HUGE_VALF;
    tassert_ne(a, b);

    a = -HUGE_VALF;
    b = nan;
    tassert_ne(a, b);

    a = HUGE_VALF;
    b = nan;
    tassert_ne(a, b);

    return EOK;
}

test$case(test_tassert_f64)
{
    f64 nan = 0.0 / 0.0;
    f64 a = 0.0;
    f64 b = 0.0;

    a = 0.0;
    b = 0.0;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = 1.0;
    b = 0.0;
    tassert_ne(a, b);

    a = 0.0;
    b = 1.0;
    tassert_ne(a, b);

    a = 0.0;
    b = 0.0;
    tassert_ge(a, b);

    a = 1.0;
    b = 0.0;
    tassert_ge(a, b);

    a = 1.0;
    b = 0.0;
    tassert_gt(a, b);

    a = 0.0;
    b = 0.0;
    tassert_le(a, b);

    a = 0.0;
    b = 1.0;
    tassert_le(a, b);

    a = 0.0;
    b = 1.0;
    tassert_lt(a, b);

    a = nan;
    b = nan;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = nan;
    b = 0;
    tassert_ne(a, b);

    a = 1.0 / 3.0;
    b = 1.0 / 3.0;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VAL;
    b = HUGE_VAL;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = -HUGE_VAL;
    b = -HUGE_VAL;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VAL;
    b = HUGE_VAL;
    tassert_ge(a, b);

    a = -HUGE_VAL;
    b = HUGE_VAL;
    tassert_lt(a, b);

    a = HUGE_VAL;
    b = -HUGE_VAL;
    tassert_gt(a, b);

    a = -HUGE_VAL;
    b = HUGE_VAL;
    tassert_ne(a, b);

    a = -HUGE_VAL;
    b = nan;
    tassert_ne(a, b);

    a = HUGE_VAL;
    b = nan;
    tassert_ne(a, b);

    return EOK;
}

test$case(test_tassert_equal_almost)
{
    f32 a = 0.0f;
    f32 b = 0.0f;

    a = 0.01f;
    b = 0.0f;
    tassert_ne(a, b);
    tassert_eq_almost(a, b, 0.01);
    tassert_eq_almost(b, a, 0.01);
    tassert_eq_almost(1, 0, 1);

    return EOK;
}

test$case(test_tassert_error)
{
    tassert_er(EOK, Error.ok);
    tassert_er(Error.argsparse, "ProgramArgsError");
    tassert_eq_ptr(Error.argsparse, Error.argsparse);

    return EOK;
}

test$case(test_tassert_eq_string)
{
    char buf[21] = { "foo" };
    char* s = "foo";
    tassert_eq(s, "foo");
    tassert_ne(s, NULL);
    tassert_eq(buf, "foo");
    tassert_eq(s, buf);
    char* s2 = NULL;
    tassert_eq(s2, NULL);
    tassert_eq("", "");

    return EOK;
}

test$case(test_tassert_eq_slice)
{
    tassert_eq(str.sstr(NULL), (str_s){ 0 });
    tassert_eq(str.sstr(""), str$s(""));
    tassert_eq(str.sstr("foo"), str$s("foo"));
    tassert_eq(str.sstr("foo"), str.sub("foobar", 0, 3));
    tassert_ne(str.sstr("foo1"), str$s("foo"));

    return EOK;
}

test$case(test_tassert_eq_arr)
{
    mem$scope(tmem$, _)
    {
        arr$(int) a = arr$new(a, _);
        arr$(i32) b = arr$new(b, _);

        arr$pushm(a, 1, 2, 3);
        arr$pushm(b, 1, 2, 3);
        tassert_eq_arr(a, b);

        // arr$pushm(b, 1, 2, 3);
        // tassert_eq_arr(a, b);
    }

    return EOK;
}

test$case(test_tassert_eq_arr_vs_static)
{
    mem$scope(tmem$, _)
    {
        i32 b[] = { 1, 2, 3 };
        arr$(int) a = arr$new(a, _);

        arr$pushm(a, 1, 2, 3);
        tassert_eq_arr(a, b);
    }

    return EOK;
}

test$case(test_tassert_static_arr_vs_static)
{
    i32 a[] = { 1, 2, 3 };
    i32 b[] = { 1, 2, 3 };
    tassert_eq_arr(a, b);
    return EOK;
}


test$case(test_tassert_static_arr_vs_null)
{
    i32* a = NULL;
    i32* b = NULL;
    tassert_eq_arr(a, b);

    return EOK;
}

test$case(test_tassert_struct_arr)
{
    mem$scope(tmem$, _)
    {

        arr$(str_s) a = arr$new(a, _);
        arr$(str_s) b = arr$new(b, _);
        str_s s = str$s("foo");

        arr$pushm(a, s, { 0 }, s);
        arr$pushm(b, s, { 0 }, s);
        tassert_eq_arr(a, b);
    }

    return EOK;
}
test$case(test_tassert_eq_mem)
{
    char* f = "foo";
    str_s a = str.sstr(f);
    str_s b = str.sstr(f);
    tassert_eq(a, b);
    tassert_eq_mem(a, b);
    tassert_eq_mem(a, (str_s){ .buf = f, .len = strlen(f) });

    return EOK;
}

test$case(test_tassert_eq_string_multiline)
{
    char* s = "1234567890\nfoo\nbar\nbaz\nfuzz";
    char* s2 = "1234567890\nfoo\nbar\nbazz\nfuz";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "A at line 003: `baz`"));
    tassert(str.find(e, "B at line 003: `bazz`"));
    tassert(str.find(e, "A at line 004: `fuzz`"));
    tassert(str.find(e, "B at line 004: `fuz`"));

    return EOK;
}

test$case(test_tassert_eq_string_multiline_a_shorter)
{
    char* s = "1234567890\nfoo\nbar\nbaz";
    char* s2 = "1234567890\nfoo\nbar\nbaz\nfuz";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "A at line 004: (too short)"));
    tassert(str.find(e, "B at line 004: `fuz`"));

    return EOK;
}

test$case(test_tassert_eq_string_multiline_b_shorter)
{
    char* s = "1234567890\nfoo\nbar\nbaz\noops";
    char* s2 = "1234567890\nfoo\nbar\nbaz";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "A at line 004: `oops`"));
    tassert(str.find(e, "B at line 004: (too short)"));

    return EOK;
}

test$case(test_tassert_eq_string_multiline_empty_a)
{
    char* s = "";
    char* s2 = "1234567890\nfoo\nbar\nbazz\nfuz";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "A at line 001: (too short)"));

    return EOK;
}

test$case(test_tassert_eq_string_multiline_empty_b)
{
    char* s = "1234567890\nfoo\nbar\nbazz\nfuz";
    char* s2 = "";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "B at line 001: (too short)"));

    return EOK;
}

test$case(test_tassert_eq_string_long_non_multiline)
{
    char* s = "1234567890foobarbazoops";
    char* s2 = "1234567890foobarbaz";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "^"));

    return EOK;
}

test$case(test_tassert_eq_string_long_non_multiline_middle)
{
    char* s = "1234567890f0obarbaz12345";
    char* s2 = "1234567890foobarbaz12345";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "^"));

    return EOK;
}

test$case(test_tassert_eq_string_long_non_multiline_start)
{
    char* s = "234567890fobarbaz12345";
    char* s2 = "1234567890foobarbaz12345";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "^"));

    return EOK;
}

test$case(test_tassert_eq_string_not_equal_end_new_line)
{
    char* s = "1234567890foobarbaz12345\n";
    char* s2 = "1234567890foobarbaz12345";
    Exc e = _check_eq_str(s, s2, 99, _cex_test_eq_op__eq);

    io.printf("Error: %s\n", e);
    tassert(str.find(e, "strings are not equal"));
    tassert(str.find(e, "^"));

    return EOK;
}

test$case(test_alloc)
{
    tassert(test$alloc != NULL);

    u8* p = mem$malloc(test$alloc, 10);
    memset(p, 0x4f, 10);

    // NOTE: no need for cleanup, no memory leaks for convenience
    tassert_eq(*p, 0x4f);

    // If this fails you still don't get asan errors about memory leaks, it's for convenience
    // tassert_eq(*p, 0x4b);


    return EOK;
}

test$case(test_alloc_muptiple_pages)
{
    // These allocations are not freed, but they don't have to trigger test memory leak error
    tassert(test$alloc != NULL);
    AllocatorArena_c* test_arena = (AllocatorArena_c*)test$alloc;
    // page created by allocation in test$setup_case
    tassert_eq(test_arena->stats.pages_created, 1);

    u8* p2 = mem$malloc(test$alloc, 10);
    tassert(p2);
    tassert_eq(test_arena->stats.pages_created, 1);

    u8* p = mem$malloc(test$alloc, 10 * 1024 * 1024);
    tassert(p);
    tassert_eq(test_arena->stats.pages_created, 2);

    // will trigger asan and test leak protection
    // p = mem$malloc(mem$, 10);
    tassert(p);

    return EOK;
}

test$case(test_alloc_oom_probability)
{
    // These allocations are not freed, but they don't have to trigger test memory leak error
    tassert(test$alloc != NULL);
    AllocatorArena_c* test_arena = (AllocatorArena_c*)test$alloc;
    tassert_eq(test_arena->stats.pages_created, 1);
    tassert_eq(test_arena->test_oom_probability, 0.0);

    // never fail
    for(u32 i = 0; i < 10000; i++){
        u8* p = mem$malloc(test$alloc, 10);
        tassert(p != NULL);
    }

    test$alloc_set_oom_probability(1.0);

    for(u32 i = 0; i < 10000; i++){
        u8* p = mem$malloc(test$alloc, 10);
        tassert(p == NULL);
    }

    // 50/50% fail
    u32 nfails = 0;
    test$alloc_set_oom_probability(0.5);
    for(u32 i = 0; i < 10000; i++){
        u8* p = mem$malloc(test$alloc, 10);
        if (p == NULL) {nfails++;}
    }
    tassertf(nfails > 4500 && nfails < 5500, "nfails=%d", nfails);

    return EOK;
}

test$case(test_alloc_oom_probability_always_reset)
{
    tassert(test$alloc != NULL);
    AllocatorArena_c* test_arena = (AllocatorArena_c*)test$alloc;
    tassert_eq(test_arena->stats.pages_created, 1);
    tassert_eq(test_arena->test_oom_probability, 0.0);
    return EOK;
}
test$main();
