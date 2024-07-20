#include <_cexlib/cextest.h>
#include <_cexlib/cex.c>
#include <_cexlib/str.c>
#include <_cexlib/sbuf.c>
#include <_cexlib/sbuf.h>
#include <_cexlib/_stb_sprintf.c>
#include <_cexlib/allocators.c>
#include <stdio.h>
#include <stdio.h>

const Allocator_i* allocator;
/*
* SUITE INIT / SHUTDOWN
*/
test$teardown(){
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

Exception
append_to_cap(sbuf_c* s){
    tassert(s != NULL);
    u8 c = 'A';
    str_c s1 = {
        .buf = (char*)&c,
        .len = 1,
    };

    for(size_t i = sbuf.len(s); i < sbuf.capacity(s); i++){
        c = 'A' + i;
        tassert_eqs(EOK, sbuf.append(s, s1));
    }

    return EOK;
}

Exception
sprintf_to_cap(sbuf_c* s){
    tassert(s != NULL);
    u8 c = 'A';
    for(size_t i = sbuf.len(s); i < sbuf.capacity(s); i++){
        c = 'A' + i;
        except_silent(err, sbuf.sprintf(s, "%c", c)) {
            return err;
        }
    }

    return EOK;
}


test$case(test_sbuf_new)
{
    sbuf_c s;
    tassert_eqs(EOK, sbuf.create(&s, 20, allocator));
    tassert(s != NULL);

    sbuf_head_s* head = sbuf__head(s);
    tassert_eqi(head->length, 0);
    tassert_eqi(head->capacity, 64 - sizeof(sbuf_head_s) - 1);
    tassert_eqi(head->header.elsize, 1);
    tassert_eqi(head->header.magic, 0xf00e);
    tassert_eqi(head->header.nullterm, 0);
    tassert(head->allocator == allocator);
    tassert_eqs(s, "");
    tassert_eqi(s[head->capacity], 0);
    tassert_eqi(s[0], 0);

    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;

}

test$case(test_sbuf_static)
{
    char buf[128] = {'a'};
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    tassert(s != NULL);
    tassert(s != buf);
    tassert(s == buf + sizeof(sbuf_head_s));

    sbuf_head_s* head = sbuf__head(s);
    tassert_eqi(head->length, 0);
    tassert_eqi(head->capacity, arr$len(buf) - sizeof(sbuf_head_s) - 1);
    tassert_eqi(head->header.elsize, 1);
    tassert(head->allocator == NULL);
    tassert_eqi(head->header.magic, 0xf00e);
    tassert_eqi(head->header.nullterm, 0);
    tassert_eqi(s[0], 0);
    tassert_eqi(s[head->capacity], 0);
    tassert_eqs(s, "");

    // All nullified + for$array works!
    for$array(it, s, sbuf.capacity(&s)) {
        tassert_eqi(*it.val, 0);
    }

    // can be also virtually destroyed
    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;

}


test$case(test_sbuf_append_char_grow)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_eqe(append_to_cap(&s), EOK);

    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);
    tassert_eqi(sbuf.len(&s), sbuf.capacity(&s));

    tassert_eqs(EOK, sbuf.append(&s, s$("B")));
    tassert_eqi(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // check null term
    tassert_eqi(s[sbuf.len(&s)], 0);
    tassert_eqi(s[sbuf.capacity(&s)], 0);

    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;

}

test$case(test_sbuf_append_str_grow)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_eqe(append_to_cap(&s), EOK);
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);
    tassert_eqi(sbuf.len(&s), sbuf.capacity(&s));

    tassert_eqs(EOK, sbuf.append(&s, str.cstr("B")));
    tassert_eqi(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // check null term
    tassert_eqi(s[sbuf.len(&s)], 0);
    tassert_eqi(s[sbuf.capacity(&s)], 0);

    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;

}

test$case(test_sbuf_clear)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_eqs(EOK, sbuf.append(&s, s$("1234567890A")));
    tassert_eqs("1234567890A", s);

    sbuf.clear(&s);
    tassert_eqi(sbuf.len(&s), 0);
    tassert_eqs("", s);
    tassert_eqi(strlen(s), 0);


    sbuf.destroy(&s);
    tassert(s == NULL);

    sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;

}

test$case(test_sbuf_destroy)
{
    sbuf_c s;
    char buf[128];
    char* alt_s = buf + sizeof(sbuf_head_s);
    tassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    tassert(buf[0] != '\0');
    tassert_eqs(EOK, sbuf.append(&s, s$("1234567890A")));
    tassert(*alt_s == '1');
    tassert_eqs("1234567890A", s);
    tassert(buf[0] != '\0');


    sbuf.destroy(&s);
    tassert(s == NULL);
    // NOTE: buffer and head were invalidated after destroy
    tassert(buf[0] == '\0');
    tassert(alt_s[0] == '\0');
    tassert_eqs("", buf);
    tassert_eqi(0, strlen(buf));
    return EOK;

}

test$case(test_sbuf_replace)
{
    sbuf_c s;
    char buf[128];

    tassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    tassert_eqs(EOK, sbuf.append(&s, s$("123123123")));
    u32 cap = sbuf.capacity(&s);

    tassert_eqs(EOK, sbuf.replace(&s, str.cstr("123"), str.cstr("456")));
    tassert_eqs(s, "456456456");
    tassert_eqi(sbuf.len(&s), 9);
    tassert_eqi(sbuf.capacity(&s), cap);

    tassert_eqs(EOK, sbuf.replace(&s, str.cstr("456"), str.cstr("78")));
    tassert_eqs(s, "787878");
    tassert_eqi(sbuf.len(&s), 6);
    tassert_eqi(sbuf.capacity(&s), cap);
    tassert_eqi(s[sbuf.len(&s)], 0);

    tassert_eqs(EOK, sbuf.replace(&s, str.cstr("78"), str.cstr("321")));
    tassert_eqs(s, "321321321");
    tassert_eqi(sbuf.len(&s), 9);
    tassert_eqi(sbuf.capacity(&s), cap);
    tassert_eqi(s[sbuf.len(&s)], 0);

    tassert_eqs(EOK, sbuf.replace(&s, str.cstr("32"), str.cstr("")));
    tassert_eqs(s, "111");
    tassert_eqi(sbuf.len(&s), 3);
    tassert_eqi(sbuf.capacity(&s), cap);
    tassert_eqi(s[sbuf.len(&s)], 0);

    tassert_eqs(EOK, sbuf.replace(&s, str.cstr("1"), str.cstr("2")));
    tassert_eqs(s, "222");
    tassert_eqi(sbuf.len(&s), 3);
    tassert_eqi(sbuf.capacity(&s), cap);
    
    tassert_eqs(EOK, sbuf.replace(&s, str.cstr("2"), str.cstr("")));
    tassert_eqs(s, "");
    tassert_eqi(sbuf.len(&s), 0);
    tassert_eqi(sbuf.capacity(&s), cap);

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_replace_resize)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_eqe(append_to_cap(&s), EOK);
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);
    tassert_eqi(sbuf.len(&s), sbuf.capacity(&s));

    size_t prev_len = sbuf.len(&s);
    tassert_eqs(EOK, sbuf.replace(&s, str.cstr("A"), str.cstr("AB")));
    tassert_eqi(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);
    tassert_eqi(sbuf.len(&s), prev_len+1);


    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_replace_error_checks)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_eqs(EOK, sbuf.append(&s, str.cstr("1234567890A")));
    tassert_eqs(Error.argument, sbuf.replace(&s, str.cstr(NULL), str.cstr("AB")));
    tassert_eqs(Error.argument, sbuf.replace(&s, str.cstr("9"), str.cstr(NULL)));
    tassert_eqs(Error.argument, sbuf.replace(&s, str.cstr(""), str.cstr("asda")));

    tassert_eqs("1234567890A", s);
    tassert_eqi(sbuf.len(&s), 11);
    sbuf.clear(&s);
    tassert_eqi(sbuf.len(&s), 0);
    tassert_eqs(Error.ok, sbuf.replace(&s, str.cstr("123"), str.cstr("asda")));


    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_sprintf)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s)+1);
    tassert_eqi(s[sbuf.len(&s)], -1);
    tassert_eqi(s[sbuf.capacity(&s)], -1);

    tassert_eqs(EOK, sbuf.sprintf(&s, "%s", "123"));
    tassert_eqs("123", s);
    tassert_eqi(sbuf.len(&s), 3);
    tassert_eqi(s[sbuf.len(&s)], '\0');
    tassert_eqi(s[sbuf.capacity(&s)], '\0');
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    tassert_eqs(EOK, sbuf.sprintf(&s, "%s", "456"));
    tassert_eqs("123456", s);
    tassert_eqi(sbuf.len(&s), 6);
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    tassert_eqs(EOK, sbuf.sprintf(&s, "%s", "7890A"));
    tassert_eqs("1234567890A", s);
    tassert_eqi(sbuf.len(&s), 11);
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    sbuf.clear(&s);
    size_t prev_cap = sbuf.capacity(&s);
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);
    tassert_eqe(EOK, append_to_cap(&s));
    tassert_eqs(EOK, sbuf.sprintf(&s, "%s", "B"));
    tassert_eqi(sbuf.len(&s), prev_cap+1);
    tassert_eqi(s[prev_cap], 'B');
    tassert_eqi(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    tassert_eqs(EOK, sbuf.sprintf(&s, "%s", "CDE"));
    tassert_eqi(sbuf.len(&s), prev_cap + 4);
    tassert_eqi(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);
    tassert_eqi(s[sbuf.len(&s)], '\0');
    tassert_eqi(s[sbuf.capacity(&s)], '\0');

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_sprintf_long_growth)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 5, allocator));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    char buf[16];
    char svbuf[16];
    const u32 n_max = 1000;
    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        tassert_eqs(EOK, sbuf.sprintf(&s, "%04d", i));

        str_c v = str.cstr(s);
        tassertf(str.ends_with(v, str.cstr(buf)), "i=%d, s=%s", i, v.buf);
        tassert_eqi(s[sbuf.len(&s)], '\0');
        tassert_eqi(s[sbuf.capacity(&s)], '\0');
    }
    tassert_eqi(n_max*4, sbuf.len(&s));

    str_c sv = str.cstr(s);

    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        str_c sub1 = str.sub(sv, i*4, i*4+4);

        tassert_eqs(EOK, str.copy(sub1, svbuf, 16));
        tassertf(str.cmp(sub1, s$(buf)) == 0, "i=%d, buf=%s sub1=%s", i, buf, sub1.buf);
    }

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_sprintf_long_growth_prebuild_buffer)
{
    sbuf_c s;

    tassert_eqs(EOK, sbuf.create(&s, 1024*1024, allocator));
    tassert_eqi(sbuf.capacity(&s), 1024*1024 - sizeof(sbuf_head_s) - 1);

    char buf[16];
    char svbuf[16];
    const u32 n_max = 1000;
    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        tassert_eqs(EOK, sbuf.sprintf(&s, "%04d", i));

        str_c v = str.cstr(s);
        tassertf(str.ends_with(v, str.cstr(buf)), "i=%d, s=%s", i, v.buf);
        tassert_eqi(s[sbuf.len(&s)], '\0');
        tassert_eqi(s[sbuf.capacity(&s)], '\0');
    }
    tassert_eqi(n_max*4, sbuf.len(&s));

    var sv2 = sbuf.tostr(&s);
    str_c sv = str.cstr(s);
    tassert_eqi(str.cmp(sv2, sv), 0);
    tassert_eqi(sv2.len, sv.len);
    tassert(sv2.buf == sv.buf);


    for(u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        str_c sub1 = str.sub(sv, i*4, i*4+4);
        tassert_eqs(EOK, str.copy(sub1, svbuf, 16));
        tassertf(str.cmp(sub1, s$(buf)) == 0, "i=%d, buf=%s sub1=%s", i, buf, sub1.buf);
    }

    sbuf.destroy(&s);
    return EOK;
}
test$case(test_sbuf_sprintf_static)
{
    sbuf_c s;
    char buf[32];

    tassert_eqs(EOK, sbuf.create_static(&s, buf, arr$len(buf)));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s)+1);
    tassert_eqi(s[sbuf.len(&s)], -1);
    tassert_eqi(s[sbuf.capacity(&s)], -1);


    tassert_eqs(EOK, sbuf.sprintf(&s, "%s", "123"));
    tassert_eqs("123", s);
    tassert_eqi(sbuf.len(&s), 3);
    tassert_eqi(s[sbuf.len(&s)], '\0');
    tassert_eqi(s[sbuf.capacity(&s)], '\0');
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    tassert_eqs(EOK, sbuf.sprintf(&s, "%s", "456"));
    tassert_eqs("123456", s);
    tassert_eqi(sbuf.len(&s), 6);
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);


    tassert_eqe(EOK, sprintf_to_cap(&s));
    tassert_eqi(s[sbuf.len(&s)], '\0');
    tassert_eqi(s[sbuf.capacity(&s)], '\0');
    tassert_eqi(sbuf.len(&s), sbuf.capacity(&s));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);

    tassert_eqs(Error.overflow, sbuf.sprintf(&s, "%s", "7"));
    tassert_eqi(sbuf.len(&s), sbuf.capacity(&s));
    tassert_eqi(sbuf.capacity(&s), 32 - sizeof(sbuf_head_s) - 1);


    sbuf.destroy(&s);
    return EOK;
}
/*
*
* MAIN (AUTO GENERATED)
*
*/
int main(int argc, char *argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_sbuf_new);
    test$run(test_sbuf_static);
    test$run(test_sbuf_append_char_grow);
    test$run(test_sbuf_append_str_grow);
    test$run(test_sbuf_clear);
    test$run(test_sbuf_destroy);
    test$run(test_sbuf_replace);
    test$run(test_sbuf_replace_resize);
    test$run(test_sbuf_replace_error_checks);
    test$run(test_sbuf_sprintf);
    test$run(test_sbuf_sprintf_long_growth);
    test$run(test_sbuf_sprintf_long_growth_prebuild_buffer);
    test$run(test_sbuf_sprintf_static);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
