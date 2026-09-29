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

test$case(test_coverage_parse_gcov)
{
    char* content = "        -:    0:Source:src/foo.c\n"
                    "        -:    0:Runs:1\n"
                    "        -:    1:int main(void) {\n"
                    "        1:    2:  return 0;\n"
                    "    #####:    3:  return 1;\n"
                    "        -:    4:}\n";
    mem$scope(tmem$, _)
    {
        char* src_path = NULL;
        arr$(u32) lines = arr$new(lines, _);
        u32 total = 0;
        e$ret(_coverage__parse_gcov(content, &src_path, &lines, &total, _));

        tassert_eq(src_path, "src/foo.c");
        tassert_eq(total, 2);
        tassert_eq(arr$len(lines), 1);
        tassert_eq(lines[0], 2);
    }
    return EOK;
}

test$case(test_coverage_parse_gcov_no_source)
{
    mem$scope(tmem$, _)
    {
        char* src_path = NULL;
        arr$(u32) lines = arr$new(lines, _);
        u32 total = 0;
        tassert_er(
            Error.integrity,
            _coverage__parse_gcov("        1:    1:int x;\n", &src_path, &lines, &total, _)
        );
    }
    return EOK;
}

test$case(test_coverage_merge_lines)
{
    mem$scope(tmem$, _)
    {
        arr$(u32) dst = arr$new(dst, _);
        arr$pushm(dst, 1, 3, 5);
        arr$(u32) src = arr$new(src, _);
        arr$pushm(src, 2, 3, 6);
        _coverage__merge_lines(&dst, src, _);

        tassert_eq(arr$len(dst), 5);
        u32 expected[] = { 1, 2, 3, 5, 6 };
        tassert_eq_arr(dst, expected);
    }
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

#if !defined(__clang__) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)

test$case(test_coverage_run_report_clean)
{
    mem$scope(tmem$, _)
    {
        char* cc[] = { cexy$cc };
        if (str.find(cc[0], "clang")) { return EOK; }

        char* src = TBUILDDIR "test_cov_tmp.c";
        e$ret(io.file.save(src, "int main(void) { return 0; }\n"));

        char* test_target = cexy.target_make(src, cexy$build_dir, ".test", _);
        char* gcno_glob = str.fmt(_, "%s-*.gcno", test_target);
        char* gcda_glob = str.fmt(_, "%s-*.gcda", test_target);

        e$ret(coverage.run(src));
        tassert(arr$len(os.fs.find(gcno_glob, false, _)) > 0);
        tassert(arr$len(os.fs.find(gcda_glob, false, _)) > 0);

        e$ret(coverage.report(src));

        e$ret(coverage.clean(src));
        tassert(arr$len(os.fs.find(gcno_glob, false, _)) == 0);
        tassert(arr$len(os.fs.find(gcda_glob, false, _)) == 0);
    }
    return EOK;
}

#endif // #if !defined(__clang__) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)

test$main();
