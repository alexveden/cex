#include "src/all.c"

typedef struct token_cmp_s
{
    char* code;
    char* exp_token;
    CexTkn_e type;
} token_cmp_s;

test$case(test_token_ident)
{
    char* code = " \t foo ";
    CexParser_c lx = CexParser_create(code, 0, false);
    tassert(lx.content == code);
    tassert(lx.cur == code);
    tassert(lx.content_end == code + strlen(code));
    tassert_eq(lx.line, 0);
#define end_brace }

    char* some_brace = "}";
    (void)some_brace;
    // some stuff }
    cex_token_s t = CexParser_next_token(&lx);
    tassert_eq(lx.line, 0);
    tassert_eq(t.type, CexTkn__ident); /* multiline }        */
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);
#undef end_brace

    t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__eof);

    return EOK;
}


test$case(test_token_new_line_skip)
{
    char* code = " \nfoo ";
    CexParser_c lx = CexParser_create(code, 0, false);

    cex_token_s t = CexParser_next_token(&lx);
    tassert_eq(lx.line, 1);
    tassert_eq(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);
    /* multiline */

    t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_new_empty)
{
    char* code = "";
    CexParser_c lx = CexParser_create(code, 0, false);

    cex_token_s t = CexParser_next_token(&lx);
    t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_two_indents)
{
    char* code = " \nfoo _Bar$s(";
    CexParser_c lx = CexParser_create(code, 0, false);

    cex_token_s t = CexParser_next_token(&lx);
    tassert_eq(lx.line, 1);
    tassert_eq(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);

    t = CexParser_next_token(&lx);
    tassert_eq(lx.line, 1);
    tassert_eq(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("_Bar$s")), "t.value=%S", t.value);
    tassert_eq(*lx.cur, '(');
    tassert(lx.cur == lx.content_end - 1);

    t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__lparen);
    tassertf(str.slice.eq(t.value, str$s("(")), "t.value='%S'", t.value);
    tassert_eq(*lx.cur, '\0');
    tassert(lx.cur == lx.content_end);

    t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_numbers)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { " 0", "0", CexTkn__number },
        { "  0  ", "0", CexTkn__number },
        { "0", "0", CexTkn__number },
        { "1234567890", "1234567890", CexTkn__number },
        { "1234567890   !  ", "1234567890", CexTkn__number },
        { "0x1234567890abcdefABCDEF   !  ", "0x1234567890abcdefABCDEF", CexTkn__number },
        { "0X1234567890abcdefABCDEF   !  ", "0X1234567890abcdefABCDEF", CexTkn__number },
        { "0b01010101   !  ", "0b01010101", CexTkn__number },
        { "0.12", "0.12", CexTkn__number },
        { "0.12,", "0.12", CexTkn__number },
        { "0.12)", "0.12", CexTkn__number },
        { "0.12]", "0.12", CexTkn__number },
        { "0.12}", "0.12", CexTkn__number },
        { "-0.12}", "-", CexTkn__minus },
        { "+0.12}", "+", CexTkn__plus },
        { "0.12(", "0.12(", CexTkn__number }, // WARNING: invalid syntax!
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf < it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_string)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { " \"hello world\" ", "hello world", CexTkn__string },
        { " \"hello \\\"world\" ", "hello \\\"world", CexTkn__string },
        { " \"hello \\world\" ", "hello \\world", CexTkn__string },
        { " 'h' ", "h", CexTkn__char },
        { "'\\''", "\\'", CexTkn__char },
        { "'\"'", "\"", CexTkn__char },
        { " \"'\" ", "'", CexTkn__string },
        { " \"hello \nworld\" ", NULL, CexTkn__error },
        { " \"hello \f world\" ", NULL, CexTkn__error },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        if (t.type != CexTkn__error) {
            tassert(t.value.buf >= it.code && t.value.buf < it.code + strlen(it.code));
        }
    }

    return EOK;
}

test$case(test_token_comment)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { " /** hello */ foo", "/** hello */", CexTkn__comment_multi },
        { " /* hello */\n foo", "/* hello */", CexTkn__comment_multi },
        { " // hello comment", "// hello comment", CexTkn__comment_single },
        { " // hello comment\n", "// hello comment", CexTkn__comment_single },
        { " /* hello */ foo", "/* hello */", CexTkn__comment_multi },
        { " /* hello\n bar */ foo", "/* hello\n bar */", CexTkn__comment_multi },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf < it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_comment_and_ident)
{
    char* code = " \n /* comm */foo";
    CexParser_c lx = CexParser_create(code, 0, false);

    cex_token_s t = CexParser_next_token(&lx);
    tassert_eq(lx.line, 1);
    tassert_eq(t.type, CexTkn__comment_multi);
    tassertf(str.slice.eq(t.value, str$s("/* comm */")), "t.value=%S", t.value);

    t = CexParser_next_token(&lx);
    tassert_eq(lx.line, 1);
    tassert_eq(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);

    t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_preproc)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "#include <foo.h>", "include <foo.h>", CexTkn__preproc },
        { "#   include <foo.h>", "include <foo.h>", CexTkn__preproc },
        { "   #   include <foo.h>", "include <foo.h>", CexTkn__preproc },
        { "   #   include <foo.h> \n bar", "include <foo.h> ", CexTkn__preproc },
        { "   #   include \"foo.h\"", "include \"foo.h\"", CexTkn__preproc },
        { "   #   define my$foo(baz, bar) bar", "define my$foo(baz, bar) bar", CexTkn__preproc },
        { "   #   define my$foo(baz, bar) bar\\another\nstop",
          "define my$foo(baz, bar) bar\\another",
          CexTkn__preproc },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_preproc_bad)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "#", NULL, CexTkn__error },
        { "#\\", NULL, CexTkn__error },
        { "#define \\", NULL, CexTkn__error },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf == NULL);
        tassert(t.value.len == 0);
    }

    return EOK;
}

test$case(test_token_brace_parens_brackets)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "{{", "{", CexTkn__lbrace },   { "}", "}", CexTkn__rbrace },
        { "[[", "[", CexTkn__lbracket }, { "]", "]", CexTkn__rbracket },
        { "((", "(", CexTkn__lparen },   { ")", ")", CexTkn__rparen },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}


test$case(test_token_paren_block)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "{ foo }", "{ foo }", CexTkn__brace_block },
        { "[ foo ]", "[ foo ]", CexTkn__bracket_block },
        { "( foo )", "( foo )", CexTkn__paren_block },
        { "{( foo )}", "{( foo )}", CexTkn__brace_block },
        { "[{ foo }]", "[{ foo }]", CexTkn__bracket_block },
        { "([ foo ])", "([ foo ])", CexTkn__paren_block },
        { "{}", "{}", CexTkn__brace_block },
        { "[]", "[]", CexTkn__bracket_block },
        { "()", "()", CexTkn__paren_block },
        { "{\n}", "{\n}", CexTkn__brace_block },
        { "[\n]", "[\n]", CexTkn__bracket_block },
        { "(\n)", "(\n)", CexTkn__paren_block },
        { "{ foo ", NULL, CexTkn__error },
        { "{ ((foo }", NULL, CexTkn__error },
        { "{ foo ]", NULL, CexTkn__error },
        { "( foo }", NULL, CexTkn__error },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, true);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(
            t.value.buf == NULL ||
            (t.value.buf >= it.code && t.value.buf <= it.code + str.len(it.code))
        );
    }

    return EOK;
}

test$case(test_token_paren_block_overflow_test)
{
    char buf[2048] = { 0 };
    char* pairs[] = { "[]", "{}", "()" };

    for$each (p, pairs) {
        memset(buf, 0, sizeof(buf));
        for$eachp(cp, buf)
        {
            usize idx = cp - buf;
            if (idx < sizeof(buf) / 2) {
                *cp = p[0];
            } else {
                *cp = p[1];
            }
        }
        buf[arr$len(buf) - 1] = '\0';
        CexParser_c lx = CexParser_create(buf, 0, true);
        cex_token_s t = CexParser_next_token(&lx);
        tassert_eq(t.type, CexTkn__error);
        tassert(t.value.buf == NULL);
        tassert(t.value.len == 0);
    }

    return EOK;
}

test$case(test_token_misc)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = { { "*", "*", CexTkn__star },  { ".", ".", CexTkn__dot },
                             { ",", ",", CexTkn__comma }, { ";", ";", CexTkn__eos },
                             { ":", ":", CexTkn__colon }, { "?", "?", CexTkn__question },
                             { "=", "=", CexTkn__eq },

                             { "=*", "=", CexTkn__eq } };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_secondary_token)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "main.", ".", CexTkn__dot },      { "234*", "*", CexTkn__star },
        { "main*", "*", CexTkn__star },     { "main(", "(", CexTkn__lparen },
        { "main[", "[", CexTkn__lbracket },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassert(t.type);
        // fetch second
        t = CexParser_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_code)
{
    // clang-format off
    char* code = 
        "#include <gfoo>\n"
        "int main(char** argv) {\n"
        "    printf(\"hello\"); \n"
        "}";
    // clang-format on
    token_cmp_s tokens[] = {
        { NULL, "include <gfoo>", CexTkn__preproc },
        { NULL, "int", CexTkn__ident },
        { NULL, "main", CexTkn__ident },
        { NULL, "(char** argv)", CexTkn__paren_block },
        { NULL, "{\n    printf(\"hello\"); \n}", CexTkn__brace_block },
    };
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    u32 nit = 0;
    for$each (it, tokens) {
        t = CexParser_next_token(&lx);
        io.printf("step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);
        tassert(t.type);
        tassertf(t.type == it.type, "step: %d type_exp=%d, type_act=%d", nit, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "step: %d val_exp='%s' val_value='%S'",
            nit,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= code && t.value.buf <= code + strlen(code));
        nit++;
    }
    t = CexParser_next_token(&lx);
    tassertf(
        t.type == CexTkn__eof,
        "step: %d type_exp=%d, t.type=%d t.value='%S'\n",
        nit,
        CexTkn__eof,
        t.type,
        t.value
    );
    return EOK;
}

test$case(test_token_this_file)
{
    char* code = io.file.load(__FILE__, mem$);
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    u32 nit = 0;
    char* prev_tok = lx.cur;
    while ((t = CexParser_next_token(&lx)).type) {
        /* NOTE:
         * Tokenize this file, and make sure that all test functions are parsed correctly
         */
        // io.printf("step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);

        tassertf(t.type != CexTkn__unk, "step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);
        tassertf(t.type != CexTkn__error, "step: %d token error starts at: ...\n%s\n", prev_tok);
        if (t.type == CexTkn__brace_block) {
            tassert(str.slice.starts_with(t.value, str$s("{")));
            tassert(str.slice.ends_with(t.value, str$s("}")));
        }
        if (t.type == CexTkn__paren_block) {
            tassert(str.slice.starts_with(t.value, str$s("(")));
            tassert(str.slice.ends_with(t.value, str$s(")")));
        }
        if (t.type == CexTkn__bracket_block) {
            tassert(str.slice.starts_with(t.value, str$s("[")));
            tassert(str.slice.ends_with(t.value, str$s("]")));
        }
        nit++;
        prev_tok = lx.cur;
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    mem$free(mem$, code);
    return EOK;
}

test$case(test_token_json)
{
    // clang-format off
    char* code = 
        "{\"hi\": {\"foo\": \"test\"}\n";

    // clang-format on
    CexParser_c lx = CexParser_create(code, 0, false);
    cex_token_s t;
    u32 nit = 0;
    while ((t = CexParser_next_token(&lx)).type) {
        io.printf("step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);
    }
    t = CexParser_next_token(&lx);
    tassertf(
        t.type == CexTkn__eof,
        "step: %d type_exp=%d, t.type=%d t.value='%S'\n",
        nit,
        CexTkn__eof,
        t.type,
        t.value
    );
    return EOK;
}

test$case(test_token_idents)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "foo", "foo", CexTkn__ident },           { "foo_bar", "foo_bar", CexTkn__ident },
        { "_foo_bar", "_foo_bar", CexTkn__ident }, { "_foo$bar", "_foo$bar", CexTkn__ident },
        { "$foo$bar", "$foo$bar", CexTkn__ident }, { "FOO", "FOO", CexTkn__ident },
        { "FOO2", "FOO2", CexTkn__ident },         { "2FOO", "2FOO", CexTkn__number },
        { "FOO2(", "FOO2", CexTkn__ident },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexParser_next_token(&lx);
        tassertf(
            t.type == it.type,
            "code='%s', type_exp=%s, type_act=%s",
            it.code,
            CexTkn_str[it.type],
            CexTkn_str[t.type]
        );
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_parser)
{
    // CexParser - has nasty edge cases of char special characters and escaping it's useful to check
    char* code = io.file.load("src/CexParser.c", mem$);
    tassert(code);
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    u32 nit = 0;
    char* prev_tok = lx.cur;
    while ((t = CexParser_next_token(&lx)).type) {
        io.printf("step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);
        tassertf(t.type != CexTkn__unk, "step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);
        tassertf(t.type != CexTkn__error, "step: %d token error starts at: ...\n%s\n", prev_tok);
        if (t.type == CexTkn__brace_block) {
            tassert(str.slice.starts_with(t.value, str$s("{")));
            tassert(str.slice.ends_with(t.value, str$s("}")));
        }
        if (t.type == CexTkn__paren_block) {
            tassert(str.slice.starts_with(t.value, str$s("(")));
            tassert(str.slice.ends_with(t.value, str$s(")")));
        }
        if (t.type == CexTkn__bracket_block) {
            tassert(str.slice.starts_with(t.value, str$s("[")));
            tassert(str.slice.ends_with(t.value, str$s("]")));
        }
        nit++;
        prev_tok = lx.cur;
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    mem$free(mem$, code);
    return EOK;
}

test$case(test_token_compound_operators)
{
    // compound operators are not lexed as a single token: '=' becomes CexTkn__unk
    token_cmp_s tokens[] = {
        { "a == b", "=", CexTkn__unk }, { "a += b", "=", CexTkn__unk },
        { "a *= b", "=", CexTkn__unk }, { "a != b", "=", CexTkn__unk },
        { "a ~= b", "=", CexTkn__unk }, { "a <= b", "=", CexTkn__unk },
        { "@", "@", CexTkn__unk },
    };
    for$each (it, tokens) {
        CexParser_c lx = CexParser_create(it.code, 0, false);
        bool found = false;
        cex_token_s t;
        while ((t = CexParser_next_token(&lx)).type != CexTkn__eof) {
            if (t.type == CexTkn__unk && str.slice.eq(t.value, str.sstr(it.exp_token))) {
                found = true;
            }
        }
        tassertf(found, "code='%s' exp='%s'", it.code, it.exp_token);
    }
    return EOK;
}

test$case(test_token_error_state)
{
    CexParser_c lx = CexParser_create("foo", 0, false);
    lx.error = "forced";
    cex_token_s t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__error);
    return EOK;
}

test$case(test_token_string_overflow)
{
    // unterminated string ending with a backslash: the escape consumes a missing char
    CexParser_c lx = CexParser_create("\"\\", 0, false);
    cex_token_s t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__error);
    return EOK;
}

test$case(test_token_scope_preproc_error)
{
    // '#' at the end of a folded scope makes the inner preproc scan fail
    CexParser_c lx = CexParser_create("{#", 0, true);
    cex_token_s t = CexParser_next_token(&lx);
    tassert_eq(t.type, CexTkn__error);
    return EOK;
}

test$main();
