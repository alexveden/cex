#pragma once
#include "coverage.h"

#if defined(CEX_BUILD) || defined(CEX_NEW)

typedef struct coverage_stat_s
{
    char* path;
    u32 total;
    arr$(u32) exec_lines;
} coverage_stat_s;

static int
_coverage__cmp_stats(const void* a, const void* b)
{
    coverage_stat_s* x = (coverage_stat_s*)a;
    coverage_stat_s* y = (coverage_stat_s*)b;
    u32 xr = x->total ? (u32)((u64)arr$len(x->exec_lines) * 100000 / x->total) : 0;
    u32 yr = y->total ? (u32)((u64)arr$len(y->exec_lines) * 100000 / y->total) : 0;
    if (xr != yr) { return xr < yr ? -1 : 1; }
    return strcmp(x->path, y->path);
}

static void
_coverage__merge_lines(arr$(u32)* dst, arr$(u32) src, IAllocator allc)
{
    arr$(u32) res = arr$new(res, allc);
    usize i = 0;
    usize j = 0;
    while (i < arr$len(*dst) || j < arr$len(src)) {
        if (j >= arr$len(src) || (i < arr$len(*dst) && (*dst)[i] < src[j])) {
            arr$push(res, (*dst)[i]);
            i++;
        } else if (i >= arr$len(*dst) || src[j] < (*dst)[i]) {
            arr$push(res, src[j]);
            j++;
        } else {
            arr$push(res, src[j]);
            i++;
            j++;
        }
    }
    *dst = res;
}

static Exception
_coverage__parse_gcov(
    char* content,
    char** out_src_path,
    arr$(u32)* out_lines,
    u32* out_total,
    IAllocator allc
)
{
    uassert(content != NULL);
    uassert(out_src_path != NULL);
    uassert(out_lines != NULL);
    uassert(out_total != NULL);

    *out_src_path = NULL;
    *out_total = 0;

    char* line_p = content;
    while (line_p != NULL && *line_p != '\0') {
        char* nl = strchr(line_p, '\n');
        str_s line = { .buf = line_p, .len = nl ? (usize)(nl - line_p) : strlen(line_p) };

        if (*out_src_path == NULL) {
            isize src_idx = str.slice.index_of(line, str$s("Source:"));
            if (src_idx >= 0) {
                str_s path = str.slice.rstrip(str.slice.sub(line, src_idx + 7, 0));
                *out_src_path = str.slice.clone(path, allc);
            }
        }

        usize i = 0;
        while (i < line.len && (line.buf[i] == ' ' || line.buf[i] == '\t')) { i++; }
        if (i >= line.len || line.buf[i] == '-') { goto next_line; }

        usize j = i;
        while (j < line.len && line.buf[j] != ':') { j++; }
        if (j == i) { goto next_line; }

        bool is_count = false;
        u64 count = 0;
        usize k = i;
        while (k < j && line.buf[k] >= '0' && line.buf[k] <= '9') {
            count = count * 10 + (u64)(line.buf[k] - '0');
            k++;
        }
        if (k > i) {
            if (k < j && line.buf[k] == '*') { k++; }
            if (k == j) { is_count = true; }
        }

        if (is_count) {
            k = j + 1;
            while (k < line.len && line.buf[k] == ' ') { k++; }
            if (k >= line.len || line.buf[k] < '0' || line.buf[k] > '9') { goto next_line; }
            u32 line_no = 0;
            while (k < line.len && line.buf[k] >= '0' && line.buf[k] <= '9') {
                line_no = line_no * 10 + (u32)(line.buf[k] - '0');
                k++;
            }
            if (line_no == 0) { goto next_line; }
            (*out_total)++;
            if (count > 0) { arr$push(*out_lines, line_no); }
        } else if (line.buf[i] == '#') {
            (*out_total)++;
        }

    next_line:
        if (nl == NULL) { break; }
        line_p = nl + 1;
    }

    if (*out_src_path == NULL) { return e$raise(Error.integrity, "gcov source header not found"); }
    return EOK;
}

static Exception
_coverage__make_gcov_backend(arr$(char*)* out)
{
    uassert(out != NULL);

    char* cc[] = { cexy$cc };
    bool is_clang = arr$len(cc) > 0 && str.find(cc[0], "clang") != NULL;
    if (is_clang) {
        if (unlikely(!os.cmd.exists("llvm-cov"))) {
            return e$raise(
                Error.runtime, "llvm-cov not found in PATH, clang coverage needs llvm-cov"
            );
        }
        arr$push(*out, "llvm-cov");
        arr$push(*out, "gcov");
    } else {
        if (unlikely(!os.cmd.exists("gcov"))) {
            return e$raise(Error.runtime, "gcov not found in PATH, gcc coverage needs gcov");
        }
        arr$push(*out, "gcov");
    }
    return EOK;
}

static Exception
_coverage__run_gcov(arr$(char*) backend, char* gcno, char* gcda, IAllocator allc)
{
    arr$(char*) args = arr$new(args, allc);
    arr$pusha(args, backend);
    arr$pushm(args, "-o", gcno, gcda, NULL);

    os_cmd_c cmd = { 0 };
    e$ret(os.cmd.create(&cmd, args, arr$len(args), &(os_cmd_flags_s){ .combine_stdouterr = true }));
    char* output = os.cmd.read_all(&cmd, allc);
    e$except (err, os.cmd.wait(&cmd, 1, 0)) {
        log$error("gcov failed for %s: %s\n", gcda, output);
        return err;
    }
    return EOK;
}

static bool
_coverage__is_project_source(char* path)
{
    if (path == NULL || !str.ends_with(path, ".c")) { return false; }
    if (str.starts_with(path, "./")) { path += 2; }

    char* dirs[] = { cexy$src_dir, "src" };
    for$each (dir, dirs) {
        if (str.starts_with(dir, "./")) { dir += 2; }
        usize dir_len = strlen(dir);
        if (strncmp(path, dir, dir_len) != 0) { continue; }
        if (path[dir_len] == '\0' || path[dir_len] == '/' || path[dir_len] == '\\') { return true; }
    }
    return false;
}

static void
_coverage__merge_stat(
    arr$(coverage_stat_s)* stats,
    char* path,
    arr$(u32) lines,
    u32 total,
    IAllocator allc
)
{
    for$eachp (st, *stats) {
        if (str.eq(st->path, path)) {
            if (total > st->total) { st->total = total; }
            _coverage__merge_lines(&st->exec_lines, lines, allc);
            return;
        }
    }
    arr$push(
        *stats,
        (coverage_stat_s){ .path = str.clone(path, allc), .total = total, .exec_lines = lines }
    );
}

static void
_coverage__remove_gcov_files(void)
{
    mem$scope(tmem$, _)
    {
        for$each (gcov_file, os.fs.find("*.gcov", false, _)) {
            if (os.fs.remove(gcov_file)) {}
        }
    }
}

static Exception
_coverage__find_gcda(char* target, arr$(char*)* out, IAllocator allc)
{
    uassert(out != NULL);
    e$ret(cexy.test.make_target_pattern(&target));

    if (str.ends_with(target, "test_*.c")) {
        *out = os.fs.find(str.fmt(allc, "%s/*.gcda", cexy$build_dir), true, allc);
        return EOK;
    }
    if (unlikely(!os.path.exists(target))) {
        log$error("Test file not found: %s\n", target);
        return e$raise(Error.not_found, "test file not found");
    }
    char* test_target = cexy.target_make(target, cexy$build_dir, ".test", allc);
    if (unlikely(test_target == NULL)) {
        return e$raise(Error.runtime, "failed to make test target");
    }
    *out = os.fs.find(str.fmt(allc, "%s-*.gcda", test_target), false, allc);
    return EOK;
}

/// Builds and runs tests with coverage instrumentation
Exception
coverage_run(char* target)
{
    uassert(target != NULL);
    char* argv[] = { "test", "--coverage", "run", target };
    return cexy.cmd.simple_test(arr$len(argv), argv, NULL);
}

/// Aggregates gcov/llvm-cov data and prints source line coverage
Exception
coverage_report(char* target)
{
    uassert(target != NULL);
    mem$scope(tmem$, _)
    {
        arr$(char*) backend = arr$new(backend, _);
        e$ret(_coverage__make_gcov_backend(&backend));

        arr$(char*) gcda_files = NULL;
        e$ret(_coverage__find_gcda(target, &gcda_files, _));
        if (unlikely(arr$len(gcda_files) == 0)) {
            log$error(
                "No coverage data for '%s', run `./cex coverage run %s` first\n", target, target
            );
            return e$raise(Error.not_found, "no coverage data, run `./cex coverage run` first");
        }
        log$info("Aggregating coverage from %u gcda files\n", arr$len(gcda_files));

        _coverage__remove_gcov_files();

        arr$(coverage_stat_s) stats = arr$new(stats, _);
        for$each (gcda_path, gcda_files) {
            char* gcno_path = str.replace(gcda_path, ".gcda", ".gcno", _);
            e$except (err, _coverage__run_gcov(backend, gcno_path, gcda_path, _)) { continue; }

            for$each (gcov_file, os.fs.find("*.gcov", false, _)) {
                char* content = io.file.load(gcov_file, _);
                if (content == NULL) { continue; }

                char* src_path = NULL;
                arr$(u32) exec_lines = arr$new(exec_lines, _);
                u32 total = 0;
                Exc parse_err = _coverage__parse_gcov(content, &src_path, &exec_lines, &total, _);
                if (parse_err == EOK && _coverage__is_project_source(src_path)) {
                    _coverage__merge_stat(&stats, src_path, exec_lines, total, _);
                }
                if (os.fs.remove(gcov_file)) {}
            }
        }

        if (arr$len(stats) == 0) {
            log$info("No project source coverage collected\n");
            return EOK;
        }

        qsort(stats, arr$len(stats), sizeof(coverage_stat_s), _coverage__cmp_stats);

        u64 t_total = 0;
        u64 t_exec = 0;
        io.printf("\nCode coverage report (line coverage)\n");
        io.printf("%8s %9s %9s  %s\n", "Covered", "Executed", "Total", "File");
        for$eachp (st, stats) {
            u64 executed = arr$len(st->exec_lines);
            f64 pct = st->total ? (f64)executed * 100.0 / st->total : 0.0;
            io.printf("%7.1f%% %9lu %9lu  %s\n", pct, executed, st->total, st->path);
            t_total += st->total;
            t_exec += executed;
        }
        io.printf(
            "\nTotal: %0.1f%% (%lu/%lu lines, %u files)\n",
            t_total ? (f64)t_exec * 100.0 / t_total : 0.0,
            t_exec,
            t_total,
            arr$len(stats)
        );
    }
    return EOK;
}

/// Removes coverage artifacts (.gcno/.gcda/.gcov) for all tests or a single file
Exception
coverage_clean(char* target)
{
    uassert(target != NULL);
    mem$scope(tmem$, _)
    {
        char* original_target = str.clone(target, _);
        e$ret(cexy.test.make_target_pattern(&target));

        if (str.ends_with(target, "test_*.c")) {
            char* patterns[] = { "%s/*.gcno", "%s/*.gcda" };
            for$each (pattern, patterns) {
                for$each (file, os.fs.find(str.fmt(_, pattern, cexy$build_dir), true, _)) {
                    if (os.fs.remove(file)) {}
                }
            }
            _coverage__remove_gcov_files();
        } else {
            if (unlikely(!os.path.exists(target))) {
                log$error("Test file not found: %s\n", target);
                return e$raise(Error.not_found, "test file not found");
            }
            char* test_target = cexy.target_make(target, cexy$build_dir, ".test", _);
            if (unlikely(test_target == NULL)) {
                return e$raise(Error.runtime, "failed to make test target");
            }
            char* exts[] = { ".gcno", ".gcda" };
            for$each (ext, exts) {
                for$each (file, os.fs.find(str.fmt(_, "%s-*%s", test_target, ext), false, _)) {
                    if (os.fs.remove(file)) {}
                }
            }
        }
        log$info("Coverage artifacts cleaned: %s\n", original_target);
    }
    return EOK;
}

/// CLI: build/run tests with coverage, then aggregate and report
Exception
coverage_cmd(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    char* process_help =
        "Manage project test coverage (gcov for gcc, llvm-cov gcov for clang)\n"
        "\n`run` builds and runs tests with the compiler --coverage flag, leaving\n"
        ".gcno/.gcda next to the test binaries in cexy$build_dir. `report` aggregates\n"
        "the data and prints per-source line coverage for cexy$src_dir. `clean` removes\n"
        "the coverage artifacts only, leaving test binaries intact.\n";
    char* epilog_help =
        "\nCommand examples: \n"
        "cex coverage run all                      - build+run all tests with coverage\n"
        "cex coverage run tests/test_foo.c         - coverage for a single test file\n"
        "cex coverage report all                   - aggregate and print coverage report\n"
        "cex coverage report tests/test_foo.c      - report for a single test file\n"
        "cex coverage clean all                    - remove all coverage artifacts\n"
        "cex coverage clean tests/test_foo.c       - remove artifacts of one test file\n";
    // clang-format on

    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "coverage {run,report,clean} all|tests/test_file.c",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(argparse$opt_help(),),
    };

    if (unlikely(argc < 2)) {
        io.printf("Usage: ./cex coverage {run,report,clean} all|tests/test_file.c\n");
        return e$raise(Error.argsparse, "coverage requires run|report|clean");
    }
    e$ret(argparse.parse(&cmd_args, argc, argv));

    char* subcmd = argparse.next(&cmd_args);
    char* target = argparse.next(&cmd_args);

    if (unlikely(subcmd == NULL || !str.match(subcmd, "(run|report|clean)"))) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Invalid coverage command, expected run|report|clean");
    }
    if (target == NULL) { target = "all"; }

    if (str.eq(subcmd, "run")) { return coverage_run(target); }
    if (str.eq(subcmd, "report")) { return coverage_report(target); }
    return coverage_clean(target);
}

CEX_NAMESPACE_DEF struct __cex_namespace__coverage coverage = {
    // Autogenerated by CEX
    // clang-format off

    .clean = coverage_clean,
    .cmd = coverage_cmd,
    .report = coverage_report,
    .run = coverage_run,

    // clang-format on
};
#endif // #if defined(CEX_BUILD) || defined(CEX_NEW)
