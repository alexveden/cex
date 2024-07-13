#include "io.h"
#include "_stb_sprintf.h"
#include "cex/cex.h"
#include <errno.h>
#include <unistd.h>

Exception
io_fopen(io_c* self, const char* filename, const char* mode, const Allocator_i* allocator)
{
    if (self == NULL) {
        uassert(self != NULL);
        return Error.argument;
    }
    if (filename == NULL) {
        uassert(filename != NULL);
        return Error.argument;
    }
    if (mode == NULL) {
        uassert(mode != NULL);
        return Error.argument;
    }
    if (allocator == NULL) {
        uassert(allocator != NULL);
        return Error.argument;
    }


    *self = (io_c){
        ._fh = allocator->fopen(filename, mode),
        ._allocator = allocator,
    };

    if (self->_fh == NULL) {
        memset(self, 0, sizeof(*self));
        return raise_exc(Error.io, "%s, file: %s\n", strerror(errno), filename);
    }

    return Error.ok;
}

Exception
io_fattach(io_c* self, FILE* fh, const Allocator_i* allocator)
{
    if (self == NULL) {
        uassert(self != NULL);
        return Error.argument;
    }
    if (allocator == NULL) {
        uassert(allocator != NULL);
        return Error.argument;
    }
    if (fh == NULL) {
        uassert(fh != NULL);
        return Error.argument;
    }

    *self = (io_c){ ._fh = fh,
                    ._allocator = allocator,
                    ._flags = {
                        .is_attached = true,
                    } };

    return Error.ok;
}

int
io_fileno(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    return fileno(self->_fh);
}

bool
io_isatty(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    return isatty(fileno(self->_fh)) == 1;
}

Exception
io_flush(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    int ret = fflush(self->_fh);
    if (unlikely(ret == -1)) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io_seek(io_c* self, long offset, int whence)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    int ret = fseek(self->_fh, offset, whence);
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

void
io_rewind(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    rewind(self->_fh);
}

Exception
io_tell(io_c* self, size_t* size)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    long ret = ftell(self->_fh);
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

size_t
io_size(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    if (self->_fsize == 0) {
        // Do some caching
        size_t old_pos = 0;
        except(err, io_tell(self, &old_pos))
        {
            return 0;
        }
        except(err, io_seek(self, 0, SEEK_END))
        {
            return 0;
        }
        except(err, io_tell(self, &self->_fsize))
        {
            return 0;
        }
        except(err, io_seek(self, old_pos, SEEK_SET))
        {
            return 0;
        }
    }

    return self->_fsize;
}

Exception
io_read(io_c* self, void* restrict obj_buffer, size_t obj_el_size, size_t* obj_count)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == NULL || *obj_count == 0) {
        return Error.argument;
    }

    const size_t ret_count = fread(obj_buffer, obj_el_size, *obj_count, self->_fh);

    if (ret_count != *obj_count) {
        if (ferror(self->_fh)) {
            *obj_count = 0;
            return Error.io;
        } else {
            *obj_count = ret_count;
            return (ret_count == 0) ? Error.eof : Error.ok;
        }
    }

    return Error.ok;
}

Exception
io_readall(io_c* self, str_c* s)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);
    uassert(s != NULL);

    // invalidate result if early exit
    *s = (str_c){
        .buf = NULL,
        .len = 0,
    };

    // Forbid console and stdin
    if (io_isatty(self)) {
        return raise_exc(Error.io, "io.readall() not allowed for pipe/socket/std[in/out/err]\n");
    }

    self->_fsize = io_size(self);

    if (unlikely(self->_fsize == 0)) {

        *s = (str_c){
            .buf = "",
            .len = 0,
        };
        except_traceback(err, io_seek(self, 0, SEEK_END))
        {
            ;
        }
        return Error.eof;
    }
    // allocate extra 16 bytes, to catch condition when file size grows
    // this may be indication we are trying to read stream
    size_t exp_size = self->_fsize + 1 + 15;

    if (self->_fbuf == NULL) {
        self->_fbuf = self->_allocator->malloc(exp_size);
        self->_fbuf_size = exp_size;
    } else {
        if (self->_fbuf_size < exp_size) {
            self->_fbuf = self->_allocator->realloc(self->_fbuf, exp_size);
            self->_fbuf_size = exp_size;
        }
    }
    if (unlikely(self->_fbuf == NULL)) {
        self->_fbuf_size = 0;
        return Error.memory;
    }

    size_t read_size = self->_fbuf_size;
    except(err, io_read(self, self->_fbuf, sizeof(char), &read_size))
    {
        return err;
    }

    if (read_size != self->_fsize) {
        utracef("%ld != %ld: %s\n", read_size, self->_fsize, strerror(errno));
        return "File size changed";
    }

    *s = (str_c){
        .buf = self->_fbuf,
        .len = read_size,
    };

    // Always null terminate
    self->_fbuf[read_size] = '\0';

    return read_size == 0 ? Error.eof : Error.ok;
}

Exception
io_readline(io_c* self, str_c* s)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);
    uassert(s != NULL);


    Exc result = Error.ok;
    size_t cursor = 0;
    FILE* fh = self->_fh;
    char* buf = self->_fbuf;
    size_t buf_size = self->_fbuf_size;

    int c = EOF;
    while ((c = fgetc(fh)) != EOF) {
        if (unlikely(c == '\n')) {
            // Handle windows \r\n new lines also
            if (cursor > 0 && buf[cursor - 1] == '\r') {
                cursor--;
            }
            break;
        }
        if (unlikely(c == '\0')) {
            // plain text file should not have any zero bytes in there
            result = Error.integrity;
            goto fail;
        }

        if (unlikely(cursor >= buf_size)) {
            if (self->_fbuf == NULL) {
                uassert(cursor == 0 && "no buf, cursor expected 0");

                self->_fbuf = buf = self->_allocator->malloc(4096);
                if (self->_fbuf == NULL) {
                    result = Error.memory;
                    goto fail;
                }
                self->_fbuf_size = buf_size = 4096 - 1; // keep extra for null
                self->_fbuf[self->_fbuf_size] = '\0';
                self->_fbuf[cursor] = '\0';
            } else {
                uassert(cursor > 0 && "no buf, cursor expected 0");
                uassert(self->_fbuf_size > 0 && "empty buffer, weird");

                if (self->_fbuf_size + 1 < 4096) {
                    // Cap minimal buf size
                    self->_fbuf_size = 4095;
                }

                // Grow initial size by factor of 2
                self->_fbuf = buf = self->_allocator->realloc(
                    self->_fbuf,
                    (self->_fbuf_size + 1) * 2
                );
                if (self->_fbuf == NULL) {
                    self->_fbuf_size = 0;
                    result = Error.memory;
                    goto fail;
                }
                self->_fbuf_size = buf_size = (self->_fbuf_size + 1) * 2 - 1;
                self->_fbuf[self->_fbuf_size] = '\0';
            }
        }
        buf[cursor] = c;
        cursor++;
    }

    if (self->_fbuf != NULL) {
        self->_fbuf[cursor] = '\0';
    }

    if (ferror(self->_fh)) {
        result = Error.io;
        goto fail;
    }

    if (cursor == 0) {
        // return valid str_c, but empty string
        *s = (str_c){
            .buf = "",
            .len = cursor,
        };
        return (feof(self->_fh) ? Error.eof : Error.ok);
    } else {
        *s = (str_c){
            .buf = self->_fbuf,
            .len = cursor,
        };
        return Error.ok;
    }

fail:
    *s = (str_c){
        .buf = NULL,
        .len = 0,
    };
    return result;
}

Exception
io_fprintf(io_c* self, const char* format, ...)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    va_list va;
    va_start(va, format);
    int result = STB_SPRINTF_DECORATE(vfprintf)(self->_fh, format, va);
    va_end(va);

    if (result == -1) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io_write(io_c* self, void* restrict obj_buffer, size_t obj_el_size, size_t obj_count)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == 0) {
        return Error.argument;
    }

    const size_t ret_count = fwrite(obj_buffer, obj_el_size, obj_count, self->_fh);

    if (ret_count != obj_count) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

void
io_close(io_c* self)
{
    if (self != NULL) {
        uassert(self->_allocator != NULL && "allocator not set");

        if (self->_fh != NULL && !self->_flags.is_attached) {
            // prevent closing attached FILE* (i.e. stdin/out or other)
            self->_allocator->fclose(self->_fh);
        }

        if (self->_fbuf != NULL) {
            self->_allocator->free(self->_fbuf);
        }

        memset(self, 0, sizeof(*self));
    }
}


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