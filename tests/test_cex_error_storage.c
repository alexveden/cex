#include "src/all.c"

static const char* const _cex_test_error_static = _cex_errors_Error_memory;

test$case(error_storage_is_canonical)
{
    struct
    {
        Exc err;
        Exc storage;
        char* text;
    } pairs[] = {
        { Error.memory, (Exc)_cex_errors_Error_memory, "MemoryError" },
        { Error.io, (Exc)_cex_errors_Error_io, "IOError" },
        { Error.overflow, (Exc)_cex_errors_Error_overflow, "OverflowError" },
        { Error.argument, (Exc)_cex_errors_Error_argument, "ArgumentError" },
        { Error.integrity, (Exc)_cex_errors_Error_integrity, "IntegrityError" },
        { Error.exists, (Exc)_cex_errors_Error_exists, "ExistsError" },
        { Error.not_found, (Exc)_cex_errors_Error_not_found, "NotFoundError" },
        { Error.skip, (Exc)_cex_errors_Error_skip, "ShouldBeSkipped" },
        { Error.null_or_empty, (Exc)_cex_errors_Error_null_or_empty, "NullOrEmptyError" },
        { Error.eof, (Exc)_cex_errors_Error_eof, "EOF" },
        { Error.argsparse, (Exc)_cex_errors_Error_argsparse, "ProgramArgsError" },
        { Error.runtime, (Exc)_cex_errors_Error_runtime, "RuntimeError" },
        { Error.assert, (Exc)_cex_errors_Error_assert, "AssertError" },
        { Error.os, (Exc)_cex_errors_Error_os, "OSError" },
        { Error.timeout, (Exc)_cex_errors_Error_timeout, "TimeoutError" },
        { Error.permission, (Exc)_cex_errors_Error_permission, "PermissionError" },
        { Error.try_again, (Exc)_cex_errors_Error_try_again, "TryAgainError" },
    };

    tassert(Error.ok == EOK);
    tassert(_cex_test_error_static == _cex_errors_Error_memory);

    for$each (p, pairs) {
        tassert_eq_ptr(p.err, p.storage);
        tassert(str.eq(p.err, p.text));
    }

    return EOK;
}

test$main();
