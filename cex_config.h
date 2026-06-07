#pragma once

// These settings can be set via `./cex -D CEX_WINE config` command
// This is for CI (allowing substitute compiler with env var)
#define cexy$cex_self_cc "cc"
#define cexy$src_dir "./examples"

#ifdef CEX_TEST_NOASAN
#    define cexy$cc_args_sanitizer "-fstack-protector-strong"
#endif

#if defined(CEX_DEBUG)
#    define cexy$cex_self_args cexy$cc_args_sanitizer
#    define CEX_LOG_LVL 5 /* 0 (mute all) - 5 (log$trace) */
#elif defined(CEX_RELEASE)
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#    define cexy$cex_self_args "-O2"
#    define cexy$cc_args "-Wall", "-Wextra", "-Werror", "-g", "-O3"
#elif defined(CEX_NDEBUG)
#    define CEX_LOG_LVL 5 /* 0 (mute all) - 5 (log$trace) */
#    define cexy$cc_args_sanitizer
#    define cexy$cc_args "-DNDEBUG", "-Wall", "-Wextra", "-Werror", "-g", "-O2"
#elif defined(CEX_X32)
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#    define cexy$cc_args "-O0", "-Wall", "-Wextra", "-Werror", "-g", "-m32"
#else
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#endif


#ifdef CEX_OLD_GCC
#    define cexy$cc "gcc-14"
#endif

#ifdef CEX_C23
#    define cexy$cc "clang"
#    define cexy$cc_args "-O0", "-Wall", "-Wextra", "-Werror", "-g", "-std=gnu23"
#endif

#ifdef CEX_VALGRIND
#    define cexy$cc_args_sanitizer "-fstack-protector-strong"
#    define cexy$cc_args "-O0", "-Wall", "-Wextra", "-Werror", "-gdwarf-4", "-DCEX_VALGRIND"
#    define cexy$debug_cmd                                                                         \
        "valgrind", "--leak-check=full", "--show-leak-kinds=definite,indirect,possible",           \
            "--error-exitcode=1", "--track-fds=yes", "--track-origins=yes"
#endif

#ifdef CEX_WASM_DEBUG
#    define cexy$build_dir "./build/wasm"

#    define cexy$cc_args                                                                           \
        "-O0", "-Wall", "-Wextra", "-Werror", "-g", "-gsource-map", "-fsanitize=address",          \
            "-sASSERTIONS", "-sINITIAL_MEMORY=1073741824", "--embed-file=tests",                   \
            "--embed-file=src"
#    define cexy$cc "emcc"
#    define cexy$debug_cmd "node", "--stack-trace-limit=500"
#    define cexy$build_ext_exe ".js"
#endif

#ifdef CEX_WASM_RELEASE
#    define cexy$build_dir "./build/wasm"

#    define cexy$cc_args                                                                           \
        "-O3", "-Wall", "-Wextra", "-sINITIAL_MEMORY=1073741824", "--embed-file=tests",            \
         "--embed-file=src", "-sASSERTIONS=2", "-sSAFE_HEAP=2", \
            "-sSTACK_OVERFLOW_CHECK=2", "-sMALLOC=emmalloc-debug"
#    define cexy$cc "emcc"
#    define cexy$debug_cmd "node"
#    define cexy$build_ext_exe ".js"
#endif

#ifdef CEX_WINE
#    define cexy$cc "x86_64-w64-mingw32-gcc"
#    define cexy$cc_args_sanitizer "-g3"
#    define cexy$debug_cmd "wine"
#    define cexy$build_ext_exe ".exe"
#endif

#ifdef CEX_FUZZ_AFL
#    define cexy$fuzzer                                                                            \
        "afl-cc", "-Wall", "-Wextra", "-Werror", "-g", "-Wno-unused-function",                     \
            "-fsanitize=address,undefined", "-fsanitize-undefined-trap-on-error"
#endif

// Strict sanitizer flags for Clang (catches UB that -fsanitize=undefined misses)
// -fsanitize=leak is Linux-only (unsupported on macOS/Windows)
#ifndef cexy$cc_args_sanitizer
#    if defined(_WIN32) || defined(__APPLE__)
#        ifdef __clang__
#            define cexy$cc_args_sanitizer                                                              \
                 "-fsanitize-address-use-after-scope", "-fsanitize=address",                            \
                     "-fsanitize=undefined",                                                            \
                     "-fsanitize=implicit-conversion", "-fsanitize=pointer-overflow",                   \
                     "-fstack-protector-strong"
#        endif
#    else
#        ifdef __clang__
#            define cexy$cc_args_sanitizer                                                              \
                 "-fsanitize-address-use-after-scope", "-fsanitize=address",                            \
                     "-fsanitize=undefined", "-fsanitize=leak",                                        \
                     "-fsanitize=implicit-conversion", "-fsanitize=pointer-overflow",                   \
                     "-fstack-protector-strong"
#        endif
#    endif
#endif
