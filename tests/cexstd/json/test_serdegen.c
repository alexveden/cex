#define CEX_IMPLEMENTATION
#define CEX_BUILD
#define CEX_TEST
#include "cex.h"
#include "cexstd/json/json.c"


#define TESTDIR "tests/cexstd/json/"

static Exception
test_serdegen_process_code(IAllocator allc, char* code)
{
    json_gen_c self = { 0 };
    e$ret(json.gen.create(&self, allc, NULL));

    arr$(cex_token_s) items = arr$new(items, allc);
    CexParser_c lx = CexParser.create(code, 0, true);
    cex_token_s t;
    bool has_serde = false;

    while ((t = CexParser.next_entity(&lx, &items)).type) {
        if (t.type == CexTkn__error) { return lx.error; }
        cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, allc);
        if (d == NULL) { continue; }
        e$ret(_cex_json__gen__process_decl(&self, d, &has_serde));
    }
    return EOK;
}

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
test$case(serdegen_field_attrs)
{
    mem$arena_scope(256 * 1024, _)
    {
        char* ok =
            "json$$struct();\n"
            "typedef struct AttrsOk_c {\n"
            "    json$$field(.nullable = true);\n    i32 a;\n"
            "    json$$field(.nullable = false);\n    i32 b;\n"
            "    json$$field(.optional = true);\n    i32 c;\n"
            "    json$$field(.optional = false);\n    i32 d;\n"
            "    json$$field(.skip = true);\n    i32 e;\n"
            "    json$$field(.skip = false);\n    i32 f;\n"
            "    json$$field(.name = \"renamed\");\n    i32 g;\n"
            "} AttrsOk_c;\n";
        tassert_er(EOK, test_serdegen_process_code(_, ok));

        char* bad_nullable =
            "json$$struct();\ntypedef struct AttrsB1_c {\n"
            "    json$$field(.nullable = bad);\n    i32 a;\n} AttrsB1_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, bad_nullable));

        char* bad_optional =
            "json$$struct();\ntypedef struct AttrsB2_c {\n"
            "    json$$field(.optional = bad);\n    i32 a;\n} AttrsB2_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, bad_optional));

        char* bad_skip =
            "json$$struct();\ntypedef struct AttrsB3_c {\n"
            "    json$$field(.skip = bad);\n    i32 a;\n} AttrsB3_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, bad_skip));

        char* bad_name =
            "json$$struct();\ntypedef struct AttrsB4_c {\n"
            "    json$$field(.name = 123);\n    i32 a;\n} AttrsB4_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, bad_name));

        char* unknown =
            "json$$struct();\ntypedef struct AttrsB5_c {\n"
            "    json$$field(.unknown = true);\n    i32 a;\n} AttrsB5_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, unknown));

        char* no_eq =
            "json$$struct();\ntypedef struct AttrsB6_c {\n"
            "    json$$field(.name \"x\");\n    i32 a;\n} AttrsB6_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, no_eq));

        char* no_ident =
            "json$$struct();\ntypedef struct AttrsB7_c {\n"
            "    json$$field(. = true);\n    i32 a;\n} AttrsB7_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, no_ident));

        char* bad_token =
            "json$$struct();\ntypedef struct AttrsB8_c {\n"
            "    json$$field(1);\n    i32 a;\n} AttrsB8_c;\n";
        tassert_er(Error.integrity, test_serdegen_process_code(_, bad_token));
    }
    return EOK;
}

test$case(serdegen_gen_create_errors)
{
    mem$arena_scope(256 * 1024, _)
    {
        json_gen_c self = { 0 };
        tassert_er(
            Error.not_found,
            json.gen.create(
                &self, _, &(json_gen_kw){ .workdir = TESTDIR "no_such_dir_xyz/" }
            )
        );
        tassert_er(
            Error.argument,
            json.gen.create(&self, _, &(json_gen_kw){ .workdir = TESTDIR "basic/Stock.h" })
        );
    }
    return EOK;
}

test$case(serdegen_gen_run_no_types)
{
    mem$arena_scope(256 * 1024, _)
    {
        char* dir = "tests/build/json_serdegen_no_types/";
        if (os.fs.remove_tree(dir)) {};
        e$ret(os.fs.mkpath(dir));
        e$ret(io.file.save(os$path_join(_, dir, "plain.h"), "int foo;\n"));

        json_gen_c self = { 0 };
        e$ret(json.gen.create(&self, _, &(json_gen_kw){ .workdir = dir, .out_dir = dir }));
        tassert_er(EOK, json.gen.run(&self));

        if (os.fs.remove_tree(dir)) {};
    }
    return EOK;
}

test$case(serdegen_gen_run_unowned_output)
{
    mem$arena_scope(256 * 1024, _)
    {
        char* dir = "tests/build/json_serdegen_unowned/";
        if (os.fs.remove_tree(dir)) {};
        e$ret(os.fs.mkpath(dir));
        e$ret(io.file.save(
            os$path_join(_, dir, "src.h"),
            "json$$struct();\ntypedef struct Unowned_c {\n    i32 a;\n} Unowned_c;\n"
        ));

        // pre-existing .c that was not generated by CexSerdeGen
        e$ret(io.file.save(os$path_join(_, dir, "serde.c"), "// not ours\n"));
        json_gen_c self = { 0 };
        e$ret(json.gen.create(
            &self,
            _,
            &(json_gen_kw){ .workdir = dir, .out_dir = dir, .out_namespace = "serde" }
        ));
        tassert_er(Error.integrity, json.gen.run(&self));

        // pre-existing .h that was not generated by CexSerdeGen
        e$ret(os.fs.remove(os$path_join(_, dir, "serde.c")));
        e$ret(io.file.save(os$path_join(_, dir, "serde.h"), "// not ours\n"));
        json_gen_c self2 = { 0 };
        e$ret(json.gen.create(
            &self2,
            _,
            &(json_gen_kw){ .workdir = dir, .out_dir = dir, .out_namespace = "serde" }
        ));
        tassert_er(Error.integrity, json.gen.run(&self2));

        if (os.fs.remove_tree(dir)) {};
    }
    return EOK;
}

test$case(serdegen_gen_process_file_parse_error)
{
    mem$arena_scope(256 * 1024, _)
    {
        char* dir = "tests/build/json_serdegen_bad_parse/";
        if (os.fs.remove_tree(dir)) {};
        e$ret(os.fs.mkpath(dir));
        char* bad = os$path_join(_, dir, "bad.h");
        // `json$$struct` without () sets the parser error
        e$ret(io.file.save(bad, "json$$struct\n"));

        json_gen_c self = { 0 };
        e$ret(json.gen.create(&self, _, &(json_gen_kw){ .workdir = dir, .out_dir = dir }));
        tassert(json.gen.process_file(&self, bad) != EOK);

        if (os.fs.remove_tree(dir)) {};
    }
    return EOK;
}

test$case(serdegen_gen_cexy_cmd)
{
    char* out_dir = "tests/build/json_serdegen_cmd/";
    if (os.fs.remove_tree(out_dir)) {};
    e$ret(os.fs.mkpath(out_dir));

    char* argv[] = { "gen", "-d", TESTDIR "basic/", "-n", "serdegen", "-o", out_dir };
    tassert_er(EOK, json.gen.cexy_cmd(arr$len(argv), argv, NULL));

    mem$scope(tmem$, _)
    {
        tassert(os.path.exists(os$path_join(_, out_dir, "serdegen.c")));
        tassert(os.path.exists(os$path_join(_, out_dir, "serdegen.h")));
    }

    if (os.fs.remove_tree(out_dir)) {};
    return EOK;
}

test$case(serdegen_gen_too_many_fields)
{
    mem$arena_scope(256 * 1024, _)
    {
        sbuf_c code = sbuf.create(4096, _);
        e$ret(sbuf.append(&code, "json$$struct();\ntypedef struct Many_c {\n"));
        for (i32 i = 0; i < 64; i++) { e$ret(sbuf.appendf(&code, "    i32 f%d;\n", i)); }
        e$ret(sbuf.append(&code, "} Many_c;\n"));

        json_gen_c self = { 0 };
        e$ret(json.gen.create(&self, _, NULL));

        arr$(cex_token_s) items = arr$new(items, _);
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        bool has_serde = false;
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            e$ret(_cex_json__gen__process_decl(&self, d, &has_serde));
        }
        tassert(has_serde);
        tassert_er(Error.overflow, json.gen.generate_full(&self));
    }
    return EOK;
}

test$case(serdegen_gen_duplicate_type)
{
    mem$arena_scope(256 * 1024, _)
    {
        char* code =
            "json$$struct();\ntypedef struct Dup_c {\n    i32 a;\n} Dup_c;\n"
            "json$$struct();\ntypedef struct Dup_c {\n    i32 b;\n} Dup_c;\n";
        tassert_er(Error.exists, test_serdegen_process_code(_, code));
    }
    return EOK;
}

test$case(serdegen_gen_body_comment)
{
    mem$arena_scope(256 * 1024, _)
    {
        char* code =
            "json$$struct();\ntypedef struct Cmt_c {\n"
            "    // comment inside the struct body\n    i32 a;\n} Cmt_c;\n";
        tassert_er(EOK, test_serdegen_process_code(_, code));
    }
    return EOK;
}

#endif // #if !defined(__EMSCRIPTEN__)

test$main();
