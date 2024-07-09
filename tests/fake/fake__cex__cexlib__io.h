// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/io.h>


FAKE_VALUE_FUNC(Exc, io_fopen, io_c*, const char*, const char*, const Allocator_c*)
FAKE_VALUE_FUNC(Exc, io_fattach, io_c*, FILE*, const Allocator_c*)
FAKE_VALUE_FUNC(int, io_fileno, io_c*)
FAKE_VALUE_FUNC(bool, io_isatty, io_c*)
FAKE_VALUE_FUNC(Exc, io_flush, io_c*)
FAKE_VALUE_FUNC(Exc, io_seek, io_c*, long, int)
FAKE_VOID_FUNC(io_rewind, io_c*)
FAKE_VALUE_FUNC(Exc, io_tell, io_c*, size_t*)
FAKE_VALUE_FUNC(size_t, io_size, io_c*)
FAKE_VALUE_FUNC(Exc, io_readall, io_c*, sview_c*)
FAKE_VALUE_FUNC(Exc, io_readline, io_c*, sview_c*)
FAKE_VOID_FUNC(io_close, io_c*)

const struct __module__io io = {
    // Autogenerated by CEX
    // clang-format off
    .fopen = io_fopen,
    .fattach = io_fattach,
    .fileno = io_fileno,
    .isatty = io_isatty,
    .flush = io_flush,
    .seek = io_seek,
    .rewind = io_rewind,
    .tell = io_tell,
    .size = io_size,
    .readall = io_readall,
    .readline = io_readline,
    .close = io_close,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__io__resetall(void) {
    RESET_FAKE(io_fopen)
    RESET_FAKE(io_fattach)
    RESET_FAKE(io_fileno)
    RESET_FAKE(io_isatty)
    RESET_FAKE(io_flush)
    RESET_FAKE(io_seek)
    RESET_FAKE(io_rewind)
    RESET_FAKE(io_tell)
    RESET_FAKE(io_size)
    RESET_FAKE(io_readall)
    RESET_FAKE(io_readline)
    RESET_FAKE(io_close)
}

