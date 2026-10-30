/*
 * Native unit tests.
 *
 * CEX tests are unity builds: the implementation is #included directly so the
 * test can see static helpers and there is no separate link step. CEX_TEST
 * enables the test runner, per-case arena and sanitizer hooks.
 */
#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/cex_demo.c"

/* Happy path: a valid request is incremented and the call returns ok (NULL). */
test$case(cex_demo_run_increments)
{
    cex_demo_request_t req = { .value = 41 };
    tassert(cex_demo_run(&req) == cex_demo_Error.ok);
    tassert_eq(req.value, 42);
    return EOK;
}

/* Error path: a NULL request is rejected with the exported argument error. */
test$case(cex_demo_run_null)
{
    tassert(cex_demo_run(NULL) == cex_demo_Error.argument);
    return EOK;
}

/* The exported error pointers must be the same objects as CEX's Error.*. */
test$case(cex_demo_error_consts)
{
    tassert(cex_demo_Error.ok == Error.ok);
    tassert(cex_demo_Error.argument == Error.argument);
    tassert(cex_demo_Error.overflow == Error.overflow);
    return EOK;
}

/* Invalid buffer arguments are rejected before touching memory. */
test$case(cex_demo_traceback_null)
{
    char buf[8];
    tassert(cex_demo_traceback(NULL, 0) == cex_demo_Error.argument);
    tassert(cex_demo_traceback(buf, 0) == cex_demo_Error.argument);
    return EOK;
}

/*
 * A too-small buffer reports overflow, keeps the truncated prefix and stays
 * NUL-terminated. A large buffer returns ok.
 */
test$case(cex_demo_traceback_overflow)
{
    tassert(cex_demo_run(NULL) == cex_demo_Error.argument);
    char tiny[8];
    tassert(cex_demo_traceback(tiny, sizeof tiny) == cex_demo_Error.overflow);
    tassert(tiny[sizeof tiny - 1] == '\0');

    char buf[512];
    tassert(cex_demo_traceback(buf, sizeof buf) == cex_demo_Error.ok);
    tassert(strstr(buf, "cex_demo_run") != NULL);
    return EOK;
}

/*
 * The traceback must expose the internal call path and the originating error,
 * proving that frames cross the library boundary.
 */
test$case(cex_demo_traceback_contents)
{
    tassert(cex_demo_run(NULL) == cex_demo_Error.argument);
    char buf[512];
    tassert(cex_demo_traceback(buf, sizeof buf) == cex_demo_Error.ok);
    tassert(strstr(buf, "cex_demo__validate_request") != NULL);
    tassert(strstr(buf, "cex_demo_run") != NULL);
    tassert(strstr(buf, "ArgumentError") != NULL);
    return EOK;
}

/* A successful call records no frames, so the traceback is empty. */
test$case(cex_demo_traceback_empty)
{
    cex_demo_request_t req = { .value = 1 };
    tassert(cex_demo_run(&req) == cex_demo_Error.ok);
    char buf[8];
    tassert(cex_demo_traceback(buf, sizeof buf) == cex_demo_Error.ok);
    tassert(buf[0] == '\0');
    return EOK;
}

test$main();
