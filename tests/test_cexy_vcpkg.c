#define TBUILDDIR "tests/build/cexyvcpkgtest/"
#define CEX_LOG_LVL 5
#define cexy$vcpkg_root TBUILDDIR "vcpkg"
#define cexy$vcpkg_triplet "x64-test"
#define cexy$pkgconf_libs "cextest"
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#define cexy$build_dir TBUILDDIR
#include "src/all.c"

#define VCPKG_BASE TBUILDDIR "vcpkg/installed/x64-test"

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

static Exception
test_vcpkg_make_lib_tree(void)
{
    e$ret(os.fs.mkpath(VCPKG_BASE "/lib/pkgconfig/"));
    e$ret(os.fs.mkpath(VCPKG_BASE "/include/"));
    e$ret(io.file.save(VCPKG_BASE "/lib/libcextest.a", "dummy"));
    return EOK;
}

test$case(test_pkgconf_vcpkg_missing_paths)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) out = arr$new(out, _);

        // root missing
        if (os.fs.remove_tree(TBUILDDIR "vcpkg")) {};
        tassert_er(Error.not_found, cexy$pkgconf(_, &out, "--cflags", "cextest"));

        // triplet missing
        e$ret(os.fs.mkpath(TBUILDDIR "vcpkg/installed/"));
        tassert_er(Error.not_found, cexy$pkgconf(_, &out, "--cflags", "cextest"));

        // lib path missing
        e$ret(os.fs.mkpath(VCPKG_BASE "/"));
        tassert_er(Error.not_found, cexy$pkgconf(_, &out, "--cflags", "cextest"));

        // include path missing
        e$ret(os.fs.mkpath(VCPKG_BASE "/lib/"));
        tassert_er(Error.not_found, cexy$pkgconf(_, &out, "--cflags", "cextest"));

        // pkgconfig path missing
        e$ret(os.fs.mkpath(VCPKG_BASE "/include/"));
        tassert_er(Error.not_found, cexy$pkgconf(_, &out, "--cflags", "cextest"));
    }
    return EOK;
}

test$case(test_pkgconf_vcpkg_lib_not_found)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) out = arr$new(out, _);
        e$ret(test_vcpkg_make_lib_tree());
        tassert_er(Error.not_found, cexy$pkgconf(_, &out, "--cflags", "no_such_lib"));
    }
    return EOK;
}

test$case(test_pkgconf_vcpkg_trace_and_tool_error)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) out = arr$new(out, _);
        e$ret(test_vcpkg_make_lib_tree());
        // libs with a space and an empty arg exercise the trace-print branches
        e$ret(io.file.save(VCPKG_BASE "/lib/foo bar.a", "dummy"));
        e$ret(io.file.save(VCPKG_BASE "/lib/.a", "dummy"));

        char* old = os.env.get("PKG_CONFIG_PATH", NULL);
        if (old) { old = str.clone(old, _); }
        e$ret(os.fs.mkpath(TBUILDDIR "emptypc/"));
        e$ret(os.env.set("PKG_CONFIG_PATH", TBUILDDIR "emptypc/"));

        tassert_er(
            Error.runtime,
            cexy$pkgconf(_, &out, "--cflags", "foo bar", "", "cextest")
        );

        if (old) {
            e$ret(os.env.set("PKG_CONFIG_PATH", old));
        } else {
            e$ret(os.env.unset("PKG_CONFIG_PATH"));
        }
    }
    return EOK;
}

test$case(test_pkgconf_vcpkg_system_ok)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) out = arr$new(out, _);
        e$ret(test_vcpkg_make_lib_tree());

        // the vcpkg branch points PKG_CONFIG_LIBDIR at the triplet pkgconfig dir
        e$ret(io.file.save(
            VCPKG_BASE "/lib/pkgconfig/cextest.pc",
            "prefix=/usr\n"
            "Name: cextest\n"
            "Description: test\n"
            "Version: 1.0\n"
            "Cflags: -I/foo/include\n"
            "Libs:\n"
        ));

        tassert_er(EOK, cexy$pkgconf(_, &out, "--cflags", "cextest"));
        tassert(arr$len(out) > 0);
    }
    return EOK;
}

test$case(test_config_vcpkg_pkgconf_error)
{
    // vcpkg lib present but no .pc file -> pkgconf fails with vcpkg configured
    e$ret(test_vcpkg_make_lib_tree());
    char* argv[] = { "config" };
    tassert_er(Error.runtime, cexy.cmd.config(arr$len(argv), argv, NULL));
    return EOK;
}

#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
