#include "src/all.c"

#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#define USE_STB 1


// stbsp_sprintf
#define CHECK_END(str)                                                                             \
    if (strcmp(buf, str) != 0 || (unsigned)ret != strlen(str)) {                                   \
        printf("< '%s'\n> '%s'\n", str, buf);                                                      \
        tassert(false && "CHECK_END() failed see ^^^");                                            \
    }

// clang-format off
#define CHECK9(str, v1, v2, v3, v4, v5, v6, v7, v8, v9) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2, v3, v4, v5, v6, v7, v8, v9); CHECK_END(str); }
#define CHECK8(str, v1, v2, v3, v4, v5, v6, v7, v8    ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2, v3, v4, v5, v6, v7, v8    ); CHECK_END(str); }
#define CHECK7(str, v1, v2, v3, v4, v5, v6, v7        ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2, v3, v4, v5, v6, v7        ); CHECK_END(str); }
#define CHECK6(str, v1, v2, v3, v4, v5, v6            ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2, v3, v4, v5, v6            ); CHECK_END(str); }
#define CHECK5(str, v1, v2, v3, v4, v5                ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2, v3, v4, v5                ); CHECK_END(str); }
#define CHECK4(str, v1, v2, v3, v4                    ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2, v3, v4                    ); CHECK_END(str); }
#define CHECK3(str, v1, v2, v3                        ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2, v3                        ); CHECK_END(str); }
#define CHECK2(str, v1, v2                            ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1, v2                            ); CHECK_END(str); }
#define CHECK1(str, v1                                ) { int ret = cexsp__snprintf(buf, arr$len(buf), v1                                ); CHECK_END(str); }
// clang-format on


static char*
ret_null_char()
{
    return NULL;
}

/*
 *
 *   TEST SUITE
 *
 */

test$case(stb_sprintf_orig)
{
    char buf[1024];
    int n = 0;
    const double pow_2_75 = 37778931862957161709568.0;
    const double pow_2_85 = 38685626227668133590597632.0;

    // integers
    CHECK4("a b     1", "%c %s     %d", 'a', "b", 1);
    CHECK2("abc     ", "%-8.3s", "abcdefgh");
    CHECK2("+5", "%+2d", 5);
    CHECK2("  6", "% 3i", 6);
    CHECK2("-7  ", "%-4d", -7);
    CHECK2("+0", "%+d", 0);
    CHECK3("     00003:     00004", "%10.5d:%10.5d", 3, 4);
    CHECK2("-100006789", "%d", -100006789);
    CHECK3("20 0020", "%u %04u", 20u, 20u);
    CHECK4("12 1e 3C", "%o %x %X", 10u, 30u, 60u);
    CHECK4(" 12 1e 3C ", "%3o %2x %-3X", 10u, 30u, 60u);
    CHECK4("012 0x1e 0X3C", "%#o %#x %#X", 10u, 30u, 60u);
    CHECK2("", "%.0x", 0);
#if USE_STB
    CHECK2("0", "%.0d", 0); // stb_sprintf gives "0"
#else
    CHECK2("", "%.0d", 0); // glibc gives "" as specified by C99(?)
#endif
    CHECK3("33 555", "%hi %d", (short)33, 555L);
    CHECK2("9888777666", "%llu", 9888777666llu);

    CHECK4("-1 2 -3", "%ji %zi %ti", (intmax_t)-1, (isize)2, (ptrdiff_t)-3);

    // floating-point numbers
    CHECK2("-3.000000", "%f", -3.0);
    CHECK2("-8.8888888800", "%.10f", -8.88888888);
    CHECK2("880.0888888800", "%.10f", 880.08888888);
    CHECK2("4.1", "%.1f", 4.1);
    CHECK2(" 0", "% .0f", 0.1);
    CHECK2("0.00", "%.2f", 1e-4);
    CHECK2("-5.20", "%+4.2f", -5.2);
    CHECK2("0.0       ", "%-10.1f", 0.);
    CHECK2("-0.000000", "%f", -0.);
    CHECK2("0.000001", "%f", 9.09834e-07);
#if USE_STB // rounding differences
    CHECK2("38685626227668133600000000.0", "%.1f", pow_2_85);
    CHECK2("0.000000499999999999999978", "%.24f", 5e-7);
#else
    CHECK2("38685626227668133590597632.0", "%.1f", pow_2_85); // exact
    CHECK2("0.000000499999999999999977", "%.24f", 5e-7);
#endif
    CHECK2("0.000000000000000020000000", "%.24f", 2e-17);
    CHECK3("0.0000000100 100000000", "%.10f %.0f", 1e-8, 1e+8);
    CHECK2("100056789.0", "%.1f", 100056789.0);
    CHECK4(" 1.23 %", "%*.*f %%", 5, 2, 1.23);
    CHECK2("-3.000000e+00", "%e", -3.0);
    CHECK2("4.1E+00", "%.1E", 4.1);
    CHECK2("-5.20e+00", "%+4.2e", -5.2);
    CHECK3("+0.3 -3", "%+g %+g", 0.3, -3.0);
    CHECK2("4", "%.1G", 4.1);
    CHECK2("-5.2", "%+4.2g", -5.2);
    CHECK2("3e-300", "%g", 3e-300);
    CHECK2("1", "%.0g", 1.2);
    CHECK3(" 3.7 3.71", "% .3g %.3g", 3.704, 3.706);
    CHECK3("2e-315:1e+308", "%g:%g", 2e-315, 1e+308);

#if __STDC_VERSION__ >= 199901L
#    if USE_STB
    CHECK4("inf -inf nan", "%g %G %f", (double)INFINITY, (double)-INFINITY, (double)NAN);
    CHECK2("n", "%.1g", (double)NAN);
#    else
    CHECK4("inf INF nan", "%g %G %f", INFINITY, INFINITY, NAN);
    CHECK2("nan", "%.1g", NAN);
#    endif
#endif

    // %n
    CHECK3("aaa ", "%.3s %n", "aaaaaaaaaaaaa", &n);
    tassert(n == 4);

#if __STDC_VERSION__ >= 199901L
    // hex floats
    CHECK2("0x1.fedcbap+98", "%a", 0x1.fedcbap+98);
    CHECK2("0x1.999999999999a0p-4", "%.14a", 0.1);
    CHECK2("0x1.0p-1022", "%.1a", 0x1.ffp-1023);
#    if USE_STB // difference in default precision and x vs X for %A
    CHECK2("0x1.009117p-1022", "%a", 2.23e-308);
    CHECK2("-0x1.AB0P-5", "%.3A", -0x1.abp-5);
#    else
    CHECK2("0x1.0091177587f83p-1022", "%a", 2.23e-308);
    CHECK2("-0X1.AB0P-5", "%.3A", -0X1.abp-5);
#    endif
#endif

    printf("libc %%p: %p\n", (void*)0x1234ABC);
    printf("libc %%p = NULL: %p\n", NULL);
    if (sizeof(usize) == 8) {
        // 64 bits
        CHECK2("0x1234abcdef0707", "%p", (void*)0x1234ABCDef0707);
    } else {
        // 32 bits
        CHECK2("0x1234abc", "%p", (void*)0x1234ABC);
    }
    CHECK2("0x0", "%p", (void*)NULL);

    // snprintf
    tassert(cexsp__snprintf(buf, 100, " %s     %d", "b", 123) == 10);
    tassert(strcmp(buf, " b     123") == 0);
    tassert(cexsp__snprintf(buf, 100, "%f", pow_2_75) == 30);
    tassert(strncmp(buf, "37778931862957161709568.000000", 17) == 0);
    n = cexsp__snprintf(buf, 10, "number %f", 123.456789);
    tassert(strcmp(buf, "number 12") == 0);
    // tassert_eq(n, 9); // written vs would-be written bytes
    tassert_eq(n, 10); // WARNING: cex changed this behavior to handle overflows!
    //
    buf[0] = '\0';
    n = cexsp__snprintf(buf, 0, "7 chars");
    tassert_eq(n, -1);
    tassert_eq(strlen(buf), 0);

    // stb_sprintf uses internal buffer of 512 chars - test longer string
    cexsp__snprintf(buf, 550, "%d  %600s", 3, "abc");
    tassert(strlen(buf) == 549);
    tassert(cexsp__snprintf(buf, 600, "%510s     %c", "a", 'b') == 516);

    // length check
    tassert(cexsp__snprintf(NULL, 0, " %s     %d", "b", 123) == -1);

    // ' modifier. Non-standard, but supported by glibc.
    CHECK2("1,200,000", "%'d", 1200000);
    CHECK2("-100,006,789", "%'d", -100006789);
#if !defined(_MSC_VER) || _MSC_VER >= 1600
    CHECK2("9,888,777,666", "%'lld", 9888777666ll);
#endif
    CHECK2("200,000,000.000000", "%'18f", 2e8);
    CHECK2("100,056,789", "%'.0f", 100056789.0);
    CHECK2("100,056,789.0", "%'.1f", 100056789.0);
    CHECK2("000,001,200,000", "%'015d", 1200000);

    // things not supported by glibc
#if USE_STB
    CHECK2("(null)", "%s", ret_null_char());
    CHECK2("123,4abc:", "%'x:", 0x1234ABC);
    CHECK2("100000000", "%b", 256);
    CHECK3("0b10 0B11", "%#b %#B", 2, 3);
    CHECK3("2 3", "%I64d %I32d", 2ll, 3);
    CHECK3("1k 2.54 M", "%$_d %$.2d", 1000, 2536000);
    CHECK3("2.42 Mi 2.4 M", "%$$.2d %$$$d", 2536000, 2536000);

    // different separators
    cexsp__set_separators(' ', ',');
    CHECK2("12 345,678900", "%'f", 12345.6789);
#endif

    return EOK;
}

test$case(stb_sprintf_integers_16bits)
{
    mem$scope(tmem$, _)
    {
        i16 _i16 = 123;
        u16 _u16 = 123;

        tassert_eq("123", str.fmt(_, "%d", _i16));
        tassert_eq("123", str.fmt(_, "%u", _u16));

        tassert_eq("-123", str.fmt(_, "%d", -_i16));

        _i16 = INT16_MAX;
        _u16 = UINT16_MAX;
        tassert_eq("32767", str.fmt(_, "%d", _i16));
        tassert_eq("65535", str.fmt(_, "%u", _u16));

        _i16 = INT16_MIN;
        _u16 = UINT16_MAX;
        tassert_eq("-32768", str.fmt(_, "%d", _i16));
        tassert_eq("65535", str.fmt(_, "%u", _u16));
        tassert_eq("65535", str.fmt(_, "%d", _u16));
    }

    return EOK;
}

test$case(stb_sprintf_integers_32bits)
{
    mem$scope(tmem$, _)
    {
        i32 _i32 = 123;
        u32 _u32 = 123;

        tassert_eq("123", str.fmt(_, "%d", _i32));
        tassert_eq("123", str.fmt(_, "%u", _u32));

        tassert_eq("-123", str.fmt(_, "%d", -_i32));

        _i32 = INT32_MAX;
        _u32 = INT32_MAX;
        tassert_eq("2147483647", str.fmt(_, "%d", _i32));
        tassert_eq("2147483647", str.fmt(_, "%u", _u32));

        _i32 = INT32_MIN;
        _u32 = UINT32_MAX;
        tassert_eq("-2147483648", str.fmt(_, "%d", _i32));
        tassert_eq("4294967295", str.fmt(_, "%u", _u32));
        tassert_eq("-1", str.fmt(_, "%d", _u32));
    }

    return EOK;
}

test$case(stb_sprintf_integers_64bits)
{
    mem$scope(tmem$, _)
    {
        i64 _i64 = 123;
        u64 _u64 = 123;

        tassert_eq("123", str.fmt(_, "%ld", _i64));
        tassert_eq("123", str.fmt(_, "%lu", _u64));

        tassert_eq("-123", str.fmt(_, "%ld", -_i64));

        _i64 = INT64_MAX;
        _u64 = INT64_MAX;
        tassert_eq("9223372036854775807", str.fmt(_, "%ld", _i64));
        tassert_eq("9223372036854775807", str.fmt(_, "%lu", _u64));

        _i64 = INT64_MIN;
        _u64 = UINT64_MAX;
        tassert_eq("-9223372036854775808", str.fmt(_, "%ld", _i64));
        tassert_eq("18446744073709551615", str.fmt(_, "%lu", _u64));
        tassert_eq("-1", str.fmt(_, "%ld", _u64));
    }

    return EOK;
}

test$case(stb_sprintf_integers_64bits_hex)
{
    mem$scope(tmem$, _)
    {
        i64 _i64 = 123;
        u64 _u64 = 123;

        tassert_eq("7b", str.fmt(_, "%lx", _i64));
        tassert_eq("7b", str.fmt(_, "%lx", _u64));

        _i64 = INT64_MAX;
        _u64 = INT64_MAX;
        tassert_eq("7fffffffffffffff", str.fmt(_, "%lx", _i64));
        tassert_eq("7fffffffffffffff", str.fmt(_, "%lx", _u64));
        tassert_eq("0x7fffffffffffffff", str.fmt(_, "%#lx", _i64));
        tassert_eq("0x7fffffffffffffff", str.fmt(_, "%#lx", _u64));

        _i64 = INT64_MIN;
        _u64 = UINT64_MAX;
        // NOTE: libc printf() also use hex without negative sign!!!
        // printf("--- %#lx \n", _i64);
        tassert_eq("0x8000000000000000", str.fmt(_, "%#lx", _i64));
        tassert_eq("0xffffffffffffffff", str.fmt(_, "%#lx", _u64));
        tassert_eq("8000000000000000", str.fmt(_, "%lx", _i64));
        tassert_eq("ffffffffffffffff", str.fmt(_, "%lx", _u64));
        tassert_eq("-1", str.fmt(_, "%ld", _u64));
    }

    return EOK;
}

test$case(stb_sprintf_size_t)
{
    mem$scope(tmem$, _)
    {
        usize _i64 = 123;
        isize _u64 = 123;

        tassert_eq("123", str.fmt(_, "%zd", _i64));
        tassert_eq("123", str.fmt(_, "%zu", _u64));

        tassert_eq("-123", str.fmt(_, "%zd", -_i64));

#if __SIZEOF_POINTER__ == 8
        _i64 = INT64_MAX;
        _u64 = INT64_MAX;
        tassert_eq("9223372036854775807", str.fmt(_, "%zd", _i64));
        tassert_eq("9223372036854775807", str.fmt(_, "%zu", _u64));

        _i64 = (usize)INT64_MIN;
        _u64 = (isize)UINT64_MAX;
        tassert_eq("-9223372036854775808", str.fmt(_, "%zd", _i64));
        tassert_eq("18446744073709551615", str.fmt(_, "%zu", _u64));
        tassert_eq("-1", str.fmt(_, "%zd", _u64));
#else
        _i64 = INT32_MAX;
        _u64 = INT32_MAX;
        tassert_eq("2147483647", str.fmt(_, "%zd", _i64));
        tassert_eq("2147483647", str.fmt(_, "%zu", _u64));

        _i64 = INT32_MIN;
        _u64 = UINT32_MAX;
        tassert_eq("-2147483648", str.fmt(_, "%zd", _i64));
        tassert_eq("4294967295", str.fmt(_, "%zu", _u64));
        tassert_eq("-1", str.fmt(_, "%zd", _u64));
#endif
    }

    return EOK;
}

int
is_little_endian()
{
    unsigned int x = 1;
    char* c = (char*)&x;
    return (int)*c;
}

test$case(stb_sprintf_strings)
{
    mem$scope(tmem$, _)
    {
        // valid array
        char buf[] = "foo";
        tassert_eq("foo", str.fmt(_, "%s", buf));

        // valid strings
        tassert_eq("bar", str.fmt(_, "%s", "bar"));
        tassert_eq("foo-bar", str.fmt(_, "%s-%s", buf, "bar"));

        // valid slice
        tassert_eq("foo", str.fmt(_, "%S", str$s("foo")));
        tassert_eq("", str.fmt(_, "%S", str$s("")));

        // "valid" NULL string and slice (common error)
        tassert_eq("(null)", str.fmt(_, "%s", NULL));
        tassert_eq("(null)", str.fmt(_, "%S", (str_s){ 0 }));

        // string with dynamic len
        tassert_eq("12345", str.fmt(_, "%.*s", 5, "123456789"));
        tassert_eq("12345", str.fmt(_, "%.*S", 5, str$s("123456789")));
        tassert_eq("123456789", str.fmt(_, "%.*s", 500, "123456789"));
        tassert_eq("123456789", str.fmt(_, "%.*S", 500, str$s("123456789")));
        tassert_eq("123456789", str.fmt(_, "%.*s", -1, "123456789"));
        tassert_eq("123456789", str.fmt(_, "%.*S", -1, str$s("123456789")));
        tassert_eq("123", str.fmt(_, "%.3s", "123456789"));
        tassert_eq("123", str.fmt(_, "%.3S", str$s("123456789")));


        if (!is_little_endian()) {
            // We in production test on big endian arch (%s-Bad) stuff likely to segfault!
            return EOK;
        }
        char* valgrind_env = getenv("CEX_VALGRIND");
        if (valgrind_env && valgrind_env[0] == '1') {
            // Running under VALGRIND, causes errors, this is expected, just skip
            return EOK;
        }

        /* (DEVELOPER MODE ONLY - early warning about wrong stuff / ASAN / VALGRIND failing)
         * Damage control wrong args (these are intentional bugs, trying to mitigate them)
         */
        // NOTE: cases below are invalid use of %s/%S and arguments
        // but CEX attempts gracefully handle them if possible
        tassert_eq("(null)", str.fmt(_, "%s", 0));

        if (os.platform.current() == OSPlatform__wasm) {
            // Wasm segfaults on the bad cases 
            return EOK;
        }

        tassert_eq("(%s-bad)", str.fmt(_, "%s", 7));
        tassert_eq("(%S-bad/overflow)", str.fmt(_, "%S", (str_s){ .len = 65536, .buf = "baz" }));

        u64 baad = 0xfe03ba0d;
        (void)baad;
#ifdef _WIN32
        // IMPORTANT: win32 va arg implementation passes str_s by pointer,
        //   therefore some error check heuristics do not work on windows (segfaults)
        // tassert_eq("(%S-bad)", str.fmt(_, "%S",  baad)); // segv
        // uassert(false && "Teest");

        tassert_eq("", str.fmt(_, "%s", str$s("")));    // win mismatch
        tassert_eq("", str.fmt(_, "%s", (str_s){ 0 })); // win mismatch ''
        tassert_eq("(%s-bad)", str.fmt(_, "%s", baad));
#    if !mem$asan_enabled()
        tassert_eq("(%S-bad/overflow)", str.fmt(_, "%S", "foo", 123)); // win asan crash
#    endif
        tassert_eq("(%S-bad)", str.fmt(_, "%S", (str_s){ .len = 6, .buf = (void*)0xfe03ba0d }));
        // tassert_eq("(%s-bad)", str.fmt(_, "%s",  str$s("123456789"))); // mismatch \t
#else
        // the below stuff is implementation specific or even UB! Trying doing the best to catch.
        char* fmt_bad = str.fmt(_, "%S", baad);
        tassertf(
            str.eq(fmt_bad, "(%S-bad/overflow)") || str.eq(fmt_bad, "(null)"),
            "(u65)baad fortatted as: %s",
            fmt_bad
        );
        fmt_bad = str.fmt(_, "%S", "foo");
        tassertf(
            str.eq(fmt_bad, "(%S-bad/overflow)") || str.eq(fmt_bad, "(null)"),
            " \"foo\" fortatted as: %s",
            fmt_bad
        );
        fmt_bad = str.fmt(_, "%S", NULL);
        tassertf(
            str.eq(fmt_bad, "") || str.eq(fmt_bad, "(null)"),
            " \"NULL\" fortatted as: %s",
            fmt_bad
        );
        tassert_eq("(null)", str.fmt(_, "%s", str$s("")));
        tassert_eq("(%s-bad)", str.fmt(_, "%s", str$s("foo"))); // win segv
        tassert_eq("(null)", str.fmt(_, "%s", (str_s){ 0 }));   // win mismatch ''
        tassert_eq("(%s-bad)", str.fmt(_, "%s", str$s("foo"))); // win segv!
        // tassert_eq("(%s-bad)", str.fmt(_, "%s",  baad)); // segv on linux
#endif
    }

    return EOK;
}

test$main();
