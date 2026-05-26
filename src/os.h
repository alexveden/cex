#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_os)
#    include "all.h"


#    ifdef _WIN32
#        define WIN32_LEAN_AND_MEAN
#        include <direct.h>
#        include <windows.h>
#    else
#        include <dirent.h>
#        include <fcntl.h>
#        include <limits.h>
#        include <sys/stat.h>
#        include <sys/types.h>
#        include <unistd.h>
#    endif

/// Additional flags for os.cmd.create()
typedef struct os_cmd_flags_s
{
    u32 combine_stdouterr : 1; // if 1 - combines output from stderr to stdout
    u32 no_inherit_env : 1;    // if 1 - disables using existing env variable of parent process
    u32 no_search_path : 1;    // if 1 - disables propagating PATH= env to command line
    u32 no_window : 1;         // if 1 - disables creation of new window if OS supported
} os_cmd_flags_s;
static_assert(sizeof(os_cmd_flags_s) == sizeof(u32), "size?");

/// Command container (current state of subprocess)
typedef struct os_cmd_c
{
    struct subprocess_s _subpr;
    os_cmd_flags_s _flags;
    bool _is_subprocess;
    i32 _ret_code;
} os_cmd_c;

/// File stats metadata (cross-platform), returned by os.fs.stats
typedef struct os_fs_stat_s
{
    u64 is_valid : 1;
    u64 is_directory : 1;
    u64 is_symlink : 1;
    u64 is_file : 1;
    u64 is_other : 1;
    u64 size : 48;
    union
    {
        Exc error;
        time_t mtime;
    };
} os_fs_stat_s;
static_assert(sizeof(os_fs_stat_s) <= sizeof(u64) * 2, "size?");

typedef Exception os_fs_dir_walk_f(char* path, os_fs_stat_s ftype, void* user_ctx);

#    define _CexOSPlatformList                                                                     \
        X(linux)                                                                                   \
        X(win)                                                                                     \
        X(macos)                                                                                   \
        X(wasm)                                                                                    \
        X(android)                                                                                 \
        X(freebsd)                                                                                 \
        X(openbsd)

#    define _CexOSArchList                                                                         \
        X(x86_32)                                                                                  \
        X(x86_64)                                                                                  \
        X(arm)                                                                                     \
        X(wasm32)                                                                                  \
        X(wasm64)                                                                                  \
        X(aarch64)                                                                                 \
        X(riscv32)                                                                                 \
        X(riscv64)                                                                                 \
        X(xtensa)

#    define X(name) OSPlatform__##name,
typedef enum OSPlatform_e
{
    OSPlatform__unknown,
    _CexOSPlatformList OSPlatform__count,
} OSPlatform_e;
#    undef X

__attribute__((unused)) static const char* OSPlatform_str[] = {
#    define X(name) #name,
    NULL,
    _CexOSPlatformList
#    undef X
};

typedef enum OSArch_e
{
#    define X(name) OSArch__##name,
    OSArch__unknown,
    _CexOSArchList OSArch__count,
#    undef X
} OSArch_e;

__attribute__((unused)) static const char* OSArch_str[] = {
#    define X(name) cex$stringize(name),
    NULL,
    _CexOSArchList
#    undef X
};

#    ifdef _WIN32
/// OS path separator, generally '\' for Windows, '/' otherwise
#        define os$PATH_SEP '\\'
#    else
#        define os$PATH_SEP '/'
#    endif

#    if defined(CEX_BUILD) && CEX_LOG_LVL > 3
#        define _os$args_print(msg, args, args_len)                                                \
            log$debug(msg "");                                                                     \
            for (u32 i = 0; i < args_len - 1; i++) {                                               \
                char* a = args[i];                                                                 \
                io.printf(" ");                                                                    \
                if (str.find(a, " ")) {                                                            \
                    io.printf("\'%s\'", a);                                                        \
                } else if (a == NULL || *a == '\0') {                                              \
                    io.printf("\'%s\'", a);                                                        \
                } else {                                                                           \
                    io.printf("%s", a);                                                            \
                }                                                                                  \
            }                                                                                      \
            io.printf("\n");                                                                       \
            fflush(stdout);

#    else
#        define _os$args_print(msg, args, args_len)
#    endif

/// Run command by dynamic or static array (returns Exc, but error check is not mandatory). Pipes
/// all IO to stdout/err/in into current terminal, feels totally interactive.
#    define os$cmda(args, args_len...)                                                             \
        ({                                                                                         \
            /* NOLINTBEGIN */                                                                      \
            static_assert(sizeof(args) > 0, "You must pass at least one item");                    \
            usize _args_len_va[] = { args_len };                                                   \
            (void)_args_len_va;                                                                    \
            usize _args_len = (sizeof(_args_len_va) > 0) ? _args_len_va[0] : arr$len(args);        \
            uassert(_args_len < PTRDIFF_MAX && "negative length or overflow");                     \
            _os$args_print("CMD:", args, _args_len);                                               \
            os_cmd_c _cmd = { 0 };                                                                 \
            Exc result = os.cmd.run(args, _args_len, &_cmd);                                       \
            if (result == EOK) { result = os.cmd.wait(&_cmd, 1, 0); };                             \
            result;                                                                                \
            /* NOLINTEND */                                                                        \
        })

/// Run command by arbitrary set of arguments (returns Exc, but error check is not mandatory). Pipes
/// all IO to stdout/err/in into current terminal, feels totally interactive.
/// Example: e$ret(os$cmd("cat", "./cex.c"))
#    define os$cmd(args...)                                                                        \
        ({                                                                                         \
            char* _args[] = { args, NULL };                                                        \
            usize _args_len = arr$len(_args);                                                      \
            os$cmda(_args, _args_len);                                                             \
        })

/// Path parts join by variable set of args: os$path_join(mem$, "foo", "bar", "cex.c")
#    define os$path_join(allocator, path_parts...)                                                 \
        ({                                                                                         \
            char* _args[] = { path_parts };                                                        \
            usize _args_len = arr$len(_args);                                                      \
            os.path.join(_args, _args_len, allocator);                                             \
        })


void _cex_os_time_scope_cleanup(f64* timer);

/// Takes time measurement of code inside the scope, prints timings into stdout via log$debug()
#    define os$time_scope()                                                                        \
        for (f64 cex$tmpname(scope_timer)                                                          \
                 __attribute__((__cleanup__(_cex_os_time_scope_cleanup))) = os.timer(),            \
                 cex$tmpname(scope_cntr) = 0;                                                      \
             cex$tmpname(scope_cntr) < 1;                                                          \
             cex$tmpname(scope_timer) = (os.timer() - cex$tmpname(scope_timer)) * -1,              \
                 cex$tmpname(scope_cntr)++,                                                        \
                 log$debug(""))

/**

Cross-platform OS related operations:

- `os.cmd.` - for running commands and interacting with them
- `os.fs.` - file-system related tasks
- `os.env.` - getting setting environment variable
- `os.path.` - file path operations
- `os.platform.` - information about current platform


Examples:

- Running simple commands
```c
// NOTE: there are many operation with os-related stuff in cexy build system
// try to play in example roulette:  ./cex help --example os.cmd.run

// Easy macro, run fixed number of arguments

e$ret(os$cmd(
    cexy$cc,
    "src/main.c",
    cexy$build_dir "/sqlite3.o",
    cexy$cc_include,
    "-lpthread",
    "-lm",
    "-o",
    cexy$build_dir "/hello_sqlite"
));


```

- Running dynamic arguments
```c
mem$scope(tmem$, _)
{
        arr$(char*) args = arr$new(args, _);
        arr$pushm(
            args,
            cexy$cc,
            "-Wall",
            "-Werror",
        );

        if (os.platform.current() == OSPlatform__win) {
            arr$pushm(args, "-lbcrypt");
        }

        arr$pushm(args, NULL); // NOTE: last element must be NULL
        e$ret(os$cmda(args));
    }
}
```

- Getting command output (low level api)
```c

test$case(os_cmd_create)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        char* args[] = { "./cex", NULL };
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        io.printf("%s\n", output);

        int err_code = 0;
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
        tassert_eq(err_code, 1);
    }
    return EOK;
}
```

- Working with files
```c

test$case(test_os_find_all_c_files)
{
    mem$scope(tmem$, _)
    {
        // Check if exists and remove
        if (os.path.exists("./cex")) { e$ret(os.fs.remove("./cex")); }

        // illustration of path combining
        char* pattern = os$path_join(_, "./", "*.c");

        // find all matching *.c files
        for$each (it, os.fs.find(pattern, _), false , _)) {
            log$debug("found file: %s\n", it);
        }
    }

    return EOK;
}
```


*/
struct __cex_namespace__os {
    // Autogenerated by CEX
    // clang-format off

    /// Get available CPU cores on system, or -1 on error
    i32             (*cpu_count)(void);
    /// Get last system API error as string representation (Exception compatible). Result content may be
    /// affected by OS locale settings.
    Exc             (*get_last_error)(void);
    /// Computes generic buffer SIP hash (platform/endiannes stable), seed can be null, or previous hash
    /// value for hash stacking  (null or empty `p` returns 0 hash). (This is the same general hash
    /// function is used in str.hash() and hm$ hashmaps)
    u64             (*hash)(void* p, usize psize, u64 seed);
    /// Sleep for `period_millisec` duration
    void            (*sleep)(u32 period_millisec);
    /// Get high performance monotonic timer value in seconds, started from the first call of the
    /// os.timer()
    f64             (*timer)(void);

    struct {
        /// Creates new os command (use os$cmd() and os$cmd() for easy cases). flags can be NULL.
        Exception       (*create)(os_cmd_c* self, char** args, usize args_len, os_cmd_flags_s* flags);
        /// Check if `cmd_exe` program name exists in PATH. cmd_exe can be absolute, or simple command name,
        /// e.g. `cat`
        bool            (*exists)(char* cmd_exe);
        /// Get running command stderr stream
        FILE*           (*fstderr)(os_cmd_c* self);
        /// Get running command stdin stream
        FILE*           (*fstdin)(os_cmd_c* self);
        /// Get running command stdout stream
        FILE*           (*fstdout)(os_cmd_c* self);
        /// Checks if process is running
        bool            (*is_alive)(os_cmd_c* self);
        /// Terminates the running process
        Exception       (*kill)(os_cmd_c* self);
        /// Read all output from process stdout, NULL if stdout is not available
        char*           (*read_all)(os_cmd_c* self, IAllocator allc);
        /// Read line from process stdout, NULL if stdout is not available
        char*           (*read_line)(os_cmd_c* self, IAllocator allc);
        /// Get return code of the finished command result, active process always return -1.
        i32             (*ret_code)(os_cmd_c* self);
        /// Run command using arguments array and resulting os_cmd_c
        Exception       (*run)(char** args, usize args_len, os_cmd_c* out_cmd);
        /// Waits until array of `procs` is finished. If timeout_sec is 0 waits indefinitely, when
        /// timeout occurs `Error.timeout` returned and `procs` untouched. Otherwise all `procs` awaited and
        /// cleaned up, `Error.runtime` returned in case of any non-zero return code.
        Exception       (*wait)(os_cmd_c* procs, usize procs_cnt, f64 timeout_sec);
        /// Writes line to the process stdin
        Exception       (*write_line)(os_cmd_c* self, char* line);
    } cmd;

    struct {
        /// Get environment variable, with `deflt` if not found
        char*           (*get)(char* name, char* deflt);
        /// Set environment variable
        Exception       (*set)(char* name, char* value);
    } env;

    struct {
        /// Change current working directory
        Exception       (*chdir)(char* path);
        /// Copy file
        Exception       (*copy)(char* src_path, char* dst_path);
        /// Copy directory recursively
        Exception       (*copy_tree)(char* src_dir, char* dst_dir);
        /// Iterates over directory (can be recursive) using callback function
        Exception       (*dir_walk)(char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx);
        /// Finds files in `dir/pattern`, for example "./mydir/*.c" (all c files), if is_recursive=true, all
        /// *.c files found in sub-directories.
        arr$(char*)     (*find)(char* path_pattern, bool is_recursive, IAllocator allc);
        /// Get current working directory
        char*           (*getcwd)(IAllocator allc);
        /// Makes directory (no error if exists)
        Exception       (*mkdir)(char* path);
        /// Makes all directories in a path
        Exception       (*mkpath)(char* path);
        /// Removes file or empty directory (also see os.fs.remove_tree)
        Exception       (*remove)(char* path);
        /// Removes directory and all its contents recursively
        Exception       (*remove_tree)(char* path);
        /// Renames file or directory
        Exception       (*rename)(char* old_path, char* new_path);
        /// Returns cross-platform path stats information (see os_fs_stat_s)
        os_fs_stat_s    (*stat)(char* path);
    } fs;

    struct {
        /// Returns absolute path from relative
        char*           (*abs)(char* path, IAllocator allc);
        /// Get file name of a path
        char*           (*basename)(char* path, IAllocator allc);
        /// Get directory name of a path
        char*           (*dirname)(char* path, IAllocator allc);
        /// Check if file/directory path exists
        bool            (*exists)(char* file_path);
        /// Join path with OS specific path separator
        char*           (*join)(char** parts, u32 parts_len, IAllocator allc);
        /// Splits path by `dir` and `file` parts, when return_dir=true - returns `dir` part, otherwise
        /// `file` part
        str_s           (*split)(char* path, bool return_dir);
    } path;

    struct {
        /// Returns OSArch from string
        OSArch_e        (*arch_from_str)(char* name);
        /// Converts arch to string
        char*           (*arch_to_str)(OSArch_e platform);
        /// Returns current OS platform, returns enum of OSPlatform__*, e.g. OSPlatform__win,
        /// OSPlatform__linux, OSPlatform__macos, etc..
        OSPlatform_e    (*current)(void);
        /// Returns string name of current platform
        char*           (*current_str)(void);
        /// Converts platform name to enum
        OSPlatform_e    (*from_str)(char* name);
        /// Converts platform enum to name
        char*           (*to_str)(OSPlatform_e platform);
    } platform;

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__os os;

#endif
