#include "src/all.c"

// Fil-C owns the crash path (like ASAN), so the address-only report is not emitted; skip
#if !defined(__EMSCRIPTEN__) && !defined(__FILC__)

/// Path to the prebuilt crash fixture (see cex.c:cmd_custom_test), e.g.
/// `build/tests/os_test/panic.c.linux`
static char*
_panic_app(IAllocator allc)
{
    return cexy.target_make(
        "tests/os_test/panic.c",
        cexy$build_dir,
        str.fmt(allc, ".%s", os.platform.to_str(os.platform.current())),
        allc
    );
}

/// Runs the fixture with `mode` and returns its combined stdout+stderr (on `allc`).
/// Returns NULL if the fixture is missing or exits successfully (it must crash).
static char*
_run_panic(char* mode, IAllocator allc)
{
    char* app = _panic_app(allc);
    if (app == NULL || !os.path.exists(app)) {
        log$error("panic fixture not built: %s\n", app ? app : "(null)");
        return NULL;
    }

    char* args[] = { app, mode, NULL };
    os_cmd_c c = { 0 };
    os_cmd_flags_s flags = { .combine_stdouterr = 1, .no_window = 1 };
    if (os.cmd.create(&c, args, arr$len(args), &flags) != EOK) { return NULL; }

    char* out = os.cmd.read_all(&c, allc);
    if (os.cmd.wait(&c, 1, 0) == EOK) {
        log$error("panic fixture did not crash: mode=%s ret_code=%d\n", mode, os.cmd.ret_code(&c));
        return NULL;
    }
    return out;
}

// NOTE: tassert() returns from the enclosing test$case, so shared assertions live in a macro
#define _panic$assert_common(out)                                                                  \
    do {                                                                                           \
        tassert(out != NULL);                                                                      \
        tassert(str.find(out, "=== CEX CRASH REPORT v1 ===") != NULL);                             \
        tassert(str.find(out, "exe_base: 0x") != NULL);                                            \
        tassert(str.find(out, "frame_count: ") != NULL);                                           \
        tassert(str.find(out, "frame: 0x") != NULL);                                               \
        tassert(str.find(out, "  app +0x") != NULL);                                               \
        tassert(str.find(out, "=== END ===") != NULL);                                             \
    } while (0)

test$case(test_panic_assert)
{
    mem$scope(tmem$, _)
    {
        char* out = _run_panic("assert", _);
        _panic$assert_common(out);
        tassert(str.find(out, "reason: assert") != NULL);
    }
    return EOK;
}

test$case(test_panic_segv)
{
    mem$scope(tmem$, _)
    {
        char* out = _run_panic("segv", _);
        _panic$assert_common(out);
#if defined(__linux__) || defined(__APPLE__)
        tassert(str.find(out, "reason: signal") != NULL);
        tassert(str.find(out, "signal: 11") != NULL);
#elif defined(_WIN32)
        tassert(str.find(out, "reason: exception") != NULL);
        tassert(str.find(out, "exception: 0x") != NULL);
#endif
    }
    return EOK;
}

test$case(test_panic_fpe)
{
    mem$scope(tmem$, _)
    {
        char* out = _run_panic("fpe", _);
        _panic$assert_common(out);
#if defined(__linux__) || defined(__APPLE__)
        tassert(str.find(out, "signal: 8") != NULL);
#endif
    }
    return EOK;
}

test$case(test_panic_abort)
{
#if defined(__linux__) && !defined(__GLIBC__)
    // musl and other non-glibc libcs lack unwind tables in abort()/raise(), so the
    // report cannot cross those frames and has no app frames; skip.
    return EOK;
#elif defined(__linux__) || defined(__APPLE__)
    mem$scope(tmem$, _)
    {
        char* out = _run_panic("abort", _);
        _panic$assert_common(out);
        tassert(str.find(out, "signal: 6") != NULL);
    }
#endif
    return EOK;
}

#undef _panic$assert_common

#endif // #if !defined(__EMSCRIPTEN__) && !defined(__FILC__)

test$main();
