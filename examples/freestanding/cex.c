#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"


Exception cmd_build(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    if (os.platform.current() != OSPlatform__linux) {
        io.printf("Only linux platform supported for this program");
        return 0;
    }
    cexy$initialize(); // cex self rebuild and init
    argparse_c args = {
        .description = cexy$description,
        .epilog = cexy$epilog,
        .usage = cexy$usage,
        argparse$cmd_list(
            { .name = "build", .func = cmd_build, .help = "Build standalone CEX program" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    void* my_user_ctx = NULL; // passed as `user_ctx` to command
    if (argparse.run_command(&args, my_user_ctx)) { return 1; }

    return 0;
}

/// Custom build command for building freestanding app on linux
Exception
cmd_build(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    if (argc != 2) {
        log$error("Usage: ./cex build freestanding_minimal\n");
        return "Missing arg";
    }


    e$ret(os.fs.mkdir(cexy$build_dir));

    mem$scope(tmem$, _)
    {
        char* src = argv[1];
        char* output = str.fmt(_, "%s/%s", cexy$build_dir, str.replace(argv[1], "src/", "", _));
        e$assertf(os.path.exists(src), "src not exists: %s", src);

        e$ret(os$cmd(cexy$cc, "-g", "-I.", "-nostdlib", "-static", "-ffreestanding", "-o", output, src));

        io.printf("---------------------------------\n");
        e$ret(os$cmd(output));
    }
    return EOK;
}
