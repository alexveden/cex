#include "cex.h"

const struct _CEX_Error_struct Error = {
    .ok = EOK,                       // Success
    .memory = "MemoryError",         // memory allocation error
    .io = "IOError",                 // IO error
    .overflow = "OverflowError",     // buffer overflow
    .argument = "ArgumentError",     // function argument error
    .integrity = "IntegrityError",   // data integrity error
    .exists = "ExistsError",         // entity or key already exists
    .not_found = "NotFoundError",    // entity or key already exists
    .skip = "ShouldBeSkipped",       // NOT an error, function result must be skipped
    .empty = "EmptyError",           // resource is empty
    .eof = "EOF",                    // end of file reached
    .argsparse = "ProgramArgsError", // program arguments empty or incorrect
    .runtime = "RuntimeError",       // generic runtime error
};
