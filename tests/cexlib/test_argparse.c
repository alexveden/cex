#include <_cexlib/argparse.c>
#include <_cexlib/allocators.c>
#include <_cexlib/argparse.h>
#include <_cexlib/cex.c>
#include <_cexlib/cex.h>
#include <_cexlib/str.c>
#include <_cexlib/cextest.h>
#include <stdio.h>
#include <x86intrin.h>

#define PERM_READ (1 << 0)
#define PERM_WRITE (1 << 1)
#define PERM_EXEC (1 << 2)

const Allocator_i* allocator;
/*
* SUITE INIT / SHUTDOWN
*/
test$teardown(){
    allocator = allocators.heap.destroy();
    return EOK;
}

test$setup()
{
    allocator = allocators.heap.create();
    return EOK;
}



test$case(test_argparse_init_short)
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
        argparse$opt_str('p', "path", &path, "path to read", .required = true, NULL, 0, 0),
        argparse$opt_i64('i', "int", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_f32('s', "float", &flt_num, "selected float", false, NULL, 0, 0),
        argparse$opt_group("Bits options"),
        argparse$opt_bit(0, "read", &perms, "read perm", false, NULL, PERM_READ, ARGPARSE_OPT_NONEG),
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

    tassert_eqs(EOK, argparse_parse(&argparse, argc, argv));
    tassert_eqi(force, 1);
    tassert_eqi(test, 1);
    tassert_eqs(path, "mypath/ok");
    tassert_eqi(int_num, 2000);
    tassert_eqf((u32)(flt_num * 100), (u32)(20.20 * 100));

    return EOK;
}

test$case(test_argparse_init_long)
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
        argparse$opt_str('p', "path", &path, "path to read", .callback = NULL, 0, 0),
        argparse$opt_i64('i', "int", &int_num, .help = "selected integer", NULL, 0, 0),
        argparse$opt_f32('s', "float", .value = &flt_num, "selected float", NULL, 0, 0),
        argparse$opt_group("Bits options"),
        argparse$opt_bit(0, "read", &perms, "read perm", false, NULL, PERM_READ, ARGPARSE_OPT_NONEG),
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


    tassert_eqs(EOK, argparse.parse(&args, argc, argv));
    tassert_eqs("test_program_name", args.program_name);
    tassert_eqi(force, 1);
    tassert_eqi(test, 1);
    tassert_eqs(path, "mypath/ok");
    tassert_eqi(int_num, 2000);
    tassert_eqf((u32)(flt_num * 100), (u32)(20.20 * 100));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_required)
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


    tassert_eqs(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eqi(options[2].required, 1);
    tassert_eqi(options[2].is_present, 0);
    tassert_eqi(force, 100);

    return EOK;
}

test$case(test_argparse_bad_opts_help)
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


    tassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_bad_opts_long)
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

    tassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_bad_opts_short)
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

    tassert_eqs(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_bad_opts_arg_value_null)
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

    tassert_eqs(Error.sanity_check, argparse.parse(&args, argc, argv));

    return EOK;
}

test$case(test_argparse_bad_opts_both_no_long_short)
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

    tassert_eqs(Error.sanity_check, argparse.parse(&args, argc, argv));

    return EOK;
}

test$case(test_argparse_help_print)
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

    tassert_eqs(Error.argsparse, argparse.parse(&args, argc, argv));

    return EOK;
}

test$case(test_argparse_help_print_long)
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

    tassert_eqs(Error.argsparse, argparse.parse(&args, argc, argv));

    return EOK;
}


test$case(test_argparse_arguments)
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


    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eqi(force, 100);
    tassert_eqi(argc, 3);                 // unchanged
    tassert_eqs(argv[0], "program_name"); // unchanged
    tassert_eqs(argv[1], "arg1");         // unchanged
    tassert_eqs(argv[2], "arg2");         // unchanged
    //
    tassert_eqi(argparse.argc(&args), 2);
    tassert_eqs(argparse.argv(&args)[0], "arg1");
    tassert_eqs(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_arguments_after_options)
{
    int force = 100;
    int int_num = 2000;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_i64('i', "int", &int_num, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-f", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eqi(force, 1);
    tassert_eqi(int_num, 100);
    tassert_eqi(argc, 5);                 // unchanged
    tassert_eqs(argv[0], "program_name"); // unchanged
    tassert_eqs(argv[1], "-f");           // unchanged
    tassert_eqs(argv[2], "--int=100");    // unchanged
    tassert_eqs(argv[3], "arg1");         // unchanged
    tassert_eqs(argv[4], "arg2");         // unchanged
    //
    // tassert_eqi(argparse.argc(&args), 2);
    tassert_eqs(argparse.argv(&args)[0], "arg1");
    tassert_eqs(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_arguments_stacked_short_opt)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_i64('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-fi", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eqi(force, 1);
    tassert_eqi(int_num, 1);
    tassert_eqi(int_num2, 100);
    tassert_eqi(argc, 5);                 // unchanged
    tassert_eqs(argv[0], "program_name"); // unchanged
    tassert_eqs(argv[1], "-fi");          // unchanged
    tassert_eqs(argv[2], "--int=100");    // unchanged
    tassert_eqs(argv[3], "arg1");         // unchanged
    tassert_eqs(argv[4], "arg2");         // unchanged
    //
    tassert_eqi(argparse.argc(&args), 2);
    tassert_eqs(argparse.argv(&args)[0], "arg1");
    tassert_eqs(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_arguments_double_dash)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_i64('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-fi", "--", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eqi(force, 1);
    tassert_eqi(int_num, 1);
    // NOTE: this must be untouched because we have -- prior
    tassert_eqi(int_num2, -100);
    tassert_eqi(argc, 6);                 // unchanged
    tassert_eqs(argv[0], "program_name"); // unchanged
    tassert_eqs(argv[1], "-fi");          // unchanged
    tassert_eqs(argv[2], "--");           // unchanged
    tassert_eqs(argv[3], "--int=100");    // unchanged
    tassert_eqs(argv[4], "arg1");         // unchanged
    tassert_eqs(argv[5], "arg2");         // unchanged
    //
    tassert_eqi(argparse.argc(&args), 3);
    tassert_eqs(argparse.argv(&args)[0], "--int=100");
    tassert_eqs(argparse.argv(&args)[1], "arg1");
    tassert_eqs(argparse.argv(&args)[2], "arg2");

    return EOK;
}

test$case(test_argparse_arguments__option_follows_argument_not_allowed)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_i64('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "arg1", "-fi", "arg2", "--int=100" };
    int argc = arr$len(argv);


    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv));

    // All untouched
    tassert_eqi(force, 100);
    tassert_eqi(int_num, -100);
    tassert_eqi(int_num2, -100);
    tassert_eqi(argc, 5);                 // unchanged
    tassert_eqs(argv[0], "program_name"); // unchanged
    tassert_eqs(argv[1], "arg1");         // unchanged
    tassert_eqs(argv[2], "-fi");          // unchanged
    tassert_eqs(argv[3], "arg2");         // unchanged
    tassert_eqs(argv[4], "--int=100");    // unchanged
    //
    tassert_eqi(argparse.argc(&args), 0);

    return EOK;
}


test$case(test_argparse_arguments__parsing_error)
{
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_i64('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "--int=foo" };
    int argc = arr$len(argv);

    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eqi(int_num2, 0);

    return EOK;
}

test$case(test_argparse_arguments__option_follows_argument__allowed)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_i64('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "arg1", "-fi", "arg2", "--int=100" };
    int argc = arr$len(argv);

    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eqi(argparse.argc(&args), 4);
    tassert_eqs(argparse.argv(&args)[0], "arg1");
    tassert_eqs(argparse.argv(&args)[1], "-fi");
    tassert_eqs(argparse.argv(&args)[2], "arg2");
    tassert_eqs(argparse.argv(&args)[3], "--int=100");

    return EOK;
}

test$case(test_argparse_arguments__command_mode)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_i64('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "cmd_name", "-fi", "--int=100", "cmd_arg_something" };
    int argc = arr$len(argv);

    tassert_eqs(args.program_name, NULL);
    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv));
    // params are untouched
    tassert_eqi(force, 100);
    tassert_eqi(int_num, -100);
    tassert_eqi(int_num2, -100);
    tassert_eqs(args.program_name, "program_name");
    tassert_eqi(argparse.argc(&args), 4);
    tassert_eqs(argparse.argv(&args)[0], "cmd_name");
    tassert_eqs(argparse.argv(&args)[1], "-fi");
    tassert_eqs(argparse.argv(&args)[2], "--int=100");
    tassert_eqs(argparse.argv(&args)[3], "cmd_arg_something");

    argparse_c cmd_args = {
        .options = options,
        .options_len = arr$len(options),
    };
    tassert_eqe(Error.ok, argparse.parse(&cmd_args, argparse.argc(&args), argparse.argv(&args)));
    tassert_eqs(args.program_name, "program_name");
    tassert_eqi(force, 1);
    tassert_eqi(int_num, 1);
    tassert_eqi(int_num2, 100);
    tassert_eqs(cmd_args.program_name, "cmd_name");

    tassert_eqi(argparse.argc(&cmd_args), 1);
    tassert_eqs(argparse.argv(&cmd_args)[0], "cmd_arg_something");

    return EOK;
}

test$case(test_argparse_arguments__command__help)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "force", &force, "force to do"),
        argparse$opt_bool('i', "int_flag", &int_num, "selected integer", false, NULL, 0, 0),
        argparse$opt_i64('z', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_opt_s cmd_options[] = {
        argparse$opt_help(),
        argparse$opt_group("Command  options"),
        argparse$opt_i64('z', "int", &int_num2, "some cmd int", false, NULL, 0, 0),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "cmd_name", "-h"};
    int argc = arr$len(argv);

    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv));

    argparse_c cmd_args = {
        .options = cmd_options,
        .options_len = arr$len(cmd_options),
        .description = "this is a command description",
        .usage = "some command usage",
        .epilog = "command epilog",
    };
    tassert_eqe(Error.argsparse, argparse.parse(&cmd_args, argparse.argc(&args), argparse.argv(&args)));

    return EOK;
}

test$case(test_argparse_arguments__int_parsing)
{
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_i64('i', "int", &int_num2, "selected integer", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv0[] = { "program_name", "--int=99"};
    int argc = arr$len(argv0);

    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv0));

    char* argv[] = { "program_name", "--int=foo1" };

    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eqi(int_num2, 0);

    int_num2 = -100;
    char* argv2[] = { "program_name", "--int=1foo" };
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv2));
    tassert_eqi(int_num2, 1); // still set, but its strtol() issue

    int_num2 = -100;
    char* argv3[] = { "program_name", "--int=9999999999999999999999999999999999999999999999999999" };
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv3));

    int_num2 = -100;
    char* argv4[] = { "program_name", "-i"};
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv4));
    tassert_eqi(int_num2, -100); 

    int_num2 = -100;
    char* argv5[] = { "program_name", "--int="};
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv5));
    tassert_eqi(int_num2, -100); 

    int_num2 = -100;
    char* argv6[] = { "program_name", "--int", "-6"};
    tassert_eqe(Error.ok, argparse.parse(&args, 3, argv6));
    tassert_eqi(int_num2, -6); 

    int_num2 = -100;
    char* argv7[] = { "program_name", "--int=9.8"};
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv7));
    tassert_eqi(int_num2, 9); 

    return EOK;
}

test$case(test_argparse_arguments__float_parsing)
{
    f32 fnum = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_f32('f', "flt", &fnum, "selected float", false, NULL, 0, 0),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv0[] = { "program_name", "--flt=99"};
    int argc = arr$len(argv0);
    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv0));

    char* argv[] = { "program_name", "--flt=foo1" };

    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eqi(fnum, 0);

    fnum = -100;
    char* argv2[] = { "program_name", "--flt=1foo" };
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv2));
    tassert_eqi(fnum, 1); // still set, but its strtol() issue

    fnum = -100;
    char* argv3[] = { "program_name", "--flt=9999999999999999999999999999999999999999999999999999" };
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv3));

    fnum = -100;
    char* argv4[] = { "program_name", "-f"};
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv4));
    tassert_eqi(fnum, -100); 

    fnum = -100;
    char* argv5[] = { "program_name", "--flt="};
    tassert_eqe(Error.argsparse, argparse.parse(&args, argc, argv5));
    tassert_eqi(fnum, -100); 

    fnum = -100;
    char* argv6[] = { "program_name", "--flt", "6"};
    tassert_eqe(Error.ok, argparse.parse(&args, 3, argv6));
    tassert_eqi(fnum, 6); 

    fnum = -100;
    char* argv7[] = { "program_name", "--flt=-9.8"};
    tassert_eqe(Error.ok, argparse.parse(&args, argc, argv7));
    tassert_eqi(fnum*100, -9.8*100); 

    return EOK;
}

/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_argparse_init_short);
    test$run(test_argparse_init_long);
    test$run(test_argparse_required);
    test$run(test_argparse_bad_opts_help);
    test$run(test_argparse_bad_opts_long);
    test$run(test_argparse_bad_opts_short);
    test$run(test_argparse_bad_opts_arg_value_null);
    test$run(test_argparse_bad_opts_both_no_long_short);
    test$run(test_argparse_help_print);
    test$run(test_argparse_help_print_long);
    test$run(test_argparse_arguments);
    test$run(test_argparse_arguments_after_options);
    test$run(test_argparse_arguments_stacked_short_opt);
    test$run(test_argparse_arguments_double_dash);
    test$run(test_argparse_arguments__option_follows_argument_not_allowed);
    test$run(test_argparse_arguments__parsing_error);
    test$run(test_argparse_arguments__option_follows_argument__allowed);
    test$run(test_argparse_arguments__command_mode);
    test$run(test_argparse_arguments__command__help);
    test$run(test_argparse_arguments__int_parsing);
    test$run(test_argparse_arguments__float_parsing);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
