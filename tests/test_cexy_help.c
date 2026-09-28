#define TBUILDDIR "tests/build/cexytest/"
#define TNSDIR "cexytest_ns_myfoo/"
#define TEXDIR "cexytest_example/"
#define CEX_LOG_LVL 4
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#include "src/all.c"

test$setup_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    e$assert(!os.path.exists(TBUILDDIR) && "must not exist!");
    e$ret(os.fs.mkpath(TBUILDDIR));
    e$assert(os.path.exists(TBUILDDIR) && "must exist!");
    return EOK;
}
test$teardown_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    if (os.fs.remove_tree(TEXDIR)) {};
    return EOK;
}

#if !defined(__EMSCRIPTEN__)

static char*
test_brief_decl_to_str(cex_decl_s* d, IAllocator alloc)
{
    char* path = TBUILDDIR "brief_decl.txt";
    FILE* out = NULL;
    e$except (err, io.fopen(&out, path, "w")) { return NULL; }
    _cexy__print_brief_decl(d, d->name, out);
    io.fclose(&out);
    return io.file.load(path, alloc);
}

static Exception
test_help_example_make_fixture(void)
{
    if (os.fs.remove_tree(TEXDIR)) {};
    e$ret(os.fs.mkpath(TEXDIR));
    e$ret(io.file.save(
        TEXDIR "example.c",
        "static int exf_used(int x) { return x + 1; }\n"
        "static int exf_usedr_caller(int x) { return exf_usedr(x); }\n"
        "static int exf_caller(int x) { return exf_used(x); }\n"
        "static int exf_usedr(int x) { return x + 2; }\n"
        "static int exf_unused(int x) { return x; }\n"
    ));
    return EOK;
}

test$case(test_print_brief_decl_func)
{
    mem$scope(tmem$, _)
    {
        sbuf_c ret = sbuf.create(32, _);
        sbuf_c args = sbuf.create(32, _);
        e$ret(sbuf.append(&ret, "char*"));
        e$ret(sbuf.append(&args, "char* s"));
        cex_decl_s d = {
            .name = str$s("str.find"),
            .docs = str$s("/// Find it\n"),
            .ret_type = ret,
            .args = args,
            .file = "src/str.c",
            .line = 10,
            .type = CexTkn__func_def,
        };
        char* content = test_brief_decl_to_str(&d, _);
        tassert(content);
        tassert(str.find(content, "char* str.find(char* s)   // src/str.c:11"));
        tassert(str.find(content, "  Find it"));
    }
    return EOK;
}

test$case(test_print_brief_decl_macro)
{
    mem$scope(tmem$, _)
    {
        sbuf_c args = sbuf.create(32, _);
        e$ret(sbuf.append(&args, "a, value..."));
        cex_decl_s d = {
            .name = str$s("arr$push"),
            .docs = str$s("/// Appends element\n"),
            .args = args,
            .file = "cex.h",
            .line = 1478,
            .type = CexTkn__macro_func,
        };
        char* content = test_brief_decl_to_str(&d, _);
        tassert(content);
        tassert(str.find(content, "#define arr$push(a, value...)   // cex.h:1479"));
        tassert(str.find(content, "  Appends element"));
    }
    return EOK;
}

test$case(test_print_brief_decl_multiline)
{
    mem$scope(tmem$, _)
    {
        sbuf_c args = sbuf.create(32, _);
        e$ret(sbuf.append(&args, "char* s"));
        cex_decl_s d = {
            .name = str$s("str.copy"),
            .docs = str$s("/// first line,\n/// second line.\n"),
            .args = args,
            .file = "src/str.c",
            .line = 165,
            .type = CexTkn__func_def,
        };
        char* content = test_brief_decl_to_str(&d, _);
        tassert(content);
        tassert(str.find(content, "  first line, second line.\n"));
        tassert(!str.find(content, "///"));
    }
    return EOK;
}

test$case(test_print_brief_decl_typedef)
{
    mem$scope(tmem$, _)
    {
        sbuf_c ret = sbuf.create(32, _);
        e$ret(sbuf.append(&ret, "typedef struct"));
        cex_decl_s d = {
            .name = str$s("str_s"),
            .ret_type = ret,
            .file = "cex.h",
            .line = 323,
            .type = CexTkn__typedef,
        };
        char* content = test_brief_decl_to_str(&d, _);
        tassert(content);
        tassert(str.find(content, "typedef struct str_s   // cex.h:324"));
    }
    return EOK;
}

test$case(test_help_brief_namespace_includes_funcs)
{
    char* out_path = TBUILDDIR "help_brief_ns.txt";
    char* argv[] = { "help",
                     "--brief",
                     "--filter",
                     "./src/str.[hc]",
                     "--out",
                     out_path,
                     "str$" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "namespace str"));
        tassert(str.find(content, "str$convert"));
        tassert(str.find(content, "str.find("));
        tassert(str.find(content, "str.slice.sub("));
        tassert(!str.find(content, "os.cmd.run"));
    }
    return EOK;
}

test$case(test_help_brief_namespace_excludes_private)
{
    char* out_path = TBUILDDIR "help_brief_ns_private.txt";
    char* argv[] = { "help",
                     "--brief",
                     "--filter",
                     "./src/str.[hc]",
                     "--out",
                     out_path,
                     "str$",
                     NULL };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(!str.find(content, "str.to_double_"));
        tassert(!str.find(content, "str.to_signed_num_"));
        tassert(!str.find(content, "str.to_unsigned_num_"));
        tassert(str.find(content, "str.find("));
    }
    return EOK;
}

test$case(test_help_brief_cexy_namespace_includes_non_prefixed_funcs)
{
    char* out_path = TBUILDDIR "help_brief_cexy_ns.txt";
    char* argv[] = { "help",
                     "--brief",
                     "--filter",
                     "./src/cexy.[hc]",
                     "--out",
                     out_path,
                     "cexy$" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "namespace cexy"));
        tassert(str.find(content, "cexy.cmd.help"));
        tassert(str.find(content, "cexy.build_self"));
        tassert(!str.find(content, "os.cmd.run"));
    }
    return EOK;
}

test$case(test_help_namespace_cex_includes_macros)
{
    char* out_path = TBUILDDIR "help_cex_ns.txt";
    char* argv[] = { "help",
                     "--filter",
                     "./src/cex_platform.[hc]",
                     "--out",
                     out_path,
                     "cex$" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "#define cex$platform_malloc"));
        tassert(str.find(content, "cex$version_major"));
    }
    return EOK;
}

test$case(test_help_namespace_cex_includes_upper_consts)
{
    char* out_path = TBUILDDIR "help_cex_consts.txt";
    char* argv[] = { "help",
                     "--filter",
                     "./src/cex_platform.[hc]",
                     "--out",
                     out_path,
                     "cex$" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "#define CEX_LOG_LVL"));
        tassert(str.find(content, "#define CEX_ALLOCATOR_MAX_SCOPE_STACK"));
    }
    return EOK;
}

test$case(test_help_namespace_str_excludes_cex_consts)
{
    char* out_path = TBUILDDIR "help_str_ns.txt";
    char* argv[] = { "help",
                     "--filter",
                     "./src/str.[hc]",
                     "--out",
                     out_path,
                     "str$" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "str$"));
        tassert(!str.find(content, "CEX_LOG_LVL"));
    }
    return EOK;
}

test$case(test_help_namespace_custom_returns_all_symbols)
{
    char* out_path = TBUILDDIR "help_myfoo_ns.txt";
    if (os.fs.remove_tree(TNSDIR)) {};
    e$ret(os.fs.mkpath(TNSDIR));
    e$ret(io.file.save(
        TNSDIR "myfoo.h",
        "/// Myfoo namespace docs\n"
        "struct __cex_namespace__myfoo {\n"
        "    /// Does foo\n"
        "    int (*foo)(int x);\n"
        "};\n"
        "\n"
        "/// Myfoo macro\n"
        "#define myfoo$bar 1\n"
        "\n"
        "/// Myfoo const\n"
        "#define MYFOO_BAZ 2\n"
        "\n"
        "/// Myfoo type\n"
        "typedef struct myfoo_thing_s { int x; } myfoo_thing_s;\n"
    ));
    char* argv[] = { "help",
                     "--filter",
                     "./" TNSDIR "*.[hc]",
                     "--out",
                     out_path,
                     "myfoo$" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "myfoo$bar"));
        tassert(str.find(content, "MYFOO_BAZ"));
        tassert(str.find(content, "myfoo_thing_s"));
    }
    if (os.fs.remove_tree(TNSDIR)) {};
    return EOK;
}

test$case(test_help_brief_batch)
{
    char* out_path = TBUILDDIR "help_batch.txt";
    char* argv[] = { "help",
                     "--brief",
                     "--filter",
                     "./src/str.c",
                     "--out",
                     out_path,
                     "str.find",
                     "str.replace" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "str.find"));
        tassert(str.find(content, "str.replace"));
        tassert(!str.find(content, "Symbol found at"));
    }
    return EOK;
}

test$case(test_help_not_found_exit)
{
    char* out_path = TBUILDDIR "help_notfound.txt";
    char* argv[] = { "help",
                     "--brief",
                     "--filter",
                     "./src/str.c",
                     "--out",
                     out_path,
                     "no_such_symbol_xyz" };
    tassert_er(Error.not_found, cexy.cmd.help(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_help_bare_shows_usage)
{
    char* argv[] = { "help", NULL };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    return EOK;
}

test$case(test_help_list_namespaces)
{
    char* out_path = TBUILDDIR "help_list.txt";
    char* argv[] = { "help",
                     "--list",
                     "--filter",
                     "./src/str.[hc]",
                     "--out",
                     out_path,
                     NULL };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "str$"));
        tassert(str.find(content, "str"));
        tassert(!str.find(content, "typedef"));
    }
    return EOK;
}

test$case(test_help_list_with_query_errors)
{
    char* argv[] = { "help", "--list", "str.find", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    return EOK;
}

test$case(test_namespace_entities)
{
#define $file "src/str"
    mem$scope(tmem$, _)
    {

        arr$(cex_token_s) items = arr$new(items, _);
        char* code_c = io.file.load($file ".c", _);
        char* code_h = io.file.load($file ".h", _);
        tassert(code_c);
        tassert(code_h);

        log$debug("Header symbols: %s\n", $file ".h");
        CexParser_c lx = CexParser.create(code_h, 0, true);
        cex_token_s t;
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            log$debug(
                "Decl: type: '%s' name: %S doc_len: %d body_len: %d ret: %s args: %s\n",
                CexTkn_str[d->type],
                d->name,
                d->docs.len,
                d->body.len,
                d->ret_type,
                d->args
            );
        }

        log$debug("\nSource symbols: %s\n", $file ".c");
        lx = CexParser.create(code_c, 0, true);
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            log$debug(
                "Decl: type: '%s' name: %S doc_len: %d body_len: %d ret: %s args: %s\n",
                CexTkn_str[t.type],
                d->name,
                d->docs.len,
                d->body.len,
                d->ret_type,
                d->args
            );
        }
        // tassert(false);
    }
#undef $file
    return EOK;
}

test$case(test_help_idioms_only)
{
    char* out_path = TBUILDDIR "help_idioms.txt";
    char* argv[] = { "help",
                     "--idioms",
                     "--filter",
                     "./src/str.[hc]",
                     "--out",
                     out_path,
                     "str$",
                     NULL };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "Pattern matching 101"));
        tassert(!str.find(content, "str.find("));
        tassert(!str.find(content, "#define str$"));
        tassert(!str.find(content, "namespace str"));
    }
    return EOK;
}

test$case(test_help_brief_idioms)
{
    char* out_path = TBUILDDIR "help_brief_idioms.txt";
    char* argv[] = { "help",
                     "--brief",
                     "--idioms",
                     "--filter",
                     "./src/str.[hc]",
                     "--out",
                     out_path,
                     "str$",
                     NULL };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "Pattern matching 101"));
        tassert(str.find(content, "namespace str"));
        tassert(str.find(content, "str.find("));
    }
    return EOK;
}

test$case(test_help_idioms_rejects_symbol)
{
    char* argv[] = { "help", "--idioms", "str.find", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    return EOK;
}

test$case(test_help_idioms_rejects_dot_form)
{
    char* argv[] = { "help", "--idioms", "str.", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    return EOK;
}

test$case(test_help_idioms_rejects_list)
{
    char* argv[] = { "help", "--idioms", "--list", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    return EOK;
}

test$case(test_help_idioms_no_docs)
{
    char* out_path = TBUILDDIR "help_idioms_nodocs.txt";
    char* argv[] = { "help",
                     "--idioms",
                     "--filter",
                     "./src/cexy.[hc]",
                     "--out",
                     out_path,
                     "cexy$",
                     NULL };
    tassert_er(Error.not_found, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    return EOK;
}

test$case(test_help_agents_out)
{
    char* out_path = TBUILDDIR "help_agents.txt";
    char* argv[] = { "help", "--agents", "--out", out_path, NULL };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "CEX string principles:"));
        tassert(str.find(content, "Generic type-safe dynamic array backed by a heap header."));
        tassert(str.find(content, "CEX Exception-based error handling."));
        tassert(str.find(content, "Unified array / hashmap / slice iteration framework."));
        tassert(!str.find(content, "no idioms"));
    }
    return EOK;
}

test$case(test_help_agents_rejects_query)
{
    char* argv[] = { "help", "--agents", "str$", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(argv) - 1, argv, NULL));
    return EOK;
}

test$case(test_help_agents_rejects_flags)
{
    char* brief[] = { "help", "--agents", "--brief", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(brief) - 1, brief, NULL));
    char* idioms[] = { "help", "--agents", "--idioms", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(idioms) - 1, idioms, NULL));
    char* list[] = { "help", "--agents", "--list", NULL };
    tassert_er(Error.argument, cexy.cmd.help(arr$len(list) - 1, list, NULL));
    return EOK;
}

test$case(test_help_example_deterministic)
{
    e$ret(test_help_example_make_fixture());
    char* out1 = TBUILDDIR "example1.txt";
    char* out2 = TBUILDDIR "example2.txt";
    char* argv1[] = { "help",
                      "--example",
                      "--filter",
                      "./" TEXDIR "*.[hc]",
                      "--out",
                      out1,
                      "exf_used" };
    char* argv2[] = { "help",
                      "--example",
                      "--filter",
                      "./" TEXDIR "*.[hc]",
                      "--out",
                      out2,
                      "exf_used" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv1), argv1, NULL));
    tassert_er(EOK, cexy.cmd.help(arr$len(argv2), argv2, NULL));
    mem$scope(tmem$, _)
    {
        char* c1 = io.file.load(out1, _);
        char* c2 = io.file.load(out2, _);
        tassert(c1 && c2);
        tassert(str.eq(c1, c2));
        tassert(str.find(c1, "Examples of 'exf_used'"));
        tassert(!str.find(c1, "try again"));
    }
    return EOK;
}

test$case(test_help_example_shows_usage_location)
{
    e$ret(test_help_example_make_fixture());
    char* out_path = TBUILDDIR "example_loc.txt";
    char* argv[] = { "help",
                     "--example",
                     "--filter",
                     "./" TEXDIR "*.[hc]",
                     "--out",
                     out_path,
                     "exf_used" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "./" TEXDIR "example.c:3"));
        tassert(str.find(content, "int exf_caller(int x)"));
        tassert(str.find(content, "return exf_used(x)"));
    }
    return EOK;
}

test$case(test_help_example_no_usages)
{
    e$ret(test_help_example_make_fixture());
    char* out_path = TBUILDDIR "example_none.txt";
    char* argv[] = { "help",
                     "--example",
                     "--filter",
                     "./" TEXDIR "*.[hc]",
                     "--out",
                     out_path,
                     "exf_unused" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "No examples of 'exf_unused'"));
    }
    return EOK;
}

test$case(test_help_example_brief_locations)
{
    e$ret(test_help_example_make_fixture());
    char* out_path = TBUILDDIR "example_brief.txt";
    char* argv[] = { "help",
                     "--brief",
                     "--example",
                     "--filter",
                     "./" TEXDIR "*.[hc]",
                     "--out",
                     out_path,
                     "exf_used" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "Examples of 'exf_used'"));
        tassert(str.find(content, "./" TEXDIR "example.c:3:"));
        tassert(str.find(content, "return exf_used(x);"));
        tassert(!str.find(content, "exf_usedr"));
    }
    return EOK;
}

test$case(test_help_example_word_boundary)
{
    e$ret(test_help_example_make_fixture());
    char* out_path = TBUILDDIR "example_boundary.txt";
    char* argv[] = { "help",
                     "--example",
                     "--filter",
                     "./" TEXDIR "*.[hc]",
                     "--out",
                     out_path,
                     "exf_used" };
    tassert_er(EOK, cexy.cmd.help(arr$len(argv), argv, NULL));
    mem$scope(tmem$, _)
    {
        char* content = io.file.load(out_path, _);
        tassert(content);
        tassert(str.find(content, "Examples of 'exf_used' (1)"));
        tassert(str.find(content, "1) ./" TEXDIR "example.c:3"));
    }
    return EOK;
}

test$case(test_find_symbol_usage_word_boundary)
{
    tassert_eq(_cexy__find_symbol_usage(str$s("str.find(x)"), str$s("str.find")), 0);
    tassert_eq(_cexy__find_symbol_usage(str$s("str.findr(x)"), str$s("str.find")), -1);
    tassert_eq(_cexy__find_symbol_usage(str$s("str.find_x"), str$s("str.find")), -1);
    tassert_eq(_cexy__find_symbol_usage(str$s("xstr.find(x)"), str$s("str.find")), -1);
    tassert_eq(_cexy__find_symbol_usage(str$s("arr$pushm(x)"), str$s("arr$push")), -1);
    tassert_eq(_cexy__find_symbol_usage(str$s("x = arr$push(a, b);"), str$s("arr$push")), 4);
    return EOK;
}

#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
