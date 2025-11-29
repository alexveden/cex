#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/json.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(my_test_case)
{
    mem$scope(tmem$, _)
    {
        json_writer_c jb;
        tassert_er(EOK, json.writer.create(&jb, 1024, 4, _));
        jw$buf(&jb, JsonType__obj)
        {
            jw$fmt("// How about a comment? %d\n", 2);
            jw$kstr("foo2", "%d", 1);
            jw$kval("foo3", "%d", 4);
            jw$karr_scope("bar")
            {
                jw$str("%s", "foo");
                jw$val("%d", 39);
                jw$arr_scope()
                {
                    for (u32 i = 0; i < 10; i++) { jw$val("%d", i); }
                }
            }
            jw$kobj_scope("far")
            {
                jw$fmt("\"%s_%d\": %d,\n", "mykey", 2, 77);
                jw$kval("zoo", "%d", 1);
            }
        }
        io.printf("JSON: \n`%s`", jb.buf);
        // tassert(false);
        tassert_eq(jb.buf, json.writer.get(&jb));
        tassert_er(EOK, jb.error);
    }
    return EOK;
}


test$case(json_reader_struct_fill)
{
    struct Foo
    {
        struct
        {
            u32 baz;
            u32 fuzz;
        } foo;
        u32 next;
        u32 baz;
    } data = { 0 };
    str_s content = str$s(
        "{ \"foo\" : {\"baz\": 3, \"fuzz\": 8, \"oops\": 0}, \"next\": 7, \"baz\": 17 }"
    );

    json_reader_c js;

    jr$scope(&js, content.buf, 0, JsonType__obj)
    {
        jr$case_invalid () {}
        jr$case ("foo") {
            jr$switch()
            {
                jr$case_invalid () {}
                jr$case ("fuzz") { e$ret(str$convert(js.val, &data.foo.fuzz)); }
                jr$case ("baz") { e$ret(str$convert(js.val, &data.foo.baz)); }
                jr$case_default (&js) { tassert_eq(js.key, str$s("oops")); }
            }
        }
        jr$case ("next") { e$ret(str$convert(js.val, &data.next)); }
        jr$case ("baz") { e$ret(str$convert(js.val, &data.baz)); }
    }
    tassert_er(js.error, EOK);

    tassert_eq(data.next, 7);
    tassert_eq(data.baz, 17);
    tassert_eq(data.foo.baz, 3);
    tassert_eq(data.foo.fuzz, 8);


    return EOK;
}
test$main();
