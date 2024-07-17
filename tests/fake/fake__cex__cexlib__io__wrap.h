// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <_cexlib/io.h>

// IMPORTANT: wrapping works only with gcc  `-Wl,--wrap=Shmem_new,--wrap=Protocol_event_emitter_new`  flag
FAKE_VALUE_FUNC(Exc, __wrap_io_fopen, io_c*, const char*, const char*, const Allocator_i*)Exception __real_io_fopen(io_c*, const char*, const char*, const Allocator_i*);

FAKE_VALUE_FUNC(Exc, __wrap_io_fattach, io_c*, FILE*, const Allocator_i*)Exception __real_io_fattach(io_c*, FILE*, const Allocator_i*);

FAKE_VALUE_FUNC(int, __wrap_io_fileno, io_c*)int __real_io_fileno(io_c*);

FAKE_VALUE_FUNC(bool, __wrap_io_isatty, io_c*)bool __real_io_isatty(io_c*);

FAKE_VALUE_FUNC(Exc, __wrap_io_flush, io_c*)Exception __real_io_flush(io_c*);

FAKE_VALUE_FUNC(Exc, __wrap_io_seek, io_c*, long, int)Exception __real_io_seek(io_c*, long, int);

FAKE_VOID_FUNC(__wrap_io_rewind, io_c*)void __real_io_rewind(io_c*);

FAKE_VALUE_FUNC(Exc, __wrap_io_tell, io_c*, size_t*)Exception __real_io_tell(io_c*, size_t*);

FAKE_VALUE_FUNC(size_t, __wrap_io_size, io_c*)size_t __real_io_size(io_c*);

FAKE_VALUE_FUNC(Exc, __wrap_io_read, io_c*, void*, size_t, size_t*)Exception __real_io_read(io_c*, void*, size_t, size_t*);

FAKE_VALUE_FUNC(Exc, __wrap_io_readall, io_c*, str_c*)Exception __real_io_readall(io_c*, str_c*);

FAKE_VALUE_FUNC(Exc, __wrap_io_readline, io_c*, str_c*)Exception __real_io_readline(io_c*, str_c*);

FAKE_VALUE_FUNC_VARARG(Exc, __wrap_io_fprintf, io_c*, const char*, ...)Exception __real_io_fprintf(io_c*, const char*, ...);

FAKE_VALUE_FUNC(Exc, __wrap_io_write, io_c*, void*, size_t, size_t)Exception __real_io_write(io_c*, void*, size_t, size_t);

FAKE_VOID_FUNC(__wrap_io_close, io_c*)void __real_io_close(io_c*);


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
    .read = io_read,
    .readall = io_readall,
    .readline = io_readline,
    .fprintf = io_fprintf,
    .write = io_write,
    .close = io_close,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__io__wrap__resetall(void) {
    RESET_FAKE(__wrap_io_fopen)
    RESET_FAKE(__wrap_io_fattach)
    RESET_FAKE(__wrap_io_fileno)
    RESET_FAKE(__wrap_io_isatty)
    RESET_FAKE(__wrap_io_flush)
    RESET_FAKE(__wrap_io_seek)
    RESET_FAKE(__wrap_io_rewind)
    RESET_FAKE(__wrap_io_tell)
    RESET_FAKE(__wrap_io_size)
    RESET_FAKE(__wrap_io_read)
    RESET_FAKE(__wrap_io_readall)
    RESET_FAKE(__wrap_io_readline)
    RESET_FAKE(__wrap_io_fprintf)
    RESET_FAKE(__wrap_io_write)
    RESET_FAKE(__wrap_io_close)
}

