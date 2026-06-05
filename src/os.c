#if !defined(cex$enable_minimal) || defined(cex$enable_os)
#    include "os.h"

#    ifndef _WIN32
#        include <dirent.h>
#    else // _WIN32
// minirent.h HEADER BEGIN
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================

struct dirent
{
    char d_name[MAX_PATH + 1];
};

typedef struct DIR DIR;

static DIR* opendir(char* dirpath);
static struct dirent* readdir(DIR* dirp);
static int closedir(DIR* dirp);

struct DIR
{
    HANDLE hFind;
    WIN32_FIND_DATA data;
    struct dirent* dirent;
};

DIR*
opendir(char* dirpath)
{
    char buffer[MAX_PATH + 10];
    snprintf(buffer, sizeof(buffer), "%s\\*", dirpath);

    DIR* dir = (DIR*)realloc(NULL, sizeof(DIR));
    if (dir == NULL) {
        errno = ENOMEM;
        goto fail;
    }
    memset(dir, 0, sizeof(DIR));

    dir->hFind = FindFirstFileA(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }

    return dir;

fail:
    if (dir) { free(dir); }

    return NULL;
}

struct dirent*
readdir(DIR* dirp)
{
    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent*)realloc(NULL, sizeof(struct dirent));
        if (dirp->dirent == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        memset(dirp->dirent, 0, sizeof(struct dirent));
    } else {
        if (!FindNextFileA(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }

            return NULL;
        }
    }

    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

    strncpy(dirp->dirent->d_name, dirp->data.cFileName, sizeof(dirp->dirent->d_name) - 1);

    return dirp->dirent;
}

int
closedir(DIR* dirp)
{
    if (!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }

    if (dirp->dirent) { free(dirp->dirent); }
    free(dirp);

    return 0;
}
#    endif // _WIN32

/// Sleep for `seconds` duration (f64). On Windows, precision is limited to ~1-2ms
/// (Sleep() uses millisecond granularity). Use os.timer() for high-resolution timing.
static void
cex_os_sleep(f64 seconds)
{
    if (seconds <= 0) return;
#    ifdef _WIN32
    Sleep((DWORD)(seconds * 1000.0));
#    else
    struct timespec ts = {
        .tv_sec = (time_t)seconds,
        .tv_nsec = (long)((seconds - (f64)(time_t)seconds) * 1e9),
    };
    nanosleep(&ts, NULL);
#    endif
}

/// Exit process with status code. Does not return.
static void
cex_os_exit(i32 code)
{
    exit(code);
}

/// Get high performance monotonic timer value in seconds, started from the first call of the
/// os.timer()
static f64
cex_os_timer(void)
{
    // NOTE: we calculate start time only once, and keep it indefinitely (because of static modifier)

#    ifdef _WIN32
    static LARGE_INTEGER frequency = { 0 };
    if (unlikely(frequency.QuadPart == 0)) { QueryPerformanceFrequency(&frequency); }

    static LARGE_INTEGER start = { 0 };
    if (unlikely(start.QuadPart == 0)) {
        QueryPerformanceCounter(&start);
        uassert(start.QuadPart > 1);
        start.QuadPart -= 1;
    }

    static LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (f64)(now.QuadPart - start.QuadPart) / (f64)frequency.QuadPart;

#    else
    static u64 start_ticks = 0;
    if (unlikely(start_ticks == 0)) {
        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);
        start_ticks = (u64)start.tv_sec * 1000000000 + (u64)start.tv_nsec;
        uassert(start_ticks > 1);
        start_ticks -= 1;
    }
    static struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    u64 now_ticks = (u64)now.tv_sec * 1000000000 + (u64)now.tv_nsec;

    return (f64)(now_ticks - start_ticks) / (f64)1e9;
#    endif
}

/// Computes generic buffer SIP hash (platform/endiannes stable), seed can be null, or previous hash
/// value for hash stacking  (null or empty `p` returns 0 hash). (This is the same general hash
/// function is used in str.hash() and hm$ hashmaps)
static u64
cex_os_hash(void* p, usize psize, u64 seed)
{
    if (unlikely(p == NULL || psize == 0)) { return 0; }
    return _cexds__hash_bytes(p, psize, seed);
}


/// Get available CPU cores on system, or -1 on error
i32
cex_os_cpu_count(void)
{
#    ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return (int)sysinfo.dwNumberOfProcessors;
#    else
    // Linux, Solaris, and other POSIX systems
    long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpus == -1) {
        // Fallback to _SC_NPROCESSORS_CONF if _SC_NPROCESSORS_ONLN fails
        ncpus = sysconf(_SC_NPROCESSORS_CONF);
    }
    return (i32)ncpus;
#    endif
}

/// Get current process ID
i32
cex_os_getpid(void)
{
#    ifdef _WIN32
    return (i32)GetCurrentProcessId();
#    else
    return (i32)getpid();
#    endif
}

/// Get last system API error as string representation (Exception compatible). Result content may be
/// affected by OS locale settings.
static Exc
cex_os_get_last_error(void)
{
#    ifdef _WIN32
    DWORD err = GetLastError();
    switch (err) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return Error.not_found;
        case ERROR_ACCESS_DENIED:
            return Error.permission;
        default:
            break;
    }

    _Thread_local static char win32_err_buf[256] = { 0 };

    DWORD err_msg_size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        LANG_USER_DEFAULT,
        win32_err_buf,
        arr$len(win32_err_buf),
        NULL
    );

    if (unlikely(err_msg_size == 0)) {
        if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(win32_err_buf, "Generic windows error code: 0x%lX", err) > 0) {
                return (char*)&win32_err_buf;
            } else {
                return NULL;
            }
        } else {
            if (sprintf(win32_err_buf, "Invalid Windows Error code (0x%lX)", err) > 0) {
                return (char*)&win32_err_buf;
            } else {
                return NULL;
            }
        }
    }

    while (err_msg_size > 1 && isspace(win32_err_buf[err_msg_size - 1])) {
        win32_err_buf[--err_msg_size] = '\0';
    }

    return win32_err_buf;
#    else
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
#    endif
}

/// Renames file or directory
static Exception
cex_os__fs__rename(char* old_path, char* new_path)
{
    if (old_path == NULL || old_path[0] == '\0') { return Error.argument; }
    if (new_path == NULL || new_path[0] == '\0') { return Error.argument; }
    if (os.path.exists(new_path)) { return Error.exists; }
#    ifdef _WIN32
    if (!MoveFileExA(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) { return os.get_last_error(); }
    return EOK;
#    else
    if (rename(old_path, new_path) < 0) { return os.get_last_error(); }
    return EOK;
#    endif // _WIN32
}

/// Makes directory (no error if exists)
static Exception
cex_os__fs__mkdir(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }
#    ifdef _WIN32
    int result = mkdir(path);
#    else
    int result = mkdir(path, 0755);
#    endif
    if (result < 0) {
        uassert(errno != 0);
        if (errno == EEXIST) { return EOK; }
        return os.get_last_error();
    }
    return EOK;
}

/// Makes all directories in a path
static Exception
cex_os__fs__mkpath(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }
    str_s dir = os.path.split(path, true);
    char dir_path[PATH_MAX] = { 0 };
    e$ret(str.slice.copy(dir_path, dir, sizeof(dir_path)));
    if (os.path.exists(dir_path)) { return EOK; }
    usize dir_path_len = 0;

    for$iter (str_s, it, str.slice.iter_split(dir, "\\/", &it.iterator)) {
        if (dir_path_len > 0) {
            uassert(dir_path_len < sizeof(dir_path) - 2);
            dir_path[dir_path_len] = os$PATH_SEP;
            dir_path_len++;
            dir_path[dir_path_len] = '\0';
        }
        e$ret(str.slice.copy(dir_path + dir_path_len, it.val, sizeof(dir_path) - dir_path_len));
        dir_path_len += it.val.len;
        e$ret(os.fs.mkdir(dir_path));
    }
    return EOK;
}

/// Returns cross-platform path stats information (see os_fs_stat_s)
static os_fs_stat_s
cex_os__fs__stat(char* path)
{
    os_fs_stat_s result = { .error = Error.argument };
    if (path == NULL || path[0] == '\0') { return result; }

#    ifdef _WIN32
    // NOTE: for mingw64 _stat() doesn't do well when path has trailing /
    usize plen = strlen(path);
    if (unlikely(plen >= PATH_MAX)) {
        result.error = Error.overflow;
        return result;
    }

    char clean_path[PATH_MAX + 10];
    memcpy(clean_path, path, plen + 1); // including \0
    if (unlikely(clean_path[plen - 1] == '/' || clean_path[plen - 1] == '\\')) {
        for (u32 i = plen; --i > 0;) {
            if (clean_path[i] == '/' || clean_path[i] == '\\') {
                clean_path[i] = '\0';
            } else {
                break;
            }
        }
    }

    struct _stat statbuf;
    if (unlikely(_stat(clean_path, &statbuf) < 0)) {
        result.error = os.get_last_error();
        return result;
    }

    result.error = EOK;
    result.is_valid = true;
    result.mtime = statbuf.st_mtime;
    result.is_other = true;
    result.is_symlink = false;

    if (statbuf.st_mode & _S_IFDIR) {
        result.is_directory = true;
        result.is_other = false;
    }

    if (statbuf.st_mode & _S_IFREG) {
        result.is_directory = false;
        result.is_other = false;
        result.is_file = true;
    }

    result.size = statbuf.st_size;

    return result;
#    else // _WIN32
    struct stat statbuf;
    if (unlikely(lstat(path, &statbuf) < 0)) {
        result.error = os.get_last_error();
        return result;
    }
    result.is_valid = true;
    result.error = EOK;
    result.is_other = true;
    result.mtime = statbuf.st_mtime;

    if (!S_ISLNK(statbuf.st_mode)) {
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    } else {
        result.is_symlink = true;
        if (unlikely(stat(path, &statbuf) < 0)) {
            result.error = os.get_last_error();
            return result;
        }
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    }
    result.size = statbuf.st_size;
    return result;
#    endif
}

/// Removes file or empty directory (also see os.fs.remove_tree)
static Exception
cex_os__fs__remove(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }

    os_fs_stat_s stat = os.fs.stat(path);
    if (!stat.is_valid) { return stat.error; }
#    ifdef _WIN32
    if (stat.is_file || stat.is_symlink) {
        if (!DeleteFileA(path)) { return os.get_last_error(); }
    } else if (stat.is_directory) {
        if (!RemoveDirectoryA(path)) { return os.get_last_error(); }
    } else {
        return "Unsupported path type";
    }
    return EOK;
#    else
    if (remove(path) < 0) { return os.get_last_error(); }
    return EOK;
#    endif
}

/// Iterates over directory (can be recursive) using callback function
Exception
cex_os__fs__dir_walk(char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx)
{
    (void)user_ctx;
    if (path == NULL || path[0] == '\0') { return Error.argument; }
    Exc result = Error.os;
    uassert(callback_fn != NULL && "you must provide callback_fn");

    DIR* dp = opendir(path);

    if (unlikely(dp == NULL)) {
        result = os.get_last_error();
        goto end;
    }

    u32 path_len = strlen(path);
    if (path_len > PATH_MAX - 2) {
        result = Error.overflow;
        goto end;
    }

    char path_buf[PATH_MAX];

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        errno = 0;
        if (str.eq(ep->d_name, ".")) { continue; }
        if (str.eq(ep->d_name, "..")) { continue; }
        memcpy(path_buf, path, path_len);
        u32 path_offset = 0;
        if (path_buf[path_len - 1] != '/' && path_buf[path_len - 1] != '\\') {
            path_buf[path_len] = os$PATH_SEP;
            path_offset = 1;
        }

        e$except_silent (
            err,
            str.copy(
                path_buf + path_len + path_offset,
                ep->d_name,
                sizeof(path_buf) - path_len - 1 - path_offset
            )
        ) {
            result = err;
            goto end;
        }

        auto ftype = os.fs.stat(path_buf);
        if (!ftype.is_valid) {
            result = ftype.error;
            goto end;
        }

        if (is_recursive && ftype.is_directory && !ftype.is_symlink) {
            e$except_silent (
                err,
                cex_os__fs__dir_walk(path_buf, is_recursive, callback_fn, user_ctx)
            ) {
                result = err;
                goto end;
            }
        }
        // After recursive call make a callback on a directory itself
        e$except_silent (err, callback_fn(path_buf, ftype, user_ctx)) {
            result = err;
            goto end;
        }
    }

    result = EOK;
end:
    if (dp != NULL) { (void)closedir(dp); }
    return result;
}

struct _os_fs_find_ctx_s
{
    char* pattern;
    arr$(char*) result;
    IAllocator allc;
};

static Exception
_os__fs__remove_tree_walker(char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)user_ctx;
    (void)ftype;
    e$except_silent (err, cex_os__fs__remove(path)) {
        log$trace("Error removing: %s\n", path);
        return err;
    }
    return EOK;
}

/// Removes directory and all its contents recursively
static Exception
cex_os__fs__remove_tree(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }
    if (!os.path.exists(path)) { return Error.not_found; }
    e$except_silent (err, cex_os__fs__dir_walk(path, true, _os__fs__remove_tree_walker, NULL)) {
        return err;
    }
    e$except_silent (err, cex_os__fs__remove(path)) {
        log$trace("Error removing: %s\n", path);
        return err;
    }
    return EOK;
}

struct _os_fs_copy_tree_ctx_s
{
    str_s src_dir;
    str_s dest_dir;
};

static Exception
_os__fs__copy_tree_walker(char* path, os_fs_stat_s ftype, void* user_ctx)
{
    struct _os_fs_copy_tree_ctx_s* ctx = user_ctx;
    mem$scope(tmem$, _)
    {
        uassert(str.starts_with(path, ctx->src_dir.buf) && "file must start with source dir");
        char* out_file = str.fmt(_, "%S/%S", ctx->dest_dir, str.sub(path, ctx->src_dir.len, 0));

        if (ftype.is_file) {
            e$ret(os.fs.mkpath(out_file));
            e$ret(os.fs.copy(path, out_file));
        } else {
            // Making empty directory if necessary
            char* out_dir = str.fmt(_, "%s/", out_file);
            e$ret(os.fs.mkpath(out_dir));
        }
    }

    return EOK;
}


/// Copy directory recursively
static Exception
cex_os__fs__copy_tree(char* src_dir, char* dst_dir)
{
    if (src_dir == NULL || src_dir[0] == '\0') { return Error.argument; }
    os_fs_stat_s s = os.fs.stat(src_dir);
    if (!s.is_valid) { return s.error; }
    if (!s.is_directory) { return Error.argument; }

    if (dst_dir == NULL || dst_dir[0] == '\0') { return Error.argument; }
    if (os.path.exists(dst_dir)) { return Error.exists; }

    // TODO: add absolute path overlap check

    struct _os_fs_copy_tree_ctx_s ctx = {
        .src_dir = str.sstr(src_dir),
        .dest_dir = str.sstr(dst_dir),
    };
    e$except_silent (err, cex_os__fs__dir_walk(src_dir, true, _os__fs__copy_tree_walker, &ctx)) {
        return err;
    }

    return EOK;
}

static Exception
_os__fs__find_walker(char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)ftype;
    struct _os_fs_find_ctx_s* ctx = (struct _os_fs_find_ctx_s*)user_ctx;
    uassert(ctx->result != NULL);
    if (ftype.is_directory) {
        return EOK; // skip, only supports finding files!
    }
    if (ftype.is_symlink) {
        return EOK; // does not follow symlinks!
    }

    str_s file_part = os.path.split(path, false);
    if (!str.match(file_part.buf, ctx->pattern)) {
        return EOK; // just skip when patten not matched
    }

    // allocate new string because path is stack allocated buffer in os__fs__dir_walk()
    char* new_path = str.clone(path, ctx->allc);
    if (new_path == NULL) { return Error.memory; }

    // Doing graceful memory check, otherwise arr$push will assert
    if (!arr$grow_check(ctx->result, 1)) { return Error.memory; }
    arr$push(ctx->result, new_path);
    return EOK;
}

/// Finds files in `dir/pattern`, for example "./mydir/*.c" (all c files), if is_recursive=true, all
/// *.c files found in sub-directories.
static arr$(char*) cex_os__fs__find(char* path_pattern, bool is_recursive, IAllocator allc)
{

    if (unlikely(allc == NULL)) { return NULL; }

    str_s dir_part = os.path.split(path_pattern, true);
    if (dir_part.buf == NULL) {
#    if defined(CEX_TEST) || defined(CEX_BUILD)
        (void)e$raise(Error.argument, "Bad path: os.fn.find('%s')", path_pattern);
#    endif
        return NULL;
    }

    if (!is_recursive) {
        auto f = os.fs.stat(path_pattern);
        if (f.is_valid && f.is_file) {
            // Find used with exact file path, we still have to return array + allocated path copy
            arr$(char*) result = arr$new(result, allc);
            char* it = str.clone(path_pattern, allc);
            arr$push(result, it);
            return result;
        }
    }

    char path_buf[PATH_MAX + 2] = { 0 };
    if (dir_part.len > 0 && str.slice.copy(path_buf, dir_part, sizeof(path_buf))) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    if (str.copy(
            path_buf + dir_part.len + 1,
            path_pattern + dir_part.len,
            sizeof(path_buf) - dir_part.len - 1
        )) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    char* dir_name = (dir_part.len > 0) ? path_buf : ".";
    char* pattern = path_buf + dir_part.len + 1;
    if (*pattern == '/' || *pattern == '\\') { pattern++; }
    if (*pattern == '\0') { pattern = "*"; }

    struct _os_fs_find_ctx_s ctx = { .result = arr$new(ctx.result, allc),
                                     .pattern = pattern,
                                     .allc = allc };
    if (unlikely(ctx.result == NULL)) { return NULL; }

    e$except_silent (err, cex_os__fs__dir_walk(dir_name, is_recursive, _os__fs__find_walker, &ctx)) {
        for$each (it, ctx.result) {
            mem$free(allc, it); // each individual item was allocated too
        }
        if (ctx.result != NULL) { arr$free(ctx.result); }
        ctx.result = NULL;
    }
    return ctx.result;
}


/// Get current working directory
static char*
cex_os__fs__getcwd(IAllocator allc)
{
    char* buf = mem$malloc(allc, PATH_MAX);

    char* result = NULL;
#    ifdef _WIN32
    result = _getcwd(buf, PATH_MAX);
#    else
    result = getcwd(buf, PATH_MAX);
#    endif
    if (result == NULL) { mem$free(allc, buf); }

    return result;
}

/// Change current working directory
static Exception
cex_os__fs__chdir(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.exists; }

    int result;
#    ifdef _WIN32
    result = _chdir(path);
#    else
    result = chdir(path);
#    endif

    if (result == -1) {
        if (errno == ENOENT) {
            return Error.not_found;
        } else {
            return strerror(errno);
        }
    }

    return EOK;
}


/// Copy file
static Exception
cex_os__fs__copy(char* src_path, char* dst_path)
{
    if (src_path == NULL || src_path[0] == '\0' || dst_path == NULL || dst_path[0] == '\0') {
        return Error.argument;
    }
    log$trace("copying %s -> %s\n", src_path, dst_path);

    if (os.path.exists(dst_path)) { return Error.exists; }

#    ifdef _WIN32
    if (!CopyFileA(src_path, dst_path, FALSE)) { return os.get_last_error(); }
    return EOK;
#    else
    int src_fd = -1;
    int dst_fd = -1;
    size_t buf_size = 32 * 1024;
    char* buf = mem$malloc(mem$, buf_size);
    if (buf == NULL) { return Error.memory; }
    Exc result = Error.runtime;

    if ((src_fd = open(src_path, O_RDONLY)) == -1) {
        result = os.get_last_error();
        goto defer;
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        result = os.get_last_error();
        goto defer;
    }

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        result = strerror(errno);
        goto defer;
    }

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) { break; }
        if (n < 0) {
            result = os.get_last_error();
            goto defer;
        }
        char* buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                result = os.get_last_error();
                goto defer;
            }
            n -= m;
            buf2 += m;
        }
    }
    result = EOK;

defer:
    mem$free(mem$, buf);
    if (src_fd >= 0) { close(src_fd); }
    if (dst_fd >= 0) { close(dst_fd); }
    return result;
#    endif
}

/// Get environment variable, with `deflt` if not found
static char*
cex_os__env__get(char* name, char* deflt)
{
    char* result = getenv(name);

    if (result == NULL) { result = deflt; }

    return result;
}

/// Set environment variable
static Exception
cex_os__env__set(char* name, char* value)
{
#    ifdef _WIN32
    _putenv_s(name, value);
#    else
    setenv(name, value, true);
#    endif
    // TODO: add error reporting
    return EOK;
}

/// Unset environment variable
static Exception
cex_os__env__unset(char* name)
{
#    ifdef _WIN32
    if (!SetEnvironmentVariable(name, NULL)) { return Error.runtime; }
#    else
    if (unsetenv(name) == -1) { return Error.runtime; }
#    endif
    return EOK;
}

/// Get path to the current executable
static char*
cex_os__env__executable_path(IAllocator allc)
{
    uassert(allc != NULL);
#    ifdef _WIN32
    char buf[MAX_PATH + 2] = { 0 };
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH + 1);
    if (len == 0 || len > MAX_PATH) return NULL;
    return str.clone(buf, allc);
#    elif defined(__APPLE__)
    char    buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return NULL;
    return str.clone(buf, allc);
#    else
    char    buf[PATH_MAX + 1] = { 0 };
    ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX);
    if (len < 0 || len >= (ssize_t)PATH_MAX) return NULL;
    buf[len] = '\0';
    return str.clone(buf, allc);
#    endif
}

/// Normalize path, resolves "." and ".." components and collapses "//"
static char*
cex_os__path__normalize(char* path, IAllocator allc)
{
    uassert(allc != NULL);
    char*      result = NULL;
    char*      work   = NULL;
    arr$(char*) parts  = NULL;
    arr$(char*) stack  = NULL;
    char*      joined = NULL;

    if (path == NULL || path[0] == '\0') {
        result = str.clone(".", allc);
        goto done;
    }

    usize len = strlen(path);
    work = mem$malloc(allc, len + 1);
    if (work == NULL) { goto done; }
    memcpy(work, path, len + 1);
    for (char* p = work; *p; p++) { if (*p == '\\') { *p = '/'; } }

    bool is_absolute = (work[0] == '/');

    parts = str.split(work, "/", allc);
    if (parts == NULL) { goto done; }

    stack = arr$new(stack, allc);
    if (stack == NULL) { goto done; }

    usize start = is_absolute ? 1 : 0;
    for (usize i = start; i < arr$len(parts); i++) {
        char* p = parts[i];
        if (p[0] == '\0' || str.eq(p, ".")) { continue; }
        if (str.eq(p, "..")) {
            if (arr$len(stack) > 0 && !str.eq(arr$last(stack), "..")) {
                arr$pop(stack);
            } else if (!is_absolute) {
                arr$push(stack, p);
            }
            continue;
        }
        arr$push(stack, p);
    }

    usize slen = arr$len(stack);
    if (slen == 0) {
        result = str.clone(is_absolute ? "/" : ".", allc);
        goto done;
    }

    if (is_absolute) {
        joined = str.join((char**)stack, slen, "/", allc);
        if (joined == NULL) { goto done; }
        usize jlen = strlen(joined);
        result = mem$malloc(allc, jlen + 2);
        if (result == NULL) { goto done; }
        result[0] = '/';
        memcpy(result + 1, joined, jlen + 1);
        result[jlen + 1] = '\0';
        goto done;
    }

    {
        char sep[2] = { os$PATH_SEP, '\0' };
        result = str.join((char**)stack, slen, sep, allc);
    }

done:
    if (parts) {
        for$each(p, parts) { mem$free(allc, p); }
        arr$free(parts);
    }
    if (stack) { arr$free(stack); }
    mem$free(allc, work);
    mem$free(allc, joined);
    return result;
}

/// Check if file/directory path exists
static bool
cex_os__path__exists(char* file_path)
{
    auto ftype = os.fs.stat(file_path);
    return ftype.is_valid;
}

/// Returns absolute path from relative (no filesystem access, does not resolve symlinks)
static char*
cex_os__path__absolute(char* path, IAllocator allc)
{
    uassert(allc != NULL);
    char* result = NULL;
    char* cwd    = NULL;
    char* full   = NULL;

    if (path == NULL || path[0] == '\0') { goto done; }
    usize path_len = strlen(path);

    bool is_abs = false;
#    ifdef _WIN32
    if ((path[0] == '\\' && path[1] == '\\') ||
        (((path[0] >= 'A' && path[0] <= 'Z') ||
          (path[0] >= 'a' && path[0] <= 'z')) &&
         path[1] == ':' && (path[2] == '\\' || path[2] == '/'))) {
        is_abs = true;
    }
#    else
    if (path[0] == '/') { is_abs = true; }
#    endif

    if (is_abs) {
        result = cex_os__path__normalize(path, allc);
        goto done;
    }

    cwd = cex_os__fs__getcwd(allc);
    if (cwd == NULL) { goto done; }

    usize cwd_len = strlen(cwd);
    full = mem$malloc(allc, cwd_len + 1 + path_len + 1);
    if (full == NULL) { goto done; }

    memcpy(full, cwd, cwd_len);
    full[cwd_len] = os$PATH_SEP;
    memcpy(full + cwd_len + 1, path, path_len + 1);

    result = cex_os__path__normalize(full, allc);

done:
    mem$free(allc, cwd);
    mem$free(allc, full);
    return result;
}

/// Join path with OS specific path separator
static char*
cex_os__path__join(char** parts, u32 parts_len, IAllocator allc)
{
    char sep[2] = { os$PATH_SEP, '\0' };
    return str.join(parts, parts_len, sep, allc);
}

/// Splits path by `dir` and `file` parts, when return_dir=true - returns `dir` part, otherwise
/// `file` part
static str_s
cex_os__path__split(char* path, bool return_dir)
{
    if (path == NULL) { return (str_s){ 0 }; }
    usize pathlen = strlen(path);
    if (pathlen == 0) { return str$s(""); }

    isize last_slash_idx = -1;

    for (usize i = pathlen; i-- > 0;) {
        if (path[i] == '/' || path[i] == '\\') {
            last_slash_idx = i;
            break;
        }
    }
    if (last_slash_idx != -1) {
        if (return_dir) {
            return str.sub(path, 0, last_slash_idx == 0 ? 1 : last_slash_idx);
        } else {
            if ((usize)last_slash_idx == pathlen - 1) {
                return str$s("");
            } else {
                return str.sub(path, last_slash_idx + 1, 0);
            }
        }

    } else {
        if (return_dir) {
            return str$s("");
        } else {
            return str.sstr(path);
        }
    }
}

/// Get file name of a path
static char*
cex_os__path__basename(char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') { return NULL; }
    str_s fname = cex_os__path__split(path, false);
    return str.slice.clone(fname, allc);
}

/// Get directory name of a path
static char*
cex_os__path__dirname(char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') { return NULL; }
    str_s fname = cex_os__path__split(path, true);
    return str.slice.clone(fname, allc);
}


/// Creates new os command (use os$cmd() and os$cmd() for easy cases). flags can be NULL.
static Exception
cex_os__cmd__create(os_cmd_c* self, char** args, usize args_len, os_cmd_flags_s* flags)
{
    uassert(self != NULL);
    if (args == NULL || args_len == 0) { return "`args` is empty or null"; }
    if (args_len == 1 || args[args_len - 1] != NULL) { return "`args` last item must be a NULL"; }
    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL) {
            return "one of `args` items is NULL, which may indicate string operation failure";
        }
    }

    *self = (os_cmd_c){
        ._is_subprocess = true,
        ._ret_code = -1,
        ._flags = (flags) ? *flags : (os_cmd_flags_s){ 0 },
    };

    int sub_flags = 0;
    if (!self->_flags.no_inherit_env) { sub_flags |= subprocess_option_inherit_environment; }
    if (self->_flags.no_window) { sub_flags |= subprocess_option_no_window; }
    if (!self->_flags.no_search_path) { sub_flags |= subprocess_option_search_user_path; }
    if (self->_flags.combine_stdouterr) { sub_flags |= subprocess_option_combined_stdout_stderr; }

    int result = subprocess_create((const char* const*)args, sub_flags, &self->_subpr);
    if (result != 0) { return os.get_last_error(); }

    return EOK;
}

/// Checks if process is running
static bool
cex_os__cmd__is_alive(os_cmd_c* self)
{
    return subprocess_alive(&self->_subpr);

    //     int is_alive = self->_subpr.alive;
    //     if (!is_alive) { return 0; }
    //
    // #    if defined(_WIN32)
    //     {
    //         const unsigned long zero = 0x0;
    //         const unsigned long wait_object_0 = 0x00000000L;
    //
    //         is_alive = wait_object_0 != WaitForSingleObject(process->hProcess, zero);
    //     }
    // #    else
    //     {
    //         int status;
    //         is_alive = 0 == waitpid(self->_subpr.child, &status, WNOHANG);
    //     }
    // #    endif
    //
    //     if (!is_alive) { self->_subpr.alive = 0; }
    //
    //     return is_alive;
}

/// Terminates the running process
static Exception
cex_os__cmd__kill(os_cmd_c* self)
{
    if (subprocess_alive(&self->_subpr)) {
        if (subprocess_terminate(&self->_subpr) != 0) { return Error.os; }
    }
    return EOK;
}

/// Waits until array of `procs` is finished. If timeout_sec is 0 waits indefinitely, when
/// timeout occurs `Error.timeout` returned and `procs` untouched. Otherwise all `procs` awaited and
/// cleaned up, `Error.runtime` returned in case of any non-zero return code.
static Exception
cex_os__cmd__wait(os_cmd_c* procs, usize procs_cnt, f64 timeout_sec)
{
    uassert(procs_cnt > 0);
    uassert(procs);
    Exc result = EOK;


    f64 timer = 0.0;
    if (timeout_sec) { timer = cex_os_timer(); }

    while (true) {
        bool is_all_done = true;

        for$eachp (it, procs, procs_cnt) { is_all_done &= !cex_os__cmd__is_alive(it); }

        if (!is_all_done) {
            cex_os_sleep(0.01);

            if (timeout_sec > 0) {
                f64 duration = cex_os_timer() - timer;
                if (duration > timeout_sec) {
                    result = Error.timeout;
                    break;
                }
            }
        } else {
            break;
        }
    }

    for$eachp (it, procs, procs_cnt) {
        // We still can have running processes in case of timeouts
        if (!cex_os__cmd__is_alive(it)) {

            if (subprocess_join(&it->_subpr, &it->_ret_code)) {
                result = result == EOK ? Error.os : result;
            } else if (it->_ret_code != 0) {
                result = result == EOK ? Error.runtime : result;
            }

            subprocess_destroy(&it->_subpr);
        }
    }

    return result;
}

/// Get return code of the finished command result, active process always return -1.
static i32
cex_os__cmd__ret_code(os_cmd_c* self)
{
    if (self->_subpr.alive) { return -1; }

    return self->_ret_code;
}

/// Get running command stdout stream
static FILE*
cex_os__cmd__fstdout(os_cmd_c* self)
{
    return self->_subpr.stdout_file;
}

/// Get running command stderr stream
static FILE*
cex_os__cmd__fstderr(os_cmd_c* self)
{
    return self->_subpr.stderr_file;
}

/// Get running command stdin stream
static FILE*
cex_os__cmd__fstdin(os_cmd_c* self)
{
    return self->_subpr.stdin_file;
}

/// Read all output from process stdout, NULL if stdout is not available
static char*
cex_os__cmd__read_all(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_all(self->_subpr.stdout_file, &out, allc)) { return NULL; }
        return out.buf;
    }
    return NULL;
}

/// Read line from process stdout, NULL if stdout is not available
static char*
cex_os__cmd__read_line(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_line(self->_subpr.stdout_file, &out, allc)) { return NULL; }
        return out.buf;
    }
    return NULL;
}

/// Writes line to the process stdin
static Exception
cex_os__cmd__write_line(os_cmd_c* self, char* line)
{
    uassert(self != NULL);
    if (line == NULL) { return Error.argument; }

    if (self->_subpr.stdin_file == NULL) { return Error.not_found; }

    e$except_silent (err, io.file.writeln(self->_subpr.stdin_file, line)) { return err; }
    fflush(self->_subpr.stdin_file);

    return EOK;
}

/// Check if `cmd_exe` program name exists in PATH. cmd_exe can be absolute, or simple command name,
/// e.g. `cat`
static bool
cex_os__cmd__exists(char* cmd_exe)
{
    if (cmd_exe == NULL || cmd_exe[0] == '\0') { return false; }
    mem$scope(tmem$, _)
    {
#    ifdef _WIN32
        char* extensions[] = { ".exe", ".cmd", ".bat" };
        bool has_ext = str.find(os.path.basename(cmd_exe, _), ".") != NULL;

        if (str.find(cmd_exe, "/") != NULL || str.find(cmd_exe, "\\") != NULL) {
            if (has_ext) {
                return os.path.exists(cmd_exe);
            } else {
                for$each (ext, extensions) {
                    char* exe = str.fmt(_, "%s%s", cmd_exe, ext);
                    os_fs_stat_s stat = os.fs.stat(exe);
                    if (stat.is_valid && stat.is_file) { return true; }
                }
                return false;
            }
        }

        str_s path_env = str.sstr(os.env.get("PATH", NULL));
        if (path_env.buf == NULL) { return false; }

        for$iter (str_s, it, str.slice.iter_split(path_env, ";", &it.iterator)) {
            if (has_ext) {
                char* exe = str.fmt(_, "%S/%s", it.val, cmd_exe);
                os_fs_stat_s stat = os.fs.stat(exe);
                if (stat.is_valid && stat.is_file) { return true; }
            } else {
                for$each (ext, extensions) {
                    char* exe = str.fmt(_, "%S/%s%s", it.val, cmd_exe, ext);
                    os_fs_stat_s stat = os.fs.stat(exe);
                    if (stat.is_valid && stat.is_file) { return true; }
                }
            }
        }
#    else
        if (str.find(cmd_exe, "/") != NULL) {
            os_fs_stat_s stat = os.fs.stat(cmd_exe);
            if (stat.is_valid && stat.is_file && access(cmd_exe, X_OK) == 0) {
                return true; // check if executable
            }
            return false;
        }

        str_s path_env = str.sstr(os.env.get("PATH", NULL));
        if (path_env.buf == NULL) { return false; }

        for$iter (str_s, it, str.slice.iter_split(path_env, ":", &it.iterator)) {
            char* exe = str.fmt(_, "%S/%s", it.val, cmd_exe);
            os_fs_stat_s stat = os.fs.stat(exe);
            if (stat.is_valid && stat.is_file && access(exe, X_OK) == 0) { return true; }
        }
#    endif
    }
    return false;
}

/// Run command using arguments array and resulting os_cmd_c
static Exception
cex_os__cmd__run(char** args, usize args_len, os_cmd_c* out_cmd)
{
    uassert(out_cmd != NULL);
    memset(out_cmd, 0, sizeof(os_cmd_c));

    if (args == NULL || args_len == 0) {
        return e$raise(Error.argument, "`args` argument is empty or null");
    }
    if (args_len == 1 || args[args_len - 1] != NULL) {
        return e$raise(Error.argument, "`args` last item must be a NULL");
    }

    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL || args[i][0] == '\0') {
            return e$raise(
                Error.argument,
                "`args` item[%d] is NULL/empty, which may indicate string operation failure",
                i
            );
        }
    }


#    ifdef _WIN32
    Exc result = Error.runtime;

    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    mem$scope(tmem$, _)
    {
        sbuf_c cmd = sbuf.create(1024, _);
        for (u32 i = 0; i < args_len - 1; i++) {
            if (str.find(args[i], " ") || str.find(args[i], "\"")) {
                char* escaped_arg = str.replace(args[i], "\"", "\\\"", _);
                e$except_silent (err, sbuf.appendf(&cmd, "\"%s\" ", escaped_arg)) {
                    result = err;
                    goto end;
                }
            } else {
                e$except_silent (err, sbuf.appendf(&cmd, "%s ", args[i])) {
                    result = err;
                    goto end;
                }
            }
        }

        if (!CreateProcessA(
                NULL, // Application name (use command line)
                cmd,  // Command line
                NULL, // Process security attributes
                NULL, // Thread security attributes
                TRUE, // Inherit handles
                0,    // Creation flags
                NULL, // Environment
                NULL, // Current directory
                &si,  // Startup info
                &pi   // Process information
            )) {
            result = os.get_last_error();
            goto end;
        }
    }

    *out_cmd = (os_cmd_c){ ._is_subprocess = false,
                           ._ret_code = -1,
                           ._subpr = {
                               .hProcess = pi.hProcess,
                               .alive = 1,
                           } };
    CloseHandle(pi.hThread);
    result = EOK;
end:
    return result;
#    else
    pid_t cpid = fork();
    if (cpid < 0) { return e$raise(Error.os, "Could not fork child process: %s", strerror(errno)); }

    if (cpid == 0) {
        if (execvp(args[0], (char* const*)args) < 0) {
            log$error("Could not exec child process: %s\n", strerror(errno));
            exit(1);
        }
        uassert(false && "unreachable");
    }

    *out_cmd = (os_cmd_c){ ._is_subprocess = false,
                           ._ret_code = -1,
                           ._subpr = {
                               .child = cpid,
                               .alive = 1,
                           } };
    return EOK;

#    endif
}

/// Returns current OS platform, returns enum of OSPlatform__*, e.g. OSPlatform__win,
/// OSPlatform__linux, OSPlatform__macos, etc..
static OSPlatform_e
cex_os__platform__current(void)
{
#    if defined(_WIN32)
    return OSPlatform__win;
#    elif defined(__linux__)
    return OSPlatform__linux;
#    elif defined(__APPLE__) || defined(__MACH__)
    return OSPlatform__macos;
#    elif defined(__unix__)
#        if defined(__FreeBSD__)
    return OSPlatform__freebsd;
#        elif defined(__NetBSD__)
    return OSPlatform__netbsd;
#        elif defined(__OpenBSD__)
    return OSPlatform__openbsd;
#        elif defined(__ANDROID__)
    return OSPlatform__android;
#        elif defined(__EMSCRIPTEN__)
    return OSPlatform__wasm;
#        else
#            error "Untested platform. Need more?"
#        endif
#    elif defined(__wasm__)
    return OSPlatform__wasm;
#    elif defined(__EMSCRIPTEN__)
    return OSPlatform__wasm;
#    else
#        error "Untested platform. Need more?"
#    endif
}

/// Returns string name of current platform
static char*
cex_os__platform__current_str(void)
{
    return os.platform.to_str(os.platform.current());
}

/// Converts platform name to enum
static OSPlatform_e
cex_os__platform__from_str(char* name)
{
    if (name == NULL || name[0] == '\0') { return OSPlatform__unknown; }
    for (u32 i = 1; i < OSPlatform__count; i++) {
        if (str.eq((char*)OSPlatform_str[i], name)) { return (OSPlatform_e)i; }
    }
    return OSPlatform__unknown;
    ;
}

/// Converts platform enum to name
static char*
cex_os__platform__to_str(OSPlatform_e platform)
{
    if (unlikely(platform <= OSPlatform__unknown || platform >= OSPlatform__count)) { return NULL; }
    return (char*)OSPlatform_str[platform];
}

/// Returns OSArch from string
static OSArch_e
cex_os__platform__arch_from_str(char* name)
{
    if (name == NULL || name[0] == '\0') { return OSArch__unknown; }
    for (u32 i = 1; i < OSArch__count; i++) {
        if (str.eq((char*)OSArch_str[i], name)) { return (OSArch_e)i; }
    }
    return OSArch__unknown;
    ;
}

/// Converts arch to string
static char*
cex_os__platform__arch_to_str(OSArch_e platform)
{
    if (unlikely(platform <= OSArch__unknown || platform >= OSArch__count)) { return NULL; }
    return (char*)OSArch_str[platform];
}

void
_cex_os_time_scope_cleanup(f64* timer)
{
    uassert(timer);
    f64 t = *timer;

    char* format = NULL;

    if (t < 0) {
        // Good, os$time_scope normally will make negative value
        format = "os$time_scope() took: %0.3f%s\n";
        t *= -1;
    } else {
        // Likely to happen when we goto label, break or return from os$time_scope()
        t = os.timer() - t;
        format = "os$time_scope() exited early: %0.3f%s\n";
    }

    f64 factor = 1.0;
    char* duration = "sec";
    if (t < 1) {
        if (t < 10e-4) {
            if (t < 10e-7) {
                duration = "ns ";
                factor = 10e8;
            } else {
                duration = "us";
                factor = 10e5;
            }
        } else {
            duration = "ms ";
            factor = 10e2;
        }
    }

    io.printf(format, t * factor, duration);
}

CEX_NAMESPACE_DEF struct __cex_namespace__os os = {
    // Autogenerated by CEX
    // clang-format off

    .cpu_count = cex_os_cpu_count,
    .exit = cex_os_exit,
    .get_last_error = cex_os_get_last_error,
    .getpid = cex_os_getpid,
    .hash = cex_os_hash,
    .sleep = cex_os_sleep,
    .timer = cex_os_timer,

    .cmd = {
        .create = cex_os__cmd__create,
        .exists = cex_os__cmd__exists,
        .fstderr = cex_os__cmd__fstderr,
        .fstdin = cex_os__cmd__fstdin,
        .fstdout = cex_os__cmd__fstdout,
        .is_alive = cex_os__cmd__is_alive,
        .kill = cex_os__cmd__kill,
        .read_all = cex_os__cmd__read_all,
        .read_line = cex_os__cmd__read_line,
        .ret_code = cex_os__cmd__ret_code,
        .run = cex_os__cmd__run,
        .wait = cex_os__cmd__wait,
        .write_line = cex_os__cmd__write_line,
    },

    .env = {
        .executable_path = cex_os__env__executable_path,
        .get = cex_os__env__get,
        .set = cex_os__env__set,
        .unset = cex_os__env__unset,
    },

    .fs = {
        .chdir = cex_os__fs__chdir,
        .copy = cex_os__fs__copy,
        .copy_tree = cex_os__fs__copy_tree,
        .dir_walk = cex_os__fs__dir_walk,
        .find = cex_os__fs__find,
        .getcwd = cex_os__fs__getcwd,
        .mkdir = cex_os__fs__mkdir,
        .mkpath = cex_os__fs__mkpath,
        .remove = cex_os__fs__remove,
        .remove_tree = cex_os__fs__remove_tree,
        .rename = cex_os__fs__rename,
        .stat = cex_os__fs__stat,
    },

    .path = {
        .absolute = cex_os__path__absolute,
        .basename = cex_os__path__basename,
        .dirname = cex_os__path__dirname,
        .exists = cex_os__path__exists,
        .join = cex_os__path__join,
        .normalize = cex_os__path__normalize,
        .split = cex_os__path__split,
    },

    .platform = {
        .arch_from_str = cex_os__platform__arch_from_str,
        .arch_to_str = cex_os__platform__arch_to_str,
        .current = cex_os__platform__current,
        .current_str = cex_os__platform__current_str,
        .from_str = cex_os__platform__from_str,
        .to_str = cex_os__platform__to_str,
    },

    // clang-format on
};
#endif
