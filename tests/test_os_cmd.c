#include "src/all.c"
#include "src/all.h"

#if defined(__SANITIZE_ADDRESS__)
#    define TEST_OS_CMD_ASAN 1
#elif defined(__clang__)
#    if __has_feature(address_sanitizer)
#        define TEST_OS_CMD_ASAN 1
#    else
#        define TEST_OS_CMD_ASAN 0
#    endif
#else
#    define TEST_OS_CMD_ASAN 0
#endif

#if defined(__linux__)
#    include <sys/resource.h>
#endif

#if !defined(_WIN32)
#    include <sys/time.h>
#endif

#if !defined(_WIN32)
static void
os_cmd_sigalrm_handler(int sig)
{
    (void)sig;
}
#endif

#define TBUILDDIR "tests/build/os_cmd_test/"
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
char*
test_app(char* app_name, IAllocator allc)
{
    uassert(app_name != NULL);
    uassert(allc != NULL);
    char* result = os$path_join(
        allc,
        "build",
        "tests",
        "os_test",
        str.fmt(
            allc,
            "%s.c.%s%s",
            app_name,
            os.platform.to_str(os.platform.current()),
            (os.platform.current() == OSPlatform__win) ? ".exe" : ""
        )
    );
    // log$debug("Making test app: %s\n", result);
    return result;
}

test$case(os_cmd_exists)
{
    log$debug("Current platform: %s\n", os.platform.current_str());
    log$debug("PATH env: %s\n", os.env.get("PATH", NULL));

    tassert_eq(false, os.cmd.exists(""));
    tassert_eq(false, os.cmd.exists(NULL));
    tassert_eq(false, os.cmd.exists("alskdislkdfjslkfjk"));
    tassert_eq(true, os.cmd.exists("./cex"));

    tassert_eq(true, os.path.exists(TBUILDDIR));
    tassert_eq(false, os.cmd.exists(TBUILDDIR));

#ifdef _WIN32
    e$ret(io.file.save(TBUILDDIR "mycmd1.exe", "test"));
    e$ret(io.file.save(TBUILDDIR "mycmd2.bat", "test"));
    e$ret(io.file.save(TBUILDDIR "mycmd3.cmd", "test"));

    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd1.exe"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd2.bat"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd3.cmd"));

    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd1"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd2"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd3"));

    tassert_eq(true, os.cmd.exists("cmd.exe"));
    tassert_eq(true, os.cmd.exists("cmd"));
    tassert_eq(true, os.cmd.exists("./cex.exe"));
    tassert_eq(true, os.cmd.exists(".\\cex.exe"));
    tassert_eq(true, os.cmd.exists(".\\cex"));
#else
    tassert_eq(true, os.cmd.exists("ls"));
#endif

    return EOK;
}

test$case(os_cmd_create)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        char* args[] = { test_app("write_lines", _), NULL };
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 1);
    }
    return EOK;
}

test$case(os_cmd_file_handles)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert(os.cmd.fstderr(&c) == c._subpr.stderr_file);
        tassert(os.cmd.fstdout(&c) == c._subpr.stdout_file);
        tassert(os.cmd.fstdin(&c) == c._subpr.stdin_file);

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 1);
    }
    return EOK;
}

test$case(os_cmd_read_all_small)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "10", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }

        // tassert_eq(strlen(output) / 10, arr$len(lines));
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(output[strlen(output) - 2], '\r');
            tassert_eq(output[strlen(output) - 1], '\n');
            tassert_eq(strlen(output) / 11, arr$len(lines));
        } else {
            tassert_eq(strlen(output) / 10, arr$len(lines));
        }
    }
    return EOK;
}

test$case(os_cmd_read_all_huge)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 100000);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }
    }
    return EOK;
}

test$case(os_cmd_read_line_huge)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* line;
        u32 lcnt = 0;
        while ((line = os.cmd.read_line(&c, _))) {
            tassert_eq(str.fmt(_, "%09d", lcnt), line);
            lcnt++;
        }
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        tassert_eq(lcnt, 100000);
    }
    return EOK;
}

test$case(os_cmd_read_all_only_stdout)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stderr", "10", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_eq(output, "");

        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);
    }
    return EOK;
}


test$case(os_cmd_read_all_combined_stderr)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stderr", "10", NULL);
        tassert_er(
            EOK,
            os.cmd.create(&c, args, arr$len(args), &(os_cmd_flags_s){ .combine_stdouterr = 1 })
        );

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }

        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(output[strlen(output) - 2], '\r');
            tassert_eq(output[strlen(output) - 1], '\n');
            tassert_eq(strlen(output) / 11, arr$len(lines));
        } else {
            tassert_eq(strlen(output) / 10, arr$len(lines));
        }
    }
    return EOK;
}

test$case(os_cmd_join_timeout)
{
    os_cmd_c c = { 0 };
    // WTF: if we call _cex_subprocess_terminate() on empty struct it will kill self!
    tassert_eq(0, os.cmd.is_alive(&c));
    e$ret(os.cmd.kill(&c));

    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep", _), "2", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert_eq(1, os.cmd.is_alive(&c));

        tassert_er(Error.timeout, os.cmd.wait(&c, 1, 1));
        tassert_eq(os.cmd.ret_code(&c), -1);

        tassert_er(EOK, os.cmd.kill(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));

        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert_er(Error.ok, os.cmd.wait(&c, 1, 3));
        tassert_eq(os.cmd.ret_code(&c), 0);
    }
    return EOK;
}

test$case(os_cmd_kill_running)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep", _), "5", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert_eq(1, os.cmd.is_alive(&c));
        tassert_er(EOK, os.cmd.kill(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_wait_survives_eintr)
{
#if defined(_WIN32)
    return EOK;
#else
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "500", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        if (c._subpr.stdin_file) {
            fclose(c._subpr.stdin_file);
            c._subpr.stdin_file = NULL;
        }

        struct sigaction sa = { 0 };
        struct sigaction old = { 0 };
        sa.sa_handler = os_cmd_sigalrm_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        int sig_rc = sigaction(SIGALRM, &sa, &old);

        int timer_rc = -1;
        int join_rc = -1;
        int ret = -1;
        if (0 == sig_rc) {
            struct itimerval timer = { 0 };
            timer.it_value.tv_usec = 50000;
            timer_rc = setitimer(ITIMER_REAL, &timer, NULL);

            if (0 == timer_rc) { join_rc = _cex_subprocess_join(&c._subpr, &ret); }

            struct itimerval disarm = { 0 };
            setitimer(ITIMER_REAL, &disarm, NULL);
            sigaction(SIGALRM, &old, NULL);
        }

        if (join_rc != 0) {
            if (os.cmd.kill(&c)) {}
            if (os.cmd.wait(&c, 1, 0)) {}
        } else {
            _cex_subprocess_destroy(&c._subpr);
        }

        tassert_eq(0, sig_rc);
        tassert_eq(0, timer_rc);
        tassert_eq(0, join_rc);
        tassert_eq(0, ret);
    }
    return EOK;
#endif
}

test$case(os_cmd_join_echild)
{
#if defined(_WIN32)
    return EOK;
#else
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        int status = 0;
        tassert_eq(c._subpr.child, waitpid(c._subpr.child, &status, 0));

        int ret = 123;
        errno = 0;
        int rc = _cex_subprocess_join(&c._subpr, &ret);
        int saved_errno = errno;
        pid_t child = c._subpr.child;
        int alive = c._subpr.alive;
        int return_status = c._subpr.return_status;

        _cex_subprocess_destroy(&c._subpr);

        tassert_eq(-1, rc);
        tassert_eq(ECHILD, saved_errno);
        tassert_eq(0, child);
        tassert_eq(0, alive);
        tassert_eq(EXIT_FAILURE, return_status);
        tassert_eq(123, ret);
    }
    return EOK;
#endif
}

test$case(os_cmd_join_clears_alive)
{
    struct _cex_subprocess_s p = { .alive = 1 };
    int ret = 123;
    int rc = _cex_subprocess_join(&p, &ret);
    int alive = p.alive;
#if !defined(_WIN32)
    pid_t child = p.child;
#endif

    tassert_eq(0, rc);
    tassert_eq(0, alive);
#if !defined(_WIN32)
    tassert_eq(0, child);
    tassert_eq(0, ret);
#endif
    return EOK;
}

test$case(os_cmd_terminate_no_process)
{
#if defined(_WIN32)
    struct _cex_subprocess_s p = { .alive = 1 };
    tassert_eq(-1, _cex_subprocess_terminate(&p));
    return EOK;
#else
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind error-exitcode overrides the forked child status
    pid_t child = fork();
    tassert(child >= 0);
    if (child == 0) {
        if (0 != setpgid(0, 0)) { _exit(2); }
        struct _cex_subprocess_s p = { .alive = 1, .child = 0 };
        _exit(-1 == _cex_subprocess_terminate(&p) ? 0 : 1);
    }

    int status = 0;
    tassert_eq(child, waitpid(child, &status, 0));
    tassert(WIFEXITED(status));
    tassert_eq(0, WEXITSTATUS(status));
    return EOK;
#endif
}

test$case(os_cmd_terminate_already_gone)
{
#if defined(_WIN32)
    return EOK;
#else
    struct _cex_subprocess_s p = { .alive = 1, .child = INT_MAX };
    tassert_eq(0, _cex_subprocess_terminate(&p));
    return EOK;
#endif
}

test$case(os_cmd_alive_after_external_reap)
{
#if defined(_WIN32)
    return EOK;
#else
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        int status = 0;
        tassert_eq(c._subpr.child, waitpid(c._subpr.child, &status, 0));

        int alive_rc = _cex_subprocess_alive(&c._subpr);
        pid_t child = c._subpr.child;
        int alive = c._subpr.alive;
        int return_status = c._subpr.return_status;

        _cex_subprocess_destroy(&c._subpr);

        tassert_eq(0, alive_rc);
        tassert_eq(0, child);
        tassert_eq(0, alive);
        tassert_eq(EXIT_FAILURE, return_status);
    }
    return EOK;
#endif
}

test$case(os_cmd_alive_closes_stdin)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        int spins = 0;
        while (os.cmd.is_alive(&c) && spins++ < 500) { os.sleep(0.01); }
        tassert(spins < 500);

        tassert(os.cmd.fstdin(&c) == NULL);
        tassert(os.cmd.fstdout(&c) != NULL);

        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_ret_code_after_is_alive)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "150", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        int spins = 0;
        while (os.cmd.is_alive(&c) && spins++ < 500) { os.sleep(0.01); }
        tassert(spins < 500);

        tassert_eq(50, os.cmd.ret_code(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_ret_code_after_is_alive_zero)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        int spins = 0;
        while (os.cmd.is_alive(&c) && spins++ < 500) { os.sleep(0.01); }
        tassert(spins < 500);

        tassert_eq(0, os.cmd.ret_code(&c));
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_ret_code_after_is_alive_killed)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep", _), "1", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert_er(EOK, os.cmd.kill(&c));

        int spins = 0;
        while (os.cmd.is_alive(&c) && spins++ < 500) { os.sleep(0.01); }
        tassert(spins < 500);

        tassert_eq(EXIT_FAILURE, os.cmd.ret_code(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_ret_code_after_is_alive_run)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "150", NULL);
        tassert_er(EOK, os.cmd.run(args, arr$len(args), &c));

        int spins = 0;
        while (os.cmd.is_alive(&c) && spins++ < 500) { os.sleep(0.01); }
        tassert(spins < 500);

        tassert_eq(50, os.cmd.ret_code(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_ret_code_not_alive_unchanged)
{
    os_cmd_c c = { ._ret_code = -1 };
    tassert_eq(false, os.cmd.is_alive(&c));
    tassert_eq(-1, os.cmd.ret_code(&c));
    return EOK;
}

test$case(os_cmd_ret_code_after_kill_dead)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        os.sleep(1.0);

        tassert_er(EOK, os.cmd.kill(&c));
        tassert_eq(0, os.cmd.ret_code(&c));
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_create_live_struct_rejected)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep", _), "1", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        Exc err = os.cmd.create(&c, args, arr$len(args), NULL);
        bool was_alive = os.cmd.is_alive(&c);

        if (os.cmd.kill(&c)) {}
        if (os.cmd.wait(&c, 1, 0)) {}

        tassert_er(Error.integrity, err);
        tassert_eq(true, was_alive);
    }
    return EOK;
}

test$case(os_cmd_run_live_struct_rejected)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep", _), "1", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        Exc err = os.cmd.run(args, arr$len(args), &c);
        bool was_alive = os.cmd.is_alive(&c);

        if (os.cmd.kill(&c)) {}
        if (os.cmd.wait(&c, 1, 0)) {}

        tassert_er(Error.integrity, err);
        tassert_eq(true, was_alive);
    }
    return EOK;
}

test$case(os_cmd_create_after_is_alive_rejected)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        int spins = 0;
        while (os.cmd.is_alive(&c) && spins++ < 500) { os.sleep(0.01); }
        tassert(spins < 500);

        Exc err = os.cmd.create(&c, args, arr$len(args), NULL);
        if (os.cmd.wait(&c, 1, 0)) {}

        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
        tassert_er(Error.integrity, err);
    }
    return EOK;
}

test$case(os_cmd_create_after_failed_create_allowed)
{
#if !defined(_WIN32)
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind posix_spawn doesn't propagate ENOENT
#endif
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        char* bad[] = { "cex_no_such_command_xyz", NULL };
        tassert_ne(EOK, os.cmd.create(&c, bad, 2, NULL));

        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_run_after_wait_allowed)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);

        tassert_er(EOK, os.cmd.run(args, arr$len(args), &c));
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));

        tassert_er(EOK, os.cmd.run(args, arr$len(args), &c));
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_create_exec_not_found)
{
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind posix_spawn doesn't propagate ENOENT
    os_cmd_c c = { 0 };
    char* args[] = { "cex_no_such_command_xyz", NULL };
    tassert_ne(EOK, os.cmd.create(&c, args, 2, NULL));
    return EOK;
}

test$case(os_cmd_create_no_search_path_not_found)
{
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind posix_spawn doesn't propagate ENOENT
    os_cmd_c c = { 0 };
    char* args[] = { "cex_no_such_command_xyz", NULL };
    tassert_ne(EOK, os.cmd.create(&c, args, 2, &(os_cmd_flags_s){ .no_search_path = 1 }));
    return EOK;
}

#if !defined(_WIN32)
static Exception
os_cmd_create_failure_probe(os_cmd_flags_s flags)
{
    int probe = open("/dev/null", O_RDONLY);
    tassert(probe >= 0);
    close(probe);

    os_cmd_c c = { 0 };
    char* args[] = { "cex_no_such_command_xyz", NULL };
    tassert_ne(EOK, os.cmd.create(&c, args, 2, &flags));

    int n_fds = flags.combine_stdouterr ? 4 : 6;
    int probes[6] = { -1 };
    for (int i = 0; i < n_fds; i++) { probes[i] = open("/dev/null", O_RDONLY); }
    for (int i = 0; i < n_fds; i++) {
        if (probes[i] >= 0) { close(probes[i]); }
    }
    for (int i = 0; i < n_fds; i++) { tassert_eq(probe + i, probes[i]); }

    tassert_eq(0, c._subpr.alive);
    tassert_eq(0, c._subpr.child);
    tassert(c._subpr.stdin_file == NULL);
    tassert(c._subpr.stdout_file == NULL);
    tassert(c._subpr.stderr_file == NULL);

    return EOK;
}

static Exception
os_cmd_pipes_are_cloexec_check(os_cmd_flags_s flags)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep", _), "1", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), &flags));

        if (flags.combine_stdouterr) {
            tassert(os.cmd.fstderr(&c) == os.cmd.fstdout(&c));
        }

        int fds[] = {
            fileno(os.cmd.fstdin(&c)),
            fileno(os.cmd.fstdout(&c)),
            fileno(os.cmd.fstderr(&c)),
        };
        for$each (fd, fds) {
            tassert(fd >= 0);
            int fd_flags = fcntl(fd, F_GETFD);
            tassert(fd_flags >= 0);
            tassert(fd_flags & FD_CLOEXEC);
        }

        tassert_er(EOK, os.cmd.kill(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}
#endif

test$case(os_cmd_create_failure_no_fd_leak)
{
#if defined(_WIN32)
    return EOK;
#else
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind posix_spawn doesn't propagate ENOENT
    e$ret(os_cmd_create_failure_probe((os_cmd_flags_s){ 0 }));
    e$ret(os_cmd_create_failure_probe((os_cmd_flags_s){ .combine_stdouterr = 1 }));
    return EOK;
#endif
}

test$case(os_cmd_create_spawn_error_state)
{
#if defined(_WIN32)
    struct _cex_subprocess_s p = { .alive = 1, .hProcess = (void*)1 };
    const char* args[] = { "cex_no_such_command_xyz", NULL };
    SetLastError(0);
    int rc = _cex_subprocess_create(args, 0, &p);

    tassert_eq(-1, rc);
    tassert(GetLastError() != 0);
    tassert_eq(0, p.alive);
    tassert(p.hProcess == NULL);
    tassert(p.hStdInput == NULL);
    tassert(p.hEventOutput == NULL);
    tassert(p.hEventError == NULL);
    tassert(p.stdin_file == NULL);
    tassert(p.stdout_file == NULL);
    tassert(p.stderr_file == NULL);
    return EOK;
#else
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind posix_spawn doesn't propagate ENOENT

    struct _cex_subprocess_s p = { .alive = 1, .child = 123 };
    const char* args[] = { "cex_no_such_command_xyz", NULL };
    errno = 0;
    int rc = _cex_subprocess_create(args, 0, &p);

    tassert_eq(-1, rc);
    tassert_eq(ENOENT, errno);
    tassert_eq(0, p.alive);
    tassert_eq(0, p.child);
    tassert(p.stdin_file == NULL);
    tassert(p.stdout_file == NULL);
    tassert(p.stderr_file == NULL);
    return EOK;
#endif
}

test$case(os_cmd_create_eacces_no_fd_leak)
{
#if defined(_WIN32)
    return EOK;
#else
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind posix_spawn doesn't propagate spawn errors
    e$ret(io.file.save(TBUILDDIR "not_exec", "test"));

    int probe = open("/dev/null", O_RDONLY);
    tassert(probe >= 0);
    close(probe);

    os_cmd_c c = { 0 };
    char* args[] = { TBUILDDIR "not_exec", NULL };
    os_cmd_flags_s flags = { .no_search_path = 1 };
    tassert_ne(EOK, os.cmd.create(&c, args, 2, &flags));
    tassert_eq(EACCES, errno);

    int probe_after = open("/dev/null", O_RDONLY);
    tassert(probe_after >= 0);
    tassert_eq(probe, probe_after);
    close(probe_after);
    return EOK;
#endif
}

test$case(os_cmd_create_inherit_env_mismatch)
{
#if defined(_WIN32)
    struct _cex_subprocess_s p = { .alive = 1, .hProcess = (void*)1 };
    const char* args[] = { "true", NULL };
    const char* env[] = { "FOO=bar", NULL };
    SetLastError(0);
    int rc = _cex_subprocess_create_ex(
        args, _cex_subprocess_option_inherit_environment, env, &p
    );

    tassert_eq(-1, rc);
    tassert_eq(ERROR_INVALID_PARAMETER, GetLastError());
    tassert_eq(0, p.alive);
    tassert(p.hProcess == NULL);
    tassert(p.hStdInput == NULL);
    tassert(p.hEventOutput == NULL);
    tassert(p.hEventError == NULL);
    tassert(p.stdin_file == NULL);
    tassert(p.stdout_file == NULL);
    tassert(p.stderr_file == NULL);
    return EOK;
#else
    struct _cex_subprocess_s p = { .alive = 1, .child = 123 };
    const char* args[] = { "true", NULL };
    const char* env[] = { "FOO=bar", NULL };
    errno = 0;
    int rc = _cex_subprocess_create_ex(
        args, _cex_subprocess_option_inherit_environment, env, &p
    );

    tassert_eq(-1, rc);
    tassert_eq(EINVAL, errno);
    tassert_eq(0, p.alive);
    tassert_eq(0, p.child);
    tassert(p.stdin_file == NULL);
    tassert(p.stdout_file == NULL);
    tassert(p.stderr_file == NULL);
    return EOK;
#endif
}

test$case(os_cmd_pipes_are_cloexec)
{
#if defined(_WIN32)
    return EOK;
#else
    e$ret(os_cmd_pipes_are_cloexec_check((os_cmd_flags_s){ 0 }));
    e$ret(os_cmd_pipes_are_cloexec_check((os_cmd_flags_s){ .combine_stdouterr = 1 }));
    return EOK;
#endif
}

test$case(os_cmd_child_does_not_inherit_pipes)
{
#if defined(_WIN32)
    return EOK;
#else
    os_cmd_c sleeper = { 0 };
    os_cmd_c checker = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) sleep_args = arr$new(sleep_args, _);
        arr$pushm(sleep_args, test_app("sleep", _), "10", NULL);
        tassert_er(EOK, os.cmd.create(&sleeper, sleep_args, arr$len(sleep_args), NULL));

        int fds[] = {
            fileno(os.cmd.fstdin(&sleeper)),
            fileno(os.cmd.fstdout(&sleeper)),
            fileno(os.cmd.fstderr(&sleeper)),
        };
        tassert(fds[0] >= 0);
        tassert(fds[1] >= 0);
        tassert(fds[2] >= 0);

        char fd_str[3][16];
        arr$(char*) check_args = arr$new(check_args, _);
        arr$push(check_args, test_app("check_fds", _));
        for (usize i = 0; i < arr$len(fds); i++) {
            snprintf(fd_str[i], sizeof(fd_str[i]), "%d", fds[i]);
            arr$push(check_args, fd_str[i]);
        }
        arr$push(check_args, NULL);

        tassert_er(EOK, os.cmd.create(&checker, check_args, arr$len(check_args), NULL));
        tassert_er(EOK, os.cmd.wait(&checker, 1, 0));
        tassert_eq(0, os.cmd.ret_code(&checker));

        tassert_er(EOK, os.cmd.kill(&sleeper));
        tassert_er(Error.runtime, os.cmd.wait(&sleeper, 1, 0));
    }
    return EOK;
#endif
}

test$case(os_cmd_destroy_partial_state)
{
#if defined(_WIN32)
    struct _cex_subprocess_s p = { 0 };
    tassert_eq(0, _cex_subprocess_destroy(&p));

    HANDLE h1 = CreateEventA(NULL, 1, 1, NULL);
    HANDLE h2 = CreateEventA(NULL, 1, 1, NULL);
    HANDLE h3 = CreateEventA(NULL, 1, 1, NULL);
    tassert(h1 != NULL && h2 != NULL && h3 != NULL);

    p = (struct _cex_subprocess_s){
        .hStdInput = h1,
        .hEventOutput = h2,
        .hEventError = h3,
    };
    tassert_eq(0, _cex_subprocess_destroy(&p));
    tassert(p.hStdInput == NULL);
    tassert(p.hEventOutput == NULL);
    tassert(p.hEventError == NULL);
    tassert_eq(0, CloseHandle(h1));
    tassert_eq(0, CloseHandle(h2));
    tassert_eq(0, CloseHandle(h3));
    return EOK;
#else
    struct _cex_subprocess_s p = { 0 };
    tassert_eq(0, _cex_subprocess_destroy(&p));

    FILE* f1 = fopen("/dev/null", "rb");
    tassert(f1 != NULL);
    int fd1 = fileno(f1);
    p = (struct _cex_subprocess_s){ .stdout_file = f1 };
    tassert_eq(0, _cex_subprocess_destroy(&p));
    int probe1 = open("/dev/null", O_RDONLY);
    tassert_eq(fd1, probe1);
    close(probe1);

    FILE* f2 = fopen("/dev/null", "rb");
    tassert(f2 != NULL);
    int fd2 = fileno(f2);
    p = (struct _cex_subprocess_s){ .stderr_file = f2 };
    tassert_eq(0, _cex_subprocess_destroy(&p));
    int probe2 = open("/dev/null", O_RDONLY);
    tassert_eq(fd2, probe2);
    close(probe2);

    FILE* f3 = fopen("/dev/null", "rb");
    tassert(f3 != NULL);
    int fd3 = fileno(f3);
    p = (struct _cex_subprocess_s){ .stdout_file = f3, .stderr_file = f3 };
    tassert_eq(0, _cex_subprocess_destroy(&p));
    int probe3 = open("/dev/null", O_RDONLY);
    tassert_eq(fd3, probe3);
    close(probe3);

    FILE* f4 = fopen("/dev/null", "rb");
    FILE* f5 = fopen("/dev/null", "rb");
    tassert(f4 != NULL && f5 != NULL);
    int fd4 = fileno(f4);
    int fd5 = fileno(f5);
    p = (struct _cex_subprocess_s){ .stdout_file = f4, .stderr_file = f5 };
    tassert_eq(0, _cex_subprocess_destroy(&p));
    int probe4 = open("/dev/null", O_RDONLY);
    int probe5 = open("/dev/null", O_RDONLY);
    tassert_eq(fd4, probe4);
    tassert_eq(fd5, probe5);
    close(probe4);
    close(probe5);

    return EOK;
#endif
}

test$case(os_cmd_create_failure_no_handle_leak)
{
#if defined(_WIN32)
    DWORD before = 0;
    DWORD after = 0;
    char* args[] = { "cex_no_such_command_xyz", NULL };
    os_cmd_flags_s flags = { .no_search_path = 1 };

    os_cmd_c warmup = { 0 };
    tassert_ne(EOK, os.cmd.create(&warmup, args, 2, &flags));

    tassert(GetProcessHandleCount(GetCurrentProcess(), &before));
    for (int i = 0; i < 10; i++) {
        os_cmd_c c = { 0 };
        tassert_ne(EOK, os.cmd.create(&c, args, 2, &flags));
    }
    tassert(GetProcessHandleCount(GetCurrentProcess(), &after));
    tassert_le(after, before);
    return EOK;
#else
    return EOK;
#endif
}

test$case(os_cmd_create_success_clears_hstdin)
{
#if defined(_WIN32)
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep_ms", _), "100", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert(c._subpr.hStdInput == NULL);
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
#else
    return EOK;
#endif
}

test$case(os_cmd_pipe_failure_no_fd_leak)
{
#if defined(__linux__)
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') { return EOK; } // valgrind needs fds
    if (TEST_OS_CMD_ASAN) { return EOK; }

    pid_t pid = fork();
    tassert(pid >= 0);
    if (pid == 0) {
        for (int fd = 3; fd < 1024; fd++) { close(fd); }

        struct rlimit lim;
        if (getrlimit(RLIMIT_NOFILE, &lim) != 0) { _exit(2); }
        rlim_t hard = lim.rlim_max;

        const char* args[] = { "sleep", "1", NULL };

        lim.rlim_cur = 4;
        lim.rlim_max = hard;
        if (setrlimit(RLIMIT_NOFILE, &lim) != 0) { _exit(3); }
        struct _cex_subprocess_s p1 = { 0 };
        if (_cex_subprocess_create(args, _cex_subprocess_option_search_user_path, &p1) != -1) {
            _exit(4);
        }
        errno = 0;
        if (fcntl(3, F_GETFD) != -1 || errno != EBADF) { _exit(5); }

        lim.rlim_cur = 5;
        lim.rlim_max = hard;
        if (setrlimit(RLIMIT_NOFILE, &lim) != 0) { _exit(6); }
        struct _cex_subprocess_s p2 = { 0 };
        if (_cex_subprocess_create(args, _cex_subprocess_option_search_user_path, &p2) != -1) {
            _exit(7);
        }
        errno = 0;
        if (fcntl(3, F_GETFD) != -1 || errno != EBADF) { _exit(8); }
        errno = 0;
        if (fcntl(4, F_GETFD) != -1 || errno != EBADF) { _exit(9); }

        lim.rlim_cur = 7;
        lim.rlim_max = hard;
        if (setrlimit(RLIMIT_NOFILE, &lim) != 0) { _exit(10); }
        struct _cex_subprocess_s p3 = { 0 };
        if (_cex_subprocess_create(args, _cex_subprocess_option_search_user_path, &p3) != -1) {
            _exit(11);
        }
        for (int fd = 3; fd <= 6; fd++) {
            errno = 0;
            if (fcntl(fd, F_GETFD) != -1 || errno != EBADF) { _exit(12); }
        }

        _exit(0);
    }

    int status = 0;
    tassert_eq(pid, waitpid(pid, &status, 0));
    tassert(WIFEXITED(status));
    tassertf(WEXITSTATUS(status) == 0, "child exit code %d", WEXITSTATUS(status));
    return EOK;
#else
    return EOK;
#endif
}

test$case(os_cmd_huge_join)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert_er(Error.timeout, os.cmd.wait(&c, 1, 1));
        tassert_eq(os.cmd.ret_code(&c), -1);

        tassert_er(EOK, os.cmd.kill(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_huge_join_stderr)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stderr", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert_er(Error.timeout, os.cmd.wait(&c, 1, 1));
        tassert_eq(os.cmd.ret_code(&c), -1);

        tassert_er(EOK, os.cmd.kill(&c));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_stdin_communucation)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("echo_server", _), NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert(os.cmd.is_alive(&c));

        tassert_eq("welcome to echo server", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "hello"));
        tassert_eq("out: hello", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "world"));
        tassert_eq("out: world", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "end"));

        tassert_er(Error.ok, os.cmd.wait(&c, 1, 1));
    }
    return EOK;
}

test$case(os_cmd_read_all_small_wdelay)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines_delay", _), "stdout", "10", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        log$debug("write_lines_delay output:\n`%s`", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }

        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(output[strlen(output) - 2], '\r');
            tassert_eq(output[strlen(output) - 1], '\n');
            tassert_eq(strlen(output) / 11, arr$len(lines));
        } else {
            tassert_eq(strlen(output) / 10, arr$len(lines));
        }
    }
    return EOK;
}

test$case(os_cmd_run)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "10", NULL);
        tassert_er(EOK, os.cmd.run(args, arr$len(args), &c));
#if defined(_WIN32)
        tassert(c._subpr.hProcess != NULL);
#endif
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_run_macro_space_in_args)
{
    mem$scope(tmem$, _)
    {
        tassert_er(Error.runtime, os$cmd(test_app("write_arg", _)));
        tassert_er(EOK, os$cmd(test_app("write_arg", _), "hello world"));
        tassert_er(EOK, os$cmd(test_app("write_arg", _), "hello=\"world\""));
        tassert_er(EOK, os$cmd(test_app("write_arg", _), "hello=\"world big world\""));
    }
    return EOK;
}

test$case(os_cmd_run_read_all)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_arg", _), "hello world", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        log$debug("write_arg output:\n`%s`", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));

        tassert(str.starts_with(output, "1st argument: 'hello world'"));
    }
    return EOK;
}

test$case(os_cmd_env_inheritance)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        e$ret(os.env.set("TEST_CEX_ENV", "cool!"));
        arr$pushm(args, test_app("write_env", _), "TEST_CEX_ENV", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        printf("%s\n", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert(str.starts_with(output, "TEST_CEX_ENV=cool!"));
        tassert_eq(os.cmd.ret_code(&c), 0);
    }
    return EOK;
}

test$case(os_cmd_env_inheritance_cmd_run)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        e$ret(os.env.set("TEST_CEX_ENV1", "cool!"));
        tassert_er(EOK, os$cmd(test_app("write_env", _), "TEST_CEX_ENV1"));
    }
    return EOK;
}

test$case(os_cmd_env_inheritance_cmd_run_not_found)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        tassert_er(Error.runtime, os$cmd(test_app("write_env", _), "TEST_CEX_ENV_ASLDKJLDJS"));
    }
    return EOK;
}

test$case(os_cmd_wait_all_infinite)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"100", "300", "400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }
        tassert_er(EOK, os.cmd.wait(c, arr$len(c), 0));
    }
    io.printf("Done waiting\n");
    return EOK;
}


test$case(os_cmd_wait_all_timeout)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"1100", "1300", "1400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            io.printf("run sleep_ms %s\n", timeouts[i]);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
            fflush(stdout);
        }

        tassert_er(Error.timeout, os.cmd.wait(c, arr$len(c), 1));

        for$eachp(it, c) {
            tassert_eq(1, os.cmd.is_alive(it));
            // living process always return -1 return code
            tassert_eq(-1, os.cmd.ret_code(it));
        }

        tassert_er(EOK, os.cmd.wait(c, arr$len(c), 1));
        for$eachp(it, c) {
            tassert_eq(0, os.cmd.is_alive(it));
            tassert_eq(0, os.cmd.ret_code(it));
        }
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_all_timeout_partial_done)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"100", "1300", "1400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }

        tassert_er(Error.timeout, os.cmd.wait(c, arr$len(c), 1));

        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(0, os.cmd.ret_code(&c[0]));
        tassert_eq(1, os.cmd.is_alive(&c[1]));
        tassert_eq(-1, os.cmd.ret_code(&c[1]));
        tassert_eq(1, os.cmd.is_alive(&c[2]));
        tassert_eq(-1, os.cmd.ret_code(&c[2]));

        tassert_er(EOK, os.cmd.wait(c, arr$len(c), 1));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(0, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_all_timeout_partial_done_errs)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"117", "1300", "1400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }

        tassert_er(Error.timeout, os.cmd.wait(c, arr$len(c), 1));

        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(1, os.cmd.is_alive(&c[1]));
        tassert_eq(-1, os.cmd.ret_code(&c[1]));
        tassert_eq(1, os.cmd.is_alive(&c[2]));
        tassert_eq(-1, os.cmd.ret_code(&c[2]));

        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 1));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));

        // redundant call, still works
        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 1));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_all_infinite_partial_done_errs)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"117", "300", "400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }

        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 0));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));

        // redundant call, still works
        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 0));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_on_zii_data)
{
    os_cmd_c c[3] = { 0 };

    tassert_er(Error.ok, os.cmd.wait(c, arr$len(c), 0));
    tassert_eq(0, os.cmd.is_alive(&c[0]));
    tassert_eq(0, os.cmd.ret_code(&c[0]));
    tassert_eq(0, os.cmd.is_alive(&c[1]));
    tassert_eq(0, os.cmd.ret_code(&c[1]));
    tassert_eq(0, os.cmd.is_alive(&c[2]));
    tassert_eq(0, os.cmd.ret_code(&c[2]));
    return EOK;
}
test$case(os_cmd_null_args)
{
    os_cmd_c c = { 0 };
    tassert_er(Error.argument, os.cmd.wait(NULL, 1, 0));
    tassert_er(Error.argument, os.cmd.wait(&c, 0, 0));
    tassert_er(Error.argument, os.cmd.write_line(NULL, "x"));
    tassert_er(Error.argument, os.cmd.write_line(&c, NULL));
    char* args[] = { "true", NULL };
    tassert_er(Error.argument, os.cmd.run(args, 2, NULL));
    tassert_er(Error.argument, os.cmd.create(NULL, args, 2, NULL));
    return EOK;
}

test$case(os_cmd_run_arg_validation)
{
    os_cmd_c c = { 0 };
    tassert_er(Error.argument, os.cmd.run(NULL, 0, &c));

    char* no_terminator[] = { "true", NULL };
    tassert_er(Error.argument, os.cmd.run(no_terminator, 1, &c));

    char* null_item[] = { "true", NULL, NULL };
    tassert_er(Error.argument, os.cmd.run(null_item, 3, &c));

    // os.cmd.create() rejects a NULL item too
    tassert_ne(EOK, os.cmd.create(&c, null_item, 3, NULL));
    return EOK;
}

test$case(os_cmd_run_exec_not_found)
{
    os_cmd_c c = { 0 };
    char* args[] = { "cex_no_such_command_xyz", NULL };
#ifdef _WIN32
    tassert_ne(EOK, os.cmd.run(args, 2, &c)); // CreateProcess fails immediately
#else
    tassert_er(EOK, os.cmd.run(args, 2, &c)); // fork succeeds, child exec fails
    tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
#endif
    return EOK;
}

test$case(os_cmd_read_no_stdout)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        char* args[] = { test_app("write_lines", _), NULL };
        tassert_er(EOK, os.cmd.run(args, 2, &c)); // os.cmd.run() creates no stdout pipe

        tassert(os.cmd.read_all(&c, _) == NULL);
        tassert(os.cmd.read_line(&c, _) == NULL);

        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}
#endif  // #if !defined(__EMSCRIPTEN__)


test$main();
