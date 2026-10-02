#define TBUILDDIR "tests/build/cexyapptest/"
#define CEX_LOG_LVL 4
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#define cexy$build_dir TBUILDDIR "build/"
#define cexy$src_dir TBUILDDIR "src/"
#include "src/all.c"

test$setup_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    e$ret(os.fs.mkpath(TBUILDDIR "placeholder"));
    e$ret(os.fs.mkpath(cexy$build_dir "placeholder"));
    e$ret(os.fs.mkpath(cexy$src_dir "placeholder"));
    return EOK;
}
test$teardown_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    return EOK;
}

#if !defined(__EMSCRIPTEN__)

test$case(test_fuzz_create)
{
    tassert_er(Error.argument, cexy.fuzz.create(TBUILDDIR "fuzz/not_prefixed.c"));

    char* target = TBUILDDIR "fuzz/fuzz_my.c";
    tassert_er(EOK, cexy.fuzz.create(target));
    tassert(os.path.exists(target));
    tassert_er(Error.exists, cexy.fuzz.create(target));

    mem$scope(tmem$, _)
    {
        char* content = io.file.load(target, _);
        tassert(content != NULL);
        tassert(str.find(content, "fuzz$main();") != NULL);
        tassert(str.find(content, "fuzz$case") != NULL);
    }
    return EOK;
}

test$case(test_app_find_target_src)
{
    mem$scope(tmem$, _)
    {
        char* out = NULL;
        tassert_er(Error.assert, cexy.app.find_app_target_src(_, "foo", NULL));
        tassert_er(Error.argsparse, cexy.app.find_app_target_src(_, NULL, &out));
        tassert_er(Error.argsparse, cexy.app.find_app_target_src(_, "all", &out));
        tassert_er(Error.argsparse, cexy.app.find_app_target_src(_, "foo*", &out));
        tassert_er(Error.argsparse, cexy.app.find_app_target_src(_, "foo/bar", &out));
        tassert_er(Error.not_found, cexy.app.find_app_target_src(_, "nope", &out));

        // <src_dir>/<name>.c form
        e$ret(os.fs.mkpath(cexy$src_dir "flat.c"));
        e$ret(io.file.save(cexy$src_dir "flat.c", "int main(void) { return 0; }\n"));
        tassert_er(EOK, cexy.app.find_app_target_src(_, "flat", &out));
        tassert(out != NULL);
    }
    return EOK;
}

test$case(test_app_create_clean)
{
    mem$scope(tmem$, _)
    {
        char* app_src = NULL;

        tassert_er(EOK, cexy.app.create("myapp"));
        tassert(os.path.exists(cexy$src_dir "myapp/myapp.c"));
        tassert(os.path.exists(cexy$src_dir "myapp/main.c"));
        tassert_er(Error.exists, cexy.app.create("myapp"));

        e$ret(cexy.app.find_app_target_src(_, "myapp", &app_src));
        char* app_exe = cexy.target_make(app_src, cexy$build_dir, "myapp", _);
        e$ret(io.file.save(app_exe, "dummy"));
        tassert(os.path.exists(app_exe));

        tassert_er(EOK, cexy.app.clean("myapp"));
        tassert(!os.path.exists(app_exe));
    }
    return EOK;
}

test$case(test_cmd_new)
{
    char* noarg[] = { "new" };
    tassert_er(Error.argsparse, cexy.cmd.new(arr$len(noarg), noarg, NULL));

    char* ok[] = { "new", TBUILDDIR "proj" };
    tassert_er(EOK, cexy.cmd.new(arr$len(ok), ok, NULL));
    tassert(os.path.exists(TBUILDDIR "proj/cex.c"));
    tassert(os.path.exists(TBUILDDIR "proj/tests/test_mylib.c"));
    return EOK;
}

test$case(test_cmd_simple_app)
{
    char* invalid[] = { "app", "bogus", "myapp" };
    tassert_er(Error.argsparse, cexy.cmd.simple_app(arr$len(invalid), invalid, NULL));

    char* create[] = { "app", "create", "myapp" };
    tassert_er(EOK, cexy.cmd.simple_app(arr$len(create), create, NULL));
    tassert(os.path.exists(cexy$src_dir "myapp/myapp.c"));
    tassert_er(Error.exists, cexy.cmd.simple_app(arr$len(create), create, NULL));

    char* build[] = { "app", "build", "myapp" };
    tassert_er(EOK, cexy.cmd.simple_app(arr$len(build), build, NULL));

    // second run reuses the built executable
    char* run[] = { "app", "run", "myapp" };
    tassert_er(EOK, cexy.cmd.simple_app(arr$len(run), run, NULL));

    char* clean[] = { "app", "clean", "myapp" };
    tassert_er(EOK, cexy.cmd.simple_app(arr$len(clean), clean, NULL));
    return EOK;
}

test$case(test_cmd_simple_fuzz)
{
    mem$scope(tmem$, _)
    {
        char* no_src[] = { "fuzz", "run" };
        tassert_er(Error.argsparse, cexy.cmd.simple_fuzz(arr$len(no_src), no_src, NULL));

        char* bad_cmd[] = { "fuzz", "bogus", "x.c" };
        tassert_er(Error.argsparse, cexy.cmd.simple_fuzz(arr$len(bad_cmd), bad_cmd, NULL));

        char* missing[] = { "fuzz", "run", TBUILDDIR "fuzz/fuzz_nope.c" };
        tassert_er(Error.not_found, cexy.cmd.simple_fuzz(arr$len(missing), missing, NULL));

        char* create[] = { "fuzz", "create", TBUILDDIR "fuzz/fuzz_my.c" };
        tassert_er(EOK, cexy.cmd.simple_fuzz(arr$len(create), create, NULL));
        tassert(os.path.exists(TBUILDDIR "fuzz/fuzz_my.c"));

        // the template traps on `CEX`, use a non-crashing target for the run path
#if !defined(__APPLE__) && !defined(_WIN32)
        char* safe_src = TBUILDDIR "fuzz/fuzz_safe.c";
        e$ret(io.file.save(
            safe_src,
            "#define CEX_IMPLEMENTATION\n"
            "#include \"cex.h\"\n"
            "int fuzz$case(const u8* data, usize size) { (void)data; (void)size; return 0; }\n"
            "fuzz$main();\n"
        ));

        char* old_dir = os.fs.getcwd(_);
        char* run[] = { "fuzz", "--max-time", "1", "run", safe_src };
        tassert_er(EOK, cexy.cmd.simple_fuzz(arr$len(run), run, NULL));
        e$ret(os.fs.chdir(old_dir));
#endif
    }
    return EOK;
}

test$case(test_add_precompiled_debug_cex_h)
{
    mem$scope(tmem$, _)
    {
        // optimization is on -> skip fast precompiled path
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "cc", "-g", "-Wall", "-O2");
        tassert_er(EOK, _cexy__add_precompiled_debug_cex_h(&args, _));
        tassert_eq((int)arr$len(args), 4);

        // passing a .c file in cc args is rejected
        arr$clear(args);
        arr$pushm(args, "cc", "-g", "-Wall", "foo.c");
        tassert_er(Error.argument, _cexy__add_precompiled_debug_cex_h(&args, _));
    }
    return EOK;
}

#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
