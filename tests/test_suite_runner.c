#include "src/all.c"

#define FIXTURE_SRC "tests/suite_runner/runner_fail_fixture.c"
#define FIXTURE_BIN "tests/build/suite_runner/runner_fail_fixture" cexy$build_ext_exe
#define FAIL_ENV "CEX_SUITE_FAIL"

#if !defined(__EMSCRIPTEN__)

test$setup_suite()
{
    e$ret(os.fs.mkpath("tests/build/suite_runner/"));
    os_cmd_c cmd = { 0 };
    char* args[] = { cexy$cc, "-I.", "-g", "-o", FIXTURE_BIN, FIXTURE_SRC, NULL };
    e$ret(os.cmd.create(
        &cmd,
        args,
        arr$len(args),
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
    if (os.fs.remove_tree("tests/build/suite_runner/")) {};
    return EOK;
}

static char*
run_fixture(char* hook, IAllocator allc)
{
    if (os.env.set(FAIL_ENV, hook)) { return NULL; }
    os_cmd_c cmd = { 0 };
    char* args[] = { FIXTURE_BIN, NULL };
    if (os.cmd.create(
            &cmd,
            args,
            arr$len(args),
            &(os_cmd_flags_s){ .combine_stdouterr = 1, .no_window = 1 }
        )) {
        if (os.env.unset(FAIL_ENV)) {}
        return NULL;
    }
    char* output = os.cmd.read_all(&cmd, allc);
    if (os.cmd.wait(&cmd, 1, 120)) {}
    if (os.env.unset(FAIL_ENV)) {}
    return output;
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

#else

test$case(not_supported_by_platform)
{
    return EOK;
}

#endif

test$main();
