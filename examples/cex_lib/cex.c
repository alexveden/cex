/*
 * CEX project CLI for the cex_demo library.
 *
 * `cex.c` is the build entry point. Besides the built-in commands (test, help,
 * ...) it registers two custom commands:
 *
 *   ./cex build-lib   compile lib/cex_demo.c into static + shared libraries
 *   ./cex consumer    build the library, then compile and run consumer/main.c
 *
 * The example is a standalone CEX project: `cex.h` is a symlink to the repo
 * root, and `cex.c` is compiled once with `cc ./cex.c -o ./cex`.
 */

#if !defined(cex$enable_minimal)

#if __has_include("cex_config.h")
// Custom config file
#    include "cex_config.h"
#else
// Tell simple-mode where to find cex.h and project sources during tests.
#    define cexy$cc_include "-I.", "-I./lib"
// Keep info-level logs visible in the example.
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#endif

/*
 * CEX_IMPLEMENTATION pulls the full CEX implementation into the CLI.
 * CEX_BUILD enables the cexy$ build-system API used by the commands below.
 */
#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

Exception cmd_build_lib(int argc, char** argv, void* user_ctx);
Exception cmd_consumer(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    // cexy$initialize() rebuilds ./cex if cex.c/cex.h changed and initializes
    // the global allocators and build config.
    cexy$initialize();

    argparse_c args = {
        .description = cexy$description,
        .epilog = cexy$epilog,
        .usage = cexy$usage,
        argparse$cmd_list(
            cexy$cmd_all,  // help, process, new, stats, config, libfetch
            cexy$cmd_test, // test build/run/debug
            { .name = "build-lib", .func = cmd_build_lib, .help = "Build static + shared library" },
            { .name = "consumer",
              .func = cmd_consumer,
              .help = "Build library and run the C consumer" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }

    void* my_user_ctx = NULL; // passed as `user_ctx` to every command
    e$except (err, argparse.run_command(&args, my_user_ctx)) {
        // Argument errors already printed their own message; everything else
        // gets a CEX traceback so the failure is debuggable.
        if (err != Error.argsparse) { e$traceback_print(stderr); }
        return 1;
    }
    return 0;
}

/*
 * Builds libcex_demo as both a static archive and a shared library.
 *
 * The library is compiled once into a position-independent object, then:
 *   - static: `ar rcs` packs the object into libcex_demo.a / .lib
 *   - shared: the compiler links the same object with the platform's shared
 *     library flag (Linux -shared, macOS -dynamiclib, Windows -shared)
 *
 * Output goes to `build/lib/` so build artifacts stay out of the source tree.
 */
Exception
cmd_build_lib(int argc, char** argv, void* user_ctx)
{
    // This command takes no options; the signature is dictated by argparse.
    (void)argc;
    (void)argv;
    (void)user_ctx;

    char* out_dir = cexy$build_dir "/lib";
    e$ret(os.fs.mkpath(str.fmt(mem$, "%s/", out_dir)));

    /*
     * -fPIC makes the object usable inside a shared library.
     * -fvisibility=hidden hides every symbol except CEX_DEMO_API ones.
     * -I. finds the cex.h symlink; -I./lib finds cex_demo.h.
     */
    char* obj = str.fmt(mem$, "%s/cex_demo.o", out_dir);
    e$ret(os$cmd(
        cexy$cc,
        "-c",
        "lib/cex_demo.c",
        "-o",
        obj,
        "-g3",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-fPIC",
        "-fvisibility=hidden",
        "-I.",
        "-I./lib"
    ));

    // Static archive: extension comes from cexy$build_ext_lib_stat (.a or .lib).
    char* stat_lib = str.fmt(mem$, "%s/libcex_demo%s", out_dir, cexy$build_ext_lib_stat);
    e$ret(os$cmd("ar", "rcs", stat_lib, obj));

    /*
     * Shared library. cexy$build_ext_lib_dyn is .dll on Windows and .so
     * elsewhere, so macOS gets an explicit .dylib name.
     */
#if defined(__APPLE__)
    char* dyn_lib = str.fmt(mem$, "%s/libcex_demo.dylib", out_dir);
#else
    char* dyn_lib = str.fmt(mem$, "%s/libcex_demo%s", out_dir, cexy$build_ext_lib_dyn);
#endif

    // Build the shared-library command as an argv array because the flags
    // differ per platform.
    arr$(char*) args = arr$new(args, mem$);
    arr$pushm(args, cexy$cc);
#if defined(_WIN32)
    arr$pushm(args, "-shared");
#elif defined(__APPLE__)
    arr$pushm(args, "-dynamiclib", "-pthread");
#else
    // -soname records the library name consumers should load at runtime.
    arr$pushm(args, "-shared", "-Wl,-soname,libcex_demo.so", "-pthread");
#endif
    arr$pushm(args, "-fPIC", "-fvisibility=hidden", "-o", dyn_lib, obj);
    arr$push(args, NULL); // os$cmda() expects a NULL-terminated argv
    e$ret(os$cmda(args));
    arr$free(args);

    log$info("built: %s\n", stat_lib);
    log$info("built: %s\n", dyn_lib);
    return EOK;
}

/*
 * Builds the library, then compiles consumer/main.c against the static
 * archive and runs it. This is the same flow an external C project would use:
 * include the public header, link libcex_demo.a, define CEX_DEMO_STATIC so the
 * export macro does not emit dllimport on Windows.
 */
Exception
cmd_consumer(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;

    // Rebuild the library first so the consumer never links a stale archive.
    char* build_argv[] = { "build-lib", NULL };
    e$ret(cmd_build_lib(1, build_argv, NULL));

    char* out_dir = str.fmt(mem$, "%s/consumer", cexy$build_dir);
    e$ret(os.fs.mkpath(str.fmt(mem$, "%s/", out_dir)));

    char* obj = str.fmt(mem$, "%s/main.o", out_dir);
    char* exe = str.fmt(mem$, "%s/main%s", out_dir, cexy$build_ext_exe);
    char* lib = str.fmt(mem$, "%s/lib/libcex_demo%s", cexy$build_dir, cexy$build_ext_lib_stat);

    // Compile the consumer with only the public header on the include path.
    e$ret(os$cmd(
        cexy$cc,
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DCEX_DEMO_STATIC",
        "-Ilib",
        "-c",
        "consumer/main.c",
        "-o",
        obj
    ));
    // Link against the static archive, then run the resulting executable.
    e$ret(os$cmd(cexy$cc, obj, lib, "-o", exe));
    return os$cmd(exe);
}

#endif
