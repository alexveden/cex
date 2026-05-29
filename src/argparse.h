#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_os)

#include "all.h"
#include "src/cexy.h"

struct argparse_c;
struct argparse_opt_s;

/// Callback function for custom option handling
typedef Exception (*argparse_callback_f)(
    struct argparse_c* self,
    struct argparse_opt_s* option,
    void* ctx
);
/// Conversion function from string to typed value
typedef Exception (*argparse_convert_f)(char* s, void* out_val);
/// Command handler function
typedef Exception (*argparse_command_f)(int argc, char** argv, void* user_ctx);

/// command line options type (prefer macros)
typedef struct argparse_opt_s
{
    u8 type;
    void* value;
    char short_name;
    char* long_name;
    char* help;
    bool required;
    argparse_callback_f callback;
    void* callback_data;
    argparse_convert_f convert;
    bool is_present; // also setting in in argparse$opt* macro, allows optional parameter sugar
} argparse_opt_s;

/// command settings type (prefer macros)
typedef struct argparse_cmd_s
{
    char* name;
    argparse_command_f func;
    char* help;
    bool is_default;
} argparse_cmd_s;

/// Option value type enum (boolean, string, i8-u64, f32-f64)
enum CexArgParseType_e
{
    CexArgParseType__na,
    CexArgParseType__group,
    CexArgParseType__boolean,
    CexArgParseType__string,
    CexArgParseType__i8,
    CexArgParseType__u8,
    CexArgParseType__i16,
    CexArgParseType__u16,
    CexArgParseType__i32,
    CexArgParseType__u32,
    CexArgParseType__i64,
    CexArgParseType__u64,
    CexArgParseType__f32,
    CexArgParseType__f64,
    CexArgParseType__generic,
};

/// main argparse struct (used as options config)
typedef struct argparse_c
{
    // user supplied options
    argparse_opt_s* options;
    u32 options_len;

    argparse_cmd_s* commands;
    u32 commands_len;

    char* usage;        // usage text (can be multiline), each line prepended by program_name
    char* description;  // a description after usage
    char* epilog;       // a description at the end
    char* program_name; // program name in usage (by default: argv[0])

    int argc;    // current argument count (excludes program_name)
    char** argv; // current arguments list

    struct
    {
        // internal context
        char** out;
        int cpidx;
        char* optvalue; // current option value
        bool has_argument;
        argparse_cmd_s* current_command;
    } _ctx;
} argparse_c;


/// holder for list of  argparse$opt()
#define argparse$opt_list(...) .options = (argparse_opt_s[]) {__VA_ARGS__ {0} /* NULL TERM */}

/// command line option record (generic type of arguments)
#define argparse$opt(value, ...)                                                                   \
    ({                                                                                             \
        u32 val_type = _Generic(                                                                   \
            (value),                                                                               \
            bool*: CexArgParseType__boolean,                                                       \
            i8*: CexArgParseType__i8,                                                              \
            u8*: CexArgParseType__u8,                                                              \
            i16*: CexArgParseType__i16,                                                            \
            u16*: CexArgParseType__u16,                                                            \
            i32*: CexArgParseType__i32,                                                            \
            u32*: CexArgParseType__u32,                                                            \
            i64*: CexArgParseType__i64,                                                            \
            u64*: CexArgParseType__u64,                                                            \
            f32*: CexArgParseType__f32,                                                            \
            f64*: CexArgParseType__f64,                                                            \
            const char**: CexArgParseType__string,                                                 \
            char**: CexArgParseType__string,                                                       \
            default: CexArgParseType__generic                                                      \
        );                                                                                         \
        argparse_convert_f conv_f = _Generic(                                                      \
            (value),                                                                               \
            bool*: NULL,                                                                           \
            const char**: NULL,                                                                    \
            char**: NULL,                                                                          \
            i8*: (argparse_convert_f)str.convert.to_i8,                                            \
            u8*: (argparse_convert_f)str.convert.to_u8,                                            \
            i16*: (argparse_convert_f)str.convert.to_i16,                                          \
            u16*: (argparse_convert_f)str.convert.to_u16,                                          \
            i32*: (argparse_convert_f)str.convert.to_i32,                                          \
            u32*: (argparse_convert_f)str.convert.to_u32,                                          \
            i64*: (argparse_convert_f)str.convert.to_i64,                                          \
            u64*: (argparse_convert_f)str.convert.to_u64,                                          \
            f32*: (argparse_convert_f)str.convert.to_f32,                                          \
            f64*: (argparse_convert_f)str.convert.to_f64,                                          \
            default: NULL                                                                          \
        );                                                                                         \
        (argparse_opt_s){ val_type,                                                                \
                          (value),                                                                 \
                          __VA_ARGS__,                                                             \
                          .convert = (argparse_convert_f)conv_f,                                   \
                          .is_present = 0 };                                                       \
    })
// clang-format off


/// holder for list of 
#define argparse$cmd_list(...) .commands = (argparse_cmd_s[]) {__VA_ARGS__ {0} /* NULL TERM */}

/// options group separator
#define argparse$opt_group(h)     { CexArgParseType__group, NULL, '\0', NULL, h, false, NULL, 0, 0, .is_present=0 }

/// built-in option for -h,--help
#define argparse$opt_help()       {CexArgParseType__boolean, NULL, 'h', "help",                           \
                                        "show this help message and exit", false,    \
                                        NULL, 0, .is_present = 0}


/**

* Command line args parsing

```c
// NOTE: Command example 

Exception cmd_build_docs(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    // clang-format off
    argparse_c args = {
        .description = "My description",
        .usage = "Usage help",
        .epilog = "Epilog text",
        argparse$cmd_list(
            { .name = "build-docs", .func = cmd_build_docs, .help = "Build CEX documentation" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    if (argparse.run_command(&args, NULL)) { return 1; }
    return 0;
}

Exception
cmd_build_docs(int argc, char** argv, void* user_ctx)
{
    // Command handling func
}

```

* Parsing custom arguments

```c
// Simple options example

int
main(int argc, char** argv)
{
    bool force = 0;
    bool test = 0;
    int int_num = 0;
    float flt_num = 0.f;
    char* path = NULL;

    char* usage = "basic [options] [[--] args]\n"
                  "basic [options]\n";

    argparse_c argparse = {
        argparse$opt_list(
            argparse$opt_help(),
            argparse$opt_group("Basic options"),
            argparse$opt(&force, 'f', "force", "force to do"),
            argparse$opt(&test, 't', "test", .help = "test only"),
            argparse$opt(&path, 'p', "path", "path to read", .required = true),
            argparse$opt_group("Another group"),
            argparse$opt(&int_num, 'i', "int", "selected integer"),
            argparse$opt(&flt_num, 's', "float", "selected float"),
        ),
        // NOTE: usage/description are optional 

        .usage = usage,
        .description = "\nA brief description of what the program does and how it works.",
        "\nAdditional description of the program after the description of the arguments.",
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }

    // NOTE: all args are filled and parsed after this line

    return 0;
}

```


*/
struct __cex_namespace__argparse {
    // Autogenerated by CEX
    // clang-format off

    char*           (*next)(argparse_c* self);
    Exception       (*parse)(argparse_c* self, int argc, char** argv);
    Exception       (*run_command)(argparse_c* self, void* user_ctx);
    void            (*usage)(argparse_c* self);

    // clang-format on
};
/**
* @brief Command line argument parsing module
*/
CEX_NAMESPACE struct __cex_namespace__argparse argparse;

#endif
