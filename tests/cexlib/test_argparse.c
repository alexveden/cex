#include <cex/cexlib/argparse.c>
#include <cex/cexlib/argparse.h>
#include <cex/cexlib/cex.c>
#include <cex/cexlib/cex.h>
#include <cex/cexlib/str.c>
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
        "test_program_name", "--force", "--test", "--path=mypath/ok", "--int=2000", "--float=20.20",
    };
    int argc = arr$len(argv);


    atassert_eqs(EOK, argparse.parse(&args, argc, argv));
    atassert_eqs("test_program_name", args.program_name);
    atassert_eqi(force, 1);
    atassert_eqi(test, 1);
    atassert_eqs(path, "mypath/ok");
    atassert_eqi(int_num, 2000);
    atassert_eqf(round(flt_num * 100), round(20.20 * 100));

    argparse.usage(&args);

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


    atassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

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

    atassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

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

    atassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

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

ATEST_F(test_argparse_help_print)
{
    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-h" };

    int argc = arr$len(argv);

    atassert_eqs(Error.argsparse, argparse.parse(&args, argc, argv));

    return NULL;
}

ATEST_F(test_argparse_help_print_long)
{
    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "--help" };

    int argc = arr$len(argv);

    atassert_eqs(Error.argsparse, argparse.parse(&args, argc, argv));

    return NULL;
}


ATEST_F(test_argparse_arguments)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "arg1", "arg2" };
    int argc = arr$len(argv);


    atassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    atassert_eqi(force, 100);
    atassert_eqi(argc, 3);                 // unchanged
    atassert_eqs(argv[0], "program_name"); // unchanged
    atassert_eqs(argv[1], "arg1");         // unchanged
    atassert_eqs(argv[2], "arg2");         // unchanged
    //
    atassert_eqi(argparse.argc(&args), 2);
    atassert_eqs(argparse.argv(&args)[0], "arg1");
    atassert_eqs(argparse.argv(&args)[1], "arg2");

    return NULL;
}

ATEST_F(test_argparse_arguments_after_options)
{
    int force = 100;
    int int_num = 2000;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_integer('i', "int", &int_num, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-f", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    atassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    atassert_eqi(force, 1);
    atassert_eqi(int_num, 100);
    atassert_eqi(argc, 5);                 // unchanged
    atassert_eqs(argv[0], "program_name"); // unchanged
    atassert_eqs(argv[1], "-f");           // unchanged
    atassert_eqs(argv[2], "--int=100");    // unchanged
    atassert_eqs(argv[3], "arg1");         // unchanged
    atassert_eqs(argv[4], "arg2");         // unchanged
    //
    // atassert_eqi(argparse.argc(&args), 2);
    atassert_eqs(argparse.argv(&args)[0], "arg1");
    atassert_eqs(argparse.argv(&args)[1], "arg2");

    return NULL;
}

ATEST_F(test_argparse_arguments_stacked_short_opt)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_integer('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-fi", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    atassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    atassert_eqi(force, 1);
    atassert_eqi(int_num, 1);
    atassert_eqi(int_num2, 100);
    atassert_eqi(argc, 5);                 // unchanged
    atassert_eqs(argv[0], "program_name"); // unchanged
    atassert_eqs(argv[1], "-fi");          // unchanged
    atassert_eqs(argv[2], "--int=100");    // unchanged
    atassert_eqs(argv[3], "arg1");         // unchanged
    atassert_eqs(argv[4], "arg2");         // unchanged
    //
    atassert_eqi(argparse.argc(&args), 2);
    atassert_eqs(argparse.argv(&args)[0], "arg1");
    atassert_eqs(argparse.argv(&args)[1], "arg2");

    return NULL;
}

ATEST_F(test_argparse_arguments_double_dash)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_integer('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-fi", "--", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    atassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    atassert_eqi(force, 1);
    atassert_eqi(int_num, 1);
    // NOTE: this must be untouched because we have -- prior
    atassert_eqi(int_num2, -100);
    atassert_eqi(argc, 6);                 // unchanged
    atassert_eqs(argv[0], "program_name"); // unchanged
    atassert_eqs(argv[1], "-fi");          // unchanged
    atassert_eqs(argv[2], "--");           // unchanged
    atassert_eqs(argv[3], "--int=100");    // unchanged
    atassert_eqs(argv[4], "arg1");         // unchanged
    atassert_eqs(argv[5], "arg2");         // unchanged
    //
    atassert_eqi(argparse.argc(&args), 3);
    atassert_eqs(argparse.argv(&args)[0], "--int=100");
    atassert_eqs(argparse.argv(&args)[1], "arg1");
    atassert_eqs(argparse.argv(&args)[2], "arg2");

    return NULL;
}

ATEST_F(test_argparse_arguments__option_follows_argument_not_allowed)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_integer('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "arg1", "-fi", "arg2", "--int=100" };
    int argc = arr$len(argv);


    atassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv));

    // All untouched
    atassert_eqi(force, 100);
    atassert_eqi(int_num, -100);
    atassert_eqi(int_num2, -100);
    atassert_eqi(argc, 5);                 // unchanged
    atassert_eqs(argv[0], "program_name"); // unchanged
    atassert_eqs(argv[1], "arg1");         // unchanged
    atassert_eqs(argv[2], "-fi");          // unchanged
    atassert_eqs(argv[3], "arg2");         // unchanged
    atassert_eqs(argv[4], "--int=100");    // unchanged
    //
    atassert_eqi(argparse.argc(&args), 0);

    return NULL;
}


ATEST_F(test_argparse_arguments__parsing_error)
{
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_integer('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "--int=foo" };
    int argc = arr$len(argv);

    atassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv));
    atassert_eqi(int_num2, 0);

    return NULL;
}

ATEST_F(test_argparse_arguments__option_follows_argument__allowed)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_integer('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "arg1", "-fi", "arg2", "--int=100" };
    int argc = arr$len(argv);

    atassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    atassert_eqi(argparse.argc(&args), 4);
    atassert_eqs(argparse.argv(&args)[0], "arg1");
    atassert_eqs(argparse.argv(&args)[1], "-fi");
    atassert_eqs(argparse.argv(&args)[2], "arg2");
    atassert_eqs(argparse.argv(&args)[3], "--int=100");

    return NULL;
}

ATEST_F(test_argparse_arguments__command_mode)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_integer('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "cmd_name", "-fi", "--int=100", "cmd_arg_something" };
    int argc = arr$len(argv);

    atassert_eqs(args.program_name, NULL);
    atassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    // params are untouched
    atassert_eqi(force, 100);
    atassert_eqi(int_num, -100);
    atassert_eqi(int_num2, -100);
    atassert_eqs(args.program_name, "program_name");
    atassert_eqi(argparse.argc(&args), 4);
    atassert_eqs(argparse.argv(&args)[0], "cmd_name");
    atassert_eqs(argparse.argv(&args)[1], "-fi");
    atassert_eqs(argparse.argv(&args)[2], "--int=100");
    atassert_eqs(argparse.argv(&args)[3], "cmd_arg_something");

    argparse_c cmd_args = {
        .options = options,
        .options_len = arr$len(options),
    };
    atassert_eqe(Error.ok, argparse.parse(&cmd_args, argparse.argc(&args), argparse.argv(&args)));
    atassert_eqs(args.program_name, "program_name");
    atassert_eqi(force, 1);
    atassert_eqi(int_num, 1);
    atassert_eqi(int_num2, 100);
    atassert_eqs(cmd_args.program_name, "cmd_name");

    atassert_eqi(argparse.argc(&cmd_args), 1);
    atassert_eqs(argparse.argv(&cmd_args)[0], "cmd_arg_something");

    return NULL;
}

ATEST_F(test_argparse_arguments__command__help)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_integer('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_opt_s cmd_options[] = {
        argparse$opt_help(),
        argparse$opt_group("Command  options"),
        argparse$opt_integer('z', "int", &int_num2, "some cmd int", false, NULL, 0, 0),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "cmd_name", "-h"};
    int argc = arr$len(argv);

    atassert_eqe(Error.ok, argparse.parse(&args, argc, argv));

    argparse_c cmd_args = {
        .options = cmd_options,
        .options_len = arr$len(cmd_options),
        .description = "this is a command description",
        .usage = "some command usage",
        .epilog = "command epilog",
    };
    atassert_eqe(Error.argsparse, argparse.parse(&cmd_args, argparse.argc(&args), argparse.argv(&args)));

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
    ATEST_RUN(test_argparse_help_print);
    ATEST_RUN(test_argparse_help_print_long);
    ATEST_RUN(test_argparse_arguments);
    ATEST_RUN(test_argparse_arguments_after_options);
    ATEST_RUN(test_argparse_arguments_stacked_short_opt);
    ATEST_RUN(test_argparse_arguments_double_dash);
    ATEST_RUN(test_argparse_arguments__option_follows_argument_not_allowed);
    ATEST_RUN(test_argparse_arguments__parsing_error);
    ATEST_RUN(test_argparse_arguments__option_follows_argument__allowed);
    ATEST_RUN(test_argparse_arguments__command_mode);
    ATEST_RUN(test_argparse_arguments__command__help);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
