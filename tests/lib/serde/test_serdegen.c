#define CEX_IMPLEMENTATION
#define CEX_BUILD
#define CEX_TEST
#include "cex.h"
#include "lib/json/CexSerdeGen.c"


#define TESTDIR "tests/lib/serde/"

test$setup_case()
{
    return EOK;
}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(serdegen_myserde_basic)
{
    if (os.path.exists(TESTDIR "basic/serdegen.h")) {
        if (os.fs.remove(TESTDIR "basic/serdegen.h")) {};
    }
    if (os.path.exists(TESTDIR "basic/serdegen.c")) {
        if (os.fs.remove(TESTDIR "basic/serdegen.c")) {};
    }

    mem$scope(tmem$, _)
    {
        CexSerdeGen_c sg;
        e$ret(CexSerdeGen.create(
            &sg,
            _,
            &(CexSerdeGen_kw){ .namespace = "serdegen",
                               .buf_initial_capacity = 32 * 1024,
                               .workdir = TESTDIR"/basic/" }
        ));
        tassert_eq(sg.namespace, "serdegen");
        tassert_eq(sbuf.capacity(&sg.c_file_content), 32 * 1024 - sizeof(sbuf_head_s) - 1);

        io.printf("\nParsing source code for serdegen\n");
        io.printf("-------------------------\n");
        e$ret(CexSerdeGen.run(&sg));
        io.printf("-------------------------\n");

        io.printf("\nCompiling and running serdegen program\n");
        io.printf("-------------------------\n");
        os_cmd_c cmd = { 0 };
#if mem$asan_enabled()
        char* cc_args[] = { "cc",
                            "-I.",
                            "-Wall",
                            "-Wextra",
                            "-Werror",
                            "-fsanitize-address-use-after-scope",
                            "-fsanitize=address",
                            "-fsanitize=undefined",
                            "-fstack-protector-strong",
                            "-g",
                            "-o",
                            TESTDIR "a.out",
                            TESTDIR "basic/serdegen_test_basic.c",
                            NULL };
#else
        char* cc_args[] = { "cc",      "-I.",           "-Wall",
                            "-Wextra", "-Werror",       "-g",
                            "-o",      TESTDIR "a.out", TESTDIR "basic/serdegen_test_basic.c",
                            NULL };
#endif
        _os$args_print("CMD: ", cc_args, arr$len(cc_args));
        e$ret(os.cmd.create(
            &cmd,
            cc_args,
            arr$len(cc_args),
            &(os_cmd_flags_s){ .combine_stdouterr = true, .no_window = true }
        ));
        char* output = os.cmd.read_all(&cmd, _);
        e$except (err, os.cmd.join(&cmd, 10, NULL)) {
            log$error("Compiler error: \n%s\n", output);
            return err;
        }
        io.printf("%s\n", output);

        io.printf("-------------------------\n");
        io.printf("\nRunning serdegen test (in separate process!)\n");
        io.printf("-------------------------\n");
        char* test_args[] = { TESTDIR "a.out", "--quiet", NULL };
        e$ret(os.cmd.create(
            &cmd,
            test_args,
            arr$len(test_args),
            &(os_cmd_flags_s){ .combine_stdouterr = true, .no_window = true }
        ));
        output = os.cmd.read_all(&cmd, _);
        e$except(err, os.cmd.join(&cmd, 10, NULL)) {
            log$error("Test error: \n%s\n", output);
            return err;
        }
        log$info("Test Passed: \n%s\n", output);
        io.printf("-------------------------\n");
    }

    // tassert_eq(1, 0);
    return EOK;
}

test$main();
