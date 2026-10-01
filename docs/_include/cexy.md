

### Build system

`cexy$` is CEX's integrated build system (no CMake/Make/Ninja). The build script is C code
in `./cex.c` — it must live in the project root next to `cex.h`. `./cex` is the project CLI
for build/test/fuzz/app/process/help. Simple config-driven mode covers most projects;
low-level compiler tools are exposed for custom builds.

- Configure with `#define cexy$<var>` before `#include "cex.h"` (or in `cex_config.h`)
- `./cex -DSOME config` rebuilds `./cex` with `-DSOME` baked in (gate overrides with `#ifdef SOME`);
  `./cex -D config` resets. The `-D` flags must come before the `config` command.
- Conventions: sources in `cexy$src_dir` (`./src`), app `main()` in `src/<name>.c` or
  `src/<name>/main.c`, tests in `tests/test_<name>.c`; unity build (no objects/link stage)
- Debug builds precompile `cex.h` into a cached `.obj`; disable with `#define cexy$disable_cex_precompiling`
- CLI: `./cex {help,process,new,stats,config,libfetch,test,fuzz,app} [options]`

#### Getting more help

- `./cex help cexy$` — all `cexy$*` config vars + the `cexy` namespace API
- `./cex config` — current `cexy$*` values and toolchain
- `./cex help --source cexy.cmd.simple_app` — source of the built-in app build routine
- `./cex help --source cexy.cmd.simple_test` — source of the built-in test runner
- `./cex help --example cexy.utils.git_lib_fetch` — real usage examples from the codebase
- `./cex test --help` / `./cex app --help` / `./cex fuzz --help` — per-command help
- `./cex help --agents` — dump the CEX namespace idioms (AGENTS.md content)

#### Example: `./cex.c` in the project root

```c
// file: ./cex.c  (project root, next to cex.h)
#if __has_include("cex_config.h")
#    include "cex_config.h"                  // persisted config, takes priority
#else
#    define cexy$cc_include "-I.", "-I./lib" // redefine any cexy$ setting
#    define CEX_LOG_LVL 4
#endif

#define CEX_IMPLEMENTATION
#define CEX_BUILD

Exception cmd_mybuild(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    cexy$initialize(); // rebuild ./cex when cex.h/cex.c change
    argparse_c args = {
        .description = cexy$description,
        .epilog = cexy$epilog,
        .usage = cexy$usage,
        argparse$cmd_list(
            cexy$cmd_all,
            cexy$cmd_test, // built-in test runner
            cexy$cmd_app,  // built-in app runner
            { .name = "my-build", .func = cmd_mybuild, .help = "Custom build command" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    e$except (err, argparse.run_command(&args, NULL)) {
        if (err != Error.argsparse) { e$traceback_print(stderr); }
        return 1;
    }
    return 0;
}

/// Custom command: run with `./cex my-build`
Exception
cmd_mybuild(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    mem$scope(tmem$, _)
    {
        char* target = cexy.target_make("src/myapp.c", cexy$build_dir, "myapp", _);
        e$ret(os$cmd(cexy$cc, "-o", target, "src/myapp.c"));
    }
    return EOK;
}
```



```c
/// Build dir for project executables and tests (may be overridden by user)
#define cexy$build_dir

/// Extension for executables (e.g. '.exe' for win32)
#define cexy$build_ext_exe

/// Extension for dynamic linked libs (".dll" win, ".so" linux)
#define cexy$build_ext_lib_dyn

/// Extension for static libs (".lib" win, ".a" linux)
#define cexy$build_ext_lib_stat

/// Default compiler for building tests/apps (by default inferred from ./cex tool compiler)
#define cexy$cc

/// Common compiler flags (may be overridden by user)
#define cexy$cc_args

/// Debug mode and tests sanitizer flags (may be overridden by user)
#define cexy$cc_args_sanitizer

/// Test runner compiler flags (may be overridden by user)
#define cexy$cc_args_test

/// Include path for the #include "some.h" (may be overridden by user)
#define cexy$cc_include

/// Compiler flags used for building ./cex.c -> ./cex (may be overridden by user)
#define cexy$cex_self_args

/// Macro constant derived from the compiler type used to initially build ./cex app
#define cexy$cex_self_cc

/// All built-in commands for ./cex tool
#define cexy$cmd_all

/// Simple app build command (unity build, simple linking, runner, debugger launch, etc)
#define cexy$cmd_app

/// Simple fuzz tests runner command
#define cexy$cmd_fuzz

/// Simple test runner command (test runner, debugger launch, etc)
#define cexy$cmd_test

/// If 1 creates `compile_flags.txt` in project dir at every ./cex run, 0 - ignores creation (default: 1)
#define cexy$create_compile_flags

/// Command for launching debugger for cex test/app debug (may be overridden)
#define cexy$debug_cmd

/// ./cex --help description
#define cexy$description

/// ./cex --help epilog
#define cexy$epilog

/// Fuzzer compilation command (supports clang libfuzzer and afl++)
#define cexy$fuzzer

/// Initialize CEX build system (build itself)
#define cexy$initialize()

/// Linker flags (e.g. -L./lib/path/ -lmylib -lm) (may be overridden)
#define cexy$ld_args

/// Helper macro for running cexy.utils.pkgconf() a dependency resolver for libs
#define cexy$pkgconf(allocator, out_cc_args, pkgconf_args...)

/// Dependency resolver command: pkg-config, pkgconf, etc. May be used in cross-platform
/// compilation, allowed multiple command arguments here
#define cexy$pkgconf_cmd

/// list of standard system project libs (for example: "lua5.3", "libz")
#define cexy$pkgconf_libs

/// Pattern for ignoring extra macro keywords in function signatures (for cex process).
#define cexy$process_ignore_kw

/// Directory for applications and code (may be overridden by user)
#define cexy$src_dir

/// ./cex --help usage
#define cexy$usage

/// Current vcpkg root path (where ./vcpkg tool is located)
#define cexy$vcpkg_root

/// Current build triplet (empty, NULL, or string like "x64-linux")
///   if you are using  `vcpkg install mydep`, ignored if blank or NULL, 
///   list of all supported triplets is here: `vcpkg help triplet`)
#define cexy$vcpkg_triplet



cexy {
    // Autogenerated by CEX
    // clang-format off

    /// Rebuilds ./cex from cex.c when cex.h or cex.c changed
    void            (*build_self)(int argc, char** argv, char* cex_source);
    /// True if any source in src_array is newer than target_path
    bool            (*src_changed)(char* target_path, char** src_array, usize src_array_len);
    /// True if src_path or one of its #includes is newer than target_path
    bool            (*src_include_changed)(char* target_path, char* src_path, arr$(char*) alt_include_path);
    /// Builds a build-dir output path from a source path and name/extension
    char*           (*target_make)(char* src_path, char* build_dir, char* name_or_extension, IAllocator allocator);

    struct {
        /// Removes a built app executable
        Exception       (*clean)(char* target);
        /// Scaffolds a new app source file
        Exception       (*create)(char* target);
        /// Finds the source path for an app target
        Exception       (*find_app_target_src)(IAllocator allc, char* target, char** out_result);
        /// Builds and runs an app (optionally under the debugger)
        Exception       (*run)(char* target, bool is_debug, int argc, char** argv);
    } app;

    struct {
        /// CLI: show project and system environment
        Exception       (*config)(int argc, char** argv, void* user_ctx);
        /// CLI: symbol/doc search over the project
        Exception       (*help)(int argc, char** argv, void* user_ctx);
        /// CLI: fetch 3rd-party libraries via git
        Exception       (*libfetch)(int argc, char** argv, void* user_ctx);
        /// CLI: scaffold a new boilerplate CEX project
        Exception       (*new)(int argc, char** argv, void* user_ctx);
        /// CLI: generate CEX namespaces from project sources
        Exception       (*process)(int argc, char** argv, void* user_ctx);
        /// CLI: simple app runner (build/run/debug/create/clean)
        Exception       (*simple_app)(int argc, char** argv, void* user_ctx);
        /// CLI: compile and run fuzz tests
        Exception       (*simple_fuzz)(int argc, char** argv, void* user_ctx);
        /// CLI: simple test runner (build/run/debug/bench/watch)
        Exception       (*simple_test)(int argc, char** argv, void* user_ctx);
        /// CLI: parse project and report code metrics
        Exception       (*stats)(int argc, char** argv, void* user_ctx);
    } cmd;

    struct {
        /// Scaffolds a new fuzz_ file
        Exception       (*create)(char* target);
    } fuzz;

    struct {
        /// Removes built test executable(s)
        Exception       (*clean)(char* target);
        /// Scaffolds a new test file (optionally with a sample case)
        Exception       (*create)(char* target, bool include_sample);
        /// Normalizes a test target (all or a path) into a build glob
        Exception       (*make_target_pattern)(char** target);
        /// Builds and runs/debugs/benches/watches a test target
        Exception       (*run)(char* target, char* cmd, int argc, char** argv);
    } test;

    struct {
        /// Returns the current git commit hash (or NULL)
        char*           (*git_hash)(IAllocator allc);
        /// Clones/updates a git dependency into out_dir
        Exception       (*git_lib_fetch)(char* git_url, char* git_label, char* out_dir, bool update_existing, bool preserve_dirs, char** repo_paths, usize repo_paths_len);
        /// Writes compile_flags.txt from the project compiler flags
        Exception       (*make_compile_flags)(char* flags_file, bool include_cexy_flags, arr$(char*) cc_flags_or_null);
        /// Creates a new project skeleton in proj_dir
        Exception       (*make_new_project)(char* proj_dir);
        /// Resolves pkg-config --cflags/--libs into out_cc_args
        Exception       (*pkgconf)(IAllocator allc, arr$(char*)* out_cc_args, char** pkgconf_args, usize pkgconf_args_len);
    } utils;

    // clang-format on
};

```
