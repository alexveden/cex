#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_io)
#include "all.h"
#include "src/cex_base.h"
#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <io.h>
#    include <sys/stat.h>
#    include <windows.h>
#elif cex$is_freestanding
// does not support unistd.h
#else
#    include <sys/stat.h>
#    include <unistd.h>
#endif

/// Opens new file: io.fopen(&file, "file.txt", "r+")
Exception
cex_io_fopen(FILE** file, char* filename, char* mode)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    *file = NULL;

    if (filename == NULL) { return Error.argument; }
    if (mode == NULL) {
        uassert(mode != NULL);
        return Error.argument;
    }

    *file = fopen(filename, mode);
    if (*file == NULL) {
        switch (errno) {
            case ENOENT:
                return Error.not_found;
            default:
                return strerror(errno);
        }
    }
    return Error.ok;
}


/// Check if current file supports ANSI colors and in interactive terminal mode
bool
cex_io_isatty(FILE* file)
{
    uassert(file != NULL);

    if (unlikely(file == NULL)) { return false; }
#ifdef _WIN32
    return _isatty(_fileno(file));
#elif cex$is_freestanding
    return false;
#else
    return isatty(fileno(file));
#endif
}

/// Flush changes to file
Exception
cex_io_fflush(FILE* file)
{
    uassert(file != NULL);

    int ret = fflush(file);
    if (unlikely(ret == -1)) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

/// Seek file position
Exception
cex_io_fseek(FILE* file, long offset, int whence)
{
    uassert(file != NULL);

    int ret = fseek(file, offset, whence);
    if (unlikely(ret == -1)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
    } else {
        return Error.ok;
    }
}

/// Rewind file cursor at the beginning
void
cex_io_rewind(FILE* file)
{
    uassert(file != NULL);
    rewind(file);
}

/// Returns current cursor position into `size` pointer
Exception
cex_io_ftell(FILE* file, usize* size)
{
    uassert(file != NULL);

    long ret = ftell(file);
    if (unlikely(ret < 0)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
        *size = 0;
    } else {
        *size = ret;
        return Error.ok;
    }
}

/// Return full file size, always 0 for NULL file or atty
usize
cex_io__file__size(FILE* file)
{
    if (unlikely(file == NULL)) { return 0; }
    if (unlikely(io.isatty(file))) { return 0; }
#ifdef _WIN32
    struct _stat win_stat;
    int ret = _fstat(fileno(file), &win_stat);
    if (ret == 0) {
        return win_stat.st_size;
    } else {
        return 0;
    }
#elif cex$is_freestanding
    return 0;
#else
    struct stat stat;
    int ret = fstat(fileno(file), &stat);
    if (ret == 0) {
        return stat.st_size;
    } else {
        return 0;
    }
#endif
}

/// Read file contents into the buf, return nbytes read (can be < buff_len), 0 on EOF, negative on
/// error (you may use os.get_last_error() for getting Exception for error, cross-platform )
isize
cex_io_fread(FILE* file, void* buff, usize buff_len)
{
    uassert(file != NULL);
    uassert(buff != NULL);
    uassert(buff_len < PTRDIFF_MAX && "Must fit to isize max");

    if (unlikely(buff_len >= PTRDIFF_MAX)) {
        return -1; // hard protecting even in production
    }

    usize ret_count = fread(buff, 1, buff_len, file);

    if (ret_count != buff_len) {
        if (ferror(file)) {
            // NOTE: use os.get_last_error() to get platform specific error
            return -1;
        }
    }

    uassert(ret_count < PTRDIFF_MAX);
    return ret_count;
}

/// Read all contents of the file, using allocator. You should free `s.buf` after.
Exception
cex_io_fread_all(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize buf_size = 0;
    char* buf = NULL;


    if (file == NULL) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);
    uassert(allc != NULL);

    // Forbid console and stdin
    if (unlikely(cex_io_isatty(file))) {
        result = "io.fread_all() not allowed for pipe/socket/std[in/out/err]";
        goto fail;
    }

    usize _fsize = cex_io__file__size(file);
    if (unlikely(_fsize > INT32_MAX)) {
        result = "io.fread_all() file is too big.";
        goto fail;
    }
    bool is_stream = false;
    if (unlikely(_fsize == 0)) {
        if (feof(file)) {
            result = EOK; // return just empty data
            goto fail;
        } else {
            is_stream = true;
        }
    }
    buf_size = (is_stream ? 4096 : _fsize) + 1 + 15;
    buf = mem$malloc(allc, buf_size);

    if (unlikely(buf == NULL)) {
        buf_size = 0;
        result = Error.memory;
        goto fail;
    }

    size_t total_read = 0;
    while (1) {
        if (total_read == buf_size) {
            if (unlikely(total_read > INT32_MAX)) {
                result = "io.fread_all() stream output is too big.";
                goto fail;
            }
            if (buf_size > 100 * 1024 * 1024) {
                buf_size *= 1.2;
            } else {
                buf_size *= 2;
            }
            char* new_buf = mem$realloc(allc, buf, buf_size);
            if (!new_buf) {
                result = Error.memory;
                goto fail;
            }
            buf = new_buf;
        }

        // Read data into the buf
        size_t bytes_read = fread(buf + total_read, 1, buf_size - total_read, file);
        if (bytes_read == 0) {
            if (feof(file)) {
                break;
            } else if (ferror(file)) {
                result = Error.io;
                goto fail;
            }
        }
        total_read += bytes_read;
    }
    char* final_buffer = mem$realloc(allc, buf, total_read + 1);
    if (!final_buffer) {
        result = Error.memory;
        goto fail;
    }
    buf = final_buffer;

    *s = (str_s){
        .buf = buf,
        .len = total_read,
    };
    buf[total_read] = '\0';
    return EOK;

fail:
    *s = (str_s){ .buf = NULL, .len = 0 };
    if (cex_io_fseek(file, 0, SEEK_END)) {}
    if (buf) { mem$free(allc, buf); }
    return result;
}

/// Reads line from a file into str_s buffer, allocates memory. You should free `s.buf` after.
Exception
cex_io_fread_line(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize cursor = 0;
    char* buf = NULL;
    usize buf_size = 0;

    if (unlikely(file == NULL)) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);

    int c = EOF;
    while ((c = fgetc(file)) != EOF) {
        if (unlikely(c == '\n')) {
            // Handle windows \r\n new lines also
            if (cursor > 0 && buf[cursor - 1] == '\r') { cursor--; }
            break;
        }
        if (unlikely(c == '\0')) {
            // plain text file should not have any zero bytes in there
            result = Error.integrity;
            goto fail;
        }

        if (unlikely(cursor >= buf_size)) {
            if (buf == NULL) {
                uassert(cursor == 0 && "no buf, cursor expected 0");

                buf = mem$malloc(allc, 256);
                if (buf == NULL) {
                    result = Error.memory;
                    goto fail;
                }
                buf_size = 256 - 1; // keep extra for null
                buf[buf_size] = '\0';
                buf[cursor] = '\0';
            } else {
                uassert(cursor > 0 && "no buf, cursor expected 0");
                uassert(buf_size > 0 && "empty buffer, weird");

                if (buf_size + 1 < 256) {
                    // Cap minimal buf size
                    buf_size = 256 - 1;
                }

                // Grow initial size by factor of 2
                buf = mem$realloc(allc, buf, (buf_size + 1) * 2);
                if (buf == NULL) {
                    buf_size = 0;
                    result = Error.memory;
                    goto fail;
                }
                buf_size = (buf_size + 1) * 2 - 1;
                buf[buf_size] = '\0';
            }
        }
        buf[cursor] = c;
        cursor++;
    }


    if (unlikely(ferror(file))) {
        result = Error.io;
        goto fail;
    }

    if (unlikely(cursor == 0 && feof(file))) {
        result = Error.eof;
        goto fail;
    }

    if (buf != NULL) {
        buf[cursor] = '\0';
    } else {
        buf = mem$malloc(allc, 1);
        buf[0] = '\0';
        cursor = 0;
    }
    *s = (str_s){
        .buf = buf,
        .len = cursor,
    };
    return Error.ok;

fail:
    if (buf) { mem$free(allc, buf); }
    *s = (str_s){
        .buf = NULL,
        .len = 0,
    };
    return result;
}

/// Prints formatted string to the file. Uses CEX printf() engine with special formatting.
Exc
cex_io_fprintf(FILE* stream, char* format, ...)
{
    uassert(stream != NULL);

    va_list va;
    va_start(va, format);
    int result = cexsp__vfprintf(stream, format, va);
    va_end(va);

    if (result == -1) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

/// Prints formatted string to stdout. Uses CEX printf() engine with special formatting.
int
cex_io_printf(char* format, ...)
{
    va_list va;
    va_start(va, format);
    int result = cexsp__vfprintf(stdout, format, va);
    va_end(va);
    return result;
}

/// Writes bytes to the file
Exception
cex_io_fwrite(FILE* file, void* buff, usize buff_len)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    if (buff == NULL) { return Error.argument; }
    if (buff_len == 0) { return Error.argument; }

    usize ret_count = fwrite(buff, 1, buff_len, file);

    if (ret_count != buff_len) {
        // return os.get_last_error();
        switch (errno) {
            case 0:
                uassert(errno != 0 && "errno is ok");
                return "Error, but errno is not set";
            case ENOENT:
                return Error.not_found;
            case EPERM:
                return Error.permission;
            case EIO:
                return Error.io;
            case EAGAIN:
                return Error.try_again;
            default:
                return strerror(errno);
        }
    } else {
        return Error.ok;
    }
}

/// Writes new line to the file
Exception
cex_io__file__writeln(FILE* file, char* line)
{
    errno = 0;
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }
    if (line == NULL) { return Error.argument; }
    usize line_len = strlen(line);
    usize ret_count = fwrite(line, 1, line_len, file);
    if (ret_count != line_len) { return Error.io; }

    char new_line[] = { '\n' };
    ret_count = fwrite(new_line, 1, sizeof(new_line), file);

    if (ret_count != sizeof(new_line)) { return Error.io; }
    return Error.ok;
}

/// Closes file and set it to NULL.
void
cex_io_fclose(FILE** file)
{
    uassert(file != NULL);

    if (*file != NULL) { fclose(*file); }
    *file = NULL;
}


/// Saves full `contents` in the file at `path`, using text mode. 
Exception
cex_io__file__save(char* path, char* contents)
{
    if (path == NULL) { return Error.argument; }

    if (contents == NULL) { return Error.argument; }

    FILE* file;
    // NOLINTNEXTLINE
    e$except_silent (err, cex_io_fopen(&file, path, "w")) { return err; }

    usize contents_len = strlen(contents);
    if (contents_len > 0) {
        e$except_silent (err, cex_io_fwrite(file, contents, contents_len)) {
            cex_io_fclose(&file);
            return err;
        }
    }

    cex_io_fclose(&file);
    return EOK;
}

/// Load full contents of the file at `path`, using text mode. Returns NULL on error.
char*
cex_io__file__load(char* path, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    FILE* file;
    // NOLINTNEXTLINE
    e$except_silent (err, cex_io_fopen(&file, path, "r")) { return NULL; }

    str_s out_content = (str_s){ 0 };
    e$except_silent (err, cex_io_fread_all(file, &out_content, allc)) {
        if (err == Error.eof) {
            uassert(out_content.buf == NULL);
            out_content.buf = mem$malloc(allc, 1);
            if (out_content.buf) { out_content.buf[0] = '\0'; }
            goto end;
        }
        if (out_content.buf) { mem$free(allc, out_content.buf); }
        goto end;
    }
end:
    cex_io_fclose(&file);

    return out_content.buf;
}

/// Reads line from file, allocates result. Returns NULL on error.
char*
cex_io__file__readln(FILE* file, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (file == NULL) {
        errno = EINVAL;
        return NULL;
    }

    str_s out_content = (str_s){ 0 };
    Exc err = cex_io_fread_line(file, &out_content, allc);
    if (err) {
        if (err == Error.eof) { return NULL; }
        if (out_content.buf) { mem$free(allc, out_content.buf); }
        return NULL;
    }
    return out_content.buf;
}

const struct __cex_namespace__io io = {
    // Autogenerated by CEX
    // clang-format off

    .fclose = cex_io_fclose,
    .fflush = cex_io_fflush,
    .fopen = cex_io_fopen,
    .fprintf = cex_io_fprintf,
    .fread = cex_io_fread,
    .fread_all = cex_io_fread_all,
    .fread_line = cex_io_fread_line,
    .fseek = cex_io_fseek,
    .ftell = cex_io_ftell,
    .fwrite = cex_io_fwrite,
    .isatty = cex_io_isatty,
    .printf = cex_io_printf,
    .rewind = cex_io_rewind,

    .file = {
        .load = cex_io__file__load,
        .readln = cex_io__file__readln,
        .save = cex_io__file__save,
        .size = cex_io__file__size,
        .writeln = cex_io__file__writeln,
    },

    // clang-format on
};
#endif
