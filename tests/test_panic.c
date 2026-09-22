#include "src/all.c"

#if defined(__linux__) || defined(__APPLE__)
#    include <sys/wait.h>
#    include <unistd.h>

static int
_run_child(void (*fn)(void), char* out, usize out_cap, int* out_sig)
{
    int fds[2];
    if (pipe(fds) != 0) { return -1; }
    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return -1;
    }
    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], 2);
        close(fds[1]);
        fn();
        _exit(0);
    }
    close(fds[1]);
    usize n = 0;
    ssize_t r = 0;
    while (n + 1 < out_cap && (r = read(fds[0], out + n, out_cap - 1 - n)) > 0) { n += (usize)r; }
    out[n] = '\0';
    close(fds[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    *out_sig = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
    return 0;
}

static void
_child_report(void)
{
    _cex__report("assert", 0, 0, 1);
}

static void
_child_uassert(void)
{
    uassert(false && "test_panic_uassert");
}

static void
_child_signal(void)
{
    __cex__catch_signals();
    raise(SIGSEGV);
}
#endif

#ifdef _WIN32
static int
_run_child_win32(const char* mode, char* out, usize out_cap, DWORD* out_code)
{
    SECURITY_ATTRIBUTES sa = { .nLength = sizeof(sa), .bInheritHandle = TRUE };
    HANDLE rd = NULL;
    HANDLE wr = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 0)) { return -1; }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = wr;
    si.hStdOutput = wr;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = { 0 };
    char exe[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, exe, sizeof(exe)) == 0) {
        CloseHandle(rd);
        CloseHandle(wr);
        return -1;
    }

    SetEnvironmentVariableA("CEX_PANIC_CHILD", mode);
    char cmd[MAX_PATH + 4] = { 0 };
    usize elen = strlen(exe);
    cmd[0] = '"';
    memcpy(cmd + 1, exe, elen);
    cmd[elen + 1] = '"';
    BOOL ok = CreateProcessA(exe, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    SetEnvironmentVariableA("CEX_PANIC_CHILD", NULL);
    CloseHandle(wr);
    if (!ok) {
        CloseHandle(rd);
        return -1;
    }

    usize n = 0;
    DWORD r = 0;
    while (n + 1 < out_cap && ReadFile(rd, out + n, (DWORD)(out_cap - 1 - n), &r, NULL) && r > 0) {
        n += r;
    }
    out[n] = '\0';
    CloseHandle(rd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    *out_code = code;
    return 0;
}
#endif

test$setup_suite()
{
#ifdef _WIN32
    const char* mode = getenv("CEX_PANIC_CHILD");
    if (mode != NULL) {
        if (strcmp(mode, "report") == 0) {
            _cex__report("assert", 0, 0, 1);
        } else if (strcmp(mode, "segv") == 0) {
            __cex__catch_signals();
            volatile int* p = NULL;
            *p = 1;
        }
        exit(0);
    }
#endif
    return EOK;
}

test$case(test_panic_report_format)
{
#if defined(__linux__) || defined(__APPLE__)
    char out[8192];
    int sig = 0;
    tassert_eq(_run_child(_child_report, out, sizeof(out), &sig), 0);
    tassert(sig == 0);
#elif defined(_WIN32)
    char out[8192];
    DWORD code = 0;
    tassert_eq(_run_child_win32("report", out, sizeof(out), &code), 0);
    tassert(code == 0);
#else
    return EOK;
#endif
    tassert(str.find(out, "=== CEX CRASH REPORT v1 ===") != NULL);
    tassert(str.find(out, "reason: assert") != NULL);
    tassert(str.find(out, "exe_base: 0x") != NULL);
    tassert(str.find(out, "frame_count: ") != NULL);
    tassert(str.find(out, "frame: 0x") != NULL);
    tassert(str.find(out, "  app +0x") != NULL);
    tassert(str.find(out, "=== END ===") != NULL);
    return EOK;
}

test$case(test_panic_uassert_crashes)
{
#if defined(__linux__) || defined(__APPLE__)
    char out[8192];
    int sig = 0;
    tassert_eq(_run_child(_child_uassert, out, sizeof(out), &sig), 0);
    tassert(sig != 0);
#else
    return EOK;
#endif
    return EOK;
}

test$case(test_panic_signal_report)
{
#if defined(__linux__) || defined(__APPLE__)
    char out[8192];
    int sig = 0;
    tassert_eq(_run_child(_child_signal, out, sizeof(out), &sig), 0);
    tassert(str.find(out, "=== CEX CRASH REPORT v1 ===") != NULL);
    tassert(str.find(out, "reason: signal") != NULL);
    tassert(str.find(out, "signal: 11") != NULL);
    tassert(sig == SIGSEGV);
#elif defined(_WIN32) && !mem$asan_enabled()
    char out[8192];
    DWORD code = 0;
    tassert_eq(_run_child_win32("segv", out, sizeof(out), &code), 0);
    tassert(str.find(out, "=== CEX CRASH REPORT v1 ===") != NULL);
    tassert(str.find(out, "reason: exception") != NULL);
    tassert(str.find(out, "exception: 0x") != NULL);
    tassert(str.find(out, "  app +0x") != NULL);
    tassert(code != 0);
#else
    return EOK;
#endif
    return EOK;
}

test$main();
