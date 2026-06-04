#define TBUILDDIR "tests/build/cexytest/"
#define CEX_LOG_LVL 8
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
test$case(test_src_namespace_gen)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "src.c";
        char* hdr = TBUILDDIR "src.h";
        sbuf_c buf = sbuf.create(1024 * 10, _);
        char* ns = "src";

        cg$init(&buf);
        cg$pn("#include \"cex.h\"");
        cg$pn("");
        cg$func("Exception %s_fn2(void)", ns)
        {
            cg$pn("return EOK;");
        }
        cg$func("arr$(char*) %s_fn1(char* foo)", ns)
        {
            cg$pn("return NULL;");
        }
        cg$func("int %s__fn1_Private(char* foo)", ns)
        {
            cg$pn("return NULL;");
        }
        cg$func("int %s__subname__fn_Sub1(char* foo)", ns)
        {
            cg$pn("return NULL;");
        }
        cg$func("arr$(char*) %s__subname__fnsub2(char* foo)", ns)
        {
            cg$pn("return NULL;");
        }
        cg$func("int %s__aubName2__fnsub1(char bar)", ns)
        {
            cg$pn("return NULL;");
        }
        cg$pn("");

        e$ret(io.file.save(src, buf));
        e$ret(io.file.save(hdr, "// header"));

        char* argv[] = { "process", src };
        int argc = arr$len(argv);

        tassert_er(EOK, cexy.cmd.process(argc, argv, NULL));

        char* src_content = io.file.load(src, _);
        char* hdr_content = io.file.load(hdr, _);
        log$info("Source: \n%s\n", src_content);
        log$info("Header: \n%s\n", hdr_content);

        tassert(str.find(src_content, ".fn_Sub1 = src__subname__fn_Sub1,"));
        tassert(str.find(src_content, "CEX_NAMESPACE_DEF struct __cex_namespace__src src = "));
        tassert(str.find(src_content, "__aubName2__fnsub1(char"));
        tassert(str.find(hdr_content, "CEX_NAMESPACE struct __cex_namespace__src src"));
        tassert(str.find(hdr_content, "struct __cex_namespace__src {"));
        tassert(str.find(hdr_content, "arr$(char*)"));
    }
    return EOK;
}

test$case(test_src_namespace_update)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "src.c";
        char* hdr = TBUILDDIR "src.h";
        sbuf_c buf = sbuf.create(1024 * 10, _);
        char* ns = "src";

        cg$init(&buf);
        cg$pn("#include \"cex.h\"");
        cg$pn("");
        cg$func("Exception %s_fn2(void)", ns)
        {
            cg$pn("return EOK;");
        }
        cg$pn("");

        e$ret(io.file.save(src, buf));
        e$ret(io.file.save(hdr, "#define FOO"));

        char* argv[] = { "process", src };
        int argc = arr$len(argv);

        tassert_er(EOK, cexy.cmd.process(argc, argv, NULL));

        char* src_content = io.file.load(src, _);
        char* hdr_content = io.file.load(hdr, _);
        log$info("Source: \n%s\n", src_content);
        log$info("Header: \n%s\n", hdr_content);

        tassert_er(EOK, cexy.cmd.process(argc, argv, NULL));
        char* new_src_content = io.file.load(src, _);
        char* new_hdr_content = io.file.load(hdr, _);
        log$info("Source: \n%s\n", new_src_content);
        log$info("Header: \n%s\n", new_hdr_content);
        tassert(str.eq(src_content, new_src_content));
        tassert(str.eq(hdr_content, new_hdr_content));
    }
    return EOK;
}

test$case(test_src_namespace_update_duplicate_code)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "src.c";
        char* hdr = TBUILDDIR "src.h";
        sbuf_c buf = sbuf.create(1024 * 10, _);
        char* ns = "src";

        cg$init(&buf);
        cg$pn("#include \"cex.h\"");
        cg$pn("");
        cg$func("Exception %s_fn2(void)", ns)
        {
            cg$pn("return EOK;");
        }
        cg$pn("");

        char* hdr_code =
            "__attribute__((visibility(\"hidden\"))) extern const struct __cex_namespace__cexy cexy;\n"
            "__attribute__((visibility(\"hidden\"))) extern const struct __cex_namespace__cexy cexy;\n";

        e$ret(io.file.save(src, buf));
        e$ret(io.file.save(hdr, hdr_code));

        char* argv[] = { "process", src };
        int argc = arr$len(argv);
        log$info("Header before: \n%s\n", hdr_code);
        tassert(str.find(hdr_code, "__cex_namespace__cexy"));

        tassert_er(EOK, cexy.cmd.process(argc, argv, NULL));

        char* src_content = io.file.load(src, _);
        char* hdr_content = io.file.load(hdr, _);
        log$info("Source: \n%s\n", src_content);
        log$info("Header After: \n%s\n", hdr_content);
        tassert(!str.find(hdr_content, "__cex_namespace__cexy"));
        tassert(str.find(hdr_content, "__cex_namespace__src"));
    }
    return EOK;
}

#else
test$case(not_supported_by_platform)
{
    return EOK;
}
#endif // #if !defined(__EMSCRIPTEN__)

test$main();
