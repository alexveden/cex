// Standalone crash fixture for tests/test_panic.c.
//
// Built by `./cex test` (see cex.c:cmd_custom_test) into
// build/tests/os_test/panic.c.<platform>[.exe] and executed as a child process.
// Unlike the test binary it is compiled without sanitizers, so it exercises the
// real crash path (uassert -> cex$platform_panic, and raw crash signals).
//
// argv[1] selects the fault: segv (default), assert, abort, fpe. The fault is
// raised at the bottom of a nested call chain, so the report carries several
// `app` frames below main().
#define CEX_IMPLEMENTATION
#include "cex.h"

static void
_panic_fixture_level4(char* mode)
{
    if (str.eq(mode, "assert")) {
        uassert(false && "panic fixture: assert");
    } else if (str.eq(mode, "abort")) {
        abort();
    } else if (str.eq(mode, "fpe")) {
#if defined(_WIN32)
        RaiseException(EXCEPTION_INT_DIVIDE_BY_ZERO, 0, 0, NULL);
#else
        raise(SIGFPE);
#endif
    } else {
        volatile int* p = NULL;
        *p = 1;
    }
}

static void
_panic_fixture_level3(char* mode)
{
    _panic_fixture_level4(mode);
}

static void
_panic_fixture_level2(char* mode)
{
    _panic_fixture_level3(mode);
}

static void
_panic_fixture_level1(char* mode)
{
    _panic_fixture_level2(mode);
}

int
main(int argc, char** argv)
{
    char* mode = (argc > 1) ? argv[1] : "segv";
    _panic_fixture_level1(mode);
    return 0;
}
