/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#include "argparse.h"
#include "cex.h"
#include "str.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const u32 ARGPARSE_OPT_UNSET = 1;
const u32 ARGPARSE_OPT_LONG = (1 << 1);

// static const u8 1 /*ARGPARSE_OPT_GROUP*/  = 1;
// static const u8 2 /*ARGPARSE_OPT_BOOLEAN*/ = 2;
// static const u8 3 /*ARGPARSE_OPT_BIT*/ = 3;
// static const u8 4 /*ARGPARSE_OPT_INTEGER*/ = 4;
// static const u8 5 /*ARGPARSE_OPT_FLOAT*/ = 5;
// static const u8 6 /*ARGPARSE_OPT_STRING*/ = 6;

static const char*
argparse__prefix_skip(const char* str, const char* prefix)
{
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static int
argparse__prefix_cmp(const char* str, const char* prefix)
{
    for (;; str++, prefix++) {
        if (!*prefix) {
            return 0;
        } else if (*str != *prefix) {
            return (unsigned char)*prefix - (unsigned char)*str;
        }
    }
}

static Exception
argparse__error(argparse_c* self, const argparse_opt_s* opt, const char* reason, int flags)
{
    (void)self;
    if (flags & ARGPARSE_OPT_LONG) {
        fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

void
argparse_usage(argparse_c* self)
{
    uassert(self->_ctx.argv != NULL && "usage before parse!");

    fprintf(stdout, "Usage:\n");
    if (self->usage) {

        for$iter(str_c, it, str.iter_split(s$(self->usage), "\n", &it.iterator))
        {
            if (it.val->len == 0) {
                break;
            }

            char* fn = strrchr(self->program_name, '/');
            if (fn != NULL) {
                fprintf(stdout, "%s ", fn + 1);
            } else {
                fprintf(stdout, "%s ", self->program_name);
            }

            if (fwrite(it.val->buf, sizeof(char), it.val->len, stdout)) {
                ;
            }

            fputc('\n', stdout);
        }
    } else {
        fprintf(stdout, "%s [options] [--] [arg1 argN]\n", self->program_name);
    }

    // print description
    if (self->description) {
        fprintf(stdout, "%s\n", self->description);
    }

    fputc('\n', stdout);

    const argparse_opt_s* options;

    // figure out best width
    size_t usage_opts_width = 0;
    size_t len;
    options = self->options;
    for (u32 i = 0; i < self->options_len; i++, options++) {
        len = 0;
        if ((options)->short_name) {
            len += 2;
        }
        if ((options)->short_name && (options)->long_name) {
            len += 2; // separator ", "
        }
        if ((options)->long_name) {
            len += strlen((options)->long_name) + 2;
        }
        if (options->type == 4 /*ARGPARSE_OPT_INTEGER*/) {
            len += strlen("=<int>");
        }
        if (options->type == 5 /*ARGPARSE_OPT_FLOAT*/) {
            len += strlen("=<flt>");
        } else if (options->type == 6 /*ARGPARSE_OPT_STRING*/) {
            len += strlen("=<str>");
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4; // 4 spaces prefix

    options = self->options;
    for (u32 i = 0; i < self->options_len; i++, options++) {
        size_t pos = 0;
        size_t pad = 0;
        if (options->type == 1 /*ARGPARSE_OPT_GROUP*/) {
            fputc('\n', stdout);
            fprintf(stdout, "%s", options->help);
            fputc('\n', stdout);
            continue;
        }
        pos = fprintf(stdout, "    ");
        if (options->short_name) {
            pos += fprintf(stdout, "-%c", options->short_name);
        }
        if (options->long_name && options->short_name) {
            pos += fprintf(stdout, ", ");
        }
        if (options->long_name) {
            pos += fprintf(stdout, "--%s", options->long_name);
        }
        if (options->type == 4 /*ARGPARSE_OPT_INTEGER*/) {
            pos += fprintf(stdout, "=<int>");
        } else if (options->type == 5 /*ARGPARSE_OPT_FLOAT*/) {
            pos += fprintf(stdout, "=<flt>");
        } else if (options->type == 6 /*ARGPARSE_OPT_STRING*/) {
            pos += fprintf(stdout, "=<str>");
        }
        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        fprintf(stdout, "%*s%s\n", (int)pad + 2, "", options->help);
    }

    // print epilog
    if (self->epilog) {
        fprintf(stdout, "%s\n", self->epilog);
    }
}
static Exception
argparse__getvalue(argparse_c* self, argparse_opt_s* opt, int flags)
{
    const char* s = NULL;
    if (!opt->value) {
        goto skipped;
    }

    switch (opt->type) {
        case 2 /*ARGPARSE_OPT_BOOLEAN*/:
            if (flags & ARGPARSE_OPT_UNSET) {
                *(int*)opt->value = 0;
            } else {
                *(int*)opt->value = 1;
            }
            opt->is_present = true;
            break;
        case 3 /*ARGPARSE_OPT_BIT*/:
            if (flags & ARGPARSE_OPT_UNSET) {
                *(int*)opt->value &= ~opt->data;
            } else {
                *(int*)opt->value |= opt->data;
            }
            opt->is_present = true;
            break;
        case 6 /*ARGPARSE_OPT_STRING*/:
            if (self->_ctx.optvalue) {
                *(const char**)opt->value = self->_ctx.optvalue;
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                *(const char**)opt->value = *++self->_ctx.argv;
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            opt->is_present = true;
            break;
        case 4 /*ARGPARSE_OPT_INTEGER*/:
            errno = 0;
            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", flags);
                }

                *(int*)opt->value = strtol(self->_ctx.optvalue, (char**)&s, 0);
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                *(int*)opt->value = strtol(*++self->_ctx.argv, (char**)&s, 0);
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) {
                return argparse__error(self, opt, "numerical result out of range", flags);
            }
            if (s[0] != '\0') { // no digits or contains invalid characters
                return argparse__error(self, opt, "expects an integer value", flags);
            }
            opt->is_present = true;
            break;
        case 5 /*ARGPARSE_OPT_FLOAT*/:
            errno = 0;

            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", flags);
                }
                *(float*)opt->value = strtof(self->_ctx.optvalue, (char**)&s);
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                *(float*)opt->value = strtof(*++self->_ctx.argv, (char**)&s);
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) {
                return argparse__error(self, opt, "numerical result out of range", flags);
            }
            if (s[0] != '\0') { // no digits or contains invalid characters
                return argparse__error(self, opt, "expects a numerical value", flags);
            }
            opt->is_present = true;
            break;
        default:
            uassert(false && "unhandled");
            return Error.sanity_check;
    }

skipped:
    if (opt->callback) {
        opt->is_present = true;
        return opt->callback(self, opt);
    } else {
        if (opt->short_name == 'h') {
            argparse_usage(self);
            return Error.argsparse;
        }
    }

    return Error.ok;
}

static Exception
argparse__options_check(argparse_c* self, bool reset)
{
    var options = self->options;

    for (u32 i = 0; i < self->options_len; i++, options++) {
        switch (options->type) {
            case 1 /*ARGPARSE_OPT_GROUP*/:
                continue;
            case 2 /*ARGPARSE_OPT_BOOLEAN*/:
            case 3 /*ARGPARSE_OPT_BIT*/:
            case 4 /*ARGPARSE_OPT_INTEGER*/:
            case 5 /*ARGPARSE_OPT_FLOAT*/:
            case 6 /*ARGPARSE_OPT_STRING*/:
                if (reset) {
                    options->is_present = 0;
                    if (!(options->short_name || options->long_name)) {
                        uassert(
                            (options->short_name || options->long_name) &&
                            "options both long/short_name NULL"
                        );
                        return Error.sanity_check;
                    }
                    if (options->value == NULL && options->short_name != 'h') {
                        uassertf(
                            options->value != NULL,
                            "option value [%c/%s] is null\n",
                            options->short_name,
                            options->long_name
                        );
                        return Error.sanity_check;
                    }
                } else {
                    if (options->required && !options->is_present) {
                        fprintf(
                            stdout,
                            "Error: missing required option: -%c/--%s\n",
                            options->short_name,
                            options->long_name
                        );
                        return Error.argsparse;
                    }
                }
                continue;
            default:
                uassertf(false, "wrong option type: %d", options->type);
        }
    }

    return Error.ok;
}

static Exception
argparse__short_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        if (options->short_name == *self->_ctx.optvalue) {
            self->_ctx.optvalue = self->_ctx.optvalue[1] ? self->_ctx.optvalue + 1 : NULL;
            return argparse__getvalue(self, options, 0);
        }
    }
    return Error.not_found;
}

static Exception
argparse__long_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        const char* rest;
        int opt_flags = 0;
        if (!options->long_name) {
            continue;
        }

        rest = argparse__prefix_skip(self->_ctx.argv[0] + 2, options->long_name);
        if (!rest) {
            // negation disabled?
            if (options->flags & ARGPARSE_OPT_NONEG) {
                continue;
            }
            // only OPT_BOOLEAN/OPT_BIT supports negation
            if (options->type != 2 /*ARGPARSE_OPT_BOOLEAN*/ &&
                options->type != 3 /*ARGPARSE_OPT_BIT*/) {
                continue;
            }

            if (argparse__prefix_cmp(self->_ctx.argv[0] + 2, "no-")) {
                continue;
            }
            rest = argparse__prefix_skip(self->_ctx.argv[0] + 2 + 3, options->long_name);
            if (!rest) {
                continue;
            }
            opt_flags |= ARGPARSE_OPT_UNSET;
        }
        if (*rest) {
            if (*rest != '=') {
                continue;
            }
            self->_ctx.optvalue = rest + 1;
        }
        return argparse__getvalue(self, options, opt_flags | ARGPARSE_OPT_LONG);
    }
    return Error.not_found;
}


static Exception
argparse__report_error(argparse_c* self, Exc err)
{
    // invalidate argc
    self->_ctx.argc = 0;

    if (err == Error.not_found) {
        fprintf(stdout, "error: unknown option `%s`\n", self->_ctx.argv[0]);
    } else if (err == Error.integrity) {
        fprintf(stdout, "error: option `%s` follows argument\n", self->_ctx.argv[0]);
    }
    return Error.argsparse;
}

Exception
argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options_len == 0) {
        return "zero options provided or self.options_len field not set";
    }
    uassert(argc > 0);
    uassert(argv != NULL);
    uassert(argv[0] != NULL);

    if (self->program_name == NULL) {
        self->program_name = argv[0];
    }

    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->_ctx.argc = argc - 1;
    self->_ctx.argv = argv + 1;
    self->_ctx.out = argv;

    except_silent(err, argparse__options_check(self, true))
    {
        return err;
    }

    for (; self->_ctx.argc; self->_ctx.argc--, self->_ctx.argv++) {
        const char* arg = self->_ctx.argv[0];
        if (arg[0] != '-' || !arg[1]) {
            self->_ctx.has_argument = true;

            if (self->flags.stop_at_non_option) {
                self->_ctx.argc--;
                self->_ctx.argv++;
                break;
            }
            continue;
        }
        // short option
        if (arg[1] != '-') {
            if (self->_ctx.has_argument) {
                // options are not allowed after arguments
                return argparse__report_error(self, Error.integrity);
            }

            self->_ctx.optvalue = arg + 1;
            self->_ctx.cpidx++;
            except_silent(err, argparse__short_opt(self, self->options))
            {
                return argparse__report_error(self, err);
            }
            while (self->_ctx.optvalue) {
                except_silent(err, argparse__short_opt(self, self->options))
                {
                    return argparse__report_error(self, err);
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->_ctx.argc--;
            self->_ctx.argv++;
            self->_ctx.cpidx++;
            break;
        }
        // long option
        if (self->_ctx.has_argument) {
            // options are not allowed after arguments
            return argparse__report_error(self, Error.integrity);
        }
        except_silent(err, argparse__long_opt(self, self->options))
        {
            return argparse__report_error(self, err);
        }
        self->_ctx.cpidx++;
        continue;
    }

    except_silent(err, argparse__options_check(self, false))
    {
        return err;
    }

    self->_ctx.out += self->_ctx.cpidx + 1; // excludes 1st argv[0], program_name
    self->_ctx.argc = argc - self->_ctx.cpidx - 1;

    return Error.ok;
}

u32
argparse_argc(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.argc;
}

char**
argparse_argv(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.out;
}


const struct __module__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off
    .usage = argparse_usage,
    .parse = argparse_parse,
    .argc = argparse_argc,
    .argv = argparse_argv,
    // clang-format on
};