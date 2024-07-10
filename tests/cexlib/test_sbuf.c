#include <cex/cextest/cextest.h>
#include <cex/cex.h>
#include <cex/cexlib/sview.c>
#include <cex/cexlib/sbuf.c>
#include <cex/cexlib/sbuf.h>
#include <cex/cexlib/allocators.c>
#include <stdio.h>
#include <stdio.h>

const Allocator_c* allocator;
/*
* SUITE INIT / SHUTDOWN
*/
void my_test_shutdown_func(void){
    AllocatorHeap_free();
}

ATEST_SETUP_F(void)
{
    allocator = AllocatorHeap_new();
    return &my_test_shutdown_func;  // return pointer to void some_shutdown_func(void)
}

/*
*
*   TEST SUITE
*
*/


ATEST_F(test_sbuf_new)
{
    sbuf_c s;
    atassert_eqs(EOK, sbuf.create(&s, 20, allocator));
    atassert(s != NULL);

    sbuf_head_s* head = sbuf__head(s);
    atassert_eqi(head->length, 0);
    atassert_eqi(head->capacity, 64 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(head->header.elsize, 1);
    atassert_eqi(head->header.magic, 0xf00e);
    atassert_eqi(head->header.nullterm, 0);
    atassert(head->allocator == allocator);
    atassert_eqs(s, "");
    atassert_eqi(s[head->capacity], 0);
    atassert_eqi(s[0], 0);

    s = sbuf.destroy(&s);
    atassert(s == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sbuf_static)
{
    char buf[128] = {'a'};
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    atassert(s != NULL);
    atassert(s != buf);
    atassert(s == buf + sizeof(sbuf_head_s));

    sbuf_head_s* head = sbuf__head(s);
    atassert_eqi(head->length, 0);
    atassert_eqi(head->capacity, arr$len(buf) - sizeof(sbuf_head_s) - 1);
    atassert_eqi(head->header.elsize, 1);
    atassert(head->allocator == NULL);
    atassert_eqi(head->header.magic, 0xf00e);
    atassert_eqi(head->header.nullterm, 0);
    atassert_eqi(s[0], 0);
    atassert_eqi(s[head->capacity], 0);
    atassert_eqs(s, "");

    // All nullified + for$array works!
    for$array(it, s, sbuf.capacity(s)) {
        atassert_eqi(*it.val, 0);
    }

    // can be also virtually destroyed
    s = sbuf.destroy(&s);
    atassert(s == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sbuf_append_char)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 20, allocator));
    atassert_eqi(sbuf.capacity(s), 64 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm (make sure append_c adds new)
    memset(s, 0xff, sbuf.capacity(s));

    atassert_eqs(Error.argument, sbuf.append_c(&s, NULL));
    atassert_eqs(EOK, sbuf.append_c(&s, ""));
    atassert_eqs("", s);
    atassert_eqi(sbuf.len(s), 0);

    atassert_eqs(EOK, sbuf.append_c(&s, "1234"));
    atassert_eqi(sbuf.capacity(s), 64 - sizeof(sbuf_head_s) - 1);
    atassert_eqs("1234", s);
    atassert_eqi(sbuf.len(s), 4);
    atassert_eqi(sbuf.len(s), strlen(s));

    s = sbuf.destroy(&s);
    atassert(s == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sbuf_append_char_grow)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(sbuf.capacity(s), 11);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(s));

    atassert_eqs(EOK, sbuf.append_c(&s, "1234567890A"));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqs("1234567890A", s);
    atassert_eqi(sbuf.len(s), 11);

    atassert_eqs(EOK, sbuf.append_c(&s, "B"));
    atassert_eqi(sbuf.capacity(s), 64 - sizeof(sbuf_head_s) - 1);
    atassert_eqs("1234567890AB", s);

    // check null term
    atassert_eqi(s[sbuf.len(s)], 0);
    atassert_eqi(s[sbuf.capacity(s)], 0);

    s = sbuf.destroy(&s);
    atassert(s == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sbuf_append_str_grow)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(sbuf.capacity(s), 11);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(s));

    atassert_eqs(EOK, sbuf.append(&s, sview.cstr("1234567890A")));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqs("1234567890A", s);
    atassert_eqi(sbuf.len(s), 11);

    atassert_eqs(EOK, sbuf.append(&s, sview.cstr("B")));
    atassert_eqi(sbuf.capacity(s), 64 - sizeof(sbuf_head_s) - 1);
    atassert_eqs("1234567890AB", s);

    // check null term
    atassert_eqi(s[sbuf.len(s)], 0);
    atassert_eqi(s[sbuf.capacity(s)], 0);

    s = sbuf.destroy(&s);
    atassert(s == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sbuf_clear)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(s));

    atassert_eqs(EOK, sbuf.append_c(&s, "1234567890A"));
    atassert_eqs("1234567890A", s);

    sbuf.clear(&s);
    atassert_eqi(sbuf.len(s), 0);
    atassert_eqs("", s);
    atassert_eqi(strlen(s), 0);


    sbuf.destroy(&s);
    atassert(s == NULL);

    sbuf.destroy(&s);
    atassert(s == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sbuf_destroy)
{
    sbuf_c s;
    char buf[128];
    char* alt_s = buf + sizeof(sbuf_head_s);
    atassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    atassert(buf[0] != '\0');
    atassert_eqs(EOK, sbuf.append_c(&s, "1234567890A"));
    atassert(*alt_s == '1');
    atassert_eqs("1234567890A", s);
    atassert(buf[0] != '\0');


    sbuf.destroy(&s);
    atassert(s == NULL);
    // NOTE: buffer and head were invalidated after destroy
    atassert(buf[0] == '\0');
    atassert(alt_s[0] == '\0');
    atassert_eqs("", buf);
    atassert_eqi(0, strlen(buf));

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_sbuf_replace)
{
    sbuf_c s;
    char buf[128];

    atassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    atassert_eqs(EOK, sbuf.append_c(&s, "123123123"));
    u32 cap = sbuf.capacity(s);

    atassert_eqs(EOK, sbuf.replace(&s, sview.cstr("123"), sview.cstr("456")));
    atassert_eqs(s, "456456456");
    atassert_eqi(sbuf.len(s), 9);
    atassert_eqi(sbuf.capacity(s), cap);

    atassert_eqs(EOK, sbuf.replace(&s, sview.cstr("456"), sview.cstr("78")));
    atassert_eqs(s, "787878");
    atassert_eqi(sbuf.len(s), 6);
    atassert_eqi(sbuf.capacity(s), cap);
    atassert_eqi(s[sbuf.len(s)], 0);

    atassert_eqs(EOK, sbuf.replace(&s, sview.cstr("78"), sview.cstr("321")));
    atassert_eqs(s, "321321321");
    atassert_eqi(sbuf.len(s), 9);
    atassert_eqi(sbuf.capacity(s), cap);
    atassert_eqi(s[sbuf.len(s)], 0);

    atassert_eqs(EOK, sbuf.replace(&s, sview.cstr("32"), sview.cstr("")));
    atassert_eqs(s, "111");
    atassert_eqi(sbuf.len(s), 3);
    atassert_eqi(sbuf.capacity(s), cap);
    atassert_eqi(s[sbuf.len(s)], 0);

    atassert_eqs(EOK, sbuf.replace(&s, sview.cstr("1"), sview.cstr("2")));
    atassert_eqs(s, "222");
    atassert_eqi(sbuf.len(s), 3);
    atassert_eqi(sbuf.capacity(s), cap);
    
    atassert_eqs(EOK, sbuf.replace(&s, sview.cstr("2"), sview.cstr("")));
    atassert_eqs(s, "");
    atassert_eqi(sbuf.len(s), 0);
    atassert_eqi(sbuf.capacity(s), cap);

    sbuf.destroy(&s);
    return NULL;
}

ATEST_F(test_sbuf_replace_resize)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(sbuf.capacity(s), 11);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(s));

    atassert_eqs(EOK, sbuf.append(&s, sview.cstr("1234567890A")));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqs("1234567890A", s);
    atassert_eqi(sbuf.len(s), 11);

    atassert_eqs(EOK, sbuf.replace(&s, sview.cstr("A"), sview.cstr("AB")));
    atassert_eqi(sbuf.capacity(s), 64 - sizeof(sbuf_head_s) - 1);
    atassert_eqs("1234567890AB", s);
    atassert_eqi(sbuf.len(s), 12);


    sbuf.destroy(&s);
    return NULL;
}

ATEST_F(test_sbuf_replace_error_checks)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(sbuf.capacity(s), 11);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(s));

    atassert_eqs(EOK, sbuf.append(&s, sview.cstr("1234567890A")));
    atassert_eqs(Error.argument, sbuf.replace(&s, sview.cstr(NULL), sview.cstr("AB")));
    atassert_eqs(Error.argument, sbuf.replace(&s, sview.cstr("9"), sview.cstr(NULL)));
    atassert_eqs(Error.argument, sbuf.replace(&s, sview.cstr(""), sview.cstr("asda")));

    atassert_eqs("1234567890A", s);
    atassert_eqi(sbuf.len(s), 11);
    sbuf.clear(&s);
    atassert_eqi(sbuf.len(s), 0);
    atassert_eqs(Error.ok, sbuf.replace(&s, sview.cstr("123"), sview.cstr("asda")));


    sbuf.destroy(&s);
    return NULL;
}

ATEST_F(test_sbuf_sprintf)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(sbuf.capacity(s), 11);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(s)+1);
    atassert_eqi(s[sbuf.len(s)], -1);
    atassert_eqi(s[sbuf.capacity(s)], -1);

    atassert_eqs(EOK, sbuf.sprintf(&s, "%s", "123"));
    atassert_eqs("123", s);
    atassert_eqi(sbuf.len(s), 3);
    atassert_eqi(s[sbuf.len(s)], '\0');
    atassert_eqi(s[sbuf.capacity(s)], '\0');
    atassert_eqi(sbuf.capacity(s), 11);

    atassert_eqs(EOK, sbuf.sprintf(&s, "%s", "456"));
    atassert_eqs("123456", s);
    atassert_eqi(sbuf.len(s), 6);
    atassert_eqi(sbuf.capacity(s), 11);

    atassert_eqs(EOK, sbuf.sprintf(&s, "%s", "7890A"));
    atassert_eqs("1234567890A", s);
    atassert_eqi(sbuf.len(s), 11);
    atassert_eqi(sbuf.capacity(s), 11);

    atassert_eqs(EOK, sbuf.sprintf(&s, "%s", "B"));
    atassert_eqs("1234567890AB", s);
    atassert_eqi(sbuf.len(s), 12);
    atassert_eqi(sbuf.capacity(s), 64 - sizeof(sbuf_head_s) - 1);

    atassert_eqs(EOK, sbuf.sprintf(&s, "%s", "CDE"));
    atassert_eqs("1234567890ABCDE", s);
    atassert_eqi(sbuf.len(s), 15);
    atassert_eqi(sbuf.capacity(s), 64 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(s[sbuf.len(s)], '\0');
    atassert_eqi(s[sbuf.capacity(s)], '\0');

    sbuf.destroy(&s);
    return NULL;
}

ATEST_F(test_sbuf_sprintf_long_growth)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(sbuf.capacity(s), 11);

    char buf[16];
    char svbuf[16];
    const u32 n_max = 1000;
    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        atassert_eqs(EOK, sbuf.sprintf(&s, "%04d", i));

        sview_c v = sview.cstr(s);
        atassertf(sview.ends_with(v, sview.cstr(buf)), "i=%d, s=%s", i, v.buf);
        atassert_eqi(s[sbuf.len(s)], '\0');
        atassert_eqi(s[sbuf.capacity(s)], '\0');
    }
    atassert_eqi(n_max*4, sbuf.len(s));

    sview_c sv = sview.cstr(s);

    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        sview_c sub1 = sview.sub(sv, i*4, i*4+4);

        atassert_eqs(EOK, sview.copy(sub1, svbuf, 16));
        // atassert_eqs(svbuf, buf);
        // atassert_eqs(sub1.buf, buf);
        atassertf(sview.cmpc(sub1, buf) == 0, "i=%d, buf=%s sub1=%s", i, buf, sub1.buf);
    }

    sbuf.destroy(&s);
    return NULL;
}

ATEST_F(test_sbuf_sprintf_long_growth_prebuild_buffer)
{
    sbuf_c s;

    atassert_eqs(EOK, sbuf.create(&s, 1024*1024, allocator));
    atassert_eqi(sbuf.capacity(s), 1024*1024 - sizeof(sbuf_head_s) - 1);

    char buf[16];
    char svbuf[16];
    const u32 n_max = 1000;
    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        atassert_eqs(EOK, sbuf.sprintf(&s, "%04d", i));

        sview_c v = sview.cstr(s);
        atassertf(sview.ends_with(v, sview.cstr(buf)), "i=%d, s=%s", i, v.buf);
        atassert_eqi(s[sbuf.len(s)], '\0');
        atassert_eqi(s[sbuf.capacity(s)], '\0');
    }
    atassert_eqi(n_max*4, sbuf.len(s));

    sview_c sv = sview.cstr(s);

    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        sview_c sub1 = sview.sub(sv, i*4, i*4+4);
        atassert_eqs(EOK, sview.copy(sub1, svbuf, 16));
        atassertf(sview.cmpc(sub1, buf) == 0, "i=%d, buf=%s sub1=%s", i, buf, sub1.buf);
    }

    sbuf.destroy(&s);
    return NULL;
}
ATEST_F(test_sbuf_sprintf_static)
{
    sbuf_c s;
    char buf[32];

    atassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(sbuf.capacity(s), 11);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(s)+1);
    atassert_eqi(s[sbuf.len(s)], -1);
    atassert_eqi(s[sbuf.capacity(s)], -1);

    atassert_eqs(EOK, sbuf.sprintf(&s, "%s", "123"));
    atassert_eqs("123", s);
    atassert_eqi(sbuf.len(s), 3);
    atassert_eqi(s[sbuf.len(s)], '\0');
    atassert_eqi(s[sbuf.capacity(s)], '\0');
    atassert_eqi(sbuf.capacity(s), 11);

    atassert_eqs(EOK, sbuf.sprintf(&s, "%s", "456"));
    atassert_eqs("123456", s);
    atassert_eqi(sbuf.len(s), 6);
    atassert_eqi(sbuf.capacity(s), 11);

    atassert_eqs(Error.overflow, sbuf.sprintf(&s, "%s", "7890AB"));
    atassert_eqs("123456", s);
    atassert_eqi(sbuf.len(s), 6);
    atassert_eqi(sbuf.capacity(s), 11);

    atassert_eqs(Error.ok, sbuf.sprintf(&s, "%s", "7890A"));
    atassert_eqs("1234567890A", s);
    atassert_eqi(sbuf.len(s), 11);
    atassert_eqi(sbuf.capacity(s), 11);
    atassert_eqi(s[sbuf.len(s)], '\0');
    atassert_eqi(s[sbuf.capacity(s)], '\0');

    atassert_eqs(Error.overflow, sbuf.sprintf(&s, "%s", "B"));
    atassert_eqs("1234567890A", s);
    atassert_eqi(sbuf.len(s), 11);
    atassert_eqi(sbuf.capacity(s), 32 - sizeof(sbuf_head_s) - 1);
    atassert_eqi(s[sbuf.len(s)], '\0');
    atassert_eqi(s[sbuf.capacity(s)], '\0');

    sbuf.destroy(&s);
    return NULL;
}
/*
*
* MAIN (AUTO GENERATED)
*
*/
int main(int argc, char *argv[])
{
    ATEST_PARSE_MAINARGS(argc, argv);
    ATEST_PRINT_HEAD();  // >>> all tests below
    
    ATEST_RUN(test_sbuf_new);
    ATEST_RUN(test_sbuf_static);
    ATEST_RUN(test_sbuf_append_char);
    ATEST_RUN(test_sbuf_append_char_grow);
    ATEST_RUN(test_sbuf_append_str_grow);
    ATEST_RUN(test_sbuf_clear);
    ATEST_RUN(test_sbuf_destroy);
    ATEST_RUN(test_sbuf_replace);
    ATEST_RUN(test_sbuf_replace_resize);
    ATEST_RUN(test_sbuf_replace_error_checks);
    ATEST_RUN(test_sbuf_sprintf);
    ATEST_RUN(test_sbuf_sprintf_long_growth);
    ATEST_RUN(test_sbuf_sprintf_long_growth_prebuild_buffer);
    ATEST_RUN(test_sbuf_sprintf_static);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
