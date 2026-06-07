#include "src/all.c"
#include "src/sbuf.c"
#include "src/test.h"

Exception
append_to_cap(sbuf_c* s)
{
    e$assert(s != NULL);
    char c[2] = { 'A', '\0' };

    for (usize i = sbuf.len(s); i < sbuf.capacity(s); i++) {
        c[0] = (char)('A' + i);
        e$ret(sbuf.append(s, c));
    }

    return EOK;
}

Exception
sprintf_to_cap(sbuf_c* s)
{
    e$assert(s != NULL);
    char c = 'A';
    for (usize i = sbuf.len(s); i < sbuf.capacity(s); i++) {
        c = 'A' + i;
        e$except_silent (err, sbuf.appendf(s, "%c", c)) { return err; }
    }

    return EOK;
}


test$case(test_sbuf_new)
{
    sbuf_c s = sbuf.create(20, mem$);
    tassert(s != NULL);

    sbuf_head_s* head = _sbuf__head(s);
    tassert_eq(head->length, 0);
    tassert_eq(head->capacity, 64 - sizeof(sbuf_head_s) - 1);
    tassert_eq(head->header.elsize, 1);
    tassert_eq(head->header.magic, 0xf00e);
    tassert_eq(head->header.nullterm, 0);
    tassert(head->allocator == mem$);
    tassert_eq(s, "");
    tassert_eq(s[head->capacity], 0);
    tassert_eq(s[0], 0);

    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;
}

test$case(test_sbuf_static)
{
    char buf[128] = { 'a' };
    sbuf_c s = sbuf.create_static(buf, arr$len(buf));
    tassert(s != NULL);
    tassert(s != buf);
    tassert(s == buf + sizeof(sbuf_head_s));

    sbuf_head_s* head = _sbuf__head(s);
    tassert_eq(head->length, 0);
    tassert_eq(head->capacity, arr$len(buf) - sizeof(sbuf_head_s) - 1);
    tassert_eq(head->header.elsize, 1);
    tassert(head->allocator == NULL);
    tassert_eq(head->header.magic, 0xf00e);
    tassert_eq(head->header.nullterm, 0);
    tassert_eq(s[0], 0);
    tassert_eq(s[head->capacity], 0);
    tassert_eq(s, "");

    // All nullified + for$each works!
    for$each (it, s, sbuf.capacity(&s)) { tassert_eq(it, 0); }

    // can be also virtually destroyed
    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;
}


test$case(test_sbuf_append_char_grow)
{
    sbuf_c s = sbuf.create(5, mem$);

    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_er(append_to_cap(&s), EOK);

    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);
    tassert_eq(sbuf.len(&s), sbuf.capacity(&s));

    tassert_eq(EOK, sbuf.append(&s, "B"));
    tassert_eq(sbuf.capacity(&s), 128 - sizeof(sbuf_head_s) - 1);

    // check null term
    tassert_eq(s[sbuf.len(&s)], 0);
    tassert_eq(s[sbuf.capacity(&s)], 0);

    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;
}

test$case(test_sbuf_append_char_grow_temp)
{
    mem$scope(tmem$, _)
    {
        sbuf_c s = sbuf.create(128, _);

        tassert_eq(sbuf.capacity(&s), 256 - sizeof(sbuf_head_s) - 1);

        // wipe all nullterm
        memset(s, 0xff, sbuf.capacity(&s));

        tassert_er(append_to_cap(&s), EOK);

        tassert_eq(sbuf.capacity(&s), 256 - sizeof(sbuf_head_s) - 1);
        tassert_eq(sbuf.len(&s), sbuf.capacity(&s)-1);

        tassert_eq(EOK, sbuf.append(&s, "B"));
        tassert_eq(sbuf.capacity(&s), 256 - sizeof(sbuf_head_s) - 1);

        // check null term
        tassert_eq(s[sbuf.len(&s)], 0);
        tassert_eq(s[sbuf.capacity(&s)], 0);

        s = sbuf.destroy(&s);
        tassert(s == NULL);
    }
    return EOK;
}

test$case(test_sbuf_append_str_grow)
{
    sbuf_c s = sbuf.create(5, mem$);

    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_er(append_to_cap(&s), EOK);
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);
    tassert_eq(sbuf.len(&s), sbuf.capacity(&s));

    tassert_eq(EOK, sbuf.append(&s, "B"));
    tassert_eq(sbuf.capacity(&s), 128 - sizeof(sbuf_head_s) - 1);

    // check null term
    tassert_eq(s[sbuf.len(&s)], 0);
    tassert_eq(s[sbuf.capacity(&s)], 0);

    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;
}

test$case(test_sbuf_clear)
{
    sbuf_c s = sbuf.create(5, mem$);
    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));

    tassert_eq(EOK, sbuf.append(&s, "1234567890A"));
    tassert_eq("1234567890A", s);

    sbuf.clear(&s);
    tassert_eq(sbuf.len(&s), 0);
    tassert_eq("", s);
    tassert_eq(strlen(s), 0);


    sbuf.destroy(&s);
    tassert(s == NULL);

    sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;
}

test$case(test_sbuf_destroy)
{
    char buf[128];
    char* alt_s = buf + sizeof(sbuf_head_s);
    sbuf_c s = sbuf.create_static(buf, arr$len(buf));
    tassert(buf[0] != '\0');
    tassert_eq(EOK, sbuf.append(&s, "1234567890A"));
    tassert(*alt_s == '1');
    tassert_eq("1234567890A", s);
    tassert(buf[0] != '\0');


    sbuf.destroy(&s);
    tassert(s == NULL);
    // NOTE: buffer and head were invalidated after destroy
    tassert(buf[0] == '\0');
    tassert(alt_s[0] == '\0');
    tassert_eq("", buf);
    tassert_eq(0, strlen(buf));
    return EOK;
}

test$case(test_sbuf_sprintf)
{
    sbuf_c s = sbuf.create(5, mem$);

    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s) + 1);
    tassert_eq((u8)s[sbuf.len(&s)], 0xff);
    tassert_eq((u8)s[sbuf.capacity(&s)], 0xff);

    tassert_eq(EOK, sbuf.appendf(&s, "%s", "123"));
    tassert_eq("123", s);
    tassert_eq(sbuf.len(&s), 3);
    tassert_eq(s[sbuf.len(&s)], '\0');
    tassert_eq(s[sbuf.capacity(&s)], '\0');
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    tassert_eq(EOK, sbuf.appendf(&s, "%s", "456"));
    tassert_eq("123456", s);
    tassert_eq(sbuf.len(&s), 6);
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    tassert_eq(EOK, sbuf.appendf(&s, "%s", "7890A"));
    tassert_eq("1234567890A", s);
    tassert_eq(sbuf.len(&s), 11);
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    sbuf.clear(&s);
    usize prev_cap = sbuf.capacity(&s);
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);
    tassert_er(EOK, append_to_cap(&s));
    tassert_eq(EOK, sbuf.appendf(&s, "%s", "B"));
    tassert_eq(sbuf.len(&s), prev_cap + 1);
    tassert_eq(s[prev_cap], 'B');
    tassert_eq(sbuf.capacity(&s), 128 - sizeof(sbuf_head_s) - 1);

    tassert_eq(EOK, sbuf.appendf(&s, "%s", "CDE"));
    tassert_eq(sbuf.len(&s), prev_cap + 4);
    tassert_eq(sbuf.capacity(&s), 128 - sizeof(sbuf_head_s) - 1);
    tassert_eq(s[sbuf.len(&s)], '\0');
    tassert_eq(s[sbuf.capacity(&s)], '\0');

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_appendf_long_growth)
{
    sbuf_c s = sbuf.create(5, mem$);

    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    char buf[16];
    char svbuf[16];
    const u32 n_max = 1000;
    for (u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        tassert_eq(EOK, sbuf.appendf(&s, "%04d", i));

        tassertf(str.ends_with(s, buf), "i=%d, s=%s", i, s);
        tassert_eq(s[sbuf.len(&s)], '\0');
        tassert_eq(s[sbuf.capacity(&s)], '\0');
    }
    tassert_eq(n_max * 4, sbuf.len(&s));

    for (u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        str_s sub1 = str.sub(s, i * 4, i * 4 + 4);

        tassert_eq(EOK, str.slice.copy(svbuf, sub1, 16));
        tassertf(str.slice.eq(sub1, str.sstr(buf)), "i=%d, buf=%s sub1=%s", i, buf, sub1.buf);
    }

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_appendf_long_growth_prebuild_buffer)
{
    sbuf_c s = sbuf.create(1024 * 1024, mem$);

    tassert_eq(sbuf.capacity(&s), 1024 * 1024 - sizeof(sbuf_head_s) - 1);

    char buf[16];
    char svbuf[16];
    const u32 n_max = 1000;
    for (u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        tassert_eq(EOK, sbuf.appendf(&s, "%04d", i));

        str_s v = str.sstr(s);
        tassertf(str.slice.ends_with(v, str.sstr(buf)), "i=%d, s=%s", i, v.buf);
        tassert_eq(s[sbuf.len(&s)], '\0');
        tassert_eq(s[sbuf.capacity(&s)], '\0');
    }
    tassert_eq(n_max * 4, sbuf.len(&s));

    auto sv2 = str.sstr(s);
    str_s sv = str.sstr(s);
    tassert_eq(str.slice.eq(sv2, sv), 1);
    tassert_eq(sv2.len, sv.len);
    tassert(sv2.buf == sv.buf);


    for (u32 i = 0; i < n_max; i++) {
        snprintf(buf, arr$len(buf), "%04d", i);
        str_s sub1 = str.slice.sub(sv, i * 4, i * 4 + 4);
        tassert_eq(EOK, str.slice.copy(svbuf, sub1, 16));
        tassertf(str.slice.eq(sub1, str.sstr(buf)), "i=%d, buf=%s sub1=%s", i, buf, sub1.buf);
    }

    sbuf.destroy(&s);
    return EOK;
}
test$case(test_sbuf_appendf_static)
{
    char buf[64];
    sbuf_c s = sbuf.create_static(buf, arr$len(buf));
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s) + 1);
    tassert_eq((u8)s[sbuf.len(&s)], 0xff);
    tassert_eq((u8)s[sbuf.capacity(&s)], 0xff);


    tassert_eq(EOK, sbuf.appendf(&s, "%s", "123"));
    tassert_eq("123", s);
    tassert_eq(sbuf.len(&s), 3);
    tassert_eq(s[sbuf.len(&s)], '\0');
    tassert_eq(s[sbuf.capacity(&s)], '\0');
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    tassert_eq(EOK, sbuf.appendf(&s, "%s", "456"));
    tassert_eq("123456", s);
    tassert_eq(sbuf.len(&s), 6);
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);


    tassert_er(EOK, sprintf_to_cap(&s));
    tassert_eq(s[sbuf.len(&s)], '\0');
    tassert_eq(s[sbuf.capacity(&s)], '\0');
    tassert_eq(sbuf.len(&s), sbuf.capacity(&s));
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    tassert_eq(Error.overflow, sbuf.appendf(&s, "%s", "7"));
    tassert_eq(sbuf.capacity(&s), 0);
    tassert_eq(sbuf.len(&s), sbuf.capacity(&s));


    sbuf.destroy(&s);
    return EOK;
}


test$case(test_sbuf__is_valid__no_null_term)
{
    sbuf_c s = sbuf.create(20, mem$);
    tassert(s != NULL);

    sbuf_head_s* head = _sbuf__head(s);
    tassert_eq(sbuf.isvalid(&s), true);

    head->header.nullterm = 1;
    tassert_eq(sbuf.isvalid(&s), false);

    // manual free (because s.destroy() does sanity checks of head)
    mem$->free(mem$, head);
    return EOK;
}

test$case(test_sbuf__is_valid__len_gt_cap)
{
    sbuf_c s = sbuf.create(20, mem$);
    tassert(s != NULL);

    sbuf_head_s* head = _sbuf__head(s);
    tassert_eq(sbuf.isvalid(&s), true);

    head->length = 10;
    head->capacity = 9;
    tassert_eq(sbuf.isvalid(&s), false);

    // manual free (because s.destroy() does sanity checks of head)
    mem$->free(mem$, head);
    return EOK;
}

test$case(test_sbuf__is_valid__zero_cap)
{
    sbuf_c s = sbuf.create(20, mem$);
    tassert(s != NULL);

    sbuf_head_s* head = _sbuf__head(s);
    tassert_eq(sbuf.isvalid(&s), true);

    head->capacity = 0;
    tassert_eq(sbuf.isvalid(&s), false);

    // manual free (because s.destroy() does sanity checks of head)
    mem$->free(mem$, head);
    return EOK;
}

test$case(test_sbuf__is_valid__bad_magic)
{
    sbuf_c s = sbuf.create(20, mem$);
    tassert(s != NULL);

    sbuf_head_s* head = _sbuf__head(s);
    tassert_eq(sbuf.isvalid(&s), true);

    head->header.magic = 1098;
    tassert_eq(sbuf.isvalid(&s), false);

    // manual free (because s.destroy() does sanity checks of head)
    mem$->free(mem$, head);
    return EOK;
}

test$case(test_sbuf__is_valid__null_pointer)
{
    sbuf_c s = { 0 };
    tassert_eq(sbuf.isvalid(&s), false);

    tassert_eq(sbuf.isvalid(NULL), false);

    // s2 initialized with some non null junk
    // WARNING: this will always segfault, so we need to s = {0}; before calling sbuf.isvalid()
    // sbuf_c s2;
    // memset(&s2, 'z', sizeof(s2));
    // tassert_eq(sbuf.isvalid(&s2), false);


    return EOK;
}

test$case(test_sbuf_appendf_error_resilience)
{
    char buf[64];
    sbuf_c s = sbuf.create_static(buf, arr$len(buf));
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    sbuf.appendf(&s, "%s", "123");
    tassert_eq(3, strlen(s));

    sbuf.appendf(&s, "%s", "456");

    tassert_er(EOK, sprintf_to_cap(&s));

    tassert_eq(Error.overflow, sbuf.appendf(&s, "%s", "7"));
    tassert_eq(sbuf.capacity(&s), 0);
    tassert_eq(sbuf.len(&s), sbuf.capacity(&s));
    tassert_eq(Error.overflow, sbuf.validate(&s));

    sbuf.appendf(&s, "%s", "456");
    tassert_eq(false, sbuf.isvalid(&s));

    tassert_eq(Error.overflow, sbuf.validate(&s));
    tassert_er(Error.overflow, sbuf.set_len(&s, 31921)); // NOTE: uses hear->err

    tassert_eq(0, strlen(s));

    tassert_eq("NULL argument", sbuf.validate(NULL));

    s = NULL;
    tassert_eq("Memory error or already free'd", sbuf.validate(&s));

    // NULL resilience check
    sbuf.appendf(&s, "%s", "456");
    sbuf.append(&s, "456");
    tassert_eq(0, sbuf.len(&s));
    tassert_eq(0, sbuf.capacity(&s));
    tassert_er(Error.runtime, sbuf.set_len(&s, 31921));
    sbuf.clear(&s);
    sbuf.set_len(&s, 0);

    tassert_eq(false, sbuf.isvalid(&s));
    tassert_eq("Memory error or already free'd", sbuf.validate(&s));

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_set_len_test)
{
    char buf[64];
    sbuf_c s = sbuf.create_static(buf, arr$len(buf));
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s) + 1);
    tassert_eq((u8)s[sbuf.len(&s)], 0xff);
    tassert_eq((u8)s[sbuf.capacity(&s)], 0xff);


    tassert_eq(EOK, sbuf.appendf(&s, "%s", "123"));
    tassert_eq("123", s);
    tassert_eq(sbuf.len(&s), 3);

    sbuf.set_len(&s, 2);
    tassert_eq(sbuf.len(&s), 2);
    tassert_eq(s[2], '\0');
    tassert_er(EOK, sbuf.validate(&s));


    tassert_eq(Error.ok, sbuf.set_len(&s, 3));
    tassert_eq(sbuf.len(&s), 3);
    tassert_er(Error.ok, sbuf.validate(&s));


    // Static buffer overflow
    tassert_eq(Error.overflow, sbuf.set_len(&s, sbuf.capacity(&s)));
    tassert_er(Error.overflow, sbuf.validate(&s));

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_sbuf_append_set_len_grow)
{
    sbuf_c s = sbuf.create(5, mem$);

    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    // wipe all nullterm
    memset(s, 0xff, sbuf.capacity(&s));
    sbuf.append(&s, "foo");

    tassert_eq(sbuf.len(&s), 3);
    tassert_eq(sbuf.capacity(&s), 64 - sizeof(sbuf_head_s) - 1);

    tassert_eq(sbuf.set_len(&s, 65), EOK);
    tassert_eq(sbuf.len(&s), 65);
    tassert_eq(s[65], '\0');

    tassert_eq(s[0], 'f');
    tassert_eq(s[1], 'o');
    tassert_eq(s[2], 'o');
    tassert_eq(s[3], '\0');

    // NOTE: sbuf.set_len() keep grown data untouched, expect garbage 

    // for(u32 i = 4; i < sbuf.len(&s); i++) {
    //     tassertf(s[i] == -1, "s[%d] %d != '\\0'", i, s[i]);
    // }

    // Ensure the new space is nullified

    s = sbuf.destroy(&s);
    tassert(s == NULL);
    return EOK;
}

test$main();
