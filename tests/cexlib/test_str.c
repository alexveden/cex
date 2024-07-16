#include <cex/cexlib/cex.c>
#include <cex/cexlib/list.h>
#include <cex/cexlib/str.c>
#include <cex/cexlib/sbuf.c>
#include <cex/cexlib/_stb_sprintf.c>
#include <cex/cextest/cextest.h>
#include <cex/cexlib/allocators.c>
#include <stdio.h>

const Allocator_i* allocator;
/*
* SUITE INIT / SHUTDOWN
*/
void my_test_shutdown_func(void){
    allocator = allocators.heap.destroy();
}

ATEST_SETUP_F(void)
{
    allocator = allocators.heap.create();
    return &my_test_shutdown_func;  // return pointer to void some_shutdown_func(void)
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
    atassert_eqi(str.len(s), 5);
    atassert_eqi(str.is_valid(s), true);


    atassert_eqi(s.len, 5); // now s.len is set

    str_c snull = str.cstr(NULL);
    atassert_eqs(snull.buf, NULL);
    atassert_eqi(snull.len, 0);
    atassert_eqi(str.len(snull), 0);
    atassert_eqi(str.is_valid(snull), false);

    str_c sempty = str.cstr("");
    atassert_eqs(sempty.buf, "");
    atassert_eqi(sempty.len, 0);
    atassert_eqi(str.len(sempty), 0);
    atassert_eqi(sempty.len, 0);
    atassert_eqi(str.is_valid(sempty), true);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_cstr_sdollar)
{
    const char* cstr = "hello";

    sbuf_c sb;
    atassert_eqs(EOK, sbuf.create(&sb, 10, allocator));
    atassert_eqs(EOK, sbuf.append(&sb, s$("hello")));
    str_c sbv = s$(sb);
    atassert_eqs(sbv.buf, cstr);
    atassert_eqi(sbv.len, 5); // lazy init until str.length() is called
    sbuf.destroy(&sb);

    // auto-type also works!
    var s = s$(cstr);
    atassert_eqs(s.buf, cstr);
    atassert_eqi(s.len, 5); // lazy init until str.length() is called
    atassert(s.buf == cstr);
    atassert_eqi(str.len(s), 5);
    atassert_eqi(str.is_valid(s), true);


    str_c s2 = s$(s);
    atassert_eqs(s2.buf, cstr);
    atassert_eqi(s2.len, 5); // lazy init until str.length() is called
    atassert(s2.buf == cstr);
    atassert_eqi(str.len(s2), 5);
    atassert_eqi(str.is_valid(s2), true);

    s2 = s$("hello");
    atassert_eqs(s2.buf, cstr);
    atassert_eqi(s2.len, 5); // lazy init until str.length() is called
    atassert(s2.buf == cstr);
    atassert_eqi(str.len(s2), 5);
    atassert_eqi(str.is_valid(s2), true);

    atassert_eqi(s.len, 5); // now s.len is set

    char* str_null = NULL;
    str_c snull = s$(str_null);
    atassert_eqs(snull.buf, NULL);
    atassert_eqi(snull.len, 0);
    atassert_eqi(str.len(snull), 0);
    atassert_eqi(str.is_valid(snull), false);

    str_c sempty = s$("");
    atassert_eqs(sempty.buf, "");
    atassert_eqi(sempty.len, 0);
    atassert_eqi(str.len(sempty), 0);
    atassert_eqi(sempty.len, 0);
    atassert_eqi(str.is_valid(sempty), true);

    // WARNING: buffers for s$() are extremely bad idea, they may not be null term!
    char buf[30] = {0};
    s = s$(buf);
    atassert_eqs(s.buf, buf);
    atassert_eqi(s.len, 0); // lazy init until str.length() is called
    atassert_eqi(str.len(s), 0);
    atassert_eqi(str.is_valid(s), true);


    memset(buf, 'z', arr$len(buf));

    // WARNING: if no null term
    // s = s$(buf); // !!!! STACK OVERFLOW, no null term!
    // 
    // Consider using str.cbuf() - it limits length by buffer size 
    s = str.cbuf(buf, arr$len(buf));
    atassert_eqi(s.len, 30); 
    atassert_eqi(str.len(s), 30);
    atassert_eqi(str.is_valid(s), true);
    atassert(s.buf == buf);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_copy)
{
    char buf[8];
    memset(buf, 'a', arr$len(buf));

    str_c s = str.cstr("1234567");
    atassert_eqi(s.len, 7);

    memset(buf, 'a', arr$len(buf));
    atassert_eqs(EOK, str.copy(s, buf, arr$len(buf)));
    atassert_eqs("1234567", buf);
    atassert_eqi(s.len, 7);
    atassert_eqi(s.buf[7], '\0');

    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.argument, str.copy(s, NULL, arr$len(buf)));
    atassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0); // untouched

    memset(buf, 'a', arr$len(buf));
    atassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);
    atassert_eqs(Error.argument, str.copy(s, buf, 0)); // untouched

    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.ok, str.copy(s$(""), buf, arr$len(buf)));
    atassert_eqs("", buf);

    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.sanity_check, str.copy(str.cstr(NULL), buf, arr$len(buf)));
    // buffer reset to "" string
    atassert_eqs("", buf);

    str_c sbig = str.cstr("12345678");
    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.overflow, str.copy(sbig, buf, arr$len(buf)));
    // string is truncated
    atassert_eqs("1234567", buf);

    str_c ssmall = str.cstr("1234");
    memset(buf, 'a', arr$len(buf));
    atassert_eqs(Error.ok, str.copy(ssmall, buf, arr$len(buf)));
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

    var sub = str.sub(s, -3, -1);
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
    char buf[128] = { 0 };
    

    nit = 0;
    const char* expected1[] = {
        "123456",
    };
    for$iter(str_c, it, str.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(str.cmp(*it.val, s$(expected1[nit])), 0);
        nit++;
    }
    atassert_eqi(nit, 1);

    nit = 0;
    s = str.cstr("123,456");
    const char* expected2[] = {
        "123",
        "456",
    };
    for$iter(str_c, it, str.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(str.cmp(*it.val, s$(expected2[nit])), 0);
        nit++;
    }
    atassert_eqi(nit, 2);

    nit = 0;
    s = str.cstr("123,456,88,99");
    const char* expected3[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(str_c, it, str.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(str.cmp(*it.val, s$(expected3[nit])), 0);
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
    s = str.cstr("123,456,88,");
    for$iter(str_c, it, str.iter_split(s, ",", &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(str.cmp(*it.val, s$(expected4[nit])), 0);
        nit++;
    }
    atassert_eqi(nit, 4);

    nit = 0;
    s = str.cstr("123,456#88@99");
    const char* expected5[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(str_c, it, str.iter_split(s, ",@#", &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(str.cmp(*it.val, s$(expected5[nit])), 0);
        nit++;
    }
    atassert_eqi(nit, 4);

    nit = 0;
    const char* expected6[] = {
        "123",
        "456",
        "",
    };
    s = str.cstr("123\n456\n");
    for$iter(str_c, it, str.iter_split(s, "\n", &it.iterator))
    {
        atassert_eqi(str.is_valid(*it.val), true);
        atassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        atassert_eqi(str.cmp(*it.val, s$(expected6[nit])), 0);
        nit++;
    }
    atassert_eqi(nit, 3);
    return NULL;
}

ATEST_F(test_find)
{

    str_c s = str.cstr("123456");
    atassert_eqi(s.len, 6);

    // match first
    atassert_eqi(0, str.find(s, str.cstr("1"), 0, 0));

    // match full
    atassert_eqi(0, str.find(s, str.cstr("123456"), 0, 0));

    // needle overflow s
    atassert_eqi(-1, str.find(s, str.cstr("1234567"), 0, 0));

    // match
    atassert_eqi(1, str.find(s, str.cstr("23"), 0, 0));

    // empty needle
    atassert_eqi(-1, str.find(s, str.cstr(""), 0, 0));

    // empty string
    atassert_eqi(-1, str.find(str.cstr(""), str.cstr("23"), 0, 0));

    // bad string
    atassert_eqi(-1, str.find(str.cstr(NULL), str.cstr("23"), 0, 0));

    // starts after match
    atassert_eqi(-1, str.find(s, str.cstr("23"), 2, 0));

    // bad needle
    atassert_eqi(-1, str.find(s, str.cstr(NULL), 0, 0));

    // no match
    atassert_eqi(-1, str.find(s, str.cstr("foo"), 0, 0));

    // match at the end
    atassert_eqi(4, str.find(s, str.cstr("56"), 0, 0));
    atassert_eqi(5, str.find(s, str.cstr("6"), 0, 0));

    // middle
    atassert_eqi(3, str.find(s, str.cstr("45"), 0, 0));

    // ends before match
    atassert_eqi(-1, str.find(s, str.cstr("56"), 0, 5));

    // ends more than length ok
    atassert_eqi(4, str.find(s, str.cstr("56"), 0, 500));

    // start >= len
    atassert_eqi(-1, str.find(s, str.cstr("1"), 6, 5));
    atassert_eqi(-1, str.find(s, str.cstr("1"), 7, 5));

    // starts from left
    atassert_eqi(0, str.find(s$("123123123"), str.cstr("123"), 0, 0));

    return NULL;
}

ATEST_F(test_rfind)
{

    str_c s = str.cstr("123456");
    atassert_eqi(s.len, 6);

    // match first
    atassert_eqi(0, str.rfind(s, str.cstr("1"), 0, 0));

    // match last
    atassert_eqi(5, str.rfind(s, str.cstr("6"), 0, 0));

    // match full
    atassert_eqi(0, str.rfind(s, str.cstr("123456"), 0, 0));

    // needle overflow s
    atassert_eqi(-1, str.rfind(s, str.cstr("1234567"), 0, 0));

    // match
    atassert_eqi(1, str.rfind(s, str.cstr("23"), 0, 0));

    // empty needle
    atassert_eqi(-1, str.rfind(s, str.cstr(""), 0, 0));

    // empty string
    atassert_eqi(-1, str.rfind(str.cstr(""), str.cstr("23"), 0, 0));

    // bad string
    atassert_eqi(-1, str.rfind(str.cstr(NULL), str.cstr("23"), 0, 0));

    // starts after match
    atassert_eqi(-1, str.rfind(s, str.cstr("23"), 2, 0));

    // bad needle
    atassert_eqi(-1, str.rfind(s, str.cstr(NULL), 0, 0));

    // no match
    atassert_eqi(-1, str.rfind(s, str.cstr("foo"), 0, 0));

    // match at the end
    atassert_eqi(4, str.rfind(s, str.cstr("56"), 0, 0));
    atassert_eqi(5, str.rfind(s, str.cstr("6"), 0, 0));

    // middle
    atassert_eqi(3, str.rfind(s, str.cstr("45"), 0, 0));

    // ends before match
    atassert_eqi(-1, str.rfind(s, str.cstr("56"), 0, 5));

    // ends more than length ok
    atassert_eqi(4, str.rfind(s, str.cstr("56"), 0, 500));

    // start >= len
    atassert_eqi(-1, str.rfind(s, str.cstr("1"), 6, 5));
    atassert_eqi(-1, str.rfind(s, str.cstr("1"), 7, 5));

    // starts from right
    atassert_eqi(6, str.rfind(s$("123123123"), str.cstr("123"), 0, 0));

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

ATEST_F(test_remove_prefix)
{
    str_c out;

    out = str.remove_prefix(s$("prefix_str_prefix"), s$("prefix"));
    atassert_eqi(str.cmp(out, s$("_str_prefix")), 0);

    // no exact match skipped
    out = str.remove_prefix(s$(" prefix_str_prefix"), s$("prefix"));
    atassert_eqi(str.cmp(out, s$(" prefix_str_prefix")), 0);

    // empty prefix
    out = str.remove_prefix(s$("prefix_str_prefix"), s$(""));
    atassert_eqi(str.cmp(out, s$("prefix_str_prefix")), 0);

    // bad prefix
    out = str.remove_prefix(s$("prefix_str_prefix"), str.cstr(NULL));
    atassert_eqi(str.cmp(out, s$("prefix_str_prefix")), 0);

    // no match
    out = str.remove_prefix(s$("prefix_str_prefix"), s$("prefi_"));
    atassert_eqi(str.cmp(out, s$("prefix_str_prefix")), 0);

    return NULL;
}

ATEST_F(test_remove_suffix)
{
    str_c out;

    out = str.remove_suffix(s$("suffix_str_suffix"), s$("suffix"));
    atassert_eqi(str.cmp(out, s$("suffix_str_")), 0);

    // no exact match skipped
    out = str.remove_suffix(s$("suffix_str_suffix "), s$("suffix"));
    atassert_eqi(str.cmp(out, s$("suffix_str_suffix ")), 0);

    // empty suffix
    out = str.remove_suffix(s$("suffix_str_suffix"), s$(""));
    atassert_eqi(str.cmp(out, s$("suffix_str_suffix")), 0);

    // bad suffix
    out = str.remove_suffix(s$("suffix_str_suffix"), str.cstr(NULL));
    atassert_eqi(str.cmp(out, s$("suffix_str_suffix")), 0);

    // no match
    out = str.remove_suffix(s$("suffix_str_suffix"), s$("_uffix"));
    atassert_eqi(str.cmp(out, s$("suffix_str_suffix")), 0);

    return NULL;
}

ATEST_F(test_strip)
{
    str_c s = str.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    (void)s;
    str_c out;

    // LEFT
    out = str.lstrip(str.cstr(NULL));
    atassert_eqi(out.len, 0);
    atassert(out.buf == NULL);

    out = str.lstrip(str.cstr(""));
    atassert_eqi(out.len, 0);
    atassert_eqs(out.buf, "");

    out = str.lstrip(s);
    atassert_eqi(out.len, 14);
    atassert_eqs(out.buf, "123\n\t\r 456 \r\n\t");

    s = str.cstr("\n\t \r\r\n\t");
    out = str.lstrip(s);
    atassert_eqi(out.len, 0);
    atassert_eqs("", out.buf);


    // RIGHT
    out = str.rstrip(str.cstr(NULL));
    atassert_eqi(out.len, 0);
    atassert(out.buf == NULL);

    out = str.rstrip(str.cstr(""));
    atassert_eqi(out.len, 0);
    atassert_eqs(out.buf, "");

    s = str.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.rstrip(s);
    atassert_eqi(out.len, 14);
    atassert_eqi(memcmp(out.buf, "\n\t \r123\n\t\r 456", out.len), 0);

    s = str.cstr("\n\t \r\r\n\t");
    out = str.rstrip(s);
    atassert_eqi(out.len, 0);
    atassert_eqi(str.cmp(out, s$("")), 0);

    // BOTH
    out = str.strip(str.cstr(NULL));
    atassert_eqi(out.len, 0);
    atassert(out.buf == NULL);

    out = str.strip(str.cstr(""));
    atassert_eqi(out.len, 0);
    atassert_eqs(out.buf, "");

    s = str.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.strip(s);
    atassert_eqi(out.len, 10);
    atassert_eqi(memcmp(out.buf, "123\n\t\r 456", out.len), 0);

    s = str.cstr("\n\t \r\r\n\t");
    out = str.strip(s);
    atassert_eqi(out.len, 0);
    atassert_eqs("", out.buf);
    return NULL;
}

ATEST_F(test_cmp)
{

    atassert_eqi(str.cmp(str.cstr("123456"), str.cstr("123456")), 0);
    atassert_eqi(str.cmp(str.cstr(NULL), str.cstr(NULL)), 0);
    atassert_eqi(str.cmp(str.cstr(""), str.cstr("")), 0);
    atassert_eqi(str.cmp(str.cstr("ABC"), str.cstr("AB")), 67);
    atassert_eqi(str.cmp(str.cstr("ABA"), str.cstr("ABZ")), -25);
    atassert_eqi(str.cmp(str.cstr("AB"), str.cstr("ABC")), -67);
    atassert_eqi(str.cmp(str.cstr("A"), str.cstr("")), (int)'A');
    atassert_eqi(str.cmp(str.cstr(""), str.cstr("A")), -1*((int)'A'));
    atassert_eqi(str.cmp(str.cstr(""), str.cstr(NULL)), 1);
    atassert_eqi(str.cmp(str.cstr(NULL), str.cstr("ABC")), -1);

    return NULL;
}

ATEST_F(test_cmpi)
{

    atassert_eqi(str.cmpi(str.cstr("123456"), str.cstr("123456")), 0);
    atassert_eqi(str.cmpi(str.cstr(NULL), str.cstr(NULL)), 0);
    atassert_eqi(str.cmpi(str.cstr(""), str.cstr("")), 0);

    atassert_eqi(str.cmpi(str.cstr("ABC"), str.cstr("ABC")), 0);
    atassert_eqi(str.cmpi(str.cstr("abc"), str.cstr("ABC")), 0);
    atassert_eqi(str.cmpi(str.cstr("ABc"), str.cstr("ABC")), 0);
    atassert_eqi(str.cmpi(str.cstr("ABC"), str.cstr("aBC")), 0);

    atassert_eqi(str.cmpi(str.cstr("ABC"), str.cstr("AB")), 67);
    atassert_eqi(str.cmpi(str.cstr("ABA"), str.cstr("ABZ")), -25);
    atassert_eqi(str.cmpi(str.cstr("AB"), str.cstr("ABC")), -67);
    atassert_eqi(str.cmpi(str.cstr("A"), str.cstr("")), (int)'A');
    atassert_eqi(str.cmpi(str.cstr(""), str.cstr("A")), -1*((int)'A'));
    atassert_eqi(str.cmpi(str.cstr(""), str.cstr(NULL)), 1);
    atassert_eqi(str.cmpi(str.cstr(NULL), str.cstr("ABC")), -1);


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
    ATEST_RUN(test_cstr_sdollar);
    ATEST_RUN(test_copy);
    ATEST_RUN(test_sub_positive_start);
    ATEST_RUN(test_sub_negative_start);
    ATEST_RUN(test_iter);
    ATEST_RUN(test_iter_split);
    ATEST_RUN(test_find);
    ATEST_RUN(test_rfind);
    ATEST_RUN(test_contains_starts_ends);
    ATEST_RUN(test_remove_prefix);
    ATEST_RUN(test_remove_suffix);
    ATEST_RUN(test_strip);
    ATEST_RUN(test_cmp);
    ATEST_RUN(test_cmpi);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
