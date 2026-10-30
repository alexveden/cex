#ifndef CEX_DEMO_H
#define CEX_DEMO_H

/*
 * Public ABI of the cex_demo library.
 *
 * This header is the only file consumers include. It deliberately uses
 * standard C types (stdint/stddef) and no CEX types, so the library can be
 * linked from plain C, C++, Python ctypes/cffi, Node, Rust, Zig, etc.
 */

#include <stddef.h>
#include <stdint.h>

/*
 * Export macro.
 *
 * The library is compiled with -fvisibility=hidden, so every symbol is hidden
 * by default. CEX_DEMO_API re-exposes only the functions and data that belong
 * to the public ABI.
 *
 * CEX_DEMO_STATIC is defined by static-link consumers. It turns the macro into
 * nothing so the compiler does not emit dllimport/export attributes.
 */
#if defined(CEX_DEMO_STATIC)
#    define CEX_DEMO_API
#elif defined(_WIN32) || defined(__CYGWIN__)
#    if defined(CEX_DEMO_BUILD)
#        define CEX_DEMO_API __declspec(dllexport)
#    else
#        define CEX_DEMO_API __declspec(dllimport)
#    endif
#elif defined(__GNUC__) || defined(__clang__)
#    define CEX_DEMO_API __attribute__((visibility("default")))
#else
#    define CEX_DEMO_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Errors are plain `const char*` pointers: NULL means success, anything else
 * is an error. They are compared by address, never with strcmp(), because the
 * pointer identity is the contract. The string is only for display/logging.
 */
typedef const char* cex_demo_exc_t;

/*
 * Exported error table.
 *
 * Consumers compare a returned `cex_demo_exc_t` against these fields by
 * pointer:
 *
 *     if (cex_demo_run(req) == cex_demo_Error.argument) { ... }
 *
 * `ok` is NULL, so a successful call returns `cex_demo_Error.ok`.
 */
typedef struct
{
    cex_demo_exc_t ok;
    cex_demo_exc_t argument;
    cex_demo_exc_t overflow;
} cex_demo_errors_t;

/*
 * Request payload. It is POD and uses a fixed-width integer so the layout is
 * identical across compilers, platforms and language bindings.
 */
typedef struct
{
    int32_t value;
} cex_demo_request_t;

/*
 * Increments req->value.
 *
 * A NULL request returns cex_demo_Error.argument; on success it returns
 * cex_demo_Error.ok (NULL).
 */
CEX_DEMO_API cex_demo_exc_t cex_demo_run(cex_demo_request_t* req);

/*
 * Copies the CEX traceback recorded by the last failing call into buf.
 *
 * buf must be non-NULL and buf_size > 0, otherwise cex_demo_Error.argument is
 * returned. The result is always NUL-terminated; if the traceback does not
 * fit, the truncated prefix is kept and cex_demo_Error.overflow is returned.
 *
 * This is the FFI-friendly replacement for e$traceback_print(): foreign
 * callers cannot read CEX's global traceback ring directly, so the library
 * formats it into a caller-owned buffer.
 */
CEX_DEMO_API cex_demo_exc_t cex_demo_traceback(char* buf, size_t buf_size);

/*
 * The error table is exported as data, not a function, so bindings can read
 * the pointers once and compare against them without extra calls.
 */
CEX_DEMO_API extern const cex_demo_errors_t cex_demo_Error;

#ifdef __cplusplus
}
#endif

#endif
