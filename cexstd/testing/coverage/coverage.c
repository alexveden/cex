#pragma once
#include "coverage.h"

#if defined(CEX_BUILD) || defined(CEX_NEW)

#define _COVERAGE_INFO_DEFAULT "coverage.info"
#define _COVERAGE_HTML_DEFAULT "coverage_html"

static bool
_coverage__is_clang(void)
{
    char* cc[] = { cexy$cc };
    return arr$len(cc) > 0 && str.find(cc[0], "clang") != NULL;
}

static Exception
_coverage__check_tool(char* tool)
{
    uassert(tool != NULL);
    if (unlikely(!os.cmd.exists(tool))) {
        log$error("Coverage requires '%s' in PATH\n", tool);
        return e$raise(Error.runtime, "coverage tool not found");
    }
    return EOK;
}

static Exception
_coverage__find_test_binaries(char* target, arr$(char*)* out, IAllocator allc)
{
    uassert(out != NULL);

    char* pattern = target;
    e$ret(cexy.test.make_target_pattern(&pattern));

    *out = arr$new(*out, allc);
    if (str.ends_with(pattern, "test_*.c")) {
        *out = os.fs.find(str.fmt(allc, "%s/tests/*.test*", cexy$build_dir), false, allc);
        return EOK;
    }
    if (unlikely(!os.path.exists(pattern))) {
        return e$raise(Error.not_found, "test file not found");
    }
    char* bin = cexy.target_make(pattern, cexy$build_dir, ".test", allc);
    if (unlikely(bin == NULL)) { return e$raise(Error.runtime, "failed to make test target"); }
    arr$push(*out, bin);
    return EOK;
}

static Exception
_coverage__llvm_add_objects(char* target, arr$(char*)* args, IAllocator allc)
{
    uassert(args != NULL);

    arr$(char*) bins = NULL;
    e$ret(_coverage__find_test_binaries(target, &bins, allc));
    if (unlikely(arr$len(bins) == 0)) {
        return e$raise(Error.not_found, "no instrumented test binaries, run `cex coverage run`");
    }
    for$each (bin, bins) {
        arr$push(*args, str.fmt(allc, "--object=%s", bin));
    }
    return EOK;
}

static Exception
_coverage__llvm_merge_profdata(char** out_profdata, IAllocator allc)
{
    uassert(out_profdata != NULL);

    *out_profdata = NULL;
    arr$(char*) profraw = os.fs.find(str.fmt(allc, "%s/*.profraw", cexy$build_dir), true, allc);
    if (unlikely(arr$len(profraw) == 0)) {
        return e$raise(Error.not_found, "no .profraw data, run `cex coverage run` first");
    }

    char* profdata = str.fmt(allc, "%s/coverage.profdata", cexy$build_dir);
    arr$(char*) args = arr$new(args, allc);
    arr$pushm(args, "llvm-profdata", "merge", "-sparse");
    arr$pusha(args, profraw);
    arr$pushm(args, "-o", profdata, NULL);
    e$ret(os$cmda(args, arr$len(args)));

    *out_profdata = profdata;
    return EOK;
}

static Exception
_coverage__capture_cmd(arr$(char*) args, char** out, IAllocator allc)
{
    uassert(out != NULL);

    *out = NULL;
    os_cmd_c cmd = { 0 };
    e$ret(os.cmd.create(&cmd, args, arr$len(args), NULL));
    *out = os.cmd.read_all(&cmd, allc);
    e$ret(os.cmd.wait(&cmd, 1, 0));
    return EOK;
}

static Exception
_coverage__run_lcov_capture(char* output, IAllocator allc)
{
    arr$(char*) gcda = os.fs.find(str.fmt(allc, "%s/*.gcda", cexy$build_dir), true, allc);
    if (unlikely(arr$len(gcda) == 0)) {
        return e$raise(Error.not_found, "no .gcda data, run `cex coverage run` first");
    }

    arr$(char*) args = arr$new(args, allc);
    arr$pushm(args, "lcov", "--capture", "--quiet", "--directory", cexy$build_dir);
    arr$pushm(
        args,
        "--ignore-errors",
        "inconsistent,inconsistent,empty,unused,source,format,unsupported,unsupported"
    );
    if (_coverage__is_clang()) { arr$push(args, "--gcov-tool"); arr$push(args, "llvm-cov,gcov"); }
    arr$pushm(args, "--output-file", output, NULL);
    e$ret(os$cmda(args, arr$len(args)));
    return EOK;
}

/// Builds and runs tests with coverage instrumentation
Exception
coverage_run(char* engine, char* target)
{
    uassert(engine != NULL);
    uassert(target != NULL);

    char* argv[] = { "test", "--coverage", "--coverage-engine", engine, "run", target };
    return cexy.cmd.simple_test(arr$len(argv), argv, NULL);
}

/// Aggregates coverage and prints a per-source report (text or html)
Exception
coverage_report(char* engine, char* format, char* output, char* target)
{
    uassert(engine != NULL);
    uassert(target != NULL);

    if (format == NULL) { format = "text"; }
    if (unlikely(!str.match(format, "(text|html)"))) {
        return e$raise(Error.argument, "invalid report format, expected text|html");
    }
    bool html = str.eq(format, "html");
    if (output == NULL) { output = _COVERAGE_HTML_DEFAULT; }

    mem$scope(tmem$, _)
    {
        if (str.eq(engine, "llvm")) {
            e$ret(_coverage__check_tool("llvm-cov"));
            e$ret(_coverage__check_tool("llvm-profdata"));

            char* profdata = NULL;
            e$ret(_coverage__llvm_merge_profdata(&profdata, _));

            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, "llvm-cov", html ? "show" : "report");
            e$ret(_coverage__llvm_add_objects(target, &args, _));
            arr$push(args, str.fmt(_, "-instr-profile=%s", profdata));
            if (html) {
                arr$pushm(args, "--format=html", str.fmt(_, "--output-dir=%s", output));
            }
            arr$push(args, NULL);
            e$ret(os$cmda(args, arr$len(args)));
        } else {
            e$ret(_coverage__check_tool("lcov"));

            char* info = str.fmt(_, "%s/coverage.info", cexy$build_dir);
            e$ret(_coverage__run_lcov_capture(info, _));

            arr$(char*) args = arr$new(args, _);
            if (html) {
                e$ret(_coverage__check_tool("genhtml"));
                arr$pushm(args, "genhtml", "--quiet", info, "-o", output, NULL);
            } else {
                arr$pushm(args, "lcov", "--quiet", "--list", info, NULL);
            }
            e$ret(os$cmda(args, arr$len(args)));
        }

        if (html) { log$info("Coverage report: %s\n", output); }
    }
    return EOK;
}

/// Exports aggregated coverage as an lcov .info tracefile
Exception
coverage_export(char* engine, char* output, char* target)
{
    uassert(engine != NULL);
    uassert(target != NULL);

    if (output == NULL) { output = _COVERAGE_INFO_DEFAULT; }
    mem$scope(tmem$, _)
    {
        if (str.eq(engine, "llvm")) {
            e$ret(_coverage__check_tool("llvm-cov"));
            e$ret(_coverage__check_tool("llvm-profdata"));

            char* profdata = NULL;
            e$ret(_coverage__llvm_merge_profdata(&profdata, _));

            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, "llvm-cov", "export", "--format=lcov");
            e$ret(_coverage__llvm_add_objects(target, &args, _));
            arr$push(args, str.fmt(_, "-instr-profile=%s", profdata));
            arr$push(args, NULL);

            char* content = NULL;
            e$ret(_coverage__capture_cmd(args, &content, _));
            if (unlikely(content == NULL || content[0] == '\0')) {
                return e$raise(Error.runtime, "failed to export llvm coverage");
            }
            e$ret(io.file.save(output, content));
        } else {
            e$ret(_coverage__check_tool("lcov"));
            e$ret(_coverage__run_lcov_capture(output, _));
        }
        log$info("Coverage exported: %s\n", output);
    }
    return EOK;
}

/// Removes coverage artifacts (.profraw/.profdata/.gcno/.gcda)
Exception
coverage_clean(char* target)
{
    uassert(target != NULL);

    mem$scope(tmem$, _)
    {
        char* pattern = target;
        e$ret(cexy.test.make_target_pattern(&pattern));

        arr$(char*) globs = arr$new(globs, _);
        if (str.ends_with(pattern, "test_*.c")) {
            arr$pushm(globs, "%s/*.gcno", "%s/*.gcda");
        } else {
            if (unlikely(!os.path.exists(pattern))) {
                return e$raise(Error.not_found, "test file not found");
            }
            char* test_target = cexy.target_make(pattern, cexy$build_dir, ".test", _);
            if (unlikely(test_target == NULL)) {
                return e$raise(Error.runtime, "failed to make test target");
            }
            arr$pushm(
                globs,
                str.fmt(_, "%s-*.gcno", test_target),
                str.fmt(_, "%s-*.gcda", test_target)
            );
        }
        arr$pushm(globs, "%s/*.profraw", "%s/*.profdata");
        for$each (glob, globs) {
            for$each (file, os.fs.find(str.fmt(_, glob, cexy$build_dir), true, _)) {
                if (os.fs.remove(file)) {}
            }
        }
        log$info("Coverage artifacts cleaned: %s\n", target);
    }
    return EOK;
}

/// CLI: build/run tests with coverage, then report or export it
Exception
coverage_cmd(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    char* engine = NULL;
    char* format = NULL;
    char* output = NULL;

    // clang-format off
    char* process_help =
        "Manage project test coverage with two engines:\n"
        "- `llvm`: clang only, source-based coverage via llvm-profdata + llvm-cov\n"
        "- `lcov`: compiler --coverage + lcov/genhtml (gcc or clang)\n"
        "\n`run` builds and runs tests with instrumentation, leaving raw data in\n"
        "cexy$build_dir (.profraw for llvm, .gcno/.gcda for lcov). `report` aggregates\n"
        "and prints per-source coverage. `export` writes an lcov .info tracefile for\n"
        "external tools (merge several with `lcov -a a.info -o merged.info`). `clean`\n"
        "removes the raw coverage artifacts only, leaving test binaries intact.\n";
    char* epilog_help =
        "\nCommand examples: \n"
        "cex coverage run all                          - build+run all tests with coverage\n"
        "cex coverage report all                       - aggregate and print text report\n"
        "cex coverage report --format=html all         - write html report\n"
        "cex coverage export -o coverage.info all      - write lcov .info tracefile\n"
        "cex coverage report --engine=lcov all         - force the lcov engine\n"
        "cex coverage clean all                        - remove coverage artifacts\n";
    // clang-format on

    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "coverage {run,report,export,clean} [options] all|tests/test_file.c",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(
            argparse$opt_help(),
            argparse$opt(&engine, '\0', "engine", .help = "Coverage engine: auto|llvm|lcov"),
            argparse$opt(&format, '\0', "format", .help = "Report format: text|html"),
            argparse$opt(&output, 'o', "output", .help = "Output file (.info) or dir (html)"),
        ),
    };

    if (unlikely(argc < 2)) {
        io.printf("Usage: ./cex coverage {run,report,export,clean} all|tests/test_file.c\n");
        return e$raise(Error.argsparse, "coverage requires run|report|export|clean");
    }

    mem$scope(tmem$, _)
    {
        // argparse expects options before positionals; hoist them so both orders work
        arr$(char*) opts = arr$new(opts, _);
        arr$(char*) pos = arr$new(pos, _);
        for (i32 i = 1; i < argc; i++) {
            char* a = argv[i];
            if (a[0] != '-') {
                arr$push(pos, a);
                continue;
            }
            arr$push(opts, a);
            bool takes_value =
                !str.eq(a, "-h") && !str.eq(a, "--help") && str.find(a, "=") == NULL;
            if (takes_value && i + 1 < argc) { arr$push(opts, argv[++i]); }
        }
        arr$(char*) ordered = arr$new(ordered, _);
        arr$push(ordered, argv[0]);
        arr$pusha(ordered, opts);
        arr$pusha(ordered, pos);

        e$ret(argparse.parse(&cmd_args, arr$len(ordered), ordered));

        char* subcmd = argparse.next(&cmd_args);
        char* target = argparse.next(&cmd_args);

        if (unlikely(subcmd == NULL || !str.match(subcmd, "(run|report|export|clean)"))) {
            argparse.usage(&cmd_args);
            return e$raise(
                Error.argsparse, "invalid coverage command, expected run|report|export|clean"
            );
        }
        if (target == NULL) { target = "all"; }

        bool is_clang = _coverage__is_clang();
        if (engine == NULL || str.eq(engine, "auto")) { engine = is_clang ? "llvm" : "lcov"; }
        if (unlikely(!str.match(engine, "(llvm|lcov)"))) {
            argparse.usage(&cmd_args);
            return e$raise(Error.argsparse, "invalid engine, expected auto|llvm|lcov");
        }
        if (unlikely(str.eq(engine, "llvm") && !is_clang)) {
            return e$raise(Error.argument, "coverage engine 'llvm' requires clang compiler");
        }

        if (str.eq(subcmd, "run")) { return coverage_run(engine, target); }
        if (str.eq(subcmd, "report")) { return coverage_report(engine, format, output, target); }
        if (str.eq(subcmd, "export")) { return coverage_export(engine, output, target); }
        return coverage_clean(target);
    }
    return EOK;
}

CEX_NAMESPACE_DEF struct __cex_namespace__coverage coverage = {
    // Autogenerated by CEX
    // clang-format off

    .clean = coverage_clean,
    .cmd = coverage_cmd,
    .export = coverage_export,
    .report = coverage_report,
    .run = coverage_run,

    // clang-format on
};
#endif // #if defined(CEX_BUILD) || defined(CEX_NEW)
