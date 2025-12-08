
CEX Error handling cheat sheet:

1. Errors can be any `char*`, or string literals.
2. EOK / Error.ok - is NULL, means no error
3. Exception return type forced to be checked by compiler
4. Error is built-in generic error type
5. Errors should be checked by pointer comparison, not string contents.
6. `e$` are helper macros for error handling
7.  DO NOT USE break/continue inside e\$except/e\$except_* scopes (these macros are for loops too)!


Generic errors:

```c
Error.ok = EOK;                            // Success
Error.memory = "MemoryError";              // memory allocation error
Error.io = "IOError";                      // IO error
Error.overflow = "OverflowError";          // buffer overflow
Error.argument = "ArgumentError";          // function argument error
Error.integrity = "IntegrityError";        // data integrity error
Error.exists = "ExistsError";              // entity or key already exists
Error.not_found = "NotFoundError";         // entity or key already exists
Error.skip = "ShouldBeSkipped";            // NOT an error, function result must be skipped
Error.null_or_empty = "NullOrEmptyError";  // value is null or resource is empty
Error.eof = "EOF";                         // end of file reached
Error.argsparse = "ProgramArgsError";      // program arguments empty or incorrect
Error.runtime = "RuntimeError";            // generic runtime error
Error.assert = "AssertError";              // generic runtime check
Error.os = "OSError";                      // generic OS check
Error.timeout = "TimeoutError";            // await interval timeout
Error.permission = "PermissionError";      // Permission denied
Error.try_again = "TryAgainError";         // EAGAIN / EWOULDBLOCK errno analog for async operations
```

```c

Exception
remove_file(char* path)
{
    if (path == NULL || path[0] == '\0') { 
        return Error.argument;  // Empty of null file
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

Exception read_file(char* filename) {
    e$assert(buff != NULL);

    int fd = 0;
    e$except_errno(fd = open(filename, O_RDONLY)) { return Error.os; }
    return EOK;
}

Exception do_stuff(char* filename) {
    // return immediately with error + prints traceback
    e$ret(read_file("foo.txt"));

    // jumps to label if read_file() fails + prints traceback
    e$goto(read_file(NULL), fail);

    // silent error handing without tracebacks
    e$except_silent (err, foo(0)) {

        // Nesting of error handlers is allowed
        e$except_silent (err, foo(2)) {
            return err;
        }

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
