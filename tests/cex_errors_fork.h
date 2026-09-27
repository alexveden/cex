#pragma once
/* Shared fork-based panic probes for the cex_errors test suite. Include AFTER src/all.c. */

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)

/// Fork a child that runs `action`; returns the raw wait() status, or -1 on failure
test$noopt int
run_child_status(void (*action)(void))
{
    pid_t pid = fork();
    if (pid < 0) { return -1; }
    if (pid == 0) {
        (void)freopen("/dev/null", "w", stdout);
        (void)freopen("/dev/null", "w", stderr);
        action();
        _exit(0);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) != pid) { return -1; }
    return status;
}

/// True when the child terminated abnormally (signal or nonzero exit)
test$noopt bool
is_fatal_in_child(void (*action)(void))
{
    int status = run_child_status(action);
    return status >= 0 && !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

/// Child's exit code, or -1 when it was signaled or failed to run
test$noopt int
get_child_exit_code(void (*action)(void))
{
    int status = run_child_status(action);
    if (status < 0 || !WIFEXITED(status)) { return -1; }
    return WEXITSTATUS(status);
}

test$noopt void
_run_uassert_panic(void)
{
    uassert(false);
}

test$noopt void
_run_uassert_always_panic(void)
{
    uassert_always(false);
}

#endif
