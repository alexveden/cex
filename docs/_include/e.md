

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




```c
/// Non disposable assert, returns Error.assert CEX exception when failed
#define e$assert(A)

/// Non disposable assert, returns Error.assert CEX exception when failed (supports formatting)
#define e$assertf(A, format, ...)

/// catches the error of function inside scope + prints traceback
#define e$except(_var_name, _func)

/// catches the error of system function (if negative value + errno), prints errno error
#define e$except_errno(_expression)

/// catches the error is expression returned null
#define e$except_null(_expression)

/// catches the error of function inside scope (without traceback)
#define e$except_silent(_var_name, _func)

/// catches the error is expression returned true
#define e$except_true(_expression)

/// `goto _label` when _func returned error + prints traceback
#define e$goto(_func, _label)

/// raises an error, code: `return e$raise(Error.integrity, "ooops: %d", i);`
#define e$raise(return_uerr, error_msg, ...)

/// immediately returns from function with _func error + prints traceback
#define e$ret(_func)




```
