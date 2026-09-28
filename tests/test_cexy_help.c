#define TBUILDDIR "tests/build/cexytest/"
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
        tassert(str.find(content, "  first line,\n  second line.\n"));
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

#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
