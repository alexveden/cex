#define TBUILDDIR "tests/build/coverage/"
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#define cexy$build_dir TBUILDDIR
#define cexy$disable_cex_precompiling
#include "src/all.c"
#include "cexstd/testing/coverage/coverage.c"

test$setup_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    e$assert(!os.path.exists(TBUILDDIR) && "must not exist!");
    e$ret(os.fs.mkpath(TBUILDDIR));
    return EOK;
}

test$teardown_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    return EOK;
}

test$case(test_coverage_cmd_invalid)
{
    char* argv[] = { "coverage", "bogus", "all" };
    tassert_er(Error.argsparse, coverage.cmd(arr$len(argv), argv, NULL));

    char* argv_empty[] = { "coverage" };
    tassert_er(Error.argsparse, coverage.cmd(arr$len(argv_empty), argv_empty, NULL));
    return EOK;
}

test$case(test_coverage_cmd_engine_invalid)
{
    char* argv[] = { "coverage", "--engine", "bogus", "report", "all" };
    tassert_er(Error.argsparse, coverage.cmd(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_coverage_cmd_engine_llvm_requires_clang)
{
    if (_coverage__is_clang()) { return EOK; }

    char* argv[] = { "coverage", "--engine", "llvm", "report", "all" };
    tassert_er(Error.argument, coverage.cmd(arr$len(argv), argv, NULL));
    return EOK;
}

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)

test$case(test_coverage_lcov_run_export_report_clean)
{
    if (!os.cmd.exists("lcov") || !os.cmd.exists("genhtml")) { return EOK; }

    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "test_cov_tmp.c";
        e$ret(io.file.save(src, "int main(void) { return 0; }\n"));

        e$ret(coverage.run("lcov", src));

        char* info = TBUILDDIR "cov.info";
        e$ret(coverage.export("lcov", info, src));
        tassert(os.path.exists(info));

        e$ret(coverage.report("lcov", "text", NULL, src));

        e$ret(coverage.clean(src));
        char* gcda_glob = str.fmt(_, "%s/*.gcda", cexy$build_dir);
        tassert(arr$len(os.fs.find(gcda_glob, true, _)) == 0);
    }
    return EOK;
}

#endif // #if !defined(_WIN32) && !defined(__EMSCRIPTEN__)

test$main();
