#include <cex/cexlib/cex.c>
#include <cex/cexlib/list.h>
#include <cex/cexlib/str.c>
#include <cex/cexlib/sbuf.c>
#include <cex/cexlib/_stb_sprintf.c>
#include <cex/cexlib/cextest.h>
#include <cex/cexlib/allocators.c>
#include <stdio.h>

const Allocator_i* allocator;
/*
* SUITE INIT / SHUTDOWN
*/
cextest$teardown(){
    allocator = allocators.heap.destroy();
    return EOK;
}

cextest$setup()
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

cextest$case(test_cstr)
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

cextest$case(test_cstr_sdollar)
{
    const char* cstr = "hello";

    sbuf_c sb;
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
    char buf[30] = {0};
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

cextest$case(test_copy)
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
    tassert_eqs(Error.sanity_check, str.copy(str.cstr(NULL), buf, arr$len(buf)));
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

cextest$case(test_sub_positive_start)
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

cextest$case(test_sub_negative_start)
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

cextest$case(test_iter)
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

cextest$case(test_iter_split)
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

cextest$case(test_find)
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

cextest$case(test_rfind)
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

cextest$case(test_contains_starts_ends)
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

cextest$case(test_remove_prefix)
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

cextest$case(test_remove_suffix)
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

cextest$case(test_strip)
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

cextest$case(test_cmp)
{

    tassert_eqi(str.cmp(str.cstr("123456"), str.cstr("123456")), 0);
    tassert_eqi(str.cmp(str.cstr(NULL), str.cstr(NULL)), 0);
    tassert_eqi(str.cmp(str.cstr(""), str.cstr("")), 0);
    tassert_eqi(str.cmp(str.cstr("ABC"), str.cstr("AB")), 67);
    tassert_eqi(str.cmp(str.cstr("ABA"), str.cstr("ABZ")), -25);
    tassert_eqi(str.cmp(str.cstr("AB"), str.cstr("ABC")), -67);
    tassert_eqi(str.cmp(str.cstr("A"), str.cstr("")), (int)'A');
    tassert_eqi(str.cmp(str.cstr(""), str.cstr("A")), -1*((int)'A'));
    tassert_eqi(str.cmp(str.cstr(""), str.cstr(NULL)), 1);
    tassert_eqi(str.cmp(str.cstr(NULL), str.cstr("ABC")), -1);

    return EOK;
}

cextest$case(test_cmpi)
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
    tassert_eqi(str.cmpi(str.cstr(""), str.cstr("A")), -1*((int)'A'));
    tassert_eqi(str.cmpi(str.cstr(""), str.cstr(NULL)), 1);
    tassert_eqi(str.cmpi(str.cstr(NULL), str.cstr("ABC")), -1);


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
    cextest$args_parse(argc, argv);
    cextest$print_header();  // >>> all tests below
    
    cextest$run(test_cstr);
    cextest$run(test_cstr_sdollar);
    cextest$run(test_copy);
    cextest$run(test_sub_positive_start);
    cextest$run(test_sub_negative_start);
    cextest$run(test_iter);
    cextest$run(test_iter_split);
    cextest$run(test_find);
    cextest$run(test_rfind);
    cextest$run(test_contains_starts_ends);
    cextest$run(test_remove_prefix);
    cextest$run(test_remove_suffix);
    cextest$run(test_strip);
    cextest$run(test_cmp);
    cextest$run(test_cmpi);
    
    cextest$print_footer();  // ^^^^^ all tests runs are above
    return cextest$exit_code();
}
