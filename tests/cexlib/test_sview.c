#include <cex/cex.h>
#include <cex/cexlib/list.h>
#include <cex/cexlib/sview.c>
#include <cex/cextest/cextest.h>
#include <stdio.h>

/*
 * SUITE INIT / SHUTDOWN
 */
void
my_test_shutdown_func(void)
{
}

ATEST_SETUP_F(void)
{
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}

/*
 *
 *   TEST SUITE
 *
 */

ATEST_F(test_cstr)
{
    const char* cstr = "hello";


    sview_c s = sview.cstr(cstr);
    atassert_eqs(s.buf, cstr);
    atassert_eqi(s.len, 5); // lazy init until sview.length() is called
    atassert(s.buf == cstr);
    atassert_eqi(sview.len(s), 5);
    atassert_eqi(sview.is_valid(s), true);


    atassert_eqi(s.len, 5); // now s.len is set

    sview_c snull = sview.cstr(NULL);
    atassert_eqs(snull.buf, NULL);
    atassert_eqi(snull.len, 0);
    atassert_eqi(sview.len(snull), 0);
    atassert_eqi(sview.is_valid(snull), false);

    sview_c sempty = sview.cstr("");
    atassert_eqs(sempty.buf, "");
    atassert_eqi(sempty.len, 0);
    atassert_eqi(sview.len(sempty), 0);
    atassert_eqi(sempty.len, 0);
    atassert_eqi(sview.is_valid(sempty), true);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_copy)
{
    char buf[8];
    memset(buf, 'a', arr$len(buf));

    sview_c s = sview.cstr("1234567");
    atassert_eqi(s.len, 7);

    memset(buf, 'a', arr$len(buf));
    atassert_eqs(EOK, sview.copy(s, buf, arr$len(buf)));
    atassert_eqs("1234567", buf);
    atassert_eqi(s.len, 7);
    atassert_eqi(s.buf[7], '\0');

    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.argument, sview.copy(s, NULL, arr$len(buf)));
    atassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);

    memset(buf, 'a', arr$len(buf));
    atassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);
    atassert_eqs(Error.argument, sview.copy(s, buf, 0));

    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.check, sview.copy((sview_c){ 0 }, buf, arr$len(buf)));
    // buffer reset to "" string if s is bad
    atassert_eqs("", buf);


    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.check, sview.copy((sview_c){ 0 }, buf, arr$len(buf)));
    // buffer reset to "" string
    atassert_eqs("", buf);

    sview_c sbig = sview.cstr("12345678");
    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.overflow, sview.copy(sbig, buf, arr$len(buf)));
    // string is truncated
    atassert_eqs("1234567", buf);

    sview_c ssmall = sview.cstr("1234");
    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.ok, sview.copy(ssmall, buf, arr$len(buf)));
    atassert_eqs("1234", buf);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sub_positive_start)
{

    sview_c sub;
    sview_c s = sview.cstr("123456");
    atassert_eqi(s.len, 6);
    atassert_eqi(sview.is_valid(s), true);

    sub = sview.sub(s, 0, s.len);
    atassert_eqi(sview.is_valid(sub), true);
    atassert(memcmp(sub.buf, "123456", s.len) == 0);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);


    sub = sview.sub(s, 1, 3);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 2);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "23", sub.len) == 0);

    sub = sview.sub(s, 1, 2);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 1);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "2", sub.len) == 0);

    sub = sview.sub(s, s.len - 1, s.len);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 1);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "6", sub.len) == 0);

    sub = sview.sub(s, 2, 0);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 4);
    atassert(memcmp(sub.buf, "3456", sub.len) == 0);

    sub = sview.sub(s, 0, s.len + 1);
    atassert_eqi(sview.is_valid(sub), false);

    sub = sview.sub(s, 2, 1);
    atassert_eqi(sview.is_valid(sub), false);

    sub = sview.sub(s, s.len, s.len + 1);
    atassert_eqi(sview.is_valid(sub), false);

    s = sview.cstr(NULL);
    sub = sview.sub(s, 1, 2);
    atassert_eqi(sview.is_valid(sub), false);

    s = sview.cstr("");
    atassert_eqi(sview.is_valid(s), true);

    sub = sview.sub(s, 0, 0);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 0);
    atassert_eqs("", sub.buf);

    sub = sview.sub(s, 1, 2);
    atassert_eqi(sview.is_valid(sub), false);

    s = sview.cstr("A");
    sub = sview.sub(s, 1, 2);
    atassert_eqi(sview.is_valid(sub), false);

    s = sview.cstr("A");
    sub = sview.sub(s, 0, 2);
    atassert_eqi(sview.is_valid(sub), false);

    s = sview.cstr("A");
    sub = sview.sub(s, 0, 0);
    atassert_eqi(sview.is_valid(sub), true);
    atassert(memcmp(sub.buf, "A", sub.len) == 0);
    atassert_eqi(sub.len, 1);


    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sub_negative_start)
{

    sview_c s = sview.cstr("123456");
    atassert_eqi(s.len, 6);
    atassert_eqi(sview.is_valid(s), true);

    let sub = sview.sub(s, -3, -1);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 2);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = sview.sub(s, -6, 0);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "123456", sub.len) == 0);


    sub = sview.sub(s, -3, -2);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 1);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "4", sub.len) == 0);

    sub = sview.sub(s, -3, -1);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 2);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = sview.sub(s, -3, -400);
    atassert_eqi(sview.is_valid(sub), false);

    s = sview.cstr("");
    sub = sview.sub(s, 0, 0);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 0);

    s = sview.cstr("");
    sub = sview.sub(s, -1, 0);
    atassert_eqi(sview.is_valid(sub), false);

    s = sview.cstr("123456");
    sub = sview.sub(s, -6, 0);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = sview.cstr("123456");
    sub = sview.sub(s, -6, 6);
    atassert_eqi(sview.is_valid(sub), true);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = sview.cstr("123456");
    sub = sview.sub(s, -7, 0);
    atassert_eqi(sview.is_valid(sub), false);

    return NULL;
}

ATEST_F(test_iter)
{

    sview_c s = sview.cstr("123456");
    atassert_eqi(s.len, 6);
    u32 nit = 0;
    for$iter(char, it, sview.iter(s, &it.iterator))
    {
        atassert_eqi(*it.val, s.buf[it.idx.i]);
        atassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    atassert_eqi(s.len, nit);
    atassert_eqi(sview.is_valid(s), true);

    s = sview.cstr(NULL);
    nit = 0;
    for$iter(char, it, sview.iter(s, &it.iterator))
    {
        nit++;
    }
    atassert_eqi(0, nit);

    s = sview.cstr("");
    nit = 0;
    for$iter(char, it, sview.iter(s, &it.iterator))
    {
        nit++;
    }
    atassert_eqi(0, nit);

    s = sview.cstr("123456");
    s = sview.sub(s, 2, 5);
    atassert(memcmp(s.buf, "345", s.len) == 0);
    atassert_eqi(s.len, 3);

    nit = 0;
    for$iter(char, it, sview.iter(s, &it.iterator))
    {
        // printf("%ld: %c\n", it.idx.i, *it.val);
        atassert_eqi(*it.val, s.buf[it.idx.i]);
        atassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    atassert_eqi(s.len, nit);
    atassert_eqi(sview.is_valid(s), true);

    nit = 0;
    for$array(it, s.buf, s.len)
    {
        // printf("%ld: %c\n", it.idx.i, *it.val);
        atassert_eqi(*it.val, s.buf[it.idx]);
        atassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    atassert_eqi(s.len, nit);
    return NULL;
}

ATEST_F(test_iter_split)
{

    sview_c s = sview.cstr("123456");
    u32 nit = 0;
    char buf[128] = { 0 };
    

    nit = 0;
    const char* expected1[] = {
        "123456",
    };
    for$iter(sview_c, it, sview.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(sview.is_valid(*it.val), true);
        atassert_eqs(Error.ok, sview.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(sview.cmpc(*it.val, expected1[nit]), 0);
        nit++;
    }
    atassert_eqi(nit, 1);

    nit = 0;
    s = sview.cstr("123,456");
    const char* expected2[] = {
        "123",
        "456",
    };
    for$iter(sview_c, it, sview.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(sview.is_valid(*it.val), true);
        atassert_eqs(Error.ok, sview.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(sview.cmpc(*it.val, expected2[nit]), 0);
        nit++;
    }
    atassert_eqi(nit, 2);

    nit = 0;
    s = sview.cstr("123,456,88,99");
    const char* expected3[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(sview_c, it, sview.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(sview.is_valid(*it.val), true);
        atassert_eqs(Error.ok, sview.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(sview.cmpc(*it.val, expected3[nit]), 0);
        nit++;
    }
    atassert_eqi(nit, 4);

    nit = 0;
    const char* expected4[] = {
        "123",
        "456",
        "88",
        "",
    };
    s = sview.cstr("123,456,88,");
    for$iter(sview_c, it, sview.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(sview.is_valid(*it.val), true);
        atassert_eqs(Error.ok, sview.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(sview.cmpc(*it.val, expected4[nit]), 0);
        nit++;
    }
    atassert_eqi(nit, 4);

    nit = 0;
    s = sview.cstr("123,456#88@99");
    const char* expected5[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(sview_c, it, sview.iter_split(s, ",@#", &it.iterator))
    {
        atassert_eqi(sview.is_valid(*it.val), true);
        atassert_eqs(Error.ok, sview.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(sview.cmpc(*it.val, expected5[nit]), 0);
        nit++;
    }
    atassert_eqi(nit, 4);
    return NULL;
}

ATEST_F(test_indexof)
{

    sview_c s = sview.cstr("123456");
    atassert_eqi(s.len, 6);

    // match first
    atassert_eqi(0, sview.indexof(s, sview.cstr("1"), 0, 0));

    // match full
    atassert_eqi(0, sview.indexof(s, sview.cstr("123456"), 0, 0));

    // needle overflow s
    atassert_eqi(-1, sview.indexof(s, sview.cstr("1234567"), 0, 0));

    // match
    atassert_eqi(1, sview.indexof(s, sview.cstr("23"), 0, 0));

    // empty needle
    atassert_eqi(-1, sview.indexof(s, sview.cstr(""), 0, 0));

    // empty string
    atassert_eqi(-1, sview.indexof(sview.cstr(""), sview.cstr("23"), 0, 0));

    // bad string
    atassert_eqi(-1, sview.indexof(sview.cstr(NULL), sview.cstr("23"), 0, 0));

    // starts after match
    atassert_eqi(-1, sview.indexof(s, sview.cstr("23"), 2, 0));

    // bad needle
    atassert_eqi(-1, sview.indexof(s, sview.cstr(NULL), 0, 0));

    // no match
    atassert_eqi(-1, sview.indexof(s, sview.cstr("foo"), 0, 0));

    // match at the end
    atassert_eqi(4, sview.indexof(s, sview.cstr("56"), 0, 0));
    atassert_eqi(5, sview.indexof(s, sview.cstr("6"), 0, 0));

    // middle
    atassert_eqi(3, sview.indexof(s, sview.cstr("45"), 0, 0));

    // ends before match
    atassert_eqi(-1, sview.indexof(s, sview.cstr("56"), 0, 5));

    // ends more than length ok
    atassert_eqi(4, sview.indexof(s, sview.cstr("56"), 0, 500));

    // start >= len
    atassert_eqi(-1, sview.indexof(s, sview.cstr("1"), 6, 5));
    atassert_eqi(-1, sview.indexof(s, sview.cstr("1"), 7, 5));


    return NULL;
}

ATEST_F(test_contains_starts_ends)
{
    sview_c s = sview.cstr("123456");
    atassert_eqi(1, sview.contains(s, sview.cstr("1")));
    atassert_eqi(1, sview.contains(s, sview.cstr("123456")));
    atassert_eqi(0, sview.contains(s, sview.cstr("1234567")));
    atassert_eqi(0, sview.contains(s, sview.cstr("foo")));
    atassert_eqi(0, sview.contains(s, sview.cstr("")));
    atassert_eqi(0, sview.contains(s, sview.cstr(NULL)));
    atassert_eqi(0, sview.contains(sview.cstr(""), sview.cstr("1")));
    atassert_eqi(0, sview.contains(sview.cstr(NULL), sview.cstr("1")));

    atassert_eqi(1, sview.starts_with(s, sview.cstr("1")));
    atassert_eqi(1, sview.starts_with(s, sview.cstr("1234")));
    atassert_eqi(0, sview.starts_with(s, sview.cstr("56")));
    atassert_eqi(0, sview.starts_with(s, sview.cstr("")));
    atassert_eqi(0, sview.starts_with(s, sview.cstr(NULL)));
    atassert_eqi(0, sview.starts_with(sview.cstr(""), sview.cstr("1")));
    atassert_eqi(0, sview.starts_with(sview.cstr(NULL), sview.cstr("1")));


    atassert_eqi(1, sview.ends_with(s, sview.cstr("6")));
    atassert_eqi(1, sview.ends_with(s, sview.cstr("123456")));
    atassert_eqi(1, sview.ends_with(s, sview.cstr("56")));
    atassert_eqi(0, sview.ends_with(s, sview.cstr("1234567")));
    atassert_eqi(0, sview.ends_with(s, sview.cstr("5")));
    atassert_eqi(0, sview.ends_with(s, sview.cstr("")));
    atassert_eqi(0, sview.ends_with(s, sview.cstr(NULL)));
    atassert_eqi(0, sview.ends_with(sview.cstr(""), sview.cstr("1")));
    atassert_eqi(0, sview.ends_with(sview.cstr(NULL), sview.cstr("1")));

    return NULL;
}

ATEST_F(test_strip)
{
    sview_c s = sview.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    (void)s;
    sview_c out;

    // LEFT
    out = sview.lstrip(sview.cstr(NULL));
    atassert_eqi(out.len, 0);
    atassert(out.buf == NULL);

    out = sview.lstrip(sview.cstr(""));
    atassert_eqi(out.len, 0);
    atassert_eqs(out.buf, "");

    out = sview.lstrip(s);
    atassert_eqi(out.len, 14);
    atassert_eqs(out.buf, "123\n\t\r 456 \r\n\t");


    // RIGHT
    out = sview.rstrip(sview.cstr(NULL));
    atassert_eqi(out.len, 0);
    atassert(out.buf == NULL);

    out = sview.rstrip(sview.cstr(""));
    atassert_eqi(out.len, 0);
    atassert_eqs(out.buf, "");

    s = sview.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = sview.rstrip(s);
    atassert_eqi(out.len, 14);
    atassert_eqi(memcmp(out.buf, "\n\t \r123\n\t\r 456", out.len), 0);

    // BOTH
    out = sview.strip(sview.cstr(NULL));
    atassert_eqi(out.len, 0);
    atassert(out.buf == NULL);

    out = sview.strip(sview.cstr(""));
    atassert_eqi(out.len, 0);
    atassert_eqs(out.buf, "");

    s = sview.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = sview.strip(s);
    atassert_eqi(out.len, 10);
    atassert_eqi(memcmp(out.buf, "123\n\t\r 456", out.len), 0);

    return NULL;
}

ATEST_F(test_cmp)
{

    atassert_eqi(sview.cmp(sview.cstr("123456"), sview.cstr("123456")), 0);
    atassert_eqi(sview.cmp(sview.cstr(NULL), sview.cstr(NULL)), 0);
    atassert_eqi(sview.cmp(sview.cstr(""), sview.cstr("")), 0);
    atassert_eqi(sview.cmp(sview.cstr("ABC"), sview.cstr("AB")), 67);
    atassert_eqi(sview.cmp(sview.cstr("ABA"), sview.cstr("ABZ")), -25);
    atassert_eqi(sview.cmp(sview.cstr("AB"), sview.cstr("ABC")), -67);
    atassert_eqi(sview.cmp(sview.cstr("A"), sview.cstr("")), (int)'A');
    atassert_eqi(sview.cmp(sview.cstr(""), sview.cstr("A")), -1*((int)'A'));
    atassert_eqi(sview.cmp(sview.cstr(""), sview.cstr(NULL)), 1);
    atassert_eqi(sview.cmp(sview.cstr(NULL), sview.cstr("ABC")), -1);

    atassert_eqi(sview.cmpc(sview.cstr("ABC"), "AB"), 67);

    return NULL;
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
__attribute__((optimize("O0"))) int
main(int argc, char* argv[])
{
    ATEST_PARSE_MAINARGS(argc, argv);
    ATEST_PRINT_HEAD();  // >>> all tests below
    
    ATEST_RUN(test_cstr);
    ATEST_RUN(test_copy);
    ATEST_RUN(test_sub_positive_start);
    ATEST_RUN(test_sub_negative_start);
    ATEST_RUN(test_iter);
    ATEST_RUN(test_iter_split);
    ATEST_RUN(test_indexof);
    ATEST_RUN(test_contains_starts_ends);
    ATEST_RUN(test_strip);
    ATEST_RUN(test_cmp);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
