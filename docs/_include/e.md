

CEX Exception-based error handling.

Errors are `char*` pointers:
- `EOK` (or `Error.ok`) = `NULL` → success
- Any non-NULL value → an error
- `Exception` return type forces the caller to check (`warn_unused_result`)
- Errors are compared by **address**, never by string content

Principles:

1. **Unambiguous** — only two states: OK or error, never mixed with valid return values
2. **General purpose** — same pattern for allocation errors, IO, argument validation, etc.
3. **Easy to report** — errors are printable strings; use `e$raise` to tag the origin location
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

Error handling macros:

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

Examples:

```c

Exception
remove_file(char* path)
{
    if (path == NULL || path[0] == '\0') {
        // WARNING: plain return doesn't register a traceback frame, prefer e$raise()
        return Error.argument;  // returns a pointer to a static const string
    }
    if (!os.path.exists(path)) {
        // WARNING: plain return doesn't register a traceback frame, prefer e$raise()
        return "Not exists"; // literal errors are allowed, but must be handled as strcmp()
    }
    if (str.eq(path, "magic.file")) {
        // Records an origin traceback frame tagged Error.integrity
        return e$raise(Error.integrity, "Removing magic file is not allowed!");
    }
    if (remove(path) < 0) {
        return strerror(errno); // using system error text (arbitrary!)
    }
    return EOK;
}

Exception
read_file(char* filename, char* buf, usize buf_size)
{
    e$assert(buf != NULL);

    int fd = 0;
    e$except_errno(fd = open(filename, O_RDONLY)) { return Error.os; }
    return EOK;
}

Exception
do_stuff(char* filename)
{
    char buf[256] = {0};

    // return immediately with error + records a traceback frame
    e$ret(read_file(filename, buf, sizeof(buf)));

    // jumps to label if read_file() fails + records a traceback frame
    e$goto(read_file(filename, buf, sizeof(buf)), fail);

    // error handling with tracebacks
    e$except (err, foo(0)) {

        // Nesting of error handlers is allowed
        e$except (err, foo(2)) { return err; }

        // NOTE: `err` is address of char* compared with address Error.os (not by string contents!)
        if (err == Error.os) {
            // Special handing
            io.printf("Ooops OS problem\n");
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

Tracebacks:

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

Asserts and panics:

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

Caveats:

- Do NOT use `break` / `continue` inside `e$except` or `e$except_*` scopes when nested inside loops — these macros are backed by `for()` loops, so `break`/`continue` affects the error-handling loop, not the outer loop.




```c
/// Non disposable assert, returns Error.assert CEX exception when failed
#define e$assert(A)

/// catches the error of function inside scope + records a traceback frame
#define e$except(_var_name, _func)

/// catches the error of system function (if negative value + errno), records a frame
#define e$except_errno(_expression)

/// catches the error is expression returned null
#define e$except_null(_expression)

/// catches the error is expression returned true
#define e$except_true(_expression)

/// `goto _label` when _func returned error + records a traceback frame
#define e$goto(_func, _label)

/// raises an error, code: `return e$raise(Error.integrity, "ooops");`, msg must be a literal
#define e$raise(return_uerr, error_msg)

/// immediately returns from function with _func error + records a traceback frame
#define e$ret(_func)

/// Recorded traceback frames array, use with for$each/for$eachp
#define e$traceback_arr((_cex_errors_traceback_s*)NULL)

/// Number of recorded traceback frames
#define e$traceback_len

/// Print the whole traceback to a FILE*
#define e$traceback_print(_stream)

/// Drop all recorded traceback frames
#define e$traceback_reset()




```
