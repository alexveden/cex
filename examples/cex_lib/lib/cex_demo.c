/*
 * Library implementation.
 *
 * CEX_IMPLEMENTATION pulls the full CEX implementation into this translation
 * unit. The library therefore embeds CEX (error handling, traceback ring,
 * formatting) and has no runtime dependency besides libc.
 */
#define CEX_IMPLEMENTATION
#include "cex.h"

/*
 * CEX_DEMO_BUILD tells the public header that we are compiling the library
 * itself, so CEX_DEMO_API becomes dllexport on Windows instead of dllimport.
 */
#define CEX_DEMO_BUILD
#include "cex_demo.h"

/*
 * Static initializer for the exported error table.
 *
 * `Error.argument` cannot be used here: `Error` is a const struct object, not
 * a compile-time constant, so it cannot initialize another global. The
 * `_cex_errors_Error_*` arrays are address constants and are the exact same
 * objects that `Error.*` points at, so pointer comparison still works.
 */
CEX_DEMO_API const cex_demo_errors_t cex_demo_Error = {
    .ok = NULL,
    .argument = _cex_errors_Error_argument,
    .overflow = _cex_errors_Error_overflow,
};

/*
 * Internal validator.
 *
 * It is deliberately a separate function so the exported traceback contains
 * two frames (`cex_demo__validate_request` and `cex_demo_run`) instead of one.
 * That is the interesting part of the example: a CEX traceback crosses the
 * library boundary and shows the internal call path.
 *
 * `static` keeps it out of the ABI; `cex_demo__` is the conventional CEX
 * prefix for internal helpers.
 */
static Exc
cex_demo__validate_request(cex_demo_request_t* req)
{
    if (!req) {
        /*
         * e$raise records a traceback frame and returns Error.argument. The
         * caller (cex_demo_run) records another frame via e$ret.
         */
        return e$raise(Error.argument, "request is NULL");
    }
    return EOK;
}

/*
 * Public entry point. e$ret propagates the error and records a frame at this
 * call site, giving the two-frame traceback consumers see.
 */
cex_demo_exc_t
cex_demo_run(cex_demo_request_t* req)
{
    e$ret(cex_demo__validate_request(req));
    req->value++;
    return cex_demo_Error.ok;
}

/*
 * Formats the CEX traceback ring into the caller's buffer. Delegating to
 * e$traceback_format() keeps the error contract (argument/overflow) and
 * avoids allocating, which matters for FFI callers.
 */
cex_demo_exc_t
cex_demo_traceback(char* buf, size_t buf_size)
{
    return e$traceback_format(buf, buf_size);
}
