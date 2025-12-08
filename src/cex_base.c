#pragma once

#include "cex_base.h"

const struct _CEX_Error_struct Error = {
    .ok = EOK,                           // Success
    .memory = "MemoryError",             // memory allocation error
    .io = "IOError",                     // IO error
    .overflow = "OverflowError",         // buffer overflow
    .argument = "ArgumentError",         // function argument error
    .integrity = "IntegrityError",       // data integrity error
    .exists = "ExistsError",             // entity or key already exists
    .not_found = "NotFoundError",        // entity or key already exists
    .skip = "ShouldBeSkipped",           // NOT an error, function result must be skipped
    .null_or_empty = "NullOrEmptyError", // value is null or resource is empty
    .eof = "EOF",                        // end of file reached
    .argsparse = "ProgramArgsError",     // program arguments empty or incorrect
    .runtime = "RuntimeError",           // generic runtime error
    .assert = "AssertError",             // generic runtime check
    .os = "OSError",                     // generic OS check
    .timeout = "TimeoutError",           // await interval timeout
    .permission = "PermissionError",     // Permission denied
    .try_again = "TryAgainError",        // EAGAIN / EWOULDBLOCK errno analog for async operations
};

#ifdef _cex$platform_panic_builtin

void
__cex__panic(void)
{
    fflush(stdout);
    fflush(stderr);
    sanitizer_stack_trace();

#    ifdef CEX_TEST
    breakpoint();
#    else
    abort();
#    endif
    return;
}

#endif
