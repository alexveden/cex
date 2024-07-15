#include <cex/cexlib/argparse.c>
#include <cex/cexlib/argparse.h>
#include <cex/cexlib/cex.c>
#include <cex/cexlib/cex.h>
#include <cex/cextest/cextest.h>
#include <math.h>
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
    uassert_enable();
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}


ATEST_F(test_argparse_init_short)
{
    int force = 0;
    int test = 0;
    int int_num = 0;
    float flt_num = 0.f;
    const char* path = NULL;
    int perms = 0;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do", false, NULL, 0, 0),
        argparse$opt_bool('t', "test", &test, "test only", false, NULL, 0, 0),
        argparse$opt_string('p', "path", &path, "path to read", .required = true, NULL, 0, 0),
        argparse$opt_integer('i', "int", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_float('s', "float", &flt_num, "selected float", false, NULL, 0, 0),
        argparse$opt_group("Bits options"),
        argparse$opt_bit(0, "read", &perms, "read perm", false, NULL, PERM_READ, OPT_NONEG),
        argparse$opt_bit(0, "write", &perms, "write perm", false, NULL, PERM_WRITE, 0),
        argparse$opt_bit(0, "exec", &perms, "exec perm", false, NULL, PERM_EXEC, 0),
    };

    const char* usage = "basic [options] [[--] args]\n"
                        "basic [options]\n";

    argparse_c argparse = {
        .options = options,
        .options_len = arr$len(options),
        .usage = usage,
        .description = "\nA brief description of what the program does and how it works.",
        "\nAdditional description of the program after the description of the arguments.",
    };

    char* argv[] = {
        "program_name", "-f", "-t", "-p", "mypath/ok", "-i", "2000", "-s", "20.20",
    };
    int argc = arr$len(argv);

    atassert_eqs(EOK, argparse_parse(&argparse, argc, argv));
    atassert_eqi(force, 1);
    atassert_eqi(test, 1);
    atassert_eqs(path, "mypath/ok");
    atassert_eqi(int_num, 2000);
    atassert_eqf(round(flt_num * 100), round(20.20 * 100));

    return NULL;
}

ATEST_F(test_argparse_init_long)
{
    int force = 0;
    int test = 0;
    int int_num = 0;
    float flt_num = 0.f;
    const char* path = NULL;
    int perms = 0;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('t', "test", &test, "test only", NULL, 0, 0),
        argparse$opt_string('p', "path", &path, "path to read", .callback = NULL, 0, 0),
        argparse$opt_integer('i', "int", &int_num, .help = "selected integer", NULL, 0, 0),
        argparse$opt_float('s', "float", .value = &flt_num, "selected float", NULL, 0, 0),
        argparse$opt_group("Bits options"),
        argparse$opt_bit(0, "read", &perms, "read perm", false, NULL, PERM_READ, OPT_NONEG),
        argparse$opt_bit(0, "write", &perms, "write perm", false, NULL, PERM_WRITE, 0),
        argparse$opt_bit(0, "exec", &perms, "exec perm", false, NULL, .data = PERM_EXEC, 0),
    };

    const char* usage = "basic [options] [[--] args]\n"
                        "basic [options]\n";

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
        .usage = usage,
        .description = "\nA brief description of what the program does and how it works.",
        .epilog = "\nEnd description after the list of the arguments.",
    };

    char* argv[] = {
        "program_name", "--force", "--test", "--path=mypath/ok", "--int=2000", "--float=20.20",
    };
    int argc = arr$len(argv);

    argparse.usage(&args);

    atassert_eqs(EOK, argparse.parse(&args, argc, argv));
    atassert_eqi(force, 1);
    atassert_eqi(test, 1);
    atassert_eqs(path, "mypath/ok");
    atassert_eqi(int_num, 2000);
    atassert_eqf(round(flt_num * 100), round(20.20 * 100));

    return NULL;
}

ATEST_F(test_argparse_required)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do", .required = true),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);


    atassert_eqs(Error.argsparse, argparse.parse(&args, argc, argv));
    atassert_eqi(options[2].required, 1);
    atassert_eqi(options[2].is_present, 0);
    atassert_eqi(force, 100);

    return NULL;
}

ATEST_F(test_argparse_bad_opts_help)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);

    argparse.usage(&args);

    atassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    return NULL;
}

ATEST_F(test_argparse_bad_opts_long)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', NULL, &force),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);

    argparse.usage(&args);

    atassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    return NULL;
}

ATEST_F(test_argparse_bad_opts_short)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('\0', "foo", &force),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);

    argparse.usage(&args);

    atassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    return NULL;
}

ATEST_F(test_argparse_bad_opts_arg_value_null)
{
    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', NULL),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-f" };

    int argc = arr$len(argv);

    uassert_disable();
    argparse.usage(&args);

    atassert_eqs(Error.sanity_check, argparse.parse(&args, argc, argv));

    return NULL;
}

ATEST_F(test_argparse_bad_opts_both_no_long_short)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('\0', NULL, &force),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);

    uassert_disable();
    argparse.usage(&args);

    atassert_eqs(Error.sanity_check, argparse.parse(&args, argc, argv));

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
    
    ATEST_RUN(test_argparse_init_short);
    ATEST_RUN(test_argparse_init_long);
    ATEST_RUN(test_argparse_required);
    ATEST_RUN(test_argparse_bad_opts_help);
    ATEST_RUN(test_argparse_bad_opts_long);
    ATEST_RUN(test_argparse_bad_opts_short);
    ATEST_RUN(test_argparse_bad_opts_arg_value_null);
    ATEST_RUN(test_argparse_bad_opts_both_no_long_short);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
