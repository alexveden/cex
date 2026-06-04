#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_os)
#include "argparse.h"
#include "cex_base.h"
#include <math.h>

static char*
_cex_argparse__prefix_skip(char* str, char* prefix)
{
    usize len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static Exception
_cex_argparse__error(argparse_c* self, argparse_opt_s* opt, char* reason, bool is_long)
{
    (void)self;
    if (is_long) {
        cexsp__fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        cexsp__fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

static void
cex_argparse_usage(argparse_c* self)
{
    uassert(self->argv != NULL && "usage before parse!");

    io.printf("Usage:\n");
    if (self->usage) {

        for$iter (str_s, it, str.slice.iter_split(str.sstr(self->usage), "\n", &it.iterator)) {
            if (it.val.len == 0) { break; }

            char* fn = str.findr(self->program_name, "/");

            if (fn != NULL) {
                io.printf("%s ", fn + 1);
            } else {
                io.printf("%s ", self->program_name);
            }

            if (io.fwrite(stdout, it.val.buf, it.val.len)) { /* error ignored */ }

            io.printf("\n");
        }
    } else {
        if (self->commands) {
            io.printf("%s {", self->program_name);
            for$eachp(c, self->commands, self->commands_len)
            {
                isize i = c - self->commands;
                if (i == 0) {
                    io.printf("%s", c->name);
                } else {
                    io.printf(",%s", c->name);
                }
            }
            io.printf("} [cmd_options] [cmd_args]\n");

        } else {
            io.printf("%s [options] [--] [arg1 argN]\n", self->program_name);
        }
    }

    // print description
    if (self->description) { io.printf("%s\n", self->description); }

    io.printf("\n");


    // figure out best width
    usize usage_opts_width = 0;
    usize len = 0;
    for$eachp(opt, self->options, self->options_len)
    {
        len = 0;
        if (opt->short_name) { len += 2; }
        if (opt->short_name && opt->long_name) {
            len += 2; // separator ", "
        }
        if (opt->long_name) { len += strlen(opt->long_name) + 2; }
        switch (opt->type) {
            case CexArgParseType__boolean:
                break;
            case CexArgParseType__string:
                break;
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                len += strlen("=<xxx>");
                break;

            default:
                break;
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) { usage_opts_width = len; }
    }
    usage_opts_width += 4; // 4 spaces prefix

    for$eachp(opt, self->options, self->options_len)
    {
        usize pos = 0;
        usize pad = 0;
        if (opt->type == CexArgParseType__group) {
            io.printf("\n%s\n", opt->help);
            continue;
        }
        pos = io.printf("    ");
        if (opt->short_name) { pos += io.printf("-%c", opt->short_name); }
        if (opt->long_name && opt->short_name) { pos += io.printf(", "); }
        if (opt->long_name) { pos += io.printf("--%s", opt->long_name); }

        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            io.printf("\n");
            pad = usage_opts_width;
        }
        if (!str.find(opt->help, "\n")) {
            io.printf("%*s%s", (int)pad + 2, "", opt->help);
        } else {
            u32 i = 0;
            for$iter (str_s, it, str.slice.iter_split(str.sstr(opt->help), "\n", &it.iterator)) {
                str_s clean = str.slice.strip(it.val);
                if (clean.len == 0) { continue; }
                if (i > 0) { io.printf("\n"); }
                io.printf("%*s%S", (i == 0) ? pad + 2 : usage_opts_width + 2, "", clean);
                i++;
            }
        }

        if (!opt->required && opt->value) {
            io.printf(" (default: ");
            switch (opt->type) {
                case CexArgParseType__boolean:
                    io.printf("%c", *(bool*)opt->value ? 'Y' : 'N');
                    break;
                case CexArgParseType__string:
                    if (*(char**)opt->value != NULL) {
                        io.printf("'%s'", *(char**)opt->value);
                    } else {
                        io.printf("''");
                    }
                    break;
                case CexArgParseType__i8:
                    io.printf("%d", *(i8*)opt->value);
                    break;
                case CexArgParseType__i16:
                    io.printf("%d", *(i16*)opt->value);
                    break;
                case CexArgParseType__i32:
                    io.printf("%d", *(i32*)opt->value);
                    break;
                case CexArgParseType__u8:
                    io.printf("%u", *(u8*)opt->value);
                    break;
                case CexArgParseType__u16:
                    io.printf("%u", *(u16*)opt->value);
                    break;
                case CexArgParseType__u32:
                    io.printf("%u", *(u32*)opt->value);
                    break;
                case CexArgParseType__i64:
                    io.printf("%ld", *(i64*)opt->value);
                    break;
                case CexArgParseType__u64:
                    io.printf("%lu", *(u64*)opt->value);
                    break;
                case CexArgParseType__f32:
                    io.printf("%g", *(f32*)opt->value);
                    break;
                case CexArgParseType__f64:
                    io.printf("%g", *(f64*)opt->value);
                    break;
                default:
                    break;
            }
            io.printf(")");
        }
        io.printf("\n");
    }

    for$eachp(cmd, self->commands, self->commands_len)
    {
        io.printf("%-20s%s%s\n", cmd->name, cmd->help, cmd->is_default ? " (default)" : "");
    }

    // print epilog
    if (self->epilog) { io.printf("%s\n", self->epilog); }
}
__attribute__((no_sanitize("undefined"))) static inline Exception
_cex_argparse__convert(char* s, argparse_opt_s* opt)
{
    // NOTE: this hits UBSAN because we casting convert function of
    // (char*, void*) into str.convert.to_u32(char*, u32*)
    // however we do explicit type checking and tagging so it should be good!
    return opt->convert(s, opt->value);
}

static Exception
_cex_argparse__getvalue(argparse_c* self, argparse_opt_s* opt, bool is_long)
{
    if (!opt->value) { goto skipped; }

    switch (opt->type) {
        case CexArgParseType__boolean:
            *(bool*)opt->value = !(*(bool*)opt->value);
            opt->is_present = true;
            break;
        case CexArgParseType__string:
            if (self->_ctx.optvalue) {
                *(char**)opt->value = self->_ctx.optvalue;
                self->_ctx.optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                self->_ctx.cpidx++;
                *(char**)opt->value = *++self->argv;
            } else {
                return _cex_argparse__error(self, opt, "requires a value", is_long);
            }
            opt->is_present = true;
            break;
        case CexArgParseType__i8:
        case CexArgParseType__u8:
        case CexArgParseType__i16:
        case CexArgParseType__u16:
        case CexArgParseType__i32:
        case CexArgParseType__u32:
        case CexArgParseType__i64:
        case CexArgParseType__u64:
        case CexArgParseType__f32:
        case CexArgParseType__f64:
            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return _cex_argparse__error(self, opt, "requires a value", is_long);
                }
                uassert(opt->convert != NULL);
                e$except_silent (err, _cex_argparse__convert(self->_ctx.optvalue, opt)) {
                    return _cex_argparse__error(self, opt, "argument parsing error", is_long);
                }
                self->_ctx.optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                self->_ctx.cpidx++;
                self->argv++;
                e$except_silent (err, _cex_argparse__convert(*self->argv, opt)) {
                    return _cex_argparse__error(self, opt, "argument parsing error", is_long);
                }
            } else {
                return _cex_argparse__error(self, opt, "requires a value", is_long);
            }
            if (opt->type == CexArgParseType__f32) {
                f32 res = *(f32*)opt->value;
                if (__builtin_isnan(res) || res == INFINITY || res == -INFINITY) {
                    return _cex_argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
            } else if (opt->type == CexArgParseType__f64) {
                f64 res = *(f64*)opt->value;
                if (__builtin_isnan(res) || res == INFINITY || res == -INFINITY) {
                    return _cex_argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
            }
            opt->is_present = true;
            break;
        default:
            uassert(false && "unhandled");
            return Error.runtime;
    }

skipped:
    if (opt->callback) {
        opt->is_present = true;
        return opt->callback(self, opt, opt->callback_data);
    } else {
        if (opt->short_name == 'h') {
            cex_argparse_usage(self);
            return Error.argsparse;
        }
    }

    return Error.ok;
}

static Exception
_cex_argparse__options_check(argparse_c* self, bool reset)
{
    for$eachp(opt, self->options, self->options_len)
    {
        if (opt->type != CexArgParseType__group) {
            if (reset) {
                opt->is_present = 0;
                if (!(opt->short_name || opt->long_name)) {
                    uassert(
                        (opt->short_name || opt->long_name) && "options both long/short_name NULL"
                    );
                    return Error.argument;
                }
                if (opt->value == NULL && opt->short_name != 'h') {
                    uassertf(
                        opt->value != NULL,
                        "option value [%c/%s] is null\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argument;
                }
            } else {
                if (opt->required && !opt->is_present) {
                    cexsp__fprintf(
                        stdout,
                        "Error: missing required option: -%c/--%s\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argsparse;
                }
            }
        }

        switch (opt->type) {
            case CexArgParseType__group:
                continue;
            case CexArgParseType__boolean:
            case CexArgParseType__string: {
                uassert(opt->convert == NULL && "unexpected to be set for strings/bools");
                uassert(opt->callback == NULL && "unexpected to be set for strings/bools");
                break;
            }
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                uassert(opt->convert != NULL && "expected to be set for standard args");
                uassert(opt->callback == NULL && "unexpected to be set for standard args");
                continue;
            case CexArgParseType__generic:
                uassert(opt->convert == NULL && "unexpected to be set for generic args");
                uassert(opt->callback != NULL && "expected to be set for generic args");
                continue;
            default:
                uassertf(false, "wrong option type: %d", opt->type);
        }
    }

    return Error.ok;
}

static Exception
_cex_argparse__short_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        if (options->short_name == *self->_ctx.optvalue) {
            self->_ctx.optvalue = self->_ctx.optvalue[1] ? self->_ctx.optvalue + 1 : NULL;
            return _cex_argparse__getvalue(self, options, false);
        }
    }
    return Error.not_found;
}

static Exception
_cex_argparse__long_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        char* rest;
        if (!options->long_name) { continue; }
        rest = _cex_argparse__prefix_skip(self->argv[0] + 2, options->long_name);
        if (!rest) {
            if (options->type != CexArgParseType__boolean) { continue; }
            rest = _cex_argparse__prefix_skip(self->argv[0] + 2 + 3, options->long_name);
            if (!rest) { continue; }
        }
        if (*rest) {
            if (*rest != '=') { continue; }
            self->_ctx.optvalue = rest + 1;
        }
        return _cex_argparse__getvalue(self, options, true);
    }
    return Error.not_found;
}


static Exception
_cex_argparse__report_error(argparse_c* self, Exc err)
{
    // invalidate argc
    self->argc = 0;

    if (err == Error.not_found) {
        if (self->options != NULL) {
            io.printf("error: unknown option `%s`\n", self->argv[0]);
        } else {
            cexsp__fprintf(
                stdout,
                "error: command name expected, got option `%s`, try --help\n",
                self->argv[0]
            );
        }
    } else if (err == Error.integrity) {
        io.printf("error: option `%s` follows argument\n", self->argv[0]);
    }
    return Error.argsparse;
}

static Exception
_cex_argparse__parse_commands(argparse_c* self)
{
    uassert(self->_ctx.current_command == NULL);
    if (self->commands_len == 0) {
        argparse_cmd_s* _cmd = self->commands;
        while (_cmd != NULL) {
            if (_cmd->name == NULL) { break; }
            self->commands_len++;
            _cmd++;
        }
    }

    argparse_cmd_s* cmd = NULL;
    char* cmd_arg = (self->argc > 0) ? self->argv[0] : NULL;

    if (str.eq(cmd_arg, "-h") || str.eq(cmd_arg, "--help")) {
        cex_argparse_usage(self);
        return Error.argsparse;
    }

    for$eachp(c, self->commands, self->commands_len)
    {
        uassert(c->func != NULL && "missing command funcion");
        uassert(c->help != NULL && "missing command help message");
        if (cmd_arg != NULL) {
            if (str.eq(cmd_arg, c->name)) {
                cmd = c;
                break;
            }
        } else {
            if (c->is_default) {
                uassert(cmd == NULL && "multiple default commands in argparse_c");
                cmd = c;
            }
        }
    }
    if (cmd == NULL) {
        cex_argparse_usage(self);
        io.printf("error: unknown command name '%s', try --help\n", (cmd_arg) ? cmd_arg : "");
        return Error.argsparse;
    }
    self->_ctx.current_command = cmd;
    self->_ctx.cpidx = 0;

    return EOK;
}

static Exception
_cex_argparse__parse_options(argparse_c* self)
{
    if (self->options_len == 0) {
        argparse_opt_s* _opt = self->options;
        while (_opt != NULL) {
            if (_opt->type == CexArgParseType__na) { break; }
            self->options_len++;
            _opt++;
        }
    }
    int initial_argc = self->argc + 1;
    e$except_silent (err, _cex_argparse__options_check(self, true)) { return err; }

    for (; self->argc; self->argc--, self->argv++) {
        char* arg = self->argv[0];
        if (arg[0] != '-' || !arg[1]) {
            self->_ctx.has_argument = true;

            if (self->commands != 0) {
                self->argc--;
                self->argv++;
                break;
            }
            continue;
        }
        // short option
        if (arg[1] != '-') {
            if (self->_ctx.has_argument) {
                // Breaking when first argument appears (more flexible support of subcommands)
                break;
            }

            self->_ctx.optvalue = arg + 1;
            self->_ctx.cpidx++;
            e$except_silent (err, _cex_argparse__short_opt(self, self->options)) {
                return _cex_argparse__report_error(self, err);
            }
            while (self->_ctx.optvalue) {
                e$except_silent (err, _cex_argparse__short_opt(self, self->options)) {
                    return _cex_argparse__report_error(self, err);
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->argc--;
            self->argv++;
            self->_ctx.cpidx++;
            break;
        }
        // long option
        if (self->_ctx.has_argument) {
            // Breaking when first argument appears (more flexible support of subcommands)
            break;
        }
        e$except_silent (err, _cex_argparse__long_opt(self, self->options)) {
            return _cex_argparse__report_error(self, err);
        }
        self->_ctx.cpidx++;
        continue;
    }

    e$except_silent (err, _cex_argparse__options_check(self, false)) { return err; }

    self->argv = self->_ctx.out + self->_ctx.cpidx + 1; // excludes 1st argv[0], program_name
    self->argc = initial_argc - self->_ctx.cpidx - 1;
    self->_ctx.cpidx = 0;

    return EOK;
}

static Exception
cex_argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options != NULL && self->commands != NULL) {
        uassert(false && "options and commands are mutually exclusive");
        return Error.integrity;
    }
    uassert(argc > 0);
    uassert(argv != NULL);
    uassert(argv[0] != NULL);

    if (self->program_name == NULL) { self->program_name = argv[0]; }

    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->argc = argc - 1;
    self->argv = argv + 1;
    self->_ctx.out = argv;

    if (self->commands) {
        return _cex_argparse__parse_commands(self);
    } else if (self->options) {
        return _cex_argparse__parse_options(self);
    }
    return Error.ok;
}

static char*
cex_argparse_next(argparse_c* self)
{
    uassert(self != NULL);
    uassert(self->argv != NULL && "forgot argparse.parse() call?");

    auto result = self->argv[0];
    if (self->argc > 0) {

        if (self->_ctx.cpidx > 0) {
            // we have --opt=foo, return 'foo' part
            result = self->argv[0] + self->_ctx.cpidx + 1;
            self->_ctx.cpidx = 0;
        } else {
            if (str.starts_with(result, "--")) {
                char* eq = str.find(result, "=");
                if (eq) {
                    static char part_arg[128]; // temp buffer sustained after scope exit
                    self->_ctx.cpidx = eq - result;
                    if ((usize)self->_ctx.cpidx + 1 >= sizeof(part_arg)) { return NULL; }
                    if (str.copy(part_arg, result, sizeof(part_arg))) { return NULL; }
                    part_arg[self->_ctx.cpidx] = '\0';
                    return part_arg;
                }
            }
        }
        self->argc--;
        self->argv++;
    }

    if (unlikely(self->argc == 0)) {
        // After reaching argc=0, argv getting stack-overflowed (ASAN issues), we set to fake NULL
        static char* null_argv[] = { NULL };
        // reset NULL every call, because static null_argv may be overwritten in user code maybe
        null_argv[0] = NULL;
        self->argv = null_argv;
    }
    return result;
}

static Exception
cex_argparse_run_command(argparse_c* self, void* user_ctx)
{
    uassert(self->_ctx.current_command != NULL && "not parsed/parse error?");
    if (self->argc == 0) {
        // seems default command (with no args)
        char* dummy_args[] = { self->_ctx.current_command->name };
        return self->_ctx.current_command->func(1, (char**)dummy_args, user_ctx);
    } else {
        return self->_ctx.current_command->func(self->argc, (char**)self->argv, user_ctx);
    }
}


CEX_NAMESPACE_DEF struct __cex_namespace__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off

    .next = cex_argparse_next,
    .parse = cex_argparse_parse,
    .run_command = cex_argparse_run_command,
    .usage = cex_argparse_usage,

    // clang-format on
};
#endif
