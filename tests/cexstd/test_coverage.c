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

test$case(test_coverage_format_text_line_report)
{
    char* info = "TN:\n"
                 "SF:src/a.c\n"
                 "DA:1,1\n"
                 "DA:2,0\n"
                 "DA:3,0\n"
                 "DA:5,2\n"
                 "LF:4\n"
                 "LH:2\n"
                 "FNF:1\n"
                 "FNH:1\n"
                 "end_of_record\n";
    char* report = _coverage__format_text(info, NULL, test$alloc);
    tassert_eq(
        report,
        "src/a.c  50.0% lines (2/4)  100.0% funcs (1/1)\n"
        "Total: 50.0% lines (2/4)  100.0% funcs (1/1)\n"
    );
    return EOK;
}

test$case(test_coverage_format_text_derives_counts_without_totals)
{
    char* info = "SF:src/b.c\n"
                 "DA:10,0\n"
                 "DA:11,1\n"
                 "DA:12,0\n"
                 "FNA:0,1,foo\n"
                 "FNA:1,0,bar\n"
                 "end_of_record\n";
    char* report = _coverage__format_text(info, NULL, test$alloc);
    tassert_eq(
        report,
        "src/b.c  33.3% lines (1/3)  50.0% funcs (1/2)\n"
        "Total: 33.3% lines (1/3)  50.0% funcs (1/2)\n"
    );
    return EOK;
}

test$case(test_coverage_format_text_skips_empty)
{
    char* info = "SF:src/c.c\n"
                 "DA:1,0\n"
                 "DA:2,0\n"
                 "DA:3,0\n"
                 "DA:7,0\n"
                 "DA:8,1\n"
                 "DA:20,0\n"
                 "LF:6\n"
                 "LH:1\n"
                 "FNDA:3,foo\n"
                 "FNDA:0,bar\n"
                 "end_of_record\n"
                 "SF:src/empty.h\n"
                 "end_of_record\n";
    char* report = _coverage__format_text(info, NULL, test$alloc);
    tassert_eq(
        report,
        "src/c.c  16.7% lines (1/6)  50.0% funcs (1/2)\n"
        "Total: 16.7% lines (1/6)  50.0% funcs (1/2)\n"
    );
    return EOK;
}

test$case(test_coverage_format_text_file_filter)
{
    char* info = "SF:src/a.c\n"
                 "DA:1,1\n"
                 "DA:2,0\n"
                 "LF:2\n"
                 "LH:1\n"
                 "FNF:1\n"
                 "FNH:1\n"
                 "end_of_record\n"
                 "SF:src/b.c\n"
                 "DA:1,1\n"
                 "LF:1\n"
                 "LH:1\n"
                 "FNF:1\n"
                 "FNH:1\n"
                 "end_of_record\n";
    char* report = _coverage__format_text(info, "src/a.c", test$alloc);
    tassert_eq(
        report,
        "src/a.c  50.0% lines (1/2)  100.0% funcs (1/1)\n"
        "Total: 50.0% lines (1/2)  100.0% funcs (1/1)\n"
    );
    return EOK;
}

test$case(test_coverage_format_text_file_filter_glob)
{
    char* info = "SF:src/a.c\n"
                 "DA:1,1\n"
                 "DA:2,0\n"
                 "LF:2\n"
                 "LH:1\n"
                 "FNF:1\n"
                 "FNH:1\n"
                 "end_of_record\n"
                 "SF:src/b.c\n"
                 "DA:1,1\n"
                 "LF:1\n"
                 "LH:1\n"
                 "FNF:1\n"
                 "FNH:1\n"
                 "end_of_record\n";
    char* report = _coverage__format_text(info, "src/*.c", test$alloc);
    tassert_eq(
        report,
        "src/a.c  50.0% lines (1/2)  100.0% funcs (1/1)\n"
        "src/b.c  100.0% lines (1/1)  100.0% funcs (1/1)\n"
        "Total: 66.7% lines (2/3)  100.0% funcs (2/2)\n"
    );
    return EOK;
}

test$case(test_coverage_format_json)
{
    char* info = "SF:src/a.c\n"
                 "DA:1,1\n"
                 "DA:2,0\n"
                 "DA:3,0\n"
                 "FNA:0,1,foo\n"
                 "FNA:1,0,bar\n"
                 "FNDA:0,baz\n"
                 "end_of_record\n";
    char* report = _coverage__format_json(info, NULL, test$alloc);
    tassert_eq(
        report,
        "{\"total\":{\"lines_hit\":1,\"lines_found\":3,\"funcs_hit\":1,\"funcs_found\":3},"
        "\"files\":[{\"path\":\"src/a.c\",\"lines_hit\":1,\"lines_found\":3,\"funcs_hit\":1,"
        "\"funcs_found\":3,\"missed_lines\":[2,3],\"uncovered_funcs\":[\"bar\",\"baz\"]}]}\n"
    );
    return EOK;
}

test$case(test_coverage_format_json_fully_uncovered)
{
    char* info = "SF:src/z.c\n"
                 "DA:1,0\n"
                 "DA:2,0\n"
                 "FNA:0,0,foo\n"
                 "end_of_record\n";
    char* report = _coverage__format_json(info, NULL, test$alloc);
    tassert_eq(
        report,
        "{\"total\":{\"lines_hit\":0,\"lines_found\":2,\"funcs_hit\":0,\"funcs_found\":1},"
        "\"files\":[{\"path\":\"src/z.c\",\"lines_hit\":0,\"lines_found\":2,\"funcs_hit\":0,"
        "\"funcs_found\":1,\"fully_uncovered\":true,\"uncovered_funcs\":[\"foo\"]}]}\n"
    );
    return EOK;
}

test$case(test_coverage_format_json_file_filter)
{
    char* info = "SF:src/a.c\n"
                 "DA:1,1\n"
                 "LF:1\n"
                 "LH:1\n"
                 "end_of_record\n"
                 "SF:src/b.c\n"
                 "DA:1,1\n"
                 "DA:2,0\n"
                 "LF:2\n"
                 "LH:1\n"
                 "end_of_record\n";
    char* report = _coverage__format_json(info, "src/b.c", test$alloc);
    tassert_eq(
        report,
        "{\"total\":{\"lines_hit\":1,\"lines_found\":2,\"funcs_hit\":0,\"funcs_found\":0},"
        "\"files\":[{\"path\":\"src/b.c\",\"lines_hit\":1,\"lines_found\":2,\"funcs_hit\":0,"
        "\"funcs_found\":0,\"missed_lines\":[2],\"uncovered_funcs\":[]}]}\n"
    );
    return EOK;
}

test$case(test_coverage_cmd_format_invalid)
{
    char* argv[] = { "coverage", "--format=bogus", "report", "all" };
    tassert_er(Error.argument, coverage.cmd(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_coverage_cmd_file_html_rejected)
{
    char* argv[] = { "coverage", "--format=html", "--file", "src/a.c", "report", "all" };
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

        char* content = io.file.load(info, _);
        tassert(content != NULL);
        u32 sf_count = 0;
        for$each (line, str.split_lines(content, _)) {
            if (!str.starts_with(line, "SF:")) { continue; }
            sf_count++;
            tassert(line[3] != '/');
        }
        tassert(sf_count > 0);

        e$ret(coverage.report("lcov", "text", NULL, NULL, src));
        e$ret(coverage.report("lcov", "json", NULL, NULL, src));

        e$ret(coverage.clean(src));
        char* gcda_glob = str.fmt(_, "%s/*.gcda", cexy$build_dir);
        tassert(arr$len(os.fs.find(gcda_glob, true, _)) == 0);
    }
    return EOK;
}

#endif // #if !defined(_WIN32) && !defined(__EMSCRIPTEN__)

test$main();
