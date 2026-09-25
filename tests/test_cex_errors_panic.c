#include "src/all.c"
#include "src/cex_errors.h"
#include "src/cex_errors.c"

#ifndef _WIN32
/// Fork a child that runs the panic; true when it terminated abnormally (signal or nonzero exit)
test$noopt bool
is_panic_fatal_in_child(bool use_unreachable)
{
    pid_t pid = fork();
    if (pid < 0) { return false; }
    if (pid == 0) {
        (void)freopen("/dev/null", "w", stdout);
        (void)freopen("/dev/null", "w", stderr);
        if (use_unreachable) { unreachable(); }
        else { uassert(false); }
        _exit(0);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) != pid) { return false; }
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

test$case(uassert_enabled_kills_process)
{
    tassert(is_panic_fatal_in_child(false));
    return EOK;
}

test$case(unreachable_kills_process)
{
    tassert(is_panic_fatal_in_child(true));
    return EOK;
}

test$case(unreachable_kills_process_when_disabled)
{
    uassert_disable();
    bool fatal = is_panic_fatal_in_child(true);
    uassert_enable();
    tassert(fatal);
    return EOK;
}
#endif

test$main();
