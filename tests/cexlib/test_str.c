#include <cex/cextest/cextest.h>
#include <cex/cex.h>
#include <cex/cexlib/str.c>
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

    str_c s = str.cstr(cstr);
    atassert_eqs(s.buf, cstr);
    atassert_eqi(s.len, 5); // lazy init until str.length() is called
    atassert(s.buf == cstr);
    atassert_eqi(str.length(s), 5);
    atassert_eqi(str.is_valid(s), true);

    atassert_eqi(s.len, 5); // now s.len is set

    str_c snull = str.cstr(NULL);
    atassert_eqs(snull.buf, NULL);
    atassert_eqi(snull.len, 0);
    atassert_eqi(str.length(snull), 0);
    atassert_eqi(str.is_valid(snull), false);

    str_c sempty = str.cstr("");
    atassert_eqs(sempty.buf, "");
    atassert_eqi(sempty.len, 0);
    atassert_eqi(str.length(sempty), 0);
    atassert_eqi(sempty.len, 0);
    atassert_eqi(str.is_valid(sempty), true);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_copy)
{
    char buf[8];
    memset(buf, 'a', len(buf));

    str_c s = str.cstr("1234567");
    atassert_eqi(s.len, 7);

    memset(buf, 'a', len(buf));
    atassert_eqs(EOK, str.copy(s, buf, len(buf)));
    atassert_eqs("1234567", buf);
    atassert_eqi(s.len, 7);
    atassert_eqi(s.buf[7], '\0');

    memset(buf, 'a', len(buf));
    atassert_eqs(Error.argument, str.copy(s, NULL, len(buf)));
    atassert(memcmp(buf, "aaaaaaaa", len(buf)) == 0);

    memset(buf, 'a', len(buf));
    atassert(memcmp(buf, "aaaaaaaa", len(buf)) == 0);
    atassert_eqs(Error.argument, str.copy(s, buf, 0));

    memset(buf, 'a', len(buf));
    atassert_eqs(Error.check, str.copy((str_c){ 0 }, buf, len(buf)));
    // buffer reset to "" string if s is bad
    atassert_eqs("", buf);


    memset(buf, 'a', len(buf));
    atassert_eqs(Error.check, str.copy((str_c){ 0 }, buf, len(buf)));
    // buffer reset to "" string
    atassert_eqs("", buf);

    str_c sbig = str.cstr("12345678");
    memset(buf, 'a', len(buf));
    atassert_eqs(Error.overflow, str.copy(sbig, buf, len(buf)));
    // string is truncated
    atassert_eqs("1234567", buf);

    str_c ssmall = str.cstr("1234");
    memset(buf, 'a', len(buf));
    atassert_eqs(Error.ok, str.copy(ssmall, buf, len(buf)));
    atassert_eqs("1234", buf);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sub_positive_start)
{

    str_c sub;
    str_c s = str.cstr("123456");
    atassert_eqi(s.len, 6);
    atassert_eqi(str.is_valid(s), true);

    sub = str.sub(s, 0, s.len);
    atassert_eqi(str.is_valid(sub), true);
    atassert(memcmp(sub.buf, "123456", s.len) == 0);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);


    sub = str.sub(s, 1, 3);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 2);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "23", sub.len) == 0);

    sub = str.sub(s, 1, 2);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 1);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "2", sub.len) == 0);

    sub = str.sub(s, s.len - 1, s.len);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 1);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "6", sub.len) == 0);

    sub = str.sub(s, 2, 0);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 4);
    atassert(memcmp(sub.buf, "3456", sub.len) == 0);

    sub = str.sub(s, 0, s.len + 1);
    atassert_eqi(str.is_valid(sub), false);

    sub = str.sub(s, 2, 1);
    atassert_eqi(str.is_valid(sub), false);

    sub = str.sub(s, s.len, s.len + 1);
    atassert_eqi(str.is_valid(sub), false);

    s = str.cstr(NULL);
    sub = str.sub(s, 1, 2);
    atassert_eqi(str.is_valid(sub), false);

    s = str.cstr("");
    atassert_eqi(str.is_valid(s), true);

    sub = str.sub(s, 0, 0);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 0);
    atassert_eqs("", sub.buf);

    sub = str.sub(s, 1, 2);
    atassert_eqi(str.is_valid(sub), false);

    s = str.cstr("A");
    sub = str.sub(s, 1, 2);
    atassert_eqi(str.is_valid(sub), false);

    s = str.cstr("A");
    sub = str.sub(s, 0, 2);
    atassert_eqi(str.is_valid(sub), false);

    s = str.cstr("A");
    sub = str.sub(s, 0, 0);
    atassert_eqi(str.is_valid(sub), true);
    atassert(memcmp(sub.buf, "A", sub.len) == 0);
    atassert_eqi(sub.len, 1);


    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sub_negative_start)
{

    str_c s = str.cstr("123456");
    atassert_eqi(s.len, 6);
    atassert_eqi(str.is_valid(s), true);

    let sub = str.sub(s, -3, -1);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 2);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = str.sub(s, -6, 0);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "123456", sub.len) == 0);


    sub = str.sub(s, -3, -2);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 1);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "4", sub.len) == 0);

    sub = str.sub(s, -3, -1);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 2);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = str.sub(s, -3, -400);
    atassert_eqi(str.is_valid(sub), false);

    s = str.cstr("");
    sub = str.sub(s, 0, 0);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 0);

    s = str.cstr("");
    sub = str.sub(s, -1, 0);
    atassert_eqi(str.is_valid(sub), false);

    s = str.cstr("123456");
    sub = str.sub(s, -6, 0);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.cstr("123456");
    sub = str.sub(s, -6, 6);
    atassert_eqi(str.is_valid(sub), true);
    atassert_eqi(sub.len, 6);
    atassert_eqi(s.len, 6);
    atassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.cstr("123456");
    sub = str.sub(s, -7, 0);
    atassert_eqi(str.is_valid(sub), false);

    return NULL;
}

ATEST_F(test_iter)
{

    str_c s = str.cstr("123456");
    atassert_eqi(s.len, 6);
    u32 nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        atassert_eqi(*it.val, s.buf[it.idx.i]);
        atassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    atassert_eqi(s.len, nit);
    atassert_eqi(str.is_valid(s), true);

    s = str.cstr(NULL);
    nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        nit++;
    }
    atassert_eqi(0, nit);

    s = str.cstr("");
    nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        nit++;
    }
    atassert_eqi(0, nit);

    s = str.cstr("123456");
    s = str.sub(s, 2, 5);
    atassert(memcmp(s.buf, "345", s.len) == 0);
    atassert_eqi(s.len, 3);

    nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        // printf("%ld: %c\n", it.idx.i, *it.val);
        atassert_eqi(*it.val, s.buf[it.idx.i]);
        atassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    atassert_eqi(s.len, nit);
    atassert_eqi(str.is_valid(s), true);

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

    str_c s = str.cstr("123456");
    u32 nit = 0;
    char buf[128] = {0};
    
    nit = 0;
    for$iter(str_c, it, str.iter_split(s, ',', &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, len(buf)));
        // printf("%d: %s\n", nit, buf);
        atassert(memcmp(it.val->buf, "123456", it.val->len) == 0);
        nit++;
    }
    atassert_eqi(nit, 1);

    nit = 0;
    s = str.cstr("123,456");
    for$iter(str_c, it, str.iter_split(s, ',', &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, len(buf)));
        // printf("%d: '%s'\n", nit, buf);
        // atassert(memcmp(it.val->_buf, "123456", it.val->_len) == 0);
        nit++;
    }
    atassert_eqi(nit, 2);

    nit = 0;
    s = str.cstr("123,456,88,99");
    for$iter(str_c, it, str.iter_split(s, ',', &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, len(buf)));
        // printf("%d: '%s'\n", nit, buf);
        // atassert(memcmp(it.val->_buf, "123456", it.val->_len) == 0);
        nit++;
    }
    atassert_eqi(nit, 4);

    nit = 0;
    s = str.cstr("123,456,88,");
    for$iter(str_c, it, str.iter_split(s, ',', &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, len(buf)));
        // atassert(memcmp(it.val->_buf, "123456", it.val->_len) == 0);
        nit++;
    }
    atassert_eqi(nit, 3);

    atassert(false && "FIX: implement str.cmp(str_c) / str.cmpc(char*)");
    return NULL;
}

ATEST_F(test_indexof)
{

    str_c s = str.cstr("123456");
    atassert_eqi(s.len, 6);

    // match first
    atassert_eqi(0, str.indexof(s, str.cstr("1"), 0, 0));

    // match full 
    atassert_eqi(0, str.indexof(s, str.cstr("123456"), 0, 0));

    // needle overflow s
    atassert_eqi(-1, str.indexof(s, str.cstr("1234567"), 0, 0));

    // match
    atassert_eqi(1, str.indexof(s, str.cstr("23"), 0, 0));

    // empty needle
    atassert_eqi(-1, str.indexof(s, str.cstr(""), 0, 0));

    // empty string
    atassert_eqi(-1, str.indexof(str.cstr(""), str.cstr("23"), 0, 0));

    // bad string
    atassert_eqi(-1, str.indexof(str.cstr(NULL), str.cstr("23"), 0, 0));

    // starts after match
    atassert_eqi(-1, str.indexof(s, str.cstr("23"), 2, 0));

    // bad needle
    atassert_eqi(-1, str.indexof(s, str.cstr(NULL), 0, 0));
    
    // no match
    atassert_eqi(-1, str.indexof(s, str.cstr("foo"), 0, 0));

    // match at the end
    atassert_eqi(4, str.indexof(s, str.cstr("56"), 0, 0));
    atassert_eqi(5, str.indexof(s, str.cstr("6"), 0, 0));

    // middle
    atassert_eqi(3, str.indexof(s, str.cstr("45"), 0, 0));

    // ends before match
    atassert_eqi(-1, str.indexof(s, str.cstr("56"), 0, 5));

    // ends more than length ok
    atassert_eqi(4, str.indexof(s, str.cstr("56"), 0, 500));

    // start >= len
    atassert_eqi(-1, str.indexof(s, str.cstr("1"), 6, 5));
    atassert_eqi(-1, str.indexof(s, str.cstr("1"), 7, 5));


    return NULL;
}

ATEST_F(test_contains_starts_ends)
{
    str_c s = str.cstr("123456");
    atassert_eqi(1, str.contains(s, str.cstr("1")));
    atassert_eqi(1, str.contains(s, str.cstr("123456")));
    atassert_eqi(0, str.contains(s, str.cstr("1234567")));
    atassert_eqi(0, str.contains(s, str.cstr("foo")));
    atassert_eqi(0, str.contains(s, str.cstr("")));
    atassert_eqi(0, str.contains(s, str.cstr(NULL)));
    atassert_eqi(0, str.contains(str.cstr(""), str.cstr("1")));
    atassert_eqi(0, str.contains(str.cstr(NULL), str.cstr("1")));

    atassert_eqi(1, str.starts_with(s, str.cstr("1")));
    atassert_eqi(1, str.starts_with(s, str.cstr("1234")));
    atassert_eqi(0, str.starts_with(s, str.cstr("56")));
    atassert_eqi(0, str.starts_with(s, str.cstr("")));
    atassert_eqi(0, str.starts_with(s, str.cstr(NULL)));
    atassert_eqi(0, str.starts_with(str.cstr(""), str.cstr("1")));
    atassert_eqi(0, str.starts_with(str.cstr(NULL), str.cstr("1")));


    atassert_eqi(1, str.ends_with(s, str.cstr("6")));
    atassert_eqi(1, str.ends_with(s, str.cstr("123456")));
    atassert_eqi(1, str.ends_with(s, str.cstr("56")));
    atassert_eqi(0, str.ends_with(s, str.cstr("1234567")));
    atassert_eqi(0, str.ends_with(s, str.cstr("5")));
    atassert_eqi(0, str.ends_with(s, str.cstr("")));
    atassert_eqi(0, str.ends_with(s, str.cstr(NULL)));
    atassert_eqi(0, str.ends_with(str.cstr(""), str.cstr("1")));
    atassert_eqi(0, str.ends_with(str.cstr(NULL), str.cstr("1")));

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
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
