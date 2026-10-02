#include "src/all.c"

#define FIXTURE_SRC "tests/suite_runner/runner_fail_fixture.c"
#define FIXTURE_BIN cexy$build_dir "/tests/runner_fail_fixture.test" cexy$build_ext_exe
#define FAIL_ENV "CEX_SUITE_FAIL"
#define ANSI_ENV "CEX_SUITE_ANSI"

#if !defined(__EMSCRIPTEN__)

test$setup_suite()
{
    e$ret(os.fs.mkpath(cexy$build_dir "/tests/"));
    os_cmd_c cmd = { 0 };
    char* engine = os.env.get("CEX_COVERAGE_ENGINE", NULL);
    char* args[12] = { cexy$cc, "-I.", "-g" };
    usize n_args = 3;
    if (engine != NULL && str.eq(engine, "llvm")) {
        args[n_args++] = "-fprofile-instr-generate";
        args[n_args++] = "-fcoverage-mapping";
    } else if (engine != NULL && str.eq(engine, "lcov")) {
        args[n_args++] = "--coverage";
    }
    args[n_args++] = "-o";
    args[n_args++] = FIXTURE_BIN;
    args[n_args++] = FIXTURE_SRC;
    args[n_args] = NULL;
    e$ret(os.cmd.create(
        &cmd,
        args,
        n_args + 1,
        &(os_cmd_flags_s){ .combine_stdouterr = 1, .no_window = 1 }
    ));
    char* output = os.cmd.read_all(&cmd, mem$);
    if (os.cmd.wait(&cmd, 1, 120)) {
        log$error("Fixture build output:\n%s\n", output);
        mem$free(mem$, output);
        return e$raise(Error.runtime, "failed to build runner fixture");
    }
    mem$free(mem$, output);
    return EOK;
}

test$teardown_suite()
{
    if (os.env.get("CEX_COVERAGE_ENGINE", NULL) == NULL) {
        if (os.fs.remove(FIXTURE_BIN)) {}
    }
    return EOK;
}

static char*
run_fixture_ex(char* hook, bool force_ansi, char** extra, usize n_extra, IAllocator allc)
{
    if (os.env.set(FAIL_ENV, hook)) { return NULL; }
    if (force_ansi) {
        if (os.env.set(ANSI_ENV, "1")) {
            if (os.env.unset(FAIL_ENV)) {}
            return NULL;
        }
    } else {
        if (os.env.unset(ANSI_ENV)) {}
    }

    os_cmd_c cmd = { 0 };
    char* args[8] = { FIXTURE_BIN };
    usize n_args = 1;
    for (usize i = 0; i < n_extra && n_args < arr$len(args) - 1; i++) { args[n_args++] = extra[i]; }
    args[n_args] = NULL;

    if (os.cmd.create(
            &cmd,
            args,
            n_args + 1,
            &(os_cmd_flags_s){ .combine_stdouterr = 1, .no_window = 1 }
        )) {
        if (os.env.unset(FAIL_ENV)) {}
        if (os.env.unset(ANSI_ENV)) {}
        return NULL;
    }
    char* output = os.cmd.read_all(&cmd, allc);
    if (os.cmd.wait(&cmd, 1, 120)) {}
    if (os.env.unset(FAIL_ENV)) {}
    if (os.env.unset(ANSI_ENV)) {}
    return output;
}

static char*
run_fixture(char* hook, IAllocator allc)
{
    return run_fixture_ex(hook, false, NULL, 0, allc);
}

test$case(runner_case_failure_prints_traceback)
{
    char* out = run_fixture("case", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "#0 (") != NULL);
    tassert(str.find(out, "[IOError]") != NULL);
    tassert(str.find(out, "case boom") != NULL);
    return EOK;
}

test$case(runner_restores_global_mem_allocator)
{
    char* out = run_fixture("alloc_replace", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "MEM_REPLACED") != NULL);
    tassert(str.find(out, "MEM_RESTORED") != NULL);
    return EOK;
}

test$case(runner_tassert_failure_resets_traceback)
{
    char* out = run_fixture("tassert", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "0 != 1") != NULL);
    tassert(str.find(out, "raise io") == NULL);
    return EOK;
}

test$case(runner_tassert_er_failure_keeps_error_traceback)
{
    char* out = run_fixture("tassert_er", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "#0 (") != NULL);
    tassert(str.find(out, "raise io") != NULL);
    return EOK;
}

test$case(runner_setup_suite_failure_prints_traceback)
{
    char* out = run_fixture("setup_suite", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "#0 (") != NULL);
    tassert(str.find(out, "setup_suite boom") != NULL);
    return EOK;
}

test$case(runner_setup_case_failure_prints_traceback)
{
    char* out = run_fixture("setup_case", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "#0 (") != NULL);
    tassert(str.find(out, "setup_case boom") != NULL);
    return EOK;
}

test$case(runner_teardown_case_failure_prints_traceback)
{
    char* out = run_fixture("teardown_case", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "#0 (") != NULL);
    tassert(str.find(out, "teardown_case boom") != NULL);
    return EOK;
}

test$case(runner_teardown_suite_failure_prints_traceback)
{
    char* out = run_fixture("teardown_suite", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "#0 (") != NULL);
    tassert(str.find(out, "teardown_suite boom") != NULL);
    return EOK;
}

test$case(runner_leak_detection_reports)
{
    char* out = run_fixture("leak", test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "Possible memory leak") != NULL);
    return EOK;
}

test$case(runner_quiet_leak_detection_reports)
{
    char* extra[] = { "-q" };
    char* out = run_fixture_ex("leak", false, extra, arr$len(extra), test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "Possible memory leak") != NULL);
    return EOK;
}

test$case(runner_quiet_mode_reports_failure)
{
    char* extra[] = { "-q" };
    char* out = run_fixture_ex("case", false, extra, arr$len(extra), test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "tests failed") != NULL);
    return EOK;
}

test$case(runner_no_capture_streams_test_output)
{
    char* extra[] = { "-o" };
    char* out = run_fixture_ex("case", false, extra, arr$len(extra), test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "case boom") != NULL);
    return EOK;
}

test$case(runner_filter_skips_cases)
{
    char* extra[] = { "--filter=nomatch" };
    char* out = run_fixture_ex("none", false, extra, arr$len(extra), test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "Skipped:") != NULL);
    return EOK;
}

test$case(runner_bench_dispatch_runs_bench_case)
{
    char* extra[] = { "--bench" };
    char* out = run_fixture_ex("none", false, extra, arr$len(extra), test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "cold:") != NULL);
    return EOK;
}

test$case(runner_ansi_success_output)
{
    char* out = run_fixture_ex("none", true, NULL, 0, test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "\033[32m") != NULL);
    return EOK;
}

test$case(runner_ansi_failure_output)
{
    char* out = run_fixture_ex("case", true, NULL, 0, test$alloc);
    tassert(out != NULL);
    tassert(str.find(out, "\033[31m") != NULL);
    return EOK;
}

#endif

test$main();
