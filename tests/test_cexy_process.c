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

test$case(test_process_all_skips_build_and_tests)
{
    mem$scope(tmem$, _)
    {
        char* old = os.fs.getcwd(_);
        e$ret(os.fs.mkpath(TBUILDDIR "build/x.c"));
        e$ret(os.fs.mkpath(TBUILDDIR "tests/x.c"));
        e$ret(io.file.save(TBUILDDIR "build/x.c", "int x(void) { return 0; }\n"));
        e$ret(io.file.save(TBUILDDIR "tests/x.c", "int x(void) { return 0; }\n"));
        // no matching .h -> skipped by the update pass
        e$ret(io.file.save(TBUILDDIR "nohdr.c", "int nh(void) { return 0; }\n"));
        // .h present but no namespace decls -> skipped
        e$ret(io.file.save(TBUILDDIR "empty.c", "int e(void) { return 0; }\n"));
        e$ret(io.file.save(TBUILDDIR "empty.h", "// empty\n"));
        // .h present and a matching function -> processed
        e$ret(io.file.save(TBUILDDIR "myns.c", "int myns_foo(void) { return 0; }\n"));
        e$ret(io.file.save(TBUILDDIR "myns.h", "// hdr\n"));

        e$ret(os.fs.chdir(TBUILDDIR));
        char* argv[] = { "process", "all" };
        Exc err = cexy.cmd.process(arr$len(argv), argv, NULL);
        e$ret(os.fs.chdir(old));
        tassert_er(EOK, err);

        // `all` is update-only: files without an existing namespace block stay untouched
        char* nohdr = io.file.load(TBUILDDIR "nohdr.c", _);
        tassert_eq(nohdr, "int nh(void) { return 0; }\n");
        char* myns = io.file.load(TBUILDDIR "myns.c", _);
        tassert_eq(myns, "int myns_foo(void) { return 0; }\n");
    }
    return EOK;
}

test$case(test_process_rejects_non_c_target)
{
    e$ret(io.file.save(TBUILDDIR "some.h", "// header only\n"));
    char* argv[] = { "process", TBUILDDIR "some.h" };
    tassert_er(Error.argument, cexy.cmd.process(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_process_missing_header_exact)
{
    e$ret(io.file.save(TBUILDDIR "nohdr2.c", "int x(void) { return 0; }\n"));
    char* argv[] = { "process", TBUILDDIR "nohdr2.c" };
    tassert_er(Error.not_found, cexy.cmd.process(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_process_parse_error)
{
    e$ret(io.file.save(TBUILDDIR "bad.c", "#define 123\n"));
    e$ret(io.file.save(TBUILDDIR "bad.h", "// header\n"));
    char* argv[] = { "process", TBUILDDIR "bad.c" };
    tassert_er(Error.integrity, cexy.cmd.process(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_process_gen_prefix_and_multiline_docs)
{
    mem$scope(tmem$, _)
    {
        sbuf_c ret = sbuf.create(32, _);
        sbuf_c args = sbuf.create(32, _);
        e$ret(sbuf.append(&ret, "int"));
        e$ret(sbuf.append(&args, "void"));

        cex_decl_s d = {
            .name = str$s("cex_src_foo"),
            .docs = str$s("/// @brief first\n///second\n* fourth\n"),
            .ret_type = ret,
            .args = args,
            .type = CexTkn__func_def,
        };
        arr$(cex_decl_s*) decls = arr$new(decls, _);
        arr$push(decls, &d);

        sbuf_c out = sbuf.create(1024, _);
        tassert_er(EOK, _cexy__process_gen_struct(str$s("src"), decls, &out));
        tassert(str.find(out, "(*foo)(void)") != NULL);
        tassert(str.find(out, "/// first") != NULL);
        tassert(str.find(out, "/// second") != NULL);
        tassert(str.find(out, "/// fourth") != NULL);

        sbuf_c out2 = sbuf.create(1024, _);
        tassert_er(EOK, _cexy__process_gen_var_def(str$s("src"), decls, &out2));
        tassert(str.find(out2, ".foo = cex_src_foo,") != NULL);
    }
    return EOK;
}

test$case(test_decl_comparator_subnamespace_order)
{
    cex_decl_s a = { .name = str$s("src_foo") };
    cex_decl_s b = { .name = str$s("src__bar__baz") };
    cex_decl_s* pa = &a;
    cex_decl_s* pb = &b;
    tassert_eq(_cexy__decl_comparator(&pa, &pb), -1);
    tassert_eq(_cexy__decl_comparator(&pb, &pa), 1);
    return EOK;
}

test$case(test_process_update_code_module_then_tokens)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "upd2.c";
        e$ret(io.file.save(
            src,
            "CEX_NAMESPACE_DEF struct __cex_namespace__upd2 upd2 = {\n};\n"
            "#include \"foo.h\"\n"
            "int x(void) { return 0; }\n"
        ));
        sbuf_c st = sbuf.create(64, _);
        sbuf_c vd = sbuf.create(64, _);
        sbuf_c cdef = sbuf.create(64, _);
        e$ret(sbuf.append(&cdef, "CEX_NAMESPACE_DEF struct __cex_namespace__upd2 upd2 = {};"));
        tassert_er(EOK, _cexy__process_update_code(src, true, st, vd, cdef));
    }
    return EOK;
}

test$case(test_process_update_code_module_then_func)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "upd3.c";
        e$ret(io.file.save(
            src,
            "CEX_NAMESPACE_DEF struct __cex_namespace__upd3 upd3 = {\n};\n"
            "int x(void) { return 0; }\n"
        ));
        sbuf_c st = sbuf.create(64, _);
        sbuf_c vd = sbuf.create(64, _);
        sbuf_c cdef = sbuf.create(64, _);
        e$ret(sbuf.append(&cdef, "CEX_NAMESPACE_DEF struct __cex_namespace__upd3 upd3 = {};"));
        tassert_er(EOK, _cexy__process_update_code(src, true, st, vd, cdef));
    }
    return EOK;
}

test$case(test_process_update_code_preproc)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "upd.c";
        e$ret(io.file.save(src, "#include \"foo.h\"\nint x(void) { return 0; }\n"));

        sbuf_c st = sbuf.create(64, _);
        sbuf_c vd = sbuf.create(64, _);
        sbuf_c cdef = sbuf.create(64, _);
        e$ret(sbuf.append(&cdef, "CEX_NAMESPACE_DEF struct __cex_namespace__upd upd = {};"));

        tassert_er(EOK, _cexy__process_update_code(src, true, st, vd, cdef));
        char* content = io.file.load(src, _);
        tassert(content != NULL);
        tassert(str.find(content, "#include \"foo.h\"") != NULL);
    }
    return EOK;
}

#endif // #if !defined(__EMSCRIPTEN__)

test$main();
