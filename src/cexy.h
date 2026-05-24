#pragma once
#if !defined(cex$enable_minimal)
#include "all.h"

#if defined(CEX_BUILD) || defined(CEX_NEW)

#    ifndef cexy$cex_self_cc
#        if defined(__clang__)
/// Macro constant derived from the compiler type used to initially build ./cex app
#            define cexy$cex_self_cc "clang"
#        elif defined(__GNUC__)
#            define cexy$cex_self_cc "gcc"
#        else
#            error "Compiler type is not supported"
#        endif
#    endif // #ifndef cexy$cex_self_cc

#    ifndef cexy$cc
/// Default compiler for building tests/apps (by default inferred from ./cex tool compiler)
#        define cexy$cc cexy$cex_self_cc
#    endif // #ifndef cexy$cc


#    ifndef cexy$cc_include
/// Include path for the #include "some.h" (may be overridden by user)
#        define cexy$cc_include "-I."
#    endif

#    ifndef cexy$build_dir
/// Build dir for project executables and tests (may be overridden by user)
#        define cexy$build_dir "./build"
#    endif

#    ifndef cexy$src_dir
/// Directory for applications and code (may be overridden by user)
#        define cexy$src_dir "./src"
#    endif

#    ifndef cexy$create_compile_flags
/// If 1 creates `compile_flags.txt` in project dir at every ./cex run, 0 - ignores creation (default: 1)
#       define cexy$create_compile_flags 1
#    endif

#    ifndef cexy$cc_args_sanitizer
#        if defined(_WIN32) || defined(MACOS_X) || defined(__APPLE__)
#            if defined(__clang__)
/// Debug mode and tests sanitizer flags (may be overridden by user)
#                define cexy$cc_args_sanitizer                                                     \
                    "-fsanitize-address-use-after-scope", "-fsanitize=address",                    \
                        "-fsanitize=undefined", "-fstack-protector-strong"
#            else
#                define cexy$cc_args_sanitizer "-fstack-protector-strong"
#            endif
#        else
#            define cexy$cc_args_sanitizer                                                         \
                "-fsanitize-address-use-after-scope", "-fsanitize=address",                        \
                    "-fsanitize=undefined", "-fsanitize=leak", "-fstack-protector-strong"
#        endif
#    endif

#    ifndef cexy$cc_args
/// Common compiler flags (may be overridden by user)
#        define cexy$cc_args "-Wall", "-Wextra", "-Werror", "-g3", cexy$cc_args_sanitizer
#    endif

#    ifndef cexy$cc_args_test
/// Test runner compiler flags (may be overridden by user)
#        define cexy$cc_args_test cexy$cc_args, "-Wno-unused-function", "-Itests/"
#    endif

#    ifndef cexy$fuzzer
/// Fuzzer compilation command (supports clang libfuzzer and afl++)
#        define cexy$fuzzer "clang", "-O0", "-Wall", "-Wextra", "-Werror", "-g", "-Wno-unused-function", "-fsanitize=address,fuzzer,undefined", "-fsanitize-undefined-trap-on-error"
#    endif

#    ifndef cexy$cex_self_args
/// Compiler flags used for building ./cex.c -> ./cex (may be overridden by user)
#        define cexy$cex_self_args
#    endif

#    ifndef cexy$pkgconf_cmd
/// Dependency resolver command: pkg-config, pkgconf, etc. May be used in cross-platform
/// compilation, allowed multiple command arguments here
#        define cexy$pkgconf_cmd "pkgconf"
#    endif

/// Helper macro for running cexy.utils.pkgconf() a dependency resolver for libs
#    define cexy$pkgconf(allocator, out_cc_args, pkgconf_args...)                                  \
        ({                                                                                         \
            char* _args[] = { pkgconf_args };                                                      \
            usize _args_len = arr$len(_args);                                                      \
            cexy.utils.pkgconf(allocator, out_cc_args, _args, _args_len);                          \
        })

#    ifndef cexy$pkgconf_libs
/// list of standard system project libs (for example: "lua5.3", "libz")
#        define cexy$pkgconf_libs
#    endif

#    ifndef cexy$vcpkg_root
/// Current vcpkg root path (where ./vcpkg tool is located)
#        define cexy$vcpkg_root NULL
#    endif

#    ifndef cexy$vcpkg_triplet
/// Current build triplet (empty, NULL, or string like "x64-linux")
///   if you are using  `vcpkg install mydep`, ignored if blank or NULL, 
///   list of all supported triplets is here: `vcpkg help triplet`)
#        define cexy$vcpkg_triplet
#    endif

#    ifndef cexy$ld_args
/// Linker flags (e.g. -L./lib/path/ -lmylib -lm) (may be overridden)
#        define cexy$ld_args
#    endif

#    ifndef cexy$debug_cmd
/// Command for launching debugger for cex test/app debug (may be overridden)
#        define cexy$debug_cmd "gdb", "-q", "--args"
#    endif

#    ifndef cexy$build_ext_exe
#        ifdef _WIN32
/// Extension for executables (e.g. '.exe' for win32)
#            define cexy$build_ext_exe ".exe"
#        else
#            define cexy$build_ext_exe ""
#        endif
#    endif

#    ifndef cexy$build_ext_lib_dyn
#        ifdef _WIN32
/// Extension for dynamic linked libs (".dll" win, ".so" linux)
#            define cexy$build_ext_lib_dyn ".dll"
#        else
#            define cexy$build_ext_lib_dyn ".so"
#        endif
#    endif

#    ifndef cexy$build_ext_lib_stat
#        ifdef _WIN32
/// Extension for static libs (".lib" win, ".a" linux)
#            define cexy$build_ext_lib_stat ".lib"
#        else
#            define cexy$build_ext_lib_stat ".a"
#        endif
#    endif

#    ifndef cexy$process_ignore_kw
/**
@brief Pattern for ignoring extra macro keywords in function signatures (for cex process).
See `cex help str.match` for more information about patter syntax.
*/
#        define cexy$process_ignore_kw ""
#    endif


#    define _cexy$cmd_process                                                                      \
        { .name = "process",                                                                       \
          .func = cexy.cmd.process,                                                                \
          .help = "Create CEX namespaces from project source code" }

#    define _cexy$cmd_new { .name = "new", .func = cexy.cmd.new, .help = "Create new CEX project" }

#    define _cexy$cmd_help                                                                         \
        { .name = "help",                                                                          \
          .func = cexy.cmd.help,                                                                   \
          .help = "Search cex.h and project symbols and extract help" }

#    define _cexy$cmd_config                                                                       \
        { .name = "config",                                                                        \
          .func = cexy.cmd.config,                                                                 \
          .help = "Check project and system environment and config" }

#    define _cexy$cmd_libfetch                                                                     \
        { .name = "libfetch",                                                                      \
          .func = cexy.cmd.libfetch,                                                               \
          .help = "Get 3rd party source code via git or install CEX libs" }

#    define _cexy$cmd_stats                                                                        \
        { .name = "stats",                                                                         \
          .func = cexy.cmd.stats,                                                                  \
          .help = "Calculate project lines of code and quality stats" }

/// Simple test runner command (test runner, debugger launch, etc)
#    define cexy$cmd_test                                                                          \
        { .name = "test",                                                                          \
          .func = cexy.cmd.simple_test,                                                            \
          .help = "Generic unit test build/run/debug" }

/// Simple fuzz tests runner command
#    define cexy$cmd_fuzz                                                                          \
        { .name = "fuzz",                                                                          \
          .func = cexy.cmd.simple_fuzz,                                                            \
          .help = "Generic fuzz tester" }


/// Simple app build command (unity build, simple linking, runner, debugger launch, etc)
#    define cexy$cmd_app                                                                           \
        { .name = "app", .func = cexy.cmd.simple_app, .help = "Generic app build/run/debug" }

/// All built-in commands for ./cex tool
#    define cexy$cmd_all                                                                           \
        _cexy$cmd_help, _cexy$cmd_process, _cexy$cmd_new, _cexy$cmd_stats, _cexy$cmd_config,       \
            _cexy$cmd_libfetch

/// Initialize CEX build system (build itself)
#    define cexy$initialize() cexy.build_self(argc, argv, __FILE__)
/// ./cex --help description
#    define cexy$description "\nCEX language (cexy$) build and project management system"
/// ./cex --help usage
#    define cexy$usage " [-D] [-D<ARG1>] [-D<ARG2>] command [options] [args]"

// clang-format off
/// ./cex --help epilog
    #define cexy$epilog \
        "\nYou may try to get help for commands as well, try `cex process --help`\n"\
        "Use `cex -DFOO -DBAR config` to set project config flags\n"\
        "Use `cex -D config` to reset all project config flags to defaults\n"
// clang-format on

// clang-format off
#define _cexy$cmd_test_help (\
        "CEX built-in simple test runner\n"\
        "\nEach cexy test is self-sufficient and unity build, which allows you to test\n"\
        "static funcions, apply mocks to some selected modules and functions, have more\n"\
        "control over your code. See `cex config --help` for customization/config info.\n"\
\
        "\nCEX test runner keep checking include modified time to track changes in the\n"\
        "source files. It expects that each #include \"myfile.c\" has \"myfile.h\" in \n" \
        "the same folder. Test runner uses cexy$cc_include for searching.\n"\
\
        "\nCEX is a test-centric language, it enables additional sanity checks then in\n" \
        "test suite, all warnings are enabled -Wall -Wextra. Sanitizers are enabled by\n"\
        "default.\n"\
\
        "\nCode requirements:\n"\
        "1. You should include all your source files directly using #include \"path/src.c\"\n"\
        "2. If needed provide linker options via cexy$ld_libs / cexy$ld_args\n"\
        "3. If needed provide compiler options via cexy$cc_args_test\n"\
        "4. All tests have to be in tests/ folder, and start with `test_` prefix \n"\
        "5. Only #include with \"\" checked for modification \n"\
\
        "\nTest suite setup/teardown:\n"\
        "// setup before every case  \n"\
        "test$setup_case() {return EOK;} \n"\
        "// teardown after every case \ntest$setup_case() {return EOK;} \n"\
        "// setup before suite (only once) \ntest$setup_suite() {return EOK;} \n"\
        "// teardown after suite (only once) \ntest$setup_suite() {return EOK;} \n"\
\
        "\nTest case:\n"\
        "\ntest$case(my_test_case_name) {\n"\
        "    // run `cex help tassert_` / `cex help tassert_eq` to get more info \n" \
        "    tassert(0 == 1);\n"\
        "    tassertf(0 == 1, \"this is a failure msg: %d\", 3);\n"\
        "    tassert_eq(buf, \"foo\");\n"\
        "    tassert_eq(1, true);\n"\
        "    tassert_eq(str.sstr(\"bar\"), str$s(\"bar\"));\n"\
        "    tassert_ne(1, 0);\n"\
        "    tassert_le(0, 1);\n"\
        "    tassert_lt(0, 1);\n"\
        "    return EOK;\n"\
        "}\n"\
        "\nBenchmarking case:\n"\
        "\ntest$bench(my_bench_case_name) {\n"\
        "    // This code only execudes when `./cex test bench <filename>` is executed\n" \
        "    some_function_of_interest(0, 1);\n"\
        "    return EOK;\n"\
        "}\n"\
        \
        "\nIf you need more control you can build your own test runner. Just use cex help\n"\
        "and get source code `./cex help --source cexy.cmd.simple_test`\n")
        
#define _cexy$cmd_test_epilog \
        "\nTest running examples: \n"\
        "cex test create tests/test_file.c        - creates new test file from template\n"\
        "cex test build all                       - build all tests\n"\
        "cex test run all                         - build and run all tests\n"\
        "cex test run tests/test_file.c           - run test by path\n"\
        "cex test debug tests/test_file.c         - run test via `cexy$debug_cmd` program\n"\
        "cex test clean all                       - delete all test executables in `cexy$build_dir`\n"\
        "cex test clean test/test_file.c          - delete specific test executable\n"\
        "cex test run tests/test_file.c [--help]  - run test with passing arguments to the test runner program\n" \
        "cex test bench test/test_file.c          - run all test$bench() functions for timing\n"


// clang-format on
struct __cex_namespace__cexy {
    // Autogenerated by CEX
    // clang-format off

    void            (*build_self)(int argc, char** argv, char* cex_source);
    bool            (*src_changed)(char* target_path, char** src_array, usize src_array_len);
    bool            (*src_include_changed)(char* target_path, char* src_path, arr$(char*) alt_include_path);
    char*           (*target_make)(char* src_path, char* build_dir, char* name_or_extension, IAllocator allocator);

    struct {
        Exception       (*clean)(char* target);
        Exception       (*create)(char* target);
        Exception       (*find_app_target_src)(IAllocator allc, char* target, char** out_result);
        Exception       (*run)(char* target, bool is_debug, int argc, char** argv);
    } app;

    struct {
        Exception       (*config)(int argc, char** argv, void* user_ctx);
        Exception       (*help)(int argc, char** argv, void* user_ctx);
        Exception       (*libfetch)(int argc, char** argv, void* user_ctx);
        Exception       (*new)(int argc, char** argv, void* user_ctx);
        Exception       (*process)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_app)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_fuzz)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_test)(int argc, char** argv, void* user_ctx);
        Exception       (*stats)(int argc, char** argv, void* user_ctx);
    } cmd;

    struct {
        Exception       (*create)(char* target);
    } fuzz;

    struct {
        Exception       (*clean)(char* target);
        Exception       (*create)(char* target, bool include_sample);
        Exception       (*make_target_pattern)(char** target);
        Exception       (*run)(char* target, char* cmd, int argc, char** argv);
    } test;

    struct {
        char*           (*git_hash)(IAllocator allc);
        Exception       (*git_lib_fetch)(char* git_url, char* git_label, char* out_dir, bool update_existing, bool preserve_dirs, char** repo_paths, usize repo_paths_len);
        Exception       (*make_compile_flags)(char* flags_file, bool include_cexy_flags, arr$(char*) cc_flags_or_null);
        Exception       (*make_new_project)(char* proj_dir);
        Exception       (*pkgconf)(IAllocator allc, arr$(char*)* out_cc_args, char** pkgconf_args, usize pkgconf_args_len);
    } utils;

    // clang-format on
};
#endif // #if defined(CEX_BUILD)
CEX_NAMESPACE struct __cex_namespace__cexy cexy;

#endif
