#pragma once

#include "cex_base.h"

const char _cex_errors_Error_memory[] = "MemoryError";
const char _cex_errors_Error_io[] = "IOError";
const char _cex_errors_Error_overflow[] = "OverflowError";
const char _cex_errors_Error_argument[] = "ArgumentError";
const char _cex_errors_Error_integrity[] = "IntegrityError";
const char _cex_errors_Error_exists[] = "ExistsError";
const char _cex_errors_Error_not_found[] = "NotFoundError";
const char _cex_errors_Error_skip[] = "ShouldBeSkipped";
const char _cex_errors_Error_null_or_empty[] = "NullOrEmptyError";
const char _cex_errors_Error_eof[] = "EOF";
const char _cex_errors_Error_argsparse[] = "ProgramArgsError";
const char _cex_errors_Error_runtime[] = "RuntimeError";
const char _cex_errors_Error_assert[] = "AssertError";
const char _cex_errors_Error_os[] = "OSError";
const char _cex_errors_Error_timeout[] = "TimeoutError";
const char _cex_errors_Error_permission[] = "PermissionError";
const char _cex_errors_Error_try_again[] = "TryAgainError";

static_assert(
    __builtin_types_compatible_p(__typeof__(&_cex_errors_Error_memory[0]), const char*),
    "error storage must be const"
);

const struct _CEX_Error_struct Error = {
    .ok = EOK,                                             // Success
    .memory = (Exc)_cex_errors_Error_memory,               // memory allocation error
    .io = (Exc)_cex_errors_Error_io,                       // IO error
    .overflow = (Exc)_cex_errors_Error_overflow,           // buffer overflow
    .argument = (Exc)_cex_errors_Error_argument,           // function argument error
    .integrity = (Exc)_cex_errors_Error_integrity,         // data integrity error
    .exists = (Exc)_cex_errors_Error_exists,               // entity or key already exists
    .not_found = (Exc)_cex_errors_Error_not_found,         // entity or key already exists
    .skip = (Exc)_cex_errors_Error_skip,                   // not an error, result must be skipped
    .null_or_empty = (Exc)_cex_errors_Error_null_or_empty, // value is null or resource is empty
    .eof = (Exc)_cex_errors_Error_eof,                     // end of file reached
    .argsparse = (Exc)_cex_errors_Error_argsparse,         // program arguments empty or incorrect
    .runtime = (Exc)_cex_errors_Error_runtime,             // generic runtime error
    .assert = (Exc)_cex_errors_Error_assert,               // generic runtime check
    .os = (Exc)_cex_errors_Error_os,                       // generic OS check
    .timeout = (Exc)_cex_errors_Error_timeout,             // await interval timeout
    .permission = (Exc)_cex_errors_Error_permission,       // Permission denied
    .try_again = (Exc)_cex_errors_Error_try_again,         // EAGAIN / EWOULDBLOCK errno analog
};
