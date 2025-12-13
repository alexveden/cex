#include "src/all.c"
#include "cexstd/json/json.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(json_reader_simple)
{
    // str_s content = str$s(str$m({ "foo" : "bar" }));
    str_s content = str$s("{ \"foo\" : \"bar\" }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = true}));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key, str$s("foo"));

    tassert_eq(json.rd.next(&js), false);
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(json.rd.next(&js), JsonType__eof);

    return EOK;
}

test$case(json_reader_simple_2elem)
{
    str_s content = str$s("{ \"foo\" : \"bar\", \"baz\": 1, }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = false}));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("1"));
    tassert_eq(js.key, str$s("baz"));

    tassert_eq(json.rd.next(&js), false);
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_eq(json.rd.next(&js), false);
    tassert_eq(js.type, JsonType__eof);
    tassert_eq(js.error, NULL);

    return EOK;
}

test$case(json_reader_simple_array)
{
    str_s content = str$s("[ \"foo\", \"bar\", \"baz\", ]");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__arr);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);

    tassert_er(EOK, json.rd.step_in(&js, JsonType__arr));
    tassertf(json.rd.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("foo"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.rd.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.rd.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("baz"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.rd.next(&js), false);
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(json.rd.next(&js), false);
    tassert_eq(js.type, JsonType__eof);

    return EOK;
}

test$case(json_reader_obj_no_step_in)
{
    str_s content = str$s("{ \"foo\" : 1, \"baz\": 3 }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);

    // NOTE: this should fail, because we missing step_in call, it's steps over
    tassertf(json.rd.next(&js) == false, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js.val.buf, NULL);
    tassert_eq(js.val.len, 0);
    tassert_eq(js.error, NULL);
    tassert_eq(js.type, JsonType__eof);

    return EOK;
}

test$case(json_reader_nested_obj)
{
    str_s content = str$s("{ \"foo\" : {\"baz\": 3 } }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 2);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("3"));
    tassert_eq(js.key, str$s("baz"));

    // End of {\"baz\": 3 }
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__eos);

    // End of {"foo": ... }
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js.type, JsonType__eos);

    return EOK;
}

test$case(json_reader_nested_obj_skip)
{
    str_s content = str$s("{ \"foo\" : {\"baz\": 3 }, \"zoo\": 8 }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("8"));
    tassert_eq(js.key, str$s("zoo"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], 0);
    tassert_eq(js._impl.scope_stack[1], 0);

    tassertf(json.rd.next(&js) == JsonType__eof, "error: %s", js.error);

    return EOK;
}

test$case(json_reader_empty_obj)
{
    str_s content = str$s("{}");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js.type, JsonType__eos);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

    tassertf(json.rd.next(&js) == JsonType__eof, "error: %s", js.error);
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
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 2);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("3"));
    tassert_eq(js.key, str$s("baz"));

    tassert_eq(json.rd.step_out(&js), EOK);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js._impl.scope_stack[1], '\0');

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("7"));
    tassert_eq(js.key, str$s("next"));

    return EOK;
}

test$case(json_reader_nested_obj_step_out_2lev)
{
    str_s content = str$s("{ \"foo\" : {\"baz\": 3, \"fuzz\": 8 }, \"next\": 7 }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, str$s("foo"));

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 2);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("3"));
    tassert_eq(js.key, str$s("baz"));

    tassert_eq(json.rd.step_out(&js), EOK);
    tassert_eq(json.rd.step_out(&js), EOK);
    tassert_eq(js._impl.scope_depth, 0);

    tassertf(json.rd.next(&js) == false, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');
    tassert_eq(js.type, JsonType__eof);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

    return EOK;
}

test$case(json_reader_bool)
{
    str_s content = str$s("{ \"foo\" : true, \"bar\": false }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__bool);
    tassert_eq(js.val, str$s("true"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__bool);
    tassert_eq(js.key, str$s("bar"));
    tassert_eq(js.val, str$s("false"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_null)
{
    str_s content = str$s("{ \"foo\" : null }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__null);
    tassert_eq(js.val.buf, NULL);
    tassert_eq(js.val.len, 1);
    tassert_eq(js.key, str$s("foo"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums)
{
    str_s content = str$s("{ \"foo\" : -1, \"bar\": +2 }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-1"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+2"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_bad)
{
    str_s content = str$s("{ \"foo\" : - 1 }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.error, "Unexpected token");
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_indent)
{
    str_s content = str$s("{ \"foo\" : -false }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.error, "Unexpected token");
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.val, (str_s){ 0 });
    tassert_eq(js.key, (str_s){ 0 });

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_inf)
{
    str_s content = str$s("{ \"foo\" : -inf, \"bar\": +Inf }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(json_reader_signed_nums_nan_inf)
{
    str_s content = str$s("{ \"foo\" : iNf, \"bar\": NaN }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("iNf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("NaN"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    return EOK;
}

test$case(jr_comments_single_line)
{
    str_s content = str$s("{ // Hi comment \n \"foo\" : -inf, \"bar\": +Inf, }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    // Strict mode fails
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = true}));
    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Unexpected token");

    return EOK;
}

test$case(jr_comments_single_line_before_obj)
{
    str_s content = str$s("// Hi comment \n { \"foo\" : -inf, \"bar\": +Inf, }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, NULL));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    // Strict mode fails
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = true}));
    tassert_er("Unexpected token", json.rd.step_in(&js, JsonType__obj));
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Unexpected token");

    return EOK;
}

test$case(jr_comments_multi_line)
{
    str_s content = str$s(
        "{ // Hi comment \n \"foo\" /* my key */ : /* my value */ -inf, \"bar\": +Inf }"
    );
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, false));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    // Strict mode fails
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = true}));
    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Unexpected token");

    return EOK;
}

test$case(jr_comments_multi_line_before_obj)
{
    str_s content = str$s(
        "/*another comment*/{ // Hi comment \n \"foo\" /* my key */ : /* my value */ -inf, \"bar\": +Inf }"
    );
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, false));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("-inf"));
    tassert_eq(js.key, str$s("foo"));

    tassertf(json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__num);
    tassert_eq(js.val, str$s("+Inf"));
    tassert_eq(js.key, str$s("bar"));

    tassertf(!json.rd.next(&js), "error: %s", js.error);

    // Strict mode fails
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = true}));
    tassert_er("Unexpected token", json.rd.step_in(&js, JsonType__obj));
    tassertf(!json.rd.next(&js), "error: %s", js.error);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Unexpected token");

    return EOK;
}

test$case(json_reader_simple_array_strict_no_commas)
{
    str_s content = str$s("[ \"foo\", \"bar\", \"baz\", ]");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = true}));

    tassert_eq(js.type, JsonType__arr);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__arr));
    tassertf(json.rd.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("foo"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.rd.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.rd.next(&js), JsonType__str);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("baz"));
    tassert_eq(js.key.buf, NULL);

    tassert_eq(json.rd.next(&js), false);
    tassert_eq(js.type, JsonType__err);
    tassert_eq(js.error, "Ending comma in array");

    return EOK;
}

test$case(json_reader_simple_strict_no_commas)
{
    // str_s content = str$s(str$m({ "foo" : "bar" }));
    str_s content = str$s("{ \"foo\" : \"bar\", }");
    json_rd_c js;
    tassert_eq(EOK, json.rd.create(&js, content.buf, 0, &(json_rd_kw){.strict_mode = true}));

    tassert_eq(js.type, JsonType__obj);
    tassert_eq(js.key.buf, NULL);
    tassert_eq(js.key.len, 0);
    tassert_eq(js._impl.scope_depth, 0);
    tassert_eq(js._impl.scope_stack[0], '\0');

    tassert_er(EOK, json.rd.step_in(&js, JsonType__obj));
    tassertf(json.rd.next(&js) == JsonType__str, "error: %s", js.error);
    tassert_eq(js._impl.scope_depth, 1);
    tassert_eq(js.type, JsonType__str);
    tassert_eq(js.val, str$s("bar"));
    tassert_eq(js.key, str$s("foo"));

    tassert_eq(json.rd.next(&js), false);
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
        json_rd_c ji;
        tassert_eq(EOK, json.rd.create(&ji, it, 0, &(json_rd_kw){.strict_mode = true}));
        while (json.rd.next(&ji)) {}
        tassertf(ji.type == JsonType__err, "source: '%s', error: %s", it, ji.error);
    }

    return EOK;
}

// FIX
// test$case(json_writer_proto)
// {
//     mem$scope(tmem$, _)
//     {
//         json_wr_c jb;
//         tassert_er(EOK, json.buf.create(&jb, 1024, 4, _));
//         json$buf(&jb, JsonType__obj)
//         {
//             json$fmt("// How about a comment? %d\n", 2);
//             json$kstr("foo2", "%d", 1);
//             json$kval("foo3", "%d", 4);
//             json$karr("bar")
//             {
//                 json$str("%s", "foo");
//                 json$val("%d", 39);
//                 json$arr()
//                 {
//                     json$val("%d", 19);
//                     json$val("%d", 45);
//                 }
//             }
//             json$kobj("far")
//             {
//                 json$fmt("\"%s_%d\": %d,\n", "mykey", 2, 77);
//                 json$kval("zoo", "%d", 1);
//             }
//         }
//         io.printf("JSON: \n`%s`", jb.buf);
//         // tassert(false);
//         tassert_eq(jb.buf, json.buf.get(&jb));
//         tassert_er(EOK, jb.error);
//     }
//     return EOK;
// }



test$main();
