#define TBUILDDIR "tests/build/cexyconfigtest/"
#define CEX_LOG_LVL 4
#define cexy$pkgconf_libs "cextest"
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#define cexy$build_dir TBUILDDIR
#define cexy$src_dir TBUILDDIR "src/"
#include "src/all.c"

test$setup_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    e$ret(os.fs.mkpath(TBUILDDIR));
    return EOK;
}
test$teardown_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    return EOK;
}

#if !defined(__EMSCRIPTEN__)

static bool
test_cfg_cmd_exists_false(char* cmd_exe)
{
    (void)cmd_exe;
    return false;
}

static OSPlatform_e
test_cfg_platform_win(void)
{
    return OSPlatform__win;
}

static OSPlatform_e
test_cfg_platform_macos(void)
{
    return OSPlatform__macos;
}

static OSPlatform_e
test_cfg_platform_linux(void)
{
    return OSPlatform__linux;
}

static Exception
test_cfg_set_pkgconf_path(char* pc_dir)
{
    if (os.fs.remove_tree(pc_dir)) {};
    e$ret(os.fs.mkpath(pc_dir));
    e$ret(io.file.save(
        os$path_join(tmem$, pc_dir, "cextest.pc"),
        "prefix=/usr\n"
        "Name: cextest\n"
        "Description: test\n"
        "Version: 1.0\n"
        "Cflags: -I/foo/include\n"
        "Libs:\n"
    ));
    return os.env.set("PKG_CONFIG_PATH", pc_dir);
}

test$case(test_config_pkgconf_missing_platform_switch)
{
    mem$scope(tmem$, _)
    {
        char* old = os.env.get("PKG_CONFIG_PATH", NULL);
        if (old) { old = str.clone(old, _); }

        e$ret(test_cfg_set_pkgconf_path(TBUILDDIR "pc/"));

        char* argv[] = { "config" };
        test$mock_scope(os) {
            os.cmd.exists = test_cfg_cmd_exists_false;
            os.platform.current = test_cfg_platform_win;
            tassert_er(EOK, cexy.cmd.config(arr$len(argv), argv, NULL));
        }
        test$mock_scope(os) {
            os.cmd.exists = test_cfg_cmd_exists_false;
            os.platform.current = test_cfg_platform_macos;
            tassert_er(EOK, cexy.cmd.config(arr$len(argv), argv, NULL));
        }
        test$mock_scope(os) {
            os.cmd.exists = test_cfg_cmd_exists_false;
            os.platform.current = test_cfg_platform_linux;
            tassert_er(EOK, cexy.cmd.config(arr$len(argv), argv, NULL));
        }

        if (old) {
            e$ret(os.env.set("PKG_CONFIG_PATH", old));
        } else {
            e$ret(os.env.unset("PKG_CONFIG_PATH"));
        }
    }
    return EOK;
}

test$case(test_config_pkgconf_error)
{
    mem$scope(tmem$, _)
    {
        char* old = os.env.get("PKG_CONFIG_PATH", NULL);
        if (old) { old = str.clone(old, _); }

        e$ret(os.fs.mkpath(TBUILDDIR "emptypc/"));
        e$ret(os.env.set("PKG_CONFIG_PATH", TBUILDDIR "emptypc/"));

        char* argv[] = { "config" };
        tassert_er(Error.runtime, cexy.cmd.config(arr$len(argv), argv, NULL));

        if (old) {
            e$ret(os.env.set("PKG_CONFIG_PATH", old));
        } else {
            e$ret(os.env.unset("PKG_CONFIG_PATH"));
        }
    }
    return EOK;
}

test$case(test_simple_app_pkgconf_branches)
{
    mem$scope(tmem$, _)
    {
        char* old = os.env.get("PKG_CONFIG_PATH", NULL);
        if (old) { old = str.clone(old, _); }
        e$ret(test_cfg_set_pkgconf_path(TBUILDDIR "pc/"));
        e$ret(os.fs.mkpath(cexy$src_dir "myapp/myapp.c"));
        e$ret(io.file.save(
            cexy$src_dir "myapp/myapp.c",
            "#include \"cex.h\"\n"
            "Exception myapp(int argc, char** argv) { (void)argc; (void)argv; return EOK; }\n"
        ));
        e$ret(os.fs.mkpath(cexy$src_dir "myapp/main.c"));
        e$ret(io.file.save(
            cexy$src_dir "myapp/main.c",
            "#define CEX_IMPLEMENTATION\n#include \"cex.h\"\n#include \"myapp.c\"\n"
            "int main(int argc, char** argv) { return myapp(argc, argv) != EOK; }\n"
        ));

        char* build[] = { "app", "build", "myapp" };
        tassert_er(EOK, cexy.cmd.simple_app(arr$len(build), build, NULL));

        if (old) {
            e$ret(os.env.set("PKG_CONFIG_PATH", old));
        } else {
            e$ret(os.env.unset("PKG_CONFIG_PATH"));
        }
    }
    return EOK;
}

test$case(test_make_compile_flags_with_pkgconf)
{
    mem$scope(tmem$, _)
    {
        char* old = os.env.get("PKG_CONFIG_PATH", NULL);
        if (old) { old = str.clone(old, _); }
        e$ret(test_cfg_set_pkgconf_path(TBUILDDIR "pc/"));

        char* flags_file = TBUILDDIR "compile_flags.txt";
        tassert_er(EOK, cexy.utils.make_compile_flags(flags_file, true, NULL));
        char* content = io.file.load(flags_file, _);
        tassert(content != NULL);
        tassert(str.find(content, "-I/foo/include") != NULL);

        if (old) {
            e$ret(os.env.set("PKG_CONFIG_PATH", old));
        } else {
            e$ret(os.env.unset("PKG_CONFIG_PATH"));
        }
    }
    return EOK;
}

test$case(test_simple_test_pkgconf_build)
{
    mem$scope(tmem$, _)
    {
        char* old = os.env.get("PKG_CONFIG_PATH", NULL);
        if (old) { old = str.clone(old, _); }
        e$ret(test_cfg_set_pkgconf_path(TBUILDDIR "pc/"));

        char* src = TBUILDDIR "test_pkgcfg.c";
        e$ret(io.file.save(
            src,
            "#define CEX_IMPLEMENTATION\n"
            "#define CEX_TEST\n"
            "#include \"cex.h\"\n"
            "test$case(ok) { tassert_eq(1, 1); return EOK; }\n"
            "test$main();\n"
        ));
        char* argv[] = { "test", "build", src };
        tassert_er(EOK, cexy.cmd.simple_test(arr$len(argv), argv, NULL));

        if (old) {
            e$ret(os.env.set("PKG_CONFIG_PATH", old));
        } else {
            e$ret(os.env.unset("PKG_CONFIG_PATH"));
        }
    }
    return EOK;
}

#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
