#include "cex.h"

const struct _CEX_Error_struct Error = {
    .ok = EOK,                     // Success
    .memory = "MemoryError",       // memory allocation error
    .io = "IOError",               // IO error
    .overflow = "OverflowError",   // buffer overflow
    .argument = "ArgumentError",   // function argument error
    .integrity = "IntegrityError", // data integrity error
    .exists = "ExistsError",       // entity or key already exists
    .not_found = "NotFoundError",  // entity or key already exists
    .skip = "ShouldBeSkipped",     // NOT an error, function result must be skipped
    .check = "SanityCheckError",   // uerrcheck() failed
    .empty = "EmptyError",         // resource is empty
    .eof = "EOF",                  // end of file reached
};


#ifndef CEXTEST

#include "cexlib/allocators.c"
#include "cexlib/list.c"
#include "cexlib/dict.c"
#include "cexlib/deque.c"
#include "cexlib/io.c"
#include "cexlib/sbuf.c"
#include "cexlib/str.c"
#include "cexlib/_stb_sprintf.c"

#endif
