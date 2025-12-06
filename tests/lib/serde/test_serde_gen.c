#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/CexSerdeGen.c"


#define TESTDIR "tests/lib/serde/"

test$setup_case()
{
    if (os.path.exists(TESTDIR "serdegen.h")) {
        if (os.fs.remove(TESTDIR "serdegen.h")) {};
    }
    if (os.path.exists(TESTDIR "serdegen.c")) {
        if (os.fs.remove(TESTDIR "serdegen.c")) {};
    }
    return EOK;
}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(my_test_case)
{
    mem$scope(tmem$, _)
    {
        char* code = io.file.load(TESTDIR "myserde.h", _);
        tassert(code && "Load failed");

        CexSerdeGen_c sg;
        e$ret(CexSerdeGen.create(
            &sg,
            _,
            &(CexSerdeGen_kw){ .namespace = "serdegen",
                               .buf_initial_capacity = 32 * 1024,
                               .workdir = TESTDIR }
        ));
        tassert_eq(sg.namespace, "serdegen");
        tassert_eq(sbuf.capacity(&sg.c_file_content), 32 * 1024 - sizeof(sbuf_head_s) - 1);
        // e$ret(CexSerdeGen.process_code(&sg, code, 0));
        // e$ret(CexSerdeGen.generate_full(&sg));

        io.printf("-------------------------\n");
        e$ret(CexSerdeGen.process(&sg));
        // io.printf("%s\n", sg.c_file_content);
        // io.printf("%s\n", sg.h_file_content);
        e$ret(io.file.save(str.fmt(_, TESTDIR "%s.c", sg.namespace), sg.c_file_content));
        e$ret(io.file.save(str.fmt(_, TESTDIR "%s.h", sg.namespace), sg.h_file_content));
        io.printf("-------------------------\n");
    }
    tassert_eq(1, 0);
    return EOK;
}

test$main();
