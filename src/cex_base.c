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

#if !cex$is_freestanding && (defined(__linux__) || defined(__APPLE__) || defined(__MINGW32__)) && \
    (!defined(cex$enable_minimal) || defined(cex$enable_str))
#    define _cex__unwind 1
#    include <unwind.h>
#else
#    define _cex__unwind 0
#endif

#if _cex__unwind && (defined(__linux__) || defined(__APPLE__))
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

#if _cex__unwind

static int _cex__in_panic;

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
    static alignas(8) char buf[256 + CEX_TRACEBACK_MAX_FRAMES * 64];

    if (__atomic_exchange_n(&_cex__in_panic, 1, __ATOMIC_RELAXED) != 0) { return; }
    int n = _cex__capture_frames(frames, skip);

    sbuf_c s = sbuf.create_static(buf, sizeof(buf));

    sbuf.appendf(&s, "\n=== CEX CRASH REPORT v1 ===\nreason: %s\n", reason);
    if (sig != 0) {
#if _cex__win32
        sbuf.appendf(&s, "exception: %p\n", (void*)(uintptr_t)(u32)sig);
#else
        sbuf.appendf(&s, "signal: %d\n", sig);
#endif
    }
    if (fault_addr != 0) { sbuf.appendf(&s, "fault_addr: %p\n", (void*)fault_addr); }
#if _cex__posix || _cex__win32
    sbuf.appendf(&s, "exe_base: %p\n", (void*)_cex__exe_lo);
#endif
    sbuf.appendf(&s, "frame_count: %d\n", n);
    for (int i = 0; i < n; i++) {
        uintptr_t ip = (uintptr_t)frames[i];
        if (_cex__is_app_addr(ip)) {
            uintptr_t off = ip - _cex__exe_lo;
            sbuf.appendf(&s, "frame: %p  app +%p\n", (void*)ip, (void*)off);
        } else {
            sbuf.appendf(&s, "frame: %p  other\n", (void*)ip);
        }
    }
    sbuf.appendf(&s, "=== END ===\n");

    _cex__fd_write(2, s, sbuf.len(&s));
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
