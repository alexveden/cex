#define CEX_IMPLEMENTATION
#define CEX_BUILD
#define CEX_TEST
#include "cex.h"
#include "cexstd/json/json.c"


#define TESTDIR "tests/cexstd/json/"

test$setup_case()
{
    return EOK;
}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

#if !defined(__EMSCRIPTEN__)
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
        json_gen_c sg;
        e$ret(json.gen.create(
            &sg,
            _,
            &(json_gen_kw){ .out_namespace = "serdegen",
                            .buf_initial_capacity = 32 * 1024,
                            .workdir = TESTDIR "/basic/" }
        ));
        tassert_eq(sg.namespace, "serdegen");
        tassert_eq(sbuf.capacity(&sg.c_file_content), 32 * 1024 - sizeof(sbuf_head_s) - 1);

        io.printf("\nParsing source code for serdegen\n");
        io.printf("-------------------------\n");
        e$ret(json.gen.run(&sg));
        io.printf("-------------------------\n");

        io.printf("\nCompiling and running serdegen program\n");
        io.printf("-------------------------\n");
        os_cmd_c cmd = { 0 };
#    if mem$asan_enabled()
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
                            "cexstd/json/json.c",
                            NULL };
#    else
        char* cc_args[] = { "cc",
                            "-I.",
                            "-Wall",
                            "-Wextra",
                            "-Werror",
                            "-g",
                            "-o",
                            TESTDIR "a.out",
                            TESTDIR "basic/serdegen_test_basic.c",
                            "cexstd/json/json.c",
                            NULL };
#    endif
        _os$args_print("CMD: ", cc_args, arr$len(cc_args));
        e$ret(os.cmd.create(
            &cmd,
            cc_args,
            arr$len(cc_args),
            &(os_cmd_flags_s){ .combine_stdouterr = true, .no_window = true }
        ));
        char* output = os.cmd.read_all(&cmd, _);
        e$except (err, os.cmd.wait(&cmd, 1, 10)) {
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
        e$except (err, os.cmd.wait(&cmd, 1, 10)) {
            log$error("Test error: \n%s\n", output);
            return err;
        }
        log$info("Test Passed: \n%s\n", output);
        io.printf("-------------------------\n");
    }

    // tassert_eq(1, 0);
    return EOK;
}

test$case(serdegen_myserde_advanced)
{
    // if (os.path.exists(TESTDIR "advanced/serdegen.h")) {
    //     if (os.fs.remove(TESTDIR "advanced/serdegen.h")) {};
    // }
    // if (os.path.exists(TESTDIR "advanced/serdegen.c")) {
    //     if (os.fs.remove(TESTDIR "advanced/serdegen.c")) {};
    // }

    mem$scope(tmem$, _)
    {
        json_gen_c sg;
        e$ret(json.gen.create(
            &sg,
            _,
            &(json_gen_kw){ .out_namespace = "serdegen",
                            .buf_initial_capacity = 32 * 1024,
                            .workdir = TESTDIR "/advanced/" }
        ));
        tassert_eq(sg.namespace, "serdegen");
        tassert_eq(sbuf.capacity(&sg.c_file_content), 32 * 1024 - sizeof(sbuf_head_s) - 1);

        io.printf("\nParsing source code for serdegen\n");
        io.printf("-------------------------\n");
        e$ret(json.gen.run(&sg));
        io.printf("-------------------------\n");

        io.printf("\nCompiling and running serdegen program\n");
        io.printf("-------------------------\n");
        os_cmd_c cmd = { 0 };
#    if mem$asan_enabled()
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
                            "cexstd/json/json.c",
                            TESTDIR "advanced/serdegen_test_advanced.c",
                            NULL };
#    else
        char* cc_args[] = { "cc",
                            "-I.",
                            "-Wall",
                            "-Wextra",
                            "-Werror",
                            "-g",
                            "-o",
                            TESTDIR "a.out",
                            TESTDIR "advanced/serdegen_test_advanced.c",
                            "cexstd/json/json.c",
                            NULL };
#    endif
        _os$args_print("CMD: ", cc_args, arr$len(cc_args));
        e$ret(os.cmd.create(
            &cmd,
            cc_args,
            arr$len(cc_args),
            &(os_cmd_flags_s){ .combine_stdouterr = true, .no_window = true }
        ));
        char* output = os.cmd.read_all(&cmd, _);
        e$except (err, os.cmd.wait(&cmd, 1, 10)) {
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
        e$except (err, os.cmd.wait(&cmd, 1, 10)) {
            log$error("Test error: \n%s\n", output);
            return err;
        }
        log$info("Test Passed: \n%s\n", output);
        io.printf("-------------------------\n");
    }

    return EOK;
}

test$case(serdegen_json_comments)
{
    mem$arena_scope(256 * 1024, _)
    {
        char* code = "json$$struct();\n"
                     "typedef struct App_c {\n"
                     "    i32 number;\n"
                     "    json$$field(.name = \"my_schema\");  // mycomment \n"
                     "    /* my comment */ \n"
                     "    u32 schema; \n"
                     "} App_c;\n";

        json_gen_c self = {0};
        e$ret(json.gen.create(&self, _, NULL));
        arr$(cex_token_s) items = arr$new(items, _);
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        bool has_serde = false;

        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__error) {
                log$error(CexParser$err_fmt(&lx, NULL));
                tassert(lx.error != EOK);
                return lx.error;
            }

            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            tassert_eq(EOK, _cex_json__gen__process_decl(&self, d, &has_serde));
        }

        tassert(has_serde);


    }


    return EOK;
}
#else
test$case(os_cmd_not_supported_by_platform)
{
    return EOK;
}
#endif // #if !defined(__EMSCRIPTEN__)

test$main();
