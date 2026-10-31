#if defined(_WIN32) || defined(__EMSCRIPTEN__)

#    include "src/all.c"
#    include "src/all.h"

test$case(os_cmd_faults_not_supported)
{
    return EOK;
}

test$main();

#else

#    include "src/all.h"

// The subprocess implementation calls fdopen/fcntl/posix_spawn_file_actions_* directly.
// These wrappers are installed before src/all.c is compiled, so the unity build resolves
// _subprocess.c calls to the wrappers below. src/all.h is included first so the real
// declarations are already parsed and the wrappers can call through to them.

static int test_fcntl_fail_on_call = 0;
static int test_fcntl_calls = 0;

static int
test_fcntl(int fd, int cmd, ...)
{
    if (test_fcntl_fail_on_call && ++test_fcntl_calls == test_fcntl_fail_on_call) {
        errno = EINVAL;
        return -1;
    }

    va_list args;
    va_start(args, cmd);

    if (cmd == F_SETFD) {
        int arg = va_arg(args, int);
        va_end(args);
        return fcntl(fd, cmd, arg);
    }

    va_end(args);
    uassert(cmd == F_GETFD);
    return fcntl(fd, cmd);
}

static int test_actions_init_fail = 0;

static int
test_actions_init(posix_spawn_file_actions_t* actions)
{
    if (test_actions_init_fail) { return ENOMEM; }
    return posix_spawn_file_actions_init(actions);
}

static int test_addclose_fail_on_call = 0;
static int test_addclose_calls = 0;

static int
test_addclose(posix_spawn_file_actions_t* actions, int fd)
{
    if (test_addclose_fail_on_call && ++test_addclose_calls == test_addclose_fail_on_call) {
        return ENOMEM;
    }
    return posix_spawn_file_actions_addclose(actions, fd);
}

static int test_adddup2_fail_on_call = 0;
static int test_adddup2_calls = 0;

static int
test_adddup2(posix_spawn_file_actions_t* actions, int fd, int new_fd)
{
    if (test_adddup2_fail_on_call && ++test_adddup2_calls == test_adddup2_fail_on_call) {
        return ENOMEM;
    }
    return posix_spawn_file_actions_adddup2(actions, fd, new_fd);
}

static int test_fdopen_fail_on_call = 0;
static int test_fdopen_calls = 0;

static FILE*
test_fdopen(int fd, const char* mode)
{
    if (test_fdopen_fail_on_call && ++test_fdopen_calls == test_fdopen_fail_on_call) {
        errno = ENOMEM;
        return NULL;
    }
    return fdopen(fd, mode);
}

#    define fcntl test_fcntl
#    define posix_spawn_file_actions_init test_actions_init
#    define posix_spawn_file_actions_addclose test_addclose
#    define posix_spawn_file_actions_adddup2 test_adddup2
#    define fdopen test_fdopen

#    include "src/all.c"

#    undef fcntl
#    undef posix_spawn_file_actions_init
#    undef posix_spawn_file_actions_addclose
#    undef posix_spawn_file_actions_adddup2
#    undef fdopen

static void
os_cmd_faults_reset(void)
{
    test_fcntl_fail_on_call = 0;
    test_fcntl_calls = 0;
    test_actions_init_fail = 0;
    test_addclose_fail_on_call = 0;
    test_addclose_calls = 0;
    test_adddup2_fail_on_call = 0;
    test_adddup2_calls = 0;
    test_fdopen_fail_on_call = 0;
    test_fdopen_calls = 0;
}

test$setup_case()
{
    os_cmd_faults_reset();
    return EOK;
}

test$teardown_case()
{
    os_cmd_faults_reset();
    return EOK;
}

enum os_cmd_fault_kind_e
{
    os_cmd_fault_fcntl,
    os_cmd_fault_actions_init,
    os_cmd_fault_addclose,
    os_cmd_fault_adddup2,
    os_cmd_fault_fdopen,
};

static Exception
os_cmd_fault_check(
    enum os_cmd_fault_kind_e kind,
    int fail_on_call,
    os_cmd_flags_s flags,
    int expected_errno
)
{
    const char* args[] = { "sleep", "1", NULL };
    int sub_flags = _cex_subprocess_option_search_user_path;
    if (flags.combine_stdouterr) {
        sub_flags |= _cex_subprocess_option_combined_stdout_stderr;
    }

    int probe = open("/dev/null", O_RDONLY);
    tassert(probe >= 0);
    close(probe);

    struct _cex_subprocess_s p = { 0 };

    switch (kind) {
    case os_cmd_fault_fcntl:
        test_fcntl_fail_on_call = fail_on_call;
        test_fcntl_calls = 0;
        break;
    case os_cmd_fault_actions_init:
        test_actions_init_fail = 1;
        break;
    case os_cmd_fault_addclose:
        test_addclose_fail_on_call = fail_on_call;
        test_addclose_calls = 0;
        break;
    case os_cmd_fault_adddup2:
        test_adddup2_fail_on_call = fail_on_call;
        test_adddup2_calls = 0;
        break;
    case os_cmd_fault_fdopen:
        test_fdopen_fail_on_call = fail_on_call;
        test_fdopen_calls = 0;
        break;
    }

    errno = 0;
    int rc = _cex_subprocess_create(args, sub_flags, &p);
    os_cmd_faults_reset();

    tassert_eq(-1, rc);
    tassert_eq(expected_errno, errno);
    tassert_eq(0, p.alive);
    tassert_eq(0, p.child);
    tassert(p.stdin_file == NULL);
    tassert(p.stdout_file == NULL);
    tassert(p.stderr_file == NULL);

    int n_fds = flags.combine_stdouterr ? 4 : 6;
    int probes[6] = { -1 };
    for (int i = 0; i < n_fds; i++) { probes[i] = open("/dev/null", O_RDONLY); }
    for (int i = 0; i < n_fds; i++) {
        if (probes[i] >= 0) { close(probes[i]); }
    }
    for (int i = 0; i < n_fds; i++) { tassert_eq(probe + i, probes[i]); }

    return EOK;
}

test$case(os_cmd_fault_fcntl)
{
    for (int call = 1; call <= 4; call++) {
        e$ret(os_cmd_fault_check(os_cmd_fault_fcntl, call, (os_cmd_flags_s){ 0 }, EINVAL));
    }
    return EOK;
}

test$case(os_cmd_fault_actions_init)
{
    e$ret(os_cmd_fault_check(
        os_cmd_fault_actions_init, 1, (os_cmd_flags_s){ 0 }, ENOMEM
    ));
    e$ret(os_cmd_fault_check(
        os_cmd_fault_actions_init, 1, (os_cmd_flags_s){ .combine_stdouterr = 1 }, ENOMEM
    ));
    return EOK;
}

test$case(os_cmd_fault_addclose)
{
    for (int call = 1; call <= 3; call++) {
        e$ret(os_cmd_fault_check(os_cmd_fault_addclose, call, (os_cmd_flags_s){ 0 }, ENOMEM));
    }
    for (int call = 1; call <= 2; call++) {
        e$ret(os_cmd_fault_check(
            os_cmd_fault_addclose, call, (os_cmd_flags_s){ .combine_stdouterr = 1 }, ENOMEM
        ));
    }
    return EOK;
}

test$case(os_cmd_fault_adddup2)
{
    for (int call = 1; call <= 3; call++) {
        e$ret(os_cmd_fault_check(os_cmd_fault_adddup2, call, (os_cmd_flags_s){ 0 }, ENOMEM));
    }
    for (int call = 1; call <= 3; call++) {
        e$ret(os_cmd_fault_check(
            os_cmd_fault_adddup2, call, (os_cmd_flags_s){ .combine_stdouterr = 1 }, ENOMEM
        ));
    }
    return EOK;
}

test$case(os_cmd_fault_fdopen)
{
    for (int call = 1; call <= 3; call++) {
        e$ret(os_cmd_fault_check(os_cmd_fault_fdopen, call, (os_cmd_flags_s){ 0 }, ENOMEM));
    }
    for (int call = 1; call <= 2; call++) {
        e$ret(os_cmd_fault_check(
            os_cmd_fault_fdopen, call, (os_cmd_flags_s){ .combine_stdouterr = 1 }, ENOMEM
        ));
    }
    return EOK;
}

test$main();

#endif
