#include <_cexcore/_stb_sprintf.c>
#include <_cexcore/allocators.c>
#include <_cexcore/cex.c>
#include <_cexcore/cextest.h>
#include <_cexcore/list.h>
#include <_cexcore/sbuf.c>
#include <_cexcore/str.c>
#include <stdio.h>

const Allocator_i* allocator;
/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    allocator = allocators.heap.destroy();
    return EOK;
}

test$setup()
{
    uassert_enable();
    allocator = allocators.heap.create();
    return EOK;
}

/*
 *
 *   TEST SUITE
 *
 */

test$case(test_cstr)
{
    const char* cstr = "hello";


    str_c s = str.cstr(cstr);
    tassert_eqs(s.buf, cstr);
    tassert_eqi(s.len, 5); // lazy init until str.length() is called
    tassert(s.buf == cstr);
    tassert_eqi(str.len(s), 5);
    tassert_eqi(str.is_valid(s), true);


    tassert_eqi(s.len, 5); // now s.len is set

    str_c snull = str.cstr(NULL);
    tassert_eqs(snull.buf, NULL);
    tassert_eqi(snull.len, 0);
    tassert_eqi(str.len(snull), 0);
    tassert_eqi(str.is_valid(snull), false);

    str_c sempty = str.cstr("");
    tassert_eqs(sempty.buf, "");
    tassert_eqi(sempty.len, 0);
    tassert_eqi(str.len(sempty), 0);
    tassert_eqi(sempty.len, 0);
    tassert_eqi(str.is_valid(sempty), true);
    return EOK;
}

test$case(test_cstr_sdollar)
{
    const char* cstr = "hello";

    sbuf_c sb = NULL;
    tassert_eqs(EOK, sbuf.create(&sb, 10, allocator));
    tassert_eqs(EOK, sbuf.append(&sb, s$("hello")));
    str_c sbv = s$(sb);
    tassert_eqs(sbv.buf, cstr);
    tassert_eqi(sbv.len, 5); // lazy init until str.length() is called
    sbuf.destroy(&sb);

    // auto-type also works!
    var s = s$(cstr);
    tassert_eqs(s.buf, cstr);
    tassert_eqi(s.len, 5); // lazy init until str.length() is called
    tassert(s.buf == cstr);
    tassert_eqi(str.len(s), 5);
    tassert_eqi(str.is_valid(s), true);


    str_c s2 = s$(s);
    tassert_eqs(s2.buf, cstr);
    tassert_eqi(s2.len, 5); // lazy init until str.length() is called
    tassert(s2.buf == cstr);
    tassert_eqi(str.len(s2), 5);
    tassert_eqi(str.is_valid(s2), true);

    s2 = s$("hello");
    tassert_eqs(s2.buf, cstr);
    tassert_eqi(s2.len, 5); // lazy init until str.length() is called
    tassert(s2.buf == cstr);
    tassert_eqi(str.len(s2), 5);
    tassert_eqi(str.is_valid(s2), true);

    tassert_eqi(s.len, 5); // now s.len is set

    char* str_null = NULL;
    str_c snull = s$(str_null);
    tassert_eqs(snull.buf, NULL);
    tassert_eqi(snull.len, 0);
    tassert_eqi(str.len(snull), 0);
    tassert_eqi(str.is_valid(snull), false);

    str_c sempty = s$("");
    tassert_eqs(sempty.buf, "");
    tassert_eqi(sempty.len, 0);
    tassert_eqi(str.len(sempty), 0);
    tassert_eqi(sempty.len, 0);
    tassert_eqi(str.is_valid(sempty), true);

    // WARNING: buffers for s$() are extremely bad idea, they may not be null term!
    char buf[30] = { 0 };
    s = s$(buf);
    tassert_eqs(s.buf, buf);
    tassert_eqi(s.len, 0); // lazy init until str.length() is called
    tassert_eqi(str.len(s), 0);
    tassert_eqi(str.is_valid(s), true);


    memset(buf, 'z', arr$len(buf));

    // WARNING: if no null term
    // s = s$(buf); // !!!! STACK OVERFLOW, no null term!
    //
    // Consider using str.cbuf() - it limits length by buffer size
    s = str.cbuf(buf, arr$len(buf));
    tassert_eqi(s.len, 30);
    tassert_eqi(str.len(s), 30);
    tassert_eqi(str.is_valid(s), true);
    tassert(s.buf == buf);
    return EOK;
}

test$case(test_copy)
{
    char buf[8];
    memset(buf, 'a', arr$len(buf));

    str_c s = str.cstr("1234567");
    tassert_eqi(s.len, 7);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(EOK, str.copy(s, buf, arr$len(buf)));
    tassert_eqs("1234567", buf);
    tassert_eqi(s.len, 7);
    tassert_eqi(s.buf[7], '\0');

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.argument, str.copy(s, NULL, arr$len(buf)));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);
    tassert_eqs(Error.argument, str.copy(s, buf, 0)); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.ok, str.copy(s$(""), buf, arr$len(buf)));
    tassert_eqs("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.argument, str.copy(str.cstr(NULL), buf, arr$len(buf)));
    // buffer reset to "" string
    tassert_eqs("", buf);

    str_c sbig = str.cstr("12345678");
    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.overflow, str.copy(sbig, buf, arr$len(buf)));
    // string is truncated
    tassert_eqs("1234567", buf);

    str_c ssmall = str.cstr("1234");
    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.ok, str.copy(ssmall, buf, arr$len(buf)));
    tassert_eqs("1234", buf);
    return EOK;
}

test$case(test_sub_positive_start)
{

    str_c sub;
    str_c s = str.cstr("123456");
    tassert_eqi(s.len, 6);
    tassert_eqi(str.is_valid(s), true);

    sub = str.sub(s, 0, s.len);
    tassert_eqi(str.is_valid(sub), true);
    tassert(memcmp(sub.buf, "123456", s.len) == 0);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);


    sub = str.sub(s, 1, 3);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 2);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "23", sub.len) == 0);

    sub = str.sub(s, 1, 2);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 1);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "2", sub.len) == 0);

    sub = str.sub(s, s.len - 1, s.len);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 1);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "6", sub.len) == 0);

    sub = str.sub(s, 2, 0);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 4);
    tassert(memcmp(sub.buf, "3456", sub.len) == 0);

    sub = str.sub(s, 0, s.len + 1);
    tassert_eqi(str.is_valid(sub), false);

    sub = str.sub(s, 2, 1);
    tassert_eqi(str.is_valid(sub), false);

    sub = str.sub(s, s.len, s.len + 1);
    tassert_eqi(str.is_valid(sub), false);

    s = str.cstr(NULL);
    sub = str.sub(s, 1, 2);
    tassert_eqi(str.is_valid(sub), false);

    s = str.cstr("");
    tassert_eqi(str.is_valid(s), true);

    sub = str.sub(s, 0, 0);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 0);
    tassert_eqs("", sub.buf);

    sub = str.sub(s, 1, 2);
    tassert_eqi(str.is_valid(sub), false);

    s = str.cstr("A");
    sub = str.sub(s, 1, 2);
    tassert_eqi(str.is_valid(sub), false);

    s = str.cstr("A");
    sub = str.sub(s, 0, 2);
    tassert_eqi(str.is_valid(sub), false);

    s = str.cstr("A");
    sub = str.sub(s, 0, 0);
    tassert_eqi(str.is_valid(sub), true);
    tassert(memcmp(sub.buf, "A", sub.len) == 0);
    tassert_eqi(sub.len, 1);
    return EOK;
}

test$case(test_sub_negative_start)
{

    str_c s = str.cstr("123456");
    tassert_eqi(s.len, 6);
    tassert_eqi(str.is_valid(s), true);

    var sub = str.sub(s, -3, -1);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 2);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = str.sub(s, -6, 0);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);


    sub = str.sub(s, -3, -2);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 1);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "4", sub.len) == 0);

    sub = str.sub(s, -3, -1);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 2);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = str.sub(s, -3, -400);
    tassert_eqi(str.is_valid(sub), false);

    s = str.cstr("");
    sub = str.sub(s, 0, 0);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 0);

    s = str.cstr("");
    sub = str.sub(s, -1, 0);
    tassert_eqi(str.is_valid(sub), false);

    s = str.cstr("123456");
    sub = str.sub(s, -6, 0);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.cstr("123456");
    sub = str.sub(s, -6, 6);
    tassert_eqi(str.is_valid(sub), true);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.cstr("123456");
    sub = str.sub(s, -7, 0);
    tassert_eqi(str.is_valid(sub), false);

    return EOK;
}

test$case(test_iter)
{

    str_c s = str.cstr("123456");
    tassert_eqi(s.len, 6);
    u32 nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        tassert_eqi(*it.val, s.buf[it.idx.i]);
        tassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    tassert_eqi(s.len, nit);
    tassert_eqi(str.is_valid(s), true);

    s = str.cstr(NULL);
    nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        nit++;
    }
    tassert_eqi(0, nit);

    s = str.cstr("");
    nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        nit++;
    }
    tassert_eqi(0, nit);

    s = str.cstr("123456");
    s = str.sub(s, 2, 5);
    tassert(memcmp(s.buf, "345", s.len) == 0);
    tassert_eqi(s.len, 3);

    nit = 0;
    for$iter(char, it, str.iter(s, &it.iterator))
    {
        // printf("%ld: %c\n", it.idx.i, *it.val);
        tassert_eqi(*it.val, s.buf[it.idx.i]);
        tassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    tassert_eqi(s.len, nit);
    tassert_eqi(str.is_valid(s), true);

    nit = 0;
    for$array(it, s.buf, s.len)
    {
        // printf("%ld: %c\n", it.idx.i, *it.val);
        tassert_eqi(*it.val, s.buf[it.idx]);
        tassert_eqi(*it.val, s.buf[nit]);
        nit++;
    }
    tassert_eqi(s.len, nit);
    return EOK;
}

test$case(test_iter_split)
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
        tassert_eqi(str.is_valid(*it.val), true);
        tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.cmp(*it.val, s$(expected1[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 1);

    nit = 0;
    s = str.cstr("123,456");
    const char* expected2[] = {
        "123",
        "456",
    };
    for$iter(str_c, it, str.iter_split(s, ",", &it.iterator))
    {
        tassert_eqi(str.is_valid(*it.val), true);
        tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.cmp(*it.val, s$(expected2[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 2);

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
        tassert_eqi(str.is_valid(*it.val), true);
        tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.cmp(*it.val, s$(expected3[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 4);

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
        tassert_eqi(str.is_valid(*it.val), true);
        tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.cmp(*it.val, s$(expected4[nit])), 0);
        tassert_eqi(it.idx.i, nit);
        nit++;
    }
    tassert_eqi(nit, 4);

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
        tassert_eqi(str.is_valid(*it.val), true);
        tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.cmp(*it.val, s$(expected5[nit])), 0);
        tassert_eqi(it.idx.i, nit);
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    const char* expected6[] = {
        "123",
        "456",
        "",
    };
    s = str.cstr("123\n456\n");
    for$iter(str_c, it, str.iter_split(s, "\n", &it.iterator))
    {
        tassert_eqi(str.is_valid(*it.val), true);
        tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.cmp(*it.val, s$(expected6[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 3);
    return EOK;
}

test$case(test_find)
{

    str_c s = str.cstr("123456");
    tassert_eqi(s.len, 6);

    // match first
    tassert_eqi(0, str.find(s, str.cstr("1"), 0, 0));

    // match full
    tassert_eqi(0, str.find(s, str.cstr("123456"), 0, 0));

    // needle overflow s
    tassert_eqi(-1, str.find(s, str.cstr("1234567"), 0, 0));

    // match
    tassert_eqi(1, str.find(s, str.cstr("23"), 0, 0));

    // empty needle
    tassert_eqi(-1, str.find(s, str.cstr(""), 0, 0));

    // empty string
    tassert_eqi(-1, str.find(str.cstr(""), str.cstr("23"), 0, 0));

    // bad string
    tassert_eqi(-1, str.find(str.cstr(NULL), str.cstr("23"), 0, 0));

    // starts after match
    tassert_eqi(-1, str.find(s, str.cstr("23"), 2, 0));

    // bad needle
    tassert_eqi(-1, str.find(s, str.cstr(NULL), 0, 0));

    // no match
    tassert_eqi(-1, str.find(s, str.cstr("foo"), 0, 0));

    // match at the end
    tassert_eqi(4, str.find(s, str.cstr("56"), 0, 0));
    tassert_eqi(5, str.find(s, str.cstr("6"), 0, 0));

    // middle
    tassert_eqi(3, str.find(s, str.cstr("45"), 0, 0));

    // ends before match
    tassert_eqi(-1, str.find(s, str.cstr("56"), 0, 5));

    // ends more than length ok
    tassert_eqi(4, str.find(s, str.cstr("56"), 0, 500));

    // start >= len
    tassert_eqi(-1, str.find(s, str.cstr("1"), 6, 5));
    tassert_eqi(-1, str.find(s, str.cstr("1"), 7, 5));

    // starts from left
    tassert_eqi(0, str.find(s$("123123123"), str.cstr("123"), 0, 0));

    return EOK;
}

test$case(test_rfind)
{

    str_c s = str.cstr("123456");
    tassert_eqi(s.len, 6);

    // match first
    tassert_eqi(0, str.rfind(s, str.cstr("1"), 0, 0));

    // match last
    tassert_eqi(5, str.rfind(s, str.cstr("6"), 0, 0));

    // match full
    tassert_eqi(0, str.rfind(s, str.cstr("123456"), 0, 0));

    // needle overflow s
    tassert_eqi(-1, str.rfind(s, str.cstr("1234567"), 0, 0));

    // match
    tassert_eqi(1, str.rfind(s, str.cstr("23"), 0, 0));

    // empty needle
    tassert_eqi(-1, str.rfind(s, str.cstr(""), 0, 0));

    // empty string
    tassert_eqi(-1, str.rfind(str.cstr(""), str.cstr("23"), 0, 0));

    // bad string
    tassert_eqi(-1, str.rfind(str.cstr(NULL), str.cstr("23"), 0, 0));

    // starts after match
    tassert_eqi(-1, str.rfind(s, str.cstr("23"), 2, 0));

    // bad needle
    tassert_eqi(-1, str.rfind(s, str.cstr(NULL), 0, 0));

    // no match
    tassert_eqi(-1, str.rfind(s, str.cstr("foo"), 0, 0));

    // match at the end
    tassert_eqi(4, str.rfind(s, str.cstr("56"), 0, 0));
    tassert_eqi(5, str.rfind(s, str.cstr("6"), 0, 0));

    // middle
    tassert_eqi(3, str.rfind(s, str.cstr("45"), 0, 0));

    // ends before match
    tassert_eqi(-1, str.rfind(s, str.cstr("56"), 0, 5));

    // ends more than length ok
    tassert_eqi(4, str.rfind(s, str.cstr("56"), 0, 500));

    // start >= len
    tassert_eqi(-1, str.rfind(s, str.cstr("1"), 6, 5));
    tassert_eqi(-1, str.rfind(s, str.cstr("1"), 7, 5));

    // starts from right
    tassert_eqi(6, str.rfind(s$("123123123"), str.cstr("123"), 0, 0));

    return EOK;
}

test$case(test_contains_starts_ends)
{
    str_c s = str.cstr("123456");
    tassert_eqi(1, str.contains(s, str.cstr("1")));
    tassert_eqi(1, str.contains(s, str.cstr("123456")));
    tassert_eqi(0, str.contains(s, str.cstr("1234567")));
    tassert_eqi(0, str.contains(s, str.cstr("foo")));
    tassert_eqi(0, str.contains(s, str.cstr("")));
    tassert_eqi(0, str.contains(s, str.cstr(NULL)));
    tassert_eqi(0, str.contains(str.cstr(""), str.cstr("1")));
    tassert_eqi(0, str.contains(str.cstr(NULL), str.cstr("1")));

    tassert_eqi(1, str.starts_with(s, str.cstr("1")));
    tassert_eqi(1, str.starts_with(s, str.cstr("1234")));
    tassert_eqi(0, str.starts_with(s, str.cstr("56")));
    tassert_eqi(0, str.starts_with(s, str.cstr("")));
    tassert_eqi(0, str.starts_with(s, str.cstr(NULL)));
    tassert_eqi(0, str.starts_with(str.cstr(""), str.cstr("1")));
    tassert_eqi(0, str.starts_with(str.cstr(NULL), str.cstr("1")));


    tassert_eqi(1, str.ends_with(s, str.cstr("6")));
    tassert_eqi(1, str.ends_with(s, str.cstr("123456")));
    tassert_eqi(1, str.ends_with(s, str.cstr("56")));
    tassert_eqi(0, str.ends_with(s, str.cstr("1234567")));
    tassert_eqi(0, str.ends_with(s, str.cstr("5")));
    tassert_eqi(0, str.ends_with(s, str.cstr("")));
    tassert_eqi(0, str.ends_with(s, str.cstr(NULL)));
    tassert_eqi(0, str.ends_with(str.cstr(""), str.cstr("1")));
    tassert_eqi(0, str.ends_with(str.cstr(NULL), str.cstr("1")));

    return EOK;
}

test$case(test_remove_prefix)
{
    str_c out;

    out = str.remove_prefix(s$("prefix_str_prefix"), s$("prefix"));
    tassert_eqi(str.cmp(out, s$("_str_prefix")), 0);

    // no exact match skipped
    out = str.remove_prefix(s$(" prefix_str_prefix"), s$("prefix"));
    tassert_eqi(str.cmp(out, s$(" prefix_str_prefix")), 0);

    // empty prefix
    out = str.remove_prefix(s$("prefix_str_prefix"), s$(""));
    tassert_eqi(str.cmp(out, s$("prefix_str_prefix")), 0);

    // bad prefix
    out = str.remove_prefix(s$("prefix_str_prefix"), str.cstr(NULL));
    tassert_eqi(str.cmp(out, s$("prefix_str_prefix")), 0);

    // no match
    out = str.remove_prefix(s$("prefix_str_prefix"), s$("prefi_"));
    tassert_eqi(str.cmp(out, s$("prefix_str_prefix")), 0);

    return EOK;
}

test$case(test_remove_suffix)
{
    str_c out;

    out = str.remove_suffix(s$("suffix_str_suffix"), s$("suffix"));
    tassert_eqi(str.cmp(out, s$("suffix_str_")), 0);

    // no exact match skipped
    out = str.remove_suffix(s$("suffix_str_suffix "), s$("suffix"));
    tassert_eqi(str.cmp(out, s$("suffix_str_suffix ")), 0);

    // empty suffix
    out = str.remove_suffix(s$("suffix_str_suffix"), s$(""));
    tassert_eqi(str.cmp(out, s$("suffix_str_suffix")), 0);

    // bad suffix
    out = str.remove_suffix(s$("suffix_str_suffix"), str.cstr(NULL));
    tassert_eqi(str.cmp(out, s$("suffix_str_suffix")), 0);

    // no match
    out = str.remove_suffix(s$("suffix_str_suffix"), s$("_uffix"));
    tassert_eqi(str.cmp(out, s$("suffix_str_suffix")), 0);

    return EOK;
}

test$case(test_strip)
{
    str_c s = str.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    (void)s;
    str_c out;

    // LEFT
    out = str.lstrip(str.cstr(NULL));
    tassert_eqi(out.len, 0);
    tassert(out.buf == NULL);

    out = str.lstrip(str.cstr(""));
    tassert_eqi(out.len, 0);
    tassert_eqs(out.buf, "");

    out = str.lstrip(s);
    tassert_eqi(out.len, 14);
    tassert_eqs(out.buf, "123\n\t\r 456 \r\n\t");

    s = str.cstr("\n\t \r\r\n\t");
    out = str.lstrip(s);
    tassert_eqi(out.len, 0);
    tassert_eqs("", out.buf);


    // RIGHT
    out = str.rstrip(str.cstr(NULL));
    tassert_eqi(out.len, 0);
    tassert(out.buf == NULL);

    out = str.rstrip(str.cstr(""));
    tassert_eqi(out.len, 0);
    tassert_eqs(out.buf, "");

    s = str.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.rstrip(s);
    tassert_eqi(out.len, 14);
    tassert_eqi(memcmp(out.buf, "\n\t \r123\n\t\r 456", out.len), 0);

    s = str.cstr("\n\t \r\r\n\t");
    out = str.rstrip(s);
    tassert_eqi(out.len, 0);
    tassert_eqi(str.cmp(out, s$("")), 0);

    // BOTH
    out = str.strip(str.cstr(NULL));
    tassert_eqi(out.len, 0);
    tassert(out.buf == NULL);

    out = str.strip(str.cstr(""));
    tassert_eqi(out.len, 0);
    tassert_eqs(out.buf, "");

    s = str.cstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.strip(s);
    tassert_eqi(out.len, 10);
    tassert_eqi(memcmp(out.buf, "123\n\t\r 456", out.len), 0);

    s = str.cstr("\n\t \r\r\n\t");
    out = str.strip(s);
    tassert_eqi(out.len, 0);
    tassert_eqs("", out.buf);
    return EOK;
}

test$case(test_cmp)
{

    tassert_eqi(str.cmp(str.cstr("123456"), str.cstr("123456")), 0);
    tassert_eqi(str.cmp(str.cstr(NULL), str.cstr(NULL)), 0);
    tassert_eqi(str.cmp(str.cstr(""), str.cstr("")), 0);
    tassert_eqi(str.cmp(str.cstr("ABC"), str.cstr("AB")), 67);
#ifdef CEX_ENV32BIT
    tassert(str.cmp(str.cstr("ABA"), str.cstr("ABZ")) < 0);
#else
    tassert_eqi(str.cmp(str.cstr("ABA"), str.cstr("ABZ")), -25);
#endif
    tassert_eqi(str.cmp(str.cstr("AB"), str.cstr("ABC")), -67);
    tassert_eqi(str.cmp(str.cstr("A"), str.cstr("")), (int)'A');
    tassert_eqi(str.cmp(str.cstr(""), str.cstr("A")), -1 * ((int)'A'));
    tassert_eqi(str.cmp(str.cstr(""), str.cstr(NULL)), 1);
    tassert_eqi(str.cmp(str.cstr(NULL), str.cstr("ABC")), -1);

    return EOK;
}

test$case(test_cmpi)
{

    tassert_eqi(str.cmpi(str.cstr("123456"), str.cstr("123456")), 0);
    tassert_eqi(str.cmpi(str.cstr(NULL), str.cstr(NULL)), 0);
    tassert_eqi(str.cmpi(str.cstr(""), str.cstr("")), 0);

    tassert_eqi(str.cmpi(str.cstr("ABC"), str.cstr("ABC")), 0);
    tassert_eqi(str.cmpi(str.cstr("abc"), str.cstr("ABC")), 0);
    tassert_eqi(str.cmpi(str.cstr("ABc"), str.cstr("ABC")), 0);
    tassert_eqi(str.cmpi(str.cstr("ABC"), str.cstr("aBC")), 0);

    tassert_eqi(str.cmpi(str.cstr("ABC"), str.cstr("AB")), 67);
    tassert_eqi(str.cmpi(str.cstr("ABA"), str.cstr("ABZ")), -25);
    tassert_eqi(str.cmpi(str.cstr("AB"), str.cstr("ABC")), -67);
    tassert_eqi(str.cmpi(str.cstr("A"), str.cstr("")), (int)'A');
    tassert_eqi(str.cmpi(str.cstr(""), str.cstr("A")), -1 * ((int)'A'));
    tassert_eqi(str.cmpi(str.cstr(""), str.cstr(NULL)), 1);
    tassert_eqi(str.cmpi(str.cstr(NULL), str.cstr("ABC")), -1);


    return EOK;
}

test$case(str_to__signed_num)
{
    i64 num;
    str_c s;

    num = 0;
    s = s$("1");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 1);

    num = 0;
    s = s$(" ");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$(" -");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$(" +");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("+");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = s$("      -100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -100);

    num = 0;
    s = s$("      +100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = s$("-100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -100);

    num = 10;
    s = s$("0");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0xf");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 15);

    num = 0;
    s = s$("-0xf");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -15);


    num = 0;
    s = s$("0x0F");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 15);


    num = 0;
    s = s$("0x0A");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = s$("0x0a");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = s$("127");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, INT8_MAX);

    num = 0;
    s = s$("-127");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -127);

    num = 0;
    s = s$("-128");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, INT8_MIN);

    num = 0;
    s = s$("-129");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("128");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("-0x80");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -128);
    tassert_eqi(num, INT8_MIN);

    num = 0;
    s = s$("-0x");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = s$("0x7f");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 127);
    tassert_eqi(num, INT8_MAX);

    num = 0;
    s = s$("0x80");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = s$("-100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, -100, 100));
    tassert_eqi(num, -100);

    num = 0;
    s = s$("-101");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, -100, 100));

    num = 0;
    s = s$("101");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, -100, 100));

    num = 0;
    s = s$("-000000000127    ");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -127);

    num = 0;
    s = s$("    ");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = s$("12 2");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = s$("-000000000127a");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = s$("-000000000127h");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = s$("-9223372036854775807");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));
    tassert_eql(num, -9223372036854775807L);

    num = 0;
    s = s$("-9223372036854775808");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));

    num = 0;
    s = s$("9223372036854775807");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));
    tassert_eql(num, 9223372036854775807L);

    num = 0;
    s = s$("9223372036854775808");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));

    return EOK;
}

test$case(test_str_to_i8)
{
    i8 num;
    str_c s;

    num = 0;
    s = s$("127");
    tassert_eqe(EOK, str.to_i8(s, &num));
    tassert_eqi(num, 127);

    num = 0;
    s = s$("128");
    tassert_eqe(Error.overflow, str.to_i8(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0");
    tassert_eqe(Error.ok, str.to_i8(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("-128");
    tassert_eqe(Error.ok, str.to_i8(s, &num));
    tassert_eqi(num, -128);


    num = 0;
    s = s$("-129");
    tassert_eqe(Error.overflow, str.to_i8(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}


test$case(test_str_to_i16)
{
    i16 num;
    str_c s;

    num = 0;
    s = s$("-32768");
    tassert_eqe(EOK, str.to_i16(s, &num));
    tassert_eqi(num, -32768);

    num = 0;
    s = s$("-32769");
    tassert_eqe(Error.overflow, str.to_i16(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("32767");
    tassert_eqe(Error.ok, str.to_i16(s, &num));
    tassert_eqi(num, 32767);

    num = 0;
    s = s$("32768");
    tassert_eqe(Error.overflow, str.to_i16(s, &num));
    tassert_eqi(num, 0);

    return EOK;
}

test$case(test_str_to_i32)
{
    i32 num;
    str_c s;

    num = 0;
    s = s$("-2147483648");
    tassert_eqe(EOK, str.to_i32(s, &num));
    tassert_eqi(num, -2147483648);

    num = 0;
    s = s$("-2147483649");
    tassert_eqe(Error.overflow, str.to_i32(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("2147483647");
    tassert_eqe(Error.ok, str.to_i32(s, &num));
    tassert_eqi(num, 2147483647);


    num = 0;
    s = s$("2147483648");
    tassert_eqe(Error.overflow, str.to_i32(s, &num));
    tassert_eqi(num, 0);

    return EOK;
}

test$case(test_str_to_i64)
{
    i64 num;
    str_c s;

    num = 0;
    s = s$("-9223372036854775807");
    tassert_eqe(Error.ok, str.to_i64(s, &num));
    tassert_eql(num, -9223372036854775807);

    num = 0;
    s = s$("-9223372036854775808");
    tassert_eqe(Error.overflow, str.to_i64(s, &num));
    // tassert_eql(num, -9223372036854775808UL);

    num = 0;
    s = s$("9223372036854775807");
    tassert_eqe(Error.ok, str.to_i64(s, &num));
    tassert_eql(num, 9223372036854775807);

    num = 0;
    s = s$("9223372036854775808");
    tassert_eqe(Error.overflow, str.to_i64(s, &num));
    // tassert_eql(num, 9223372036854775807);

    return EOK;
}

test$case(str_to__unsigned_num)
{
    u64 num;
    str_c s;

    num = 0;
    s = s$("1");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 1);

    num = 0;
    s = s$(" ");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$(" -");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$(" +");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("+");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("100");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = s$("      -100");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("      +100");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = s$("-100");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 10;
    s = s$("0");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0xf");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 15);

    num = 0;
    s = s$("0x");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("-0xf");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));


    num = 0;
    s = s$("0x0F");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 15);


    num = 0;
    s = s$("0x0A");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = s$("0x0a");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = s$("127");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, INT8_MAX);

    num = 0;
    s = s$("-127");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("-128");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("255");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 255);

    num = 0;
    s = s$("256");
    tassert_eqe(Error.overflow, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0xff");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 255);
    tassert_eqi(num, UINT8_MAX);


    num = 0;
    s = s$("100");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, 100));
    tassert_eqi(num, 100);

    num = 0;
    s = s$("101");
    tassert_eqe(Error.overflow, str__to_unsigned_num(s, &num, 100));


    num = 0;
    s = s$("000000000127    ");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 127);

    num = 0;
    s = s$("    ");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("12 2");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("000000000127a");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("000000000127h");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = s$("9223372036854775807");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT64_MAX));
    tassert_eql(num, 9223372036854775807L);

    num = 0;
    s = s$("9223372036854775807");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT64_MAX));
    tassert_eql(num, 9223372036854775807L);

    num = 0;
    s = s$("18446744073709551615");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT64_MAX));
    tassert(num == __UINT64_C(18446744073709551615));

    num = 0;
    s = s$("18446744073709551616");
    tassert_eqe(Error.overflow, str__to_unsigned_num(s, &num, UINT64_MAX));

    return EOK;
}


test$case(test_str_to_u8)
{
    u8 num;
    str_c s;

    num = 0;
    s = s$("255");
    tassert_eqe(EOK, str.to_u8(s, &num));
    tassert_eqi(num, 255);
    tassert(num == UINT8_MAX);

    num = 0;
    s = s$("256");
    tassert_eqe(Error.overflow, str.to_u8(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0");
    tassert_eqe(Error.ok, str.to_u8(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}


test$case(test_str_to_u16)
{
    u16 num;
    str_c s;

    num = 0;
    s = s$("65535");
    tassert_eqe(EOK, str.to_u16(s, &num));
    tassert_eqi(num, 65535);
    tassert(num == UINT16_MAX);

    num = 0;
    s = s$("65536");
    tassert_eqe(Error.overflow, str.to_u16(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0");
    tassert_eqe(Error.ok, str.to_u16(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}


test$case(test_str_to_u32)
{
    u32 num;
    str_c s;

    num = 0;
    s = s$("4294967295");
    tassert_eqe(EOK, str.to_u32(s, &num));
    tassert_eql(num, 4294967295U);
    tassert(num == UINT32_MAX);

    num = 0;
    s = s$("4294967296");
    tassert_eqe(Error.overflow, str.to_u32(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0");
    tassert_eqe(Error.ok, str.to_u32(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}

test$case(test_str_to_u64)
{
    u64 num;
    str_c s;

    num = 0;
    s = s$("18446744073709551615");
    tassert_eqe(EOK, str.to_u64(s, &num));
    tassert(num ==__UINT64_C(18446744073709551615));
    tassert(num == UINT64_MAX);

    num = 0;
    s = s$("18446744073709551616");
    tassert_eqe(Error.overflow, str.to_u64(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = s$("0");
    tassert_eqe(Error.ok, str.to_u64(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}
test$case(str_to__double)
{
    double num;
    str_c s;

    num = 0;
    s = s$("1");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1.0);

    num = 0;
    s = s$("1.5");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1.5);

    num = 0;
    s = s$("-1.5");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -1.5);

    num = 0;
    s = s$("1e3");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1000);

    num = 0;
    s = s$("1e-3");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1e-3);

    num = 0;
    s = s$("123e-30");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 123e-30);

    num = 0;
    s = s$("123e+30");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 123e+30);

    num = 0;
    s = s$("123.321E+30");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 123.321e+30);

    num = 0;
    s = s$("-0.");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -0.0);

    num = 0;
    s = s$("+.5");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.5);

    num = 0;
    s = s$(".0e10");
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.0);

    num = 0;
    s = s$(".");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.0);

    num = 0;
    s = s$(".e10");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.0);

    num = 0;
    s = s$("00000000001.e00000010");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 10000000000.0);

    num = 0;
    s = s$("e10");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("10e");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("10e0.3");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("10a");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("10.0a");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("10.0e-a");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("      10.5     ");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 10.5);

    num = 0;
    s = s$("      10.5     z");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("n");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("na");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("nan");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, NAN);

    num = 0;
    s = s$("   NAN    ");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, NAN);

    num = 0;
    s = s$("   NaN    ");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, NAN);

    num = 0;
    s = s$("   nan    y");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("   nanny");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("inf");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, INFINITY);

    num = 0;
    s = s$("INF");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, INFINITY);
    tassert(isinf(num));

    num = 0;
    s = s$("-iNf");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -INFINITY);
    tassert(isinf(num));

    num = 0;
    s = s$("-infinity");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -INFINITY);
    tassert(isinf(num));

    num = 0;
    s = s$("   INFINITY   ");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, INFINITY);
    tassert(isinf(num));

    num = 0;
    s = s$("INFINITY0");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("INFO");
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("1e100");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("1e101");
    tassert_eqe(Error.overflow, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("1e-100");
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));

    num = 0;
    s = s$("1e-101");
    tassert_eqe(Error.overflow, str__to_double(s, &num, -100, 100));

    return EOK;
}
test$case(test_str_to_f32)
{
    f32 num;
    str_c s;

    num = 0;
    s = s$("1.4");
    tassert_eqe(EOK, str.to_f32(s, &num));
    tassert_eqf(num, 1.4f);


    num = 0;
    s = s$("nan");
    tassert_eqe(EOK, str.to_f32(s, &num));
    tassert_eqf(num, NAN);

    num = 0;
    s = s$("1e38");
    tassert_eqe(EOK, str.to_f32(s, &num));

    num = 0;
    s = s$("1e39");
    tassert_eqe(Error.overflow, str.to_f32(s, &num));

    
    num = 0;
    s = s$("1e-37");
    tassert_eqe(EOK, str.to_f32(s, &num));

    num = 0;
    s = s$("1e-38");
    tassert_eqe(Error.overflow, str.to_f32(s, &num));
    
    
    return EOK;
}

test$case(test_str_to_f64)
{
    f64 num;
    str_c s;

    num = 0;
    s = s$("1.4");
    tassert_eqe(EOK, str.to_f64(s, &num));
    tassert_eqf(num, 1.4);


    num = 0;
    s = s$("nan");
    tassert_eqe(EOK, str.to_f64(s, &num));
    tassert_eqf(num, NAN);

    num = 0;
    s = s$("1e308");
    tassert_eqe(EOK, str.to_f64(s, &num));

    num = 0;
    s = s$("1e309");
    tassert_eqe(Error.overflow, str.to_f64(s, &num));

    
    num = 0;
    s = s$("1e-307");
    tassert_eqe(EOK, str.to_f64(s, &num));

    num = 0;
    s = s$("1e-308");
    tassert_eqe(Error.overflow, str.to_f64(s, &num));
    
    
    return EOK;
}

test$case(test_str_sprintf)
{
    char buffer[10] = {0};

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(EOK, str.sprintf(buffer, sizeof(buffer), "%s", "1234"));
    tassert_eqs("1234", buffer);


    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(EOK, str.sprintf(buffer, sizeof(buffer), "%s", "123456789"));
    tassert_eqs("123456789", buffer);

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.overflow, str.sprintf(buffer, sizeof(buffer), "%s", "1234567890"));
    tassert_eqs("123456789", buffer);

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.overflow, str.sprintf(buffer, sizeof(buffer), "%s", "12345678900129830128308"));
    tassert_eqs("123456789", buffer);

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.ok, str.sprintf(buffer, sizeof(buffer), "%s", ""));
    tassert_eqs("", buffer);

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.argument, str.sprintf(NULL, sizeof(buffer), "%s", ""));
    tassert_eqi(buffer[0], 'z'); // untouched!

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.argument, str.sprintf(buffer, 0, "%s", ""));
    tassert_eqi(buffer[0], 'z'); // untouched!

    return EOK;
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_cstr);
    test$run(test_cstr_sdollar);
    test$run(test_copy);
    test$run(test_sub_positive_start);
    test$run(test_sub_negative_start);
    test$run(test_iter);
    test$run(test_iter_split);
    test$run(test_find);
    test$run(test_rfind);
    test$run(test_contains_starts_ends);
    test$run(test_remove_prefix);
    test$run(test_remove_suffix);
    test$run(test_strip);
    test$run(test_cmp);
    test$run(test_cmpi);
    test$run(str_to__signed_num);
    test$run(test_str_to_i8);
    test$run(test_str_to_i16);
    test$run(test_str_to_i32);
    test$run(test_str_to_i64);
    test$run(str_to__unsigned_num);
    test$run(test_str_to_u8);
    test$run(test_str_to_u16);
    test$run(test_str_to_u32);
    test$run(test_str_to_u64);
    test$run(str_to__double);
    test$run(test_str_to_f32);
    test$run(test_str_to_f64);
    test$run(test_str_sprintf);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
