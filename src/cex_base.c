#pragma once

#include "cex_base.h"

const struct _CEX_Error_struct Error = {
    .ok = EOK,                           // Success
    .memory = "MemoryError",             // memory allocation error
    .io = "IOError",                     // IO error
    .overflow = "OverflowError",         // buffer overflow
    .argument = "ArgumentError",         // function argument error
    .integrity = "IntegrityError",       // data integrity error
    .exists = "ExistsError",             // entity or key already exists
    .not_found = "NotFoundError",        // entity or key already exists
    .skip = "ShouldBeSkipped",           // NOT an error, function result must be skipped
    .null_or_empty = "NullOrEmptyError", // value is null or resource is empty
    .eof = "EOF",                        // end of file reached
    .argsparse = "ProgramArgsError",     // program arguments empty or incorrect
    .runtime = "RuntimeError",           // generic runtime error
    .assert = "AssertError",             // generic runtime check
    .os = "OSError",                     // generic OS check
    .timeout = "TimeoutError",           // await interval timeout
    .permission = "PermissionError",     // Permission denied
    .try_again = "TryAgainError",        // EAGAIN / EWOULDBLOCK errno analog for async operations
};

#ifdef _cex$platform_panic_builtin

// Crash report: async-signal-safe, address-only traceback that survives symbol stripping.
// Frames are tagged `app` (inside the executable) or `other`; symbolize offline with addr2line.
#ifndef CEX_TRACEBACK_MAX_FRAMES
#    define CEX_TRACEBACK_MAX_FRAMES 64
#endif

#if !cex$is_freestanding && (defined(__linux__) || defined(__APPLE__) || defined(__MINGW32__))
#    define _cex__unwind 1
#    include <unwind.h>
#else
#    define _cex__unwind 0
#endif

#if !cex$is_freestanding && (defined(__linux__) || defined(__APPLE__))
#    define _cex__posix 1
#    include <signal.h>
#    include <unistd.h>
#    if defined(__APPLE__)
#        include <mach-o/dyld.h>
#        include <mach-o/loader.h>
#    endif
#else
#    define _cex__posix 0
#endif

#if _cex__unwind && defined(_WIN32) && !defined(CEX_NO_WIN32_TYPES)
#    define _cex__win32 1
#else
#    define _cex__win32 0
#endif

static int _cex__in_panic;

#if _cex__unwind

#if _cex__posix
static void
_cex__fd_write(int fd, const char* s, usize n)
{
    while (n > 0) {
        ssize_t w = write(fd, s, n);
        if (w <= 0) { return; }
        s += w;
        n -= (usize)w;
    }
}
#elif _cex__win32
static void
_cex__fd_write(int fd, const char* s, usize n)
{
    (void)fd;
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), s, (DWORD)n, &written, NULL);
}
#else
static void
_cex__fd_write(int fd, const char* s, usize n)
{
    (void)fd;
    fwrite(s, 1, n, stderr);
}
#endif

static void
_cex__put(int fd, const char* s)
{
    usize n = 0;
    while (s[n] != '\0') { n++; }
    _cex__fd_write(fd, s, n);
}

static void
_cex__put_hex(int fd, uintptr_t v)
{
    static const char hex[] = "0123456789abcdef";
    char buf[2 + sizeof(uintptr_t) * 2];
    int digits = (int)(sizeof(uintptr_t) * 2);
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < digits; i++) { buf[2 + i] = hex[(v >> ((digits - 1 - i) * 4)) & 0xf]; }
    _cex__fd_write(fd, buf, sizeof(buf));
}

static void
_cex__put_dec(int fd, long v)
{
    char buf[24];
    int i = (int)sizeof(buf);
    bool neg = v < 0;
    unsigned long u = neg ? (unsigned long)(-v) : (unsigned long)v;
    if (u == 0) { buf[--i] = '0'; }
    while (u > 0) {
        buf[--i] = (char)('0' + (u % 10));
        u /= 10;
    }
    if (neg) { buf[--i] = '-'; }
    _cex__fd_write(fd, &buf[i], sizeof(buf) - (usize)i);
}

struct _cex__unwind_s {
    void* frames[CEX_TRACEBACK_MAX_FRAMES];
    int n;
    int skip;
};

static _Unwind_Reason_Code
_cex__unwind_cb(struct _Unwind_Context* ctx, void* arg)
{
    struct _cex__unwind_s* u = arg;
    uintptr_t ip = (uintptr_t)_Unwind_GetIP(ctx);
    if (u->skip > 0) {
        u->skip--;
        return _URC_NO_REASON;
    }
    if (ip != 0 && u->n < CEX_TRACEBACK_MAX_FRAMES) { u->frames[u->n++] = (void*)ip; }
    return u->n < CEX_TRACEBACK_MAX_FRAMES ? _URC_NO_REASON : _URC_END_OF_STACK;
}

__attribute__((noinline)) static int
_cex__capture_frames(void** out, int skip)
{
    struct _cex__unwind_s u = { .n = 0, .skip = skip };
    _Unwind_Backtrace(_cex__unwind_cb, &u);
    for (int i = 0; i < u.n; i++) { out[i] = u.frames[i]; }
    return u.n;
}

static uintptr_t _cex__exe_lo;
static uintptr_t _cex__exe_hi;

static bool
_cex__is_app_addr(uintptr_t ip)
{
    return _cex__exe_lo != 0 && ip >= _cex__exe_lo && ip < _cex__exe_hi;
}

#if _cex__posix
#    if defined(__linux__)
extern char __ehdr_start[] __attribute__((weak));
extern char __executable_start[] __attribute__((weak));
extern char _end[] __attribute__((weak));
static void
_cex__init_app_range(void)
{
    uintptr_t lo = (uintptr_t)__ehdr_start;
    if (lo == 0) { lo = (uintptr_t)__executable_start; }
    _cex__exe_lo = lo;
    _cex__exe_hi = (uintptr_t)_end;
}
#    else // __APPLE__
static void
_cex__init_app_range(void)
{
    const struct mach_header_64* h = (const struct mach_header_64*)_dyld_get_image_header(0);
    if (h == NULL) { return; }
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    _cex__exe_lo = (uintptr_t)h;
    uintptr_t hi = _cex__exe_lo;
    const unsigned char* p = (const unsigned char*)h + sizeof(struct mach_header_64);
    for (uint32_t i = 0; i < h->ncmds; i++) {
        const struct load_command* lc = (const struct load_command*)p;
        if (lc->cmdsize == 0) { break; }
        if (lc->cmd == LC_SEGMENT_64) {
            const struct segment_command_64* sg = (const struct segment_command_64*)lc;
            uintptr_t end = (uintptr_t)(sg->vmaddr + (uint64_t)slide + sg->vmsize);
            if (end > hi) { hi = end; }
        }
        p += lc->cmdsize;
    }
    _cex__exe_hi = hi;
}
#    endif
#elif _cex__win32
static void
_cex__init_app_range(void)
{
    const unsigned char* base = (const unsigned char*)GetModuleHandleA(NULL);
    if (base == NULL) { return; }
    u32 e_lfanew = 0;
    memcpy(&e_lfanew, base + 0x3c, 4);
    if (e_lfanew < 0x40 || e_lfanew > 0x1000) { return; }
    // optional header starts at signature (4) + COFF header (20); SizeOfImage at +56
    u32 size_of_image = 0;
    memcpy(&size_of_image, base + e_lfanew + 24 + 56, 4);
    if (size_of_image == 0) { return; }
    _cex__exe_lo = (uintptr_t)base;
    _cex__exe_hi = _cex__exe_lo + size_of_image;
}
#endif // _cex__posix || _cex__win32

__attribute__((noinline)) static void
_cex__report(const char* reason, int sig, uintptr_t fault_addr, int skip)
{
    void* frames[CEX_TRACEBACK_MAX_FRAMES];
    int fd = 2;

    if (__atomic_exchange_n(&_cex__in_panic, 1, __ATOMIC_RELAXED) != 0) { return; }
    int n = _cex__capture_frames(frames, skip);

    _cex__put(fd, "\n=== CEX CRASH REPORT v1 ===\n");
    _cex__put(fd, "reason: ");
    _cex__put(fd, reason);
    _cex__put(fd, "\n");
    if (sig != 0) {
#if _cex__win32
        _cex__put(fd, "exception: ");
        _cex__put_hex(fd, (uintptr_t)(u32)sig);
#else
        _cex__put(fd, "signal: ");
        _cex__put_dec(fd, sig);
#endif
        _cex__put(fd, "\n");
    }
    if (fault_addr != 0) {
        _cex__put(fd, "fault_addr: ");
        _cex__put_hex(fd, fault_addr);
        _cex__put(fd, "\n");
    }
#if _cex__posix || _cex__win32
    _cex__put(fd, "exe_base: ");
    _cex__put_hex(fd, _cex__exe_lo);
    _cex__put(fd, "\n");
#endif
    _cex__put(fd, "frame_count: ");
    _cex__put_dec(fd, n);
    _cex__put(fd, "\n");
    for (int i = 0; i < n; i++) {
        uintptr_t ip = (uintptr_t)frames[i];
        _cex__put(fd, "frame: ");
        _cex__put_hex(fd, ip);
        if (_cex__is_app_addr(ip)) {
            _cex__put(fd, "  app +");
            _cex__put_hex(fd, ip - _cex__exe_lo);
        } else {
            _cex__put(fd, "  other");
        }
        _cex__put(fd, "\n");
    }
    _cex__put(fd, "=== END ===\n");
}

#else // _cex__unwind
__attribute__((noinline)) static void
_cex__report(const char* reason, int sig, uintptr_t fault_addr, int skip)
{
    (void)reason;
    (void)sig;
    (void)fault_addr;
    (void)skip;
}
#endif // _cex__unwind

void
__cex__panic(void)
{
    fflush(stdout);
    fflush(stderr);
    sanitizer_stack_trace();

    if (!mem$asan_enabled()) { _cex__report("assert", 0, 0, 3); }

#    ifdef CEX_TEST
    breakpoint();
#    endif
    abort();
}

#if _cex__posix
static void
_cex__reraise_signal(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
    signal(sig, SIG_DFL);
    raise(sig);
    _exit(128 + sig);
}

static void
_cex__signal_handler(int sig, siginfo_t* info, void* ucontext)
{
    (void)ucontext;
    uintptr_t fault = (info != NULL) ? (uintptr_t)info->si_addr : 0;
    _cex__report("signal", sig, fault, 3);
    _cex__reraise_signal(sig);
}

void
__cex__catch_signals(void)
{
    static const int sigs[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS };
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = _cex__signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    for (usize i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) { sigaction(sigs[i], &sa, NULL); }
}
#elif _cex__win32
static long __stdcall
_cex__win32_exception_filter(EXCEPTION_POINTERS* ep)
{
    int code = 0;
    uintptr_t fault = 0;
    if (ep != NULL && ep->ExceptionRecord != NULL) {
        code = (int)ep->ExceptionRecord->ExceptionCode;
        fault = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;
    }
    _cex__report("exception", code, fault, 3);
    return EXCEPTION_CONTINUE_SEARCH;
}

void
__cex__catch_signals(void)
{
    SetUnhandledExceptionFilter(_cex__win32_exception_filter);
}
#endif // _cex__posix / _cex__win32

#if _cex__posix || _cex__win32
__attribute__((constructor)) static void
_cex__crash_init(void)
{
    _cex__init_app_range();
#    ifndef CEX_DISABLE_SIGNAL_PANIC
    if (!mem$asan_enabled()) {
        __cex__catch_signals();
#        if _cex__win32
        SetErrorMode(SEM_NOGPFAULTERRORBOX);
#        endif
    }
#    endif
}
#endif

#endif // _cex$platform_panic_builtin
