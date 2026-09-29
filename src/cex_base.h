/**
 
Core foundation of CEX — bundled into `cex.h`.

Provides primitive type aliases (`u8` … `u64`, `i8` … `i64`, `f32`/`f64`,
`usize`/`isize`), `IAllocator` allocator interface, `str_s` string slice,
error handling (`__e$`), logging (`__log$`), assertions (`uassert`), and
utility macros (`unlikely`/`likely`/`breakpoint`/token concat).

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
/// Branch prediction: likely condition
#define likely(expr) __builtin_expect(!!(expr), 1)
/// Compiler fallthrough annotation for switch cases
#define fallthrough() __attribute__((fallthrough));

/*
 *                 ERRORS
 */
/**

## CEX Exception-based error handling.

Errors are `char*` pointers:
- `EOK` (or `Error.ok`) = `NULL` → success
- Any non-NULL value → an error
- `Exception` return type forces the caller to check (`warn_unused_result`)
- Errors are compared by **address**, never by string content

1. **Unambiguous** — only two states: OK or error, never mixed with valid return values
2. **General purpose** — same pattern for allocation errors, IO, argument validation, etc.
3. **Easy to report** — errors are printable strings; use `e$raise` to tag the origin location
4. **Bubbling up** — pass the same error pointer upward; no error-code translation needed
5. **Extensible** — define custom error structs with your own string constants
6. **Low overhead** — one pointer (one register), comparison is a single instruction
7. **Natural** — regular `if` works; `e$` macros are optional helpers
8. **Mandatory checking** — `Exception` return type triggers `-Werror=unused-result` if ignored

### Standard errors

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

### Error handling macros

| Macro                       | Type      | Description                                          |
| --------------------------- | --------- | ---------------------------------------------------- |
| `e$raise(err, "msg")`       | origin    | return `err` (`msg` must be a literal)               |
| `e$assert(cond)`            | origin    | return `Error.assert` when false                     |
| `e$except_errno(expr) { }`  | origin    | error handler for `-1`+`errno`, with the errno text  |
| `e$except_null(expr) { }`   | origin    | error handler for `NULL`, with `Error.null_or_empty` |
| `e$except_true(expr) { }`   | origin    | error handler for non-zero, with `Error.runtime`     |
| `e$except(err, expr) { }`   | handler   | bind `err` to `expr`, record a frame on error        |
| `e$ret(expr)`               | handler   | record a frame and return `expr` on error            |
| `e$goto(expr, label)`       | handler   | record a frame and `goto label` on error             |
| `e$traceback_print(stream)` | traceback | print recorded frames to a `FILE*`                   |
| `e$traceback_arr`           | traceback | recorded frames array, iterate with `for$each`       |
| `e$traceback_len`           | traceback | number of recorded frames                            |
| `e$traceback_reset()`       | traceback | drop all recorded frames                             |

### Examples

```c

Exception
remove_file(char* path)
{
    e$assert(path != NULL && path[0] != '\0');

    if (!os.path.exists(path)) {
        // records an origin traceback frame tagged Error.not_found
        return e$raise(Error.not_found, "file does not exist");
    }
    if (remove(path) < 0) {
        return strerror(errno); // plain return: no traceback frame
    }
    return EOK;
}

Exception
do_stuff(char* path)
{
    int fd = 0;

    // return immediately with the error + record a traceback frame
    e$ret(remove_file(path));

    // jump to `fail` on error + record a traceback frame
    e$goto(remove_file(path), fail);

    // origin error handler for -1 + errno
    e$except_errno(fd = open(path, O_RDONLY)) { return Error.os; }

    // handle a specific error, propagate the rest; `err` compares by address
    e$except (err, foo(path)) {
        if (err == Error.not_found) {
            io.printf("oops\n");
        } else {
            return err;
        }
    }
    return EOK;

fail:
    return Error.io;
}
```

### Making custom user exceptions

For errors you must handle specifically, declare a global const struct of `Exc` fields:

```c
// myerr.h
extern const struct _MyError_struct
{
    Exc foo;
    Exc bar;
} MyError;

// myerr.c
const struct _MyError_struct MyError = {
    .foo = "FooError",
    .bar = "BarError",
};

// other.c
if (err == MyError.foo) { // pointer address comparison, not string content
    // handle
}
```

WARNING: all struct fields must be initialized — an uninitialized field is `NULL`, which is
success (`EOK`).

### Tracebacks

`e$` macros record a traceback instead of printing it immediately. Verbosity is a
compile-time knob `CEX_TRACEBACK_VERBOSITY` (0..3, default 2):

| Level | Behavior                                              |
| ----- | ----------------------------------------------------- |
| 0     | No tracebacks, ring disabled (`e$traceback_len == 0`) |
| 1     | Ring records `{err, file, line}`                      |
| 2     | Ring records `{err, file, func, msg}` (default)       |
| 3     | Immediate logging to stdout, no ring                  |

Buffered levels (1, 2) are not printed automatically. Flush the ring at the top-level
sink when `main()` gets a non-`EOK` result:

```c
int
main(int argc, char** argv)
{
    e$except (err, app_main(argc, argv)) {
        e$traceback_print(stderr);
        return 1;
    }
    return 0;
}
```

Ring capacity is `CEX_TRACEBACK_CAP` (default 32 frames): the oldest frames are kept
and extra ones are dropped. Traceback read-back API:

- `e$traceback_print(stream)` — print all recorded frames to a `FILE*`
- `e$traceback_arr` — recorded frames array, use with `for$each`/`for$eachp`
- `e$traceback_len` — number of recorded frames
- `e$traceback_reset()` — drop all recorded frames

A frame prints as `#N (file:line func()) [err] msg` at level 2, and as
`#N (file:line) [err]` at level 1. `e$raise(err, "msg")` requires the message to be a
string literal.

### Asserts and panics

Hard-fail assertions (`uassert()` / `uassert_always()`) are controlled by
`CEX_PANIC_VERBOSITY` (0..2, default 1):

| Level | Behavior                                       |
| ----- | ---------------------------------------------- |
| 0     | Trap with `__builtin_trap()`, no report        |
| 1     | Print `file:line` (default)                    |
| 2     | Print `file:line:func` + the failed expression |

`uassert()` is stripped by `NDEBUG`; `uassert_always()` always terminates. In unit
tests `uassert_disable()` / `uassert_enable()` toggle reporting, and a disabled
`uassert()` returns instead of aborting.

### Caveats

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


#if !defined(CEX_TEST)

#ifndef _WIN32
/// Marks a variable as a CEX namespace struct (visibility("hidden") on non-Win32)
#    define CEX_NAMESPACE __attribute__((visibility("hidden"))) extern const
#else
#    define CEX_NAMESPACE extern const
#endif

#    define CEX_NAMESPACE_DEF const
#else
#    define CEX_NAMESPACE extern
#    define CEX_NAMESPACE_DEF
#endif
/**

## Logging

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
/// Compile-time log level (0 mute .. 5 trace), default 4
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
/// Log trace (when CEX_LOG_LVL > 4)
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
