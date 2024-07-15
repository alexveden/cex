#include <cex/cexlib/argparse.c>
#include <cex/cexlib/cex.c>
#include <cex/cextest/cextest.h>
#include <errno.h>
#include <stdio.h>
#include <x86intrin.h>

#define PERM_READ (1 << 0)
#define PERM_WRITE (1 << 1)
#define PERM_EXEC (1 << 2)

/*
 * SUITE INIT / SHUTDOWN
 */
void
my_test_shutdown_func(void)
{
}

ATEST_SETUP_F(void)
{
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}


ATEST_F(test_argparse_init)
{
    int force = 0;
    int test = 0;
    int int_num = 0;
    float flt_num = 0.f;
    const char* path = NULL;
    int perms = 0;
    argparse_option_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do", NULL, 0, 0),
        argparse$opt_bool('t', "test", &test, "test only", NULL, 0, 0),
        argparse$opt_string('p', "path", &path, "path to read", NULL, 0, 0),
        argparse$opt_integer('i', "int", &int_num, "selected integer", NULL, 0, 0),
        argparse$opt_float('s', "float", &flt_num, "selected float", NULL, 0, 0),
        argparse$opt_group("Bits options"),
        argparse$opt_bit(0, "read", &perms, "read perm", NULL, PERM_READ, OPT_NONEG),
        argparse$opt_bit(0, "write", &perms, "write perm", NULL, PERM_WRITE, 0),
        argparse$opt_bit(0, "exec", &perms, "exec perm", NULL, PERM_EXEC, 0),
        argparse$opt_end(),
    };

    static const char* const usages[] = {
        "basic [options] [[--] args]",
        "basic [options]",
        NULL,
    };

    argparse_c argparse = {
        .options = options,
        .usages = usages,
        .description = "\nA brief description of what the program does and how it works.",
        "\nAdditional description of the program after the description of the arguments.",
    };

    char* argv[] = {
        "program_name",
        "-f",
        "-t",
        "-p",
        "mypath/ok",
        "-i",
        "2000",
        "-s",
        "20.20",
    };
    int argc = arr$len(argv);

    atassert_eqs(EOK, argparse_parse(&argparse, argc, argv));
    atassert_eqi(force, 1);
    atassert_eqi(test, 1);
    atassert_eqs(path, "mypath/ok");
    atassert_eqi(int_num, 2000);
    atassert_eqf(round(flt_num*100), round(20.20*100));

    return NULL;
}

/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    ATEST_PARSE_MAINARGS(argc, argv);
    ATEST_PRINT_HEAD();  // >>> all tests below
    
    ATEST_RUN(test_argparse_init);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
