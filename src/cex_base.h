/**
 
Core foundation of CEX — bundled into `cex.h`.

Provides primitive type aliases (`u8` … `u64`, `i8` … `i64`, `f32`/`f64`,
`usize`/`isize`), `IAllocator` allocator interface, `str_s` string slice,
error handling (`__e$`), logging (`__log$`), assertions (`uassert`), and
utility macros (`unlikely`/`likely`/`breakpoint`/`unreachable`/token concat).

| Type                | Description                               |
| auto                | Automatically inferred variable type      |
| bool                | Boolean type                              |
| u8 / i8 … u64/i64  | Fixed-width integer types                 |
| f32 / f64           | 32/64-bit floating point                  |
| usize / isize       | Unsigned/signed size types                |
| char*               | Null-terminated string                    |
| str_s               | String slice (buf + len)                  |
| Exc / Exception     | Error type (NULL = success)               |
| Error.*             | Standard error constants                  |
| IAllocator          | Allocator interface vtable                |

*/
#pragma once
#include "cex_header.h"

/*
 *                 CORE TYPES
 */
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef size_t usize;
typedef ptrdiff_t isize;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L
/// automatic variable type, supported by GCC/Clang or C23
#    define auto __auto_type
#endif

/**

IAllocator — 64-byte allocator vtable interface.

Every function that may allocate memory takes an `IAllocator` parameter.
The same code works with heap allocators, arena allocators, or temp allocators.

Fields:
- `malloc` / `calloc` / `realloc` / `free` — standard allocation (with alignment)
- `scope_enter` / `scope_exit` / `scope_depth` — arena scope management (for `mem$scope`)
- `meta.is_arena` / `meta.is_temp` / `meta.magic_id` — allocator type identification

Size is exactly 64 bytes (one cache line).

*/

// clang-format off
#define IAllocator const struct Allocator_i* 
typedef struct Allocator_i
{
    // >>> cacheline
    alignas(64) void* (*const malloc)(IAllocator self, usize size, usize alignment);
    void* (*const calloc)(IAllocator self, usize nmemb, usize size, usize alignment);
    void* (*const realloc)(IAllocator self, void* ptr, usize new_size, usize alignment);
    void* (*const free)(IAllocator self, void* ptr);
    const struct Allocator_i* (*const scope_enter)(IAllocator self);   /* Only for arenas/temp alloc! */
    void (*const scope_exit)(IAllocator self);    /* Only for arenas/temp alloc! */
    u32 (*const scope_depth)(IAllocator self);  /* Current mem$scope depth */
    struct {
        u32 magic_id;
        bool is_arena;
        bool is_temp;
    } meta;
    //<<< 64 byte cacheline
} Allocator_i;
// clang-format on
static_assert(alignof(Allocator_i) == 64, "size");
static_assert(sizeof(Allocator_i) == 64, "size");
static_assert(sizeof((Allocator_i){ 0 }.meta) == 8, "size");


/// Represents char* slice (string view) + may not be null-term at len!
typedef struct
{
    usize len;
    char* buf;
} str_s;

static_assert(alignof(str_s) == alignof(usize), "align");
static_assert(sizeof(str_s) == sizeof(usize) * 2, "size");


/**

Creates `str_s` from string literals at compile time: `str$s("my string")`.
Only works with string literals — not `char*` pointers.

*/
#define str$s(string)                                                                              \
    (str_s){ .buf = /* WARNING: only literals!!!*/ "" string, .len = sizeof((string)) - 1 }

/*
 *                 BRANCH MANAGEMENT
 * `if(unlikely(condition)) {...}` is helpful for error management, to let compiler
 *  know that the scope inside in if() is less likely to occur (or exceptionally unlikely)
 *  this allows compiler to organize code with less failed branch predictions and faster
 *  performance overall.
 *
 *  Example:
 *  char* s = malloc(128);
 *  if(unlikely(s == NULL)) {
 *      printf("Memory error\n");
 *      abort();
 *  }
 */
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr) __builtin_expect(!!(expr), 1)
#define fallthrough() __attribute__((fallthrough));

/*
 *                 ERRORS
 */
/**

CEX Exception-based error handling.

Errors are `char*` pointers:
- `EOK` (or `Error.ok`) = `NULL` → success
- Any non-NULL value → an error
- `Exception` return type forces the caller to check (`warn_unused_result`)
- Errors are compared by **address**, never by string content

Principles:

1. **Unambiguous** — only two states: OK or error, never mixed with valid return values
2. **General purpose** — same pattern for allocation errors, IO, argument validation, etc.
3. **Easy to report** — errors are printable strings; use `e$raise` for location-tagged logging
4. **Bubbling up** — pass the same error pointer upward; no error-code translation needed
5. **Extensible** — define custom error structs with your own string constants
6. **Low overhead** — one pointer (one register), comparison is a single instruction
7. **Natural** — regular `if` works; `e$` macros are optional helpers
8. **Mandatory checking** — `Exception` return type triggers `-Werror=unused-result` if ignored

Standard errors:

| Error.*         | String Value           | Description                           |
| --------------- | ---------------------- | ------------------------------------- |
| Error.ok        | EOK (NULL)             | Success (no error)                    |
| Error.memory    | "MemoryError"          | Memory allocation error               |
| Error.io        | "IOError"              | I/O error                             |
| Error.overflow  | "OverflowError"        | Buffer overflow                       |
| Error.argument  | "ArgumentError"        | Invalid function argument             |
| Error.integrity | "IntegrityError"       | Data integrity violation              |
| Error.exists    | "ExistsError"          | Entity or key already exists          |
| Error.not_found | "NotFoundError"        | Entity or key not found               |
| Error.skip      | "ShouldBeSkipped"      | Not an error — result must be skipped |
| Error.null_or_empty | "NullOrEmptyError" | Value is NULL or resource is empty    |
| Error.eof       | "EOF"                  | End of file reached                   |
| Error.argsparse | "ProgramArgsError"     | Program arguments error               |
| Error.runtime   | "RuntimeError"         | Generic runtime error                 |
| Error.assert    | "AssertError"          | Assertion failure                     |
| Error.os        | "OSError"              | OS-level error                        |
| Error.timeout   | "TimeoutError"         | Interval timeout                      |
| Error.permission | "PermissionError"     | Permission denied                     |
| Error.try_again | "TryAgainError"        | EAGAIN / EWOULDBLOCK analog           |

Examples:

```c

Exception
remove_file(char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;  // Empty path
    }
    if (!os.path.exists(path)) {
        return "Not exists" // literal error are allowed, but must be handled as strcmp()
    }
    if (str.eq(path, "magic.file")) {
        // Returns an Error.integrity and logs error at current line to stdout
        return e$raise(Error.integrity, "Removing magic file is not allowed!");
    }
    if (remove(path) < 0) {
        return strerror(errno); // using system error text (arbitrary!)
    }
    return EOK;
}

Exception
read_file(char* filename)
{
    e$assert(buff != NULL);

    int fd = 0;
    e$except_errno(fd = open(filename, O_RDONLY)) { return Error.os; }
    return EOK;
}

Exception
do_stuff(char* filename)
{
    // return immediately with error + prints traceback
    e$ret(read_file("foo.txt"));

    // jumps to label if read_file() fails + prints traceback
    e$goto(read_file(NULL), fail);

    // silent error handling without tracebacks
    e$except_silent (err, foo(0)) {

        // Nesting of error handlers is allowed
        e$except_silent (err, foo(2)) { return err; }

        // NOTE: `err` is address of char* compared with address Error.os (not by string contents!)
        if (err == Error.os) {
            // Special handing
            io.print("Ooops OS problem\n");
        } else {
            // propagate
            return err;
        }
    }
    return EOK;

fail:
    // TODO: cleanup here
    return Error.io;
}
```

Caveats:

- Do NOT use `break` / `continue` inside `e$except` or `e$except_*` scopes when nested inside loops — these macros are backed by `for()` loops, so `break`/`continue` affects the error-handling loop, not the outer loop.


*/
#define __e$

/// Generic CEX error is a char*, where NULL means success(no error)
typedef char* Exc;

/// Equivalent of Error.ok, execution success
#define EOK (Exc) NULL

/// Use `Exception` in function signatures, to force developer to check return value
/// of the function.
#define Exception Exc __attribute__((warn_unused_result))


/**

Generic errors as constant pointers — compare by address, never by strcmp().

*/
extern const struct _CEX_Error_struct
{
    Exc ok; // NOTE: must be the 1st, same as EOK
    Exc memory;
    Exc io;
    Exc overflow;
    Exc argument;
    Exc integrity;
    Exc exists;
    Exc not_found;
    Exc skip;
    Exc null_or_empty;
    Exc eof;
    Exc argsparse;
    Exc runtime;
    Exc assert;
    Exc os;
    Exc timeout;
    Exc permission;
    Exc try_again;
} Error;

#ifndef __cex__fprintf

// NOTE: you may try to define our own fprintf
#    define __cex__fprintf(stream, prefix, filename, line, func, format, ...)                      \
        cexsp__fprintf(                                                                            \
            stream,                                                                                \
            "%s ( %s:%d %s() ) " format,                                                           \
            prefix,                                                                                \
            filename,                                                                              \
            line,                                                                                  \
            func,                                                                                  \
            ##__VA_ARGS__                                                                          \
        )

static inline bool
__cex__fprintf_dummy(void)
{
    return true; // WARN: must always return true!
}

#endif

#ifndef __FILE_NAME__
#    define __FILE_NAME__                                                                          \
        (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef _WIN32
#    define CEX_NAMESPACE __attribute__((visibility("hidden"))) extern const
#else
#    define CEX_NAMESPACE extern const
#endif

/**

Simple console logging with file:line location prefix.

`log$error` / `log$warn` / `log$info` / `log$debug` / `log$trace`

- Output format: `[INFO]    ( file.c:14 func() ) message`
- Supports CEX formatting engine
- Compile-time level control via `#define CEX_LOG_LVL <level>`

Log levels:

- 0 — mute all (including asserts, tracebacks, errors)
- 1 — `log$error` + asserts + tracebacks
- 2 — `log$warn`
- 3 — `log$info`
- 4 — `log$debug` (default)
- 5 — `log$trace`

Example:
```c
#define CEX_LOG_LVL 3
int main(void)
{
    log$info("Hello from CEX\n");
    return 0;
}
```

*/
#define __log$

#ifndef CEX_LOG_LVL
// LVL Value
// 0 - mute all including assert messages, tracebacks, errors
// 1 - allow log$error + assert messages, tracebacks
// 2 - allow log$warn
// 3 - allow log$info
// 4 - allow log$debug  (default level if CEX_LOG_LVL is not set)
// 5 - allow log$trace
// NOTE: you may override this level to manage log$* verbosity
#    define CEX_LOG_LVL 4
#endif

#if CEX_LOG_LVL > 0
/// Log error (when CEX_LOG_LVL > 0)
#    define log$error(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[ERROR]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$error(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 1
/// Log warning  (when CEX_LOG_LVL > 1)
#    define log$warn(format, ...)                                                                  \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[WARN]   ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$warn(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 2
/// Log info  (when CEX_LOG_LVL > 2)
#    define log$info(format, ...)                                                                  \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[INFO]   ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$info(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 3
/// Log debug (when CEX_LOG_LVL > 3)
#    define log$debug(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[DEBUG]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$debug(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 4
/// Log tace (when CEX_LOG_LVL > 4)
#    define log$trace(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[TRACE]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$trace(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 0
#    define __cex__traceback(uerr, fail_func)                                                      \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[^STCK]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            "^^^^^ [%s] in function call `%s`\n",                                                  \
            uerr,                                                                                  \
            fail_func                                                                              \
        ))

/// Non disposable assert, returns Error.assert CEX exception when failed
#    define e$assert(A)                                                                             \
        ({                                                                                          \
            if (unlikely(!((A)))) {                                                                 \
                __cex__fprintf(stdout, "[ASSERT] ", __FILE_NAME__, __LINE__, __func__, "%s\n", #A); \
                return Error.assert;                                                                \
            }                                                                                       \
        })


/// Non disposable assert, returns Error.assert CEX exception when failed (supports formatting)
#    define e$assertf(A, format, ...)                                                              \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    stdout,                                                                        \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    format "\n",                                                                   \
                    ##__VA_ARGS__                                                                  \
                );                                                                                 \
                return Error.assert;                                                               \
            }                                                                                      \
        })
#else // #if CEX_LOG_LVL > 0
#    define __cex__traceback(uerr, fail_func) __cex__fprintf_dummy()
#    define e$assert(A)                                                                            \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })


#    define e$assertf(A, format, ...)                                                              \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })
#endif // #if CEX_LOG_LVL > 0


/**
 
Assertion macros, ASAN detection, and stack-trace helpers.

- `mem$asan_enabled()` — compile-time check for Address Sanitizer
- `sanitizer_stack_trace()` — prints ASAN stack trace when available
- `uassert(A)` — hard assertion, prints file:line:func + traceback, then aborts
- `uassertf(A, format, ...)` — assertion with formatted message
- `uassert_disable()` / `uassert_enable()` — suppress assertions in test mode

*/
#ifndef mem$asan_enabled
#    if defined(__has_feature)
#        if __has_feature(address_sanitizer)
/// true - if program was compiled with address sanitizer support
#            define mem$asan_enabled() 1
#        else
#            define mem$asan_enabled() 0
#        endif
#    else
#        if defined(__SANITIZE_ADDRESS__)
#            define mem$asan_enabled() 1
#        else
#            define mem$asan_enabled() 0
#        endif
#    endif
#endif // mem$asan_enabled

#if mem$asan_enabled()
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#    define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#else
#    define sanitizer_stack_trace() ((void)(0))
#endif

#if defined(__clang_analyzer__)
#    include <assert.h>
#    define uassert(cond) assert(cond)
#    define uassertf(cond, format, ...) assert(cond)
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#    define __cex_test_postmortem_exists() 0
#elif defined(NDEBUG)
#    define uassertf(cond, format, ...) ((void)(0))
#    define uassert(cond) ((void)(0))
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#    define __cex_test_postmortem_exists() 0
#else

#    ifdef CEX_TEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
int __cex_test_uassert_enabled = 1;
#        define uassert_disable() __cex_test_uassert_enabled = 0
#        define uassert_enable() __cex_test_uassert_enabled = 1
#        define uassert_is_enabled() (__cex_test_uassert_enabled)
#    else
#        define uassert_disable()                                                                  \
            static_assert(false, "uassert_disable() allowed only when compiled with -DCEX_TEST")
#        define uassert_enable() (void)0
#        define uassert_is_enabled() true
#        define __cex_test_postmortem_ctx NULL
#        define __cex_test_postmortem_exists() 0
#        define __cex_test_postmortem_f(ctx)
#    endif // #ifdef CEX_TEST


/// Hard assertion with ASAN stack trace on failure. Aborts via `cex$platform_panic()`.
#    define uassert(A)                                                                             \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    (uassert_is_enabled() ? stderr : stdout),                                      \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    "%s\n",                                                                        \
                    #A                                                                             \
                );                                                                                 \
                if (uassert_is_enabled()) { cex$platform_panic(); }                                \
            }                                                                                      \
        })

#    define uassertf(A, format, ...)                                                               \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    (uassert_is_enabled() ? stderr : stdout),                                      \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    format "\n",                                                                   \
                    ##__VA_ARGS__                                                                  \
                );                                                                                 \
                if (uassert_is_enabled()) { cex$platform_panic(); }                                \
            }                                                                                      \
        })
#endif


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#    undef unreachable
#endif

#ifdef NDEBUG
#    define unreachable() __builtin_unreachable()
#else
#    define unreachable()                                                                          \
        ({                                                                                         \
            __cex__fprintf(stderr, "[UNREACHABLE] ", __FILE_NAME__, __LINE__, __func__, "\n");     \
            cex$platform_panic();                                                                  \
            __builtin_unreachable();                                                               \
        })
#endif

/// Cross-platform debugger breakpoint.
#if defined(_WIN32) || defined(_WIN64)
#    define breakpoint() __debugbreak()
#elif defined(__APPLE__)
#    define breakpoint() __builtin_debugtrap()
#elif defined(__linux__) || defined(__unix__)
#    define breakpoint() __builtin_trap()
#else
#    warning "breakpoint() is not supported by this arch"
#    define breakpoint()
#endif

/// Concatenate textually c##a##b
#define cex$concat3(c, a, b) c##a##b

/// Concatenate textually a##b
#define cex$concat(a, b) a##b

#define _cex$stringize(...) #__VA_ARGS__

/// Produces a literal string of any text inside the (...)
#define cex$stringize(...) _cex$stringize(__VA_ARGS__)

/// makes a new variable with __cex__ prefix
#define cex$varname(a, b) cex$concat3(__cex__, a, b)

/// cex$tmpname - internal macro for generating temporary variable names (unique__line_num)
#define cex$tmpname(base) cex$varname(base, __LINE__)

/// raises an error, code: `return e$raise(Error.integrity, "ooops: %d", i);`
#define e$raise(return_uerr, error_msg, ...)                                                       \
    (log$error("[%s] " error_msg "\n", return_uerr, ##__VA_ARGS__), (return_uerr))

/// catches the error of function inside scope + prints traceback
#define e$except(_var_name, _func)                                                                 \
    for (Exc _var_name = _func;                                                                    \
         unlikely((_var_name != EOK) && (__cex__traceback(_var_name, #_func), 1));                 \
         _var_name = EOK)

#if defined(CEX_TEST) || defined(CEX_BUILD)
#    define e$except_silent(_var_name, _func) e$except (_var_name, _func)
#else
/// catches the error of function inside scope (without traceback)
#    define e$except_silent(_var_name, _func)                                                      \
        for (Exc _var_name = _func; unlikely(_var_name != EOK); _var_name = EOK)
#endif

/// catches the error of system function (if negative value + errno), prints errno error
#define e$except_errno(_expression)                                                                \
    for (int _tmp_errno = 0; unlikely(                                                             \
             ((_tmp_errno == 0) && ((_expression) < 0) && ((_tmp_errno = errno), 1) &&             \
              (log$error(                                                                          \
                   "`%s` failed errno: %d, msg: %s\n",                                             \
                   #_expression,                                                                   \
                   _tmp_errno,                                                                     \
                   strerror(_tmp_errno)                                                            \
               ),                                                                                  \
               1) &&                                                                               \
              (errno = _tmp_errno, 1))                                                             \
         );                                                                                        \
         _tmp_errno = 1)

/// catches the error is expression returned null
#define e$except_null(_expression)                                                                 \
    if (unlikely(((_expression) == NULL) && (log$error("`%s` returned NULL\n", #_expression), 1)))

/// catches the error is expression returned true
#define e$except_true(_expression)                                                                 \
    if (unlikely(((_expression)) && (log$error("`%s` returned non zero\n", #_expression), 1)))

/// immediately returns from function with _func error + prints traceback
#define e$ret(_func)                                                                               \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                      \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    return cex$tmpname(__cex_err_traceback_)

/// `goto _label` when _func returned error + prints traceback
#define e$goto(_func, _label)                                                                      \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                      \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    goto _label


#if defined(__GNUC__) && !defined(__clang__)
// NOTE: GCC < 12, has some weird warnings for arr$len temp pragma push + missing-field-initializers
#    if (__GNUC__ < 12)
#        pragma GCC diagnostic ignored "-Wsizeof-pointer-div"
#        pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#    endif
#endif

#if defined(__STDC_HOSTED__)
#    if __STDC_HOSTED__ == 0
/// Set to 1 if current platform is freestanding (no OS, or libc)
#        define cex$is_freestanding 1
#    else
#        define cex$is_freestanding 0
#    endif
#else
// If __STDC_HOSTED__ is not defined, we're likely freestanding
#    define cex$is_freestanding 1
#endif


#ifndef json$$struct
/// JSON Generator attribute, put it before your `typedef struct` to enable JSON code generation
/// Implemented in: cexstd/json/json.h
#define json$$struct(...)
#endif

#ifndef json$$field
/// JSON field metadata attribute, used for adjusting json.gen. behavior for specific field
/// Implemented in: cexstd/json/json.h
#define json$$field(...)
#endif
