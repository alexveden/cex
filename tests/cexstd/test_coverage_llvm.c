#define TBUILDDIR "tests/build/coverage_llvm/"
#define cexy$cc "clang"
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

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)

test$case(test_coverage_llvm_engine_detected)
{
    tassert(_coverage__is_clang());

    char* argv[] = { "coverage", "--engine=llvm", "clean", "all" };
    e$ret(coverage.cmd(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_coverage_llvm_run_export_report_clean)
{
    if (!os.cmd.exists("clang") || !os.cmd.exists("llvm-cov") ||
        !os.cmd.exists("llvm-profdata")) {
        return EOK;
    }

    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "test_cov_tmp.c";
        e$ret(io.file.save(src, "int main(void) { return 0; }\n"));

        e$ret(coverage.run("llvm", src));

        char* profraw_glob = str.fmt(_, "%s/*.profraw", cexy$build_dir);
        tassert(arr$len(os.fs.find(profraw_glob, true, _)) > 0);

        char* info = TBUILDDIR "cov.info";
        e$ret(coverage.export("llvm", info, src));
        tassert(os.path.exists(info));

        char* content = io.file.load(info, _);
        tassert(content != NULL);
        u32 sf_count = 0;
        for$each (line, str.split_lines(content, _)) {
            if (!str.starts_with(line, "SF:")) { continue; }
            sf_count++;
            tassert(line[3] != '/');
        }
        tassert(sf_count > 0);

        e$ret(coverage.report("llvm", "text", NULL, src));

        e$ret(coverage.clean(src));
        tassert(arr$len(os.fs.find(profraw_glob, true, _)) == 0);
    }
    return EOK;
}

#endif // #if !defined(_WIN32) && !defined(__EMSCRIPTEN__)

test$main();
