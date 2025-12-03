#include "src/all.c"
#include "lib/json/json.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(json_reader_simple)
{
    // str_s content = str$s(str$m({ "foo" : "bar" }));
    str_s content = str$s("{ \"foo\" : \"bar\" }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, true));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key, str$s("foo"));

    tassert_eq(json.reader.next(&js), false);
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(json.reader.next(&js), JsonType__eof);

    return EOK;
}

test$case(json_reader_simple_2elem)
{
    // str_s content = str$s(str$m({ "foo" : "bar" }));
    str_s content = str$s("{ \"foo\" : \"bar\", \"baz\": 1, }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("1"));
    tassert_eq(js.key, str$s("baz"));

    tassert_eq(json.reader.next(&js), false);
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_eq(json.reader.next(&js), false);
    tassert_eq(js.type, JsonType__eof);
    tassert_eq(js.error, NULL);

    return EOK;
}

test$case(json_reader_simple_array)
{
    str_s content = str$s("[ \"foo\", \"bar\", \"baz\", ]");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__arr);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__arr));
    tassertf(json.reader.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '[');
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("foo"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.reader.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.reader.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("baz"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.reader.next(&js), false);
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(json.reader.next(&js), false);
    tassert_eq(js.type, JsonType__eof);

    return EOK;
}

test$case(json_reader_obj_no_step_in)
{
    str_s content = str$s("{ \"foo\" : 1, \"baz\": 3 }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassertf(json.reader.next(&js) == false, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js.val.buf, NULL);
    tassert_eq(js.val.len, 0);
    tassert_eq(js.error, NULL);

    return EOK;
}

test$case(json_reader_nested_obj)
{
    str_s content = str$s("{ \"foo\" : {\"baz\": 3 } }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 2);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js._impl.scope_stack[1], '{');
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("3"));
    tassert_eq(js.key, str$s("baz"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js._impl.scope_stack[1], '\0');

    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js._impl.scope_stack[1], '\0');

    return EOK;
}

test$case(json_reader_nested_obj_skip)
{
    str_s content = str$s("{ \"foo\" : {\"baz\": 3 }, \"zoo\": 8 }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js._impl.scope_stack[1], '\0');
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("8"));
    tassert_eq(js.key, str$s("zoo"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js._impl.scope_stack[1], '\0');

    tassertf(json.reader.next(&js) == JsonType__eof, "error: %s", js.error);

    return EOK;
}

test$case(json_reader_empty_obj)
{
    str_s content = str$s("{}");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

    tassertf(json.reader.next(&js) == JsonType__eof, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js.val.buf, NULL);
    tassert_eq(js.val.len, 0);

    return EOK;
}

test$case(json_reader_nested_obj_step_out)
{
    str_s content = str$s("{ \"foo\" : {\"baz\": 3, \"fuzz\": 8 }, \"next\": 7 }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 2);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js._impl.scope_stack[1], '{');
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("3"));
    tassert_eq(js.key, str$s("baz"));

    tassert_eq(json.reader.step_out(&js), EOK);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js._impl.scope_stack[1], '\0');

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("7"));
    tassert_eq(js.key, str$s("next"));

    return EOK;
}

test$case(json_reader_nested_obj_step_out_2lev)
{
    str_s content = str$s("{ \"foo\" : {\"baz\": 3, \"fuzz\": 8 }, \"next\": 7 }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 2);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js._impl.scope_stack[1], '{');
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("3"));
    tassert_eq(js.key, str$s("baz"));

    tassert_eq(json.reader.step_out(&js), EOK);
    tassert_eq(json.reader.step_out(&js), EOK);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js._impl.scope_stack[1], '\0');

    tassertf(json.reader.next(&js) == false, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js.type, JsonType__eof);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

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
    jr_c js;

    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));
    if (json.reader.next(&js)) { e$ret(json.reader.step_in(&js, JsonType__obj)); }

    while (json.reader.next(&js)) {
        json$key_invalid (&js) {}
        json$key_match (&js, "foo") {
            e$ret(json.reader.step_in(&js, JsonType__obj));
            while (json.reader.next(&js)) {
                json$key_invalid (&js) {}
                json$key_match (&js, "fuzz") { e$ret(str$convert(js.val, &data.foo.fuzz)); }
                json$key_match (&js, "baz") { e$ret(str$convert(js.val, &data.foo.baz)); }
                json$key_unmatched(&js)
                {
                    tassert_eq(js.key, str$s("oops"));
                }
            }
        }
        json$key_match (&js, "next") { e$ret(str$convert(js.val, &data.next)); }
        json$key_match (&js, "baz") { e$ret(str$convert(js.val, &data.baz)); }
    }
    tassert_eq(data.next, 7);
    tassert_eq(data.baz, 17);
    tassert_eq(data.foo.baz, 3);
    tassert_eq(data.foo.fuzz, 8);
    tassert_er(js.error, EOK);


    return EOK;
}

test$case(json_reader_bool)
{
    str_s content = str$s("{ \"foo\" : true, \"bar\": false }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__bool);
    tassert_eq(js.val, str$s("true"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__bool);
    tassert_eq(js.key, str$s("bar"));
    tassert_eq(js.val, str$s("false"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_null)
{
    str_s content = str$s("{ \"foo\" : null }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__null);
    tassert_eq(js.val, str$s("null"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums)
{
    str_s content = str$s("{ \"foo\" : -1, \"bar\": +2 }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-1"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+2"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_bad)
{
    str_s content = str$s("{ \"foo\" : - 1 }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.error, "Unexpected token");
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_indent)
{
    str_s content = str$s("{ \"foo\" : -false }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.error, "Unexpected token");
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_inf)
{
    str_s content = str$s("{ \"foo\" : -inf, \"bar\": +Inf }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_nan_inf)
{
    str_s content = str$s("{ \"foo\" : iNf, \"bar\": NaN }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("iNf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("NaN"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(jr_comments_single_line)
{
    str_s content = str$s("{ // Hi comment \n \"foo\" : -inf, \"bar\": +Inf, }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    // Strict mode fails
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, true));
    tassert_eq(json.reader.next(&js), true);
    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Unexpected token");

    return EOK;
}

test$case(jr_comments_multi_line)
{
    str_s content = str$s(
        "{ // Hi comment \n \"foo\" /* my key */ : /* my value */ -inf, \"bar\": +Inf }"
    );
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, false));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.reader.next(&js), "error: %s", js.error);

    // Strict mode fails
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, true));
    tassert_eq(json.reader.next(&js), true);
    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(!json.reader.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Unexpected token");

    return EOK;
}

test$case(json_reader_simple_array_strict_no_commas)
{
    str_s content = str$s("[ \"foo\", \"bar\", \"baz\", ]");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, true));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__arr);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__arr));
    tassertf(json.reader.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '[');
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("foo"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.reader.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.reader.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("baz"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.reader.next(&js), false);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Ending comma in array");

    return EOK;
}

test$case(json_reader_simple_strict_no_commas)
{
    // str_s content = str$s(str$m({ "foo" : "bar" }));
    str_s content = str$s("{ \"foo\" : \"bar\", }");
    jr_c js;
    tassert_eq(EOK, json.reader.create(&js, content.buf, 0, true));

    tassert_eq(json.reader.next(&js), true);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.reader.step_in(&js, JsonType__obj));
    tassertf(json.reader.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[0], '{');
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key, str$s("foo"));

    tassert_eq(json.reader.next(&js), false);
    tassert_eq(js.error, "Ending comma in object");
    tassert_eq(js.type, JsonType__err);

    return EOK;
}

test$case(json_reader_bad_stuff_handling)
{
    char* variants[] = {
        "{ \"foo\" : \"bar\" ",
        "\"foo\" : \"bar\" }",
        "[\"foo\" : \"bar\" }",
        "[\"foo\" : \"bar\" ]",
        "[,\"foo\",  \"bar\" ]",
        "[\"foo\"  \"bar\" ]",
        "[\"foo\", \"bar\",, ]",
        "\"foo\", \"bar\"]",
        "[\"foo\", \"bar\"",
        "[ True ]",
        "[true, foo]",
        "[\"oops \n new line \"]",
        "{ foo : \"bar\" }",
        "{ null : \"bar\" }",
        "{ false : \"bar\" }",
        "{ true : \"bar\" }",
        "{ \"foo\" ,: \"bar\" }",
        "{ \"foo\" :, \"bar\" }",
        "{ \"foo\" :: \"bar\" }",
        "{ \"foo\" : \"bar\": }",
        "{ \"foo\" : \"bar\" } {}",
        "{ \"foo\" : \"bar\" } []",
        "[] 1",
        "[:]",
        "null, 1",
        "null 1",
        "{ \"foo\" : {\"bar\": 1 }",
        "{\"foo\":\"bar\"}2",
    };

    for$each (it, variants) {
        jr_c ji;
        tassert_eq(EOK, json.reader.create(&ji, it, 0, true));
        while (json.reader.next(&ji)) {}
        tassertf(ji.type == JsonType__err, "source: '%s', error: %s", it, ji.error);
    }

    return EOK;
}

test$case(json_writer_proto)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        tassert_er(EOK, json.buf.create(&jb, 1024, 4, _));
        json$buf(&jb, JsonType__obj)
        {
            json$fmt("// How about a comment? %d\n", 2);
            json$kstr("foo2", "%d", 1);
            json$kval("foo3", "%d", 4);
            json$karr("bar")
            {
                json$str("%s", "foo");
                json$val("%d", 39);
                json$arr()
                {
                    json$val("%d", 19);
                    json$val("%d", 45);
                }
            }
            json$kobj("far")
            {
                json$fmt("\"%s_%d\": %d,\n", "mykey", 2, 77);
                json$kval("zoo", "%d", 1);
            }
        }
        io.printf("JSON: \n`%s`", jb.buf);
        // tassert(false);
        tassert_eq(jb.buf, json.buf.get(&jb));
        tassert_er(EOK, jb.error);
    }
    return EOK;
}


test$case(json_writer_simple_obj)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        tassert_er(EOK, json.buf.create(&jb, 1024, 0, _));
        json$buf(&jb, JsonType__obj)
        {
            json$kstr("foo2", "%d", 1);
            json$kval("foo3", "%d", 4);
        }
        tassert_eq("{\"foo2\": \"1\", \"foo3\": 4}", json.buf.get(&jb));
        tassert_er(EOK, jb.error);
    }
    return EOK;
}

test$case(json_writer_simple_nested_obj)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        tassert_er(EOK, json.buf.create(&jb, 1024, 0, _));
        json$buf(&jb, JsonType__obj)
        {
            json$kstr("foo2", "%d", 1);
            json$kobj("foo3")
            {
                json$kval("bar", "%s", "3");
            }
            json$karr("foo3")
            {
                json$val("%d", 8);
                json$str("%d", 9);
            }
        }
        tassert_eq(
            "{\"foo2\": \"1\", \"foo3\": {\"bar\": 3},\"foo3\": [8, \"9\"]}",
            json.buf.get(&jb)
        );
        tassert_er(EOK, jb.error);
    }
    return EOK;
}

test$case(json_writer_simple_root_array)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        tassert_er(EOK, json.buf.create(&jb, 1024, 0, _));
        json$buf(&jb, JsonType__arr)
        {
            json$val("%d", 8);
            json$str("%d", 9);
            json$obj()
            {
                json$kval("bar", "%s", "3");
                json$kval("baz", "%s", "3");
            }
        }
        tassert_eq("[8, \"9\", {\"bar\": 3, \"baz\": 3}]", json.buf.get(&jb));
        tassert_er(EOK, jb.error);
    }
    return EOK;
}

test$case(json_writer_simple_fmt_arbitrary)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        tassert_er(EOK, json.buf.create(&jb, 1024, 0, _));
        json$buf(&jb, JsonType__obj)
        {
            json$fmt("%s: %s", "foo", "bar");
        }
        tassert_eq("{foo: bar}", json.buf.get(&jb));
        tassert_er(EOK, jb.error);
    }
    return EOK;
}

test$main();
