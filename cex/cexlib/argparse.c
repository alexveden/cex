/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#include "argparse.h"
#include "cex.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPT_UNSET 1
#define OPT_LONG (1 << 1)

static const char*
prefix_skip(const char* str, const char* prefix)
{
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static int
prefix_cmp(const char* str, const char* prefix)
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
    if (flags & OPT_LONG) {
        fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

static Exception
argparse__getvalue(argparse_c* self, argparse_opt_s* opt, int flags)
{
    const char* s = NULL;
    if (!opt->value) {
        goto skipped;
    }

    switch (opt->type) {
        case ARGPARSE_OPT_BOOLEAN:
            if (flags & OPT_UNSET) {
                *(int*)opt->value = *(int*)opt->value - 1;
            } else {
                *(int*)opt->value = *(int*)opt->value + 1;
            }
            if (*(int*)opt->value < 0) {
                *(int*)opt->value = 0;
            }
            opt->is_present = true;
            break;
        case ARGPARSE_OPT_BIT:
            if (flags & OPT_UNSET) {
                *(int*)opt->value &= ~opt->data;
            } else {
                *(int*)opt->value |= opt->data;
            }
            opt->is_present = true;
            break;
        case ARGPARSE_OPT_STRING:
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
        case ARGPARSE_OPT_INTEGER:
            errno = 0;
            if (self->_ctx.optvalue) {
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
        case ARGPARSE_OPT_FLOAT:
            errno = 0;
            if (self->_ctx.optvalue) {
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
    }

    return Error.ok;
}

static Exception
argparse__options_check(argparse_c* self, bool reset)
{
    var options = self->options;

    for (u32 i = 0; i < self->options_len; i++, options++) {
        switch (options->type) {
            case ARGPARSE_OPT_GROUP:
                continue;
            case ARGPARSE_OPT_BOOLEAN:
            case ARGPARSE_OPT_BIT:
            case ARGPARSE_OPT_INTEGER:
            case ARGPARSE_OPT_FLOAT:
            case ARGPARSE_OPT_STRING:
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
    return Error.sanity_check;
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

        rest = prefix_skip(self->_ctx.argv[0] + 2, options->long_name);
        if (!rest) {
            // negation disabled?
            if (options->flags & OPT_NONEG) {
                continue;
            }
            // only OPT_BOOLEAN/OPT_BIT supports negation
            if (options->type != ARGPARSE_OPT_BOOLEAN && options->type != ARGPARSE_OPT_BIT) {
                continue;
            }

            if (prefix_cmp(self->_ctx.argv[0] + 2, "no-")) {
                continue;
            }
            rest = prefix_skip(self->_ctx.argv[0] + 2 + 3, options->long_name);
            if (!rest) {
                continue;
            }
            opt_flags |= OPT_UNSET;
        }
        if (*rest) {
            if (*rest != '=') {
                continue;
            }
            self->_ctx.optvalue = rest + 1;
        }
        return argparse__getvalue(self, options, opt_flags | OPT_LONG);
    }
    return Error.argument;
}

void
argparse_usage(argparse_c* self)
{
    if (self->usage) {
        fprintf(stdout, "Usage: \n%s\n", self->usage);
    } else {
        fprintf(stdout, "Usage:\n");
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
        if (options->type == ARGPARSE_OPT_INTEGER) {
            len += strlen("=<int>");
        }
        if (options->type == ARGPARSE_OPT_FLOAT) {
            len += strlen("=<flt>");
        } else if (options->type == ARGPARSE_OPT_STRING) {
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
        if (options->type == ARGPARSE_OPT_GROUP) {
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
        if (options->type == ARGPARSE_OPT_INTEGER) {
            pos += fprintf(stdout, "=<int>");
        } else if (options->type == ARGPARSE_OPT_FLOAT) {
            pos += fprintf(stdout, "=<flt>");
        } else if (options->type == ARGPARSE_OPT_STRING) {
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

Exception
argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options_len == 0) {
        return raise_exc(
            Error.sanity_check,
            "zero options provided or self.options_len field not set\n"
        );
    }
    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->_ctx.argc = argc - 1;
    self->_ctx.argv = argv + 1;
    self->_ctx.out = argv;

    except(err, argparse__options_check(self, true))
    {
        return err;
    }

    for (; self->_ctx.argc; self->_ctx.argc--, self->_ctx.argv++) {
        const char* arg = self->_ctx.argv[0];
        if (arg[0] != '-' || !arg[1]) {
            if (self->flags & ARGPARSE_STOP_AT_NON_OPTION) {
                goto end;
            }
            // if it's not option or is a single char '-', copy verbatim
            self->_ctx.out[self->_ctx.cpidx++] = self->_ctx.argv[0];
            continue;
        }
        // short option
        if (arg[1] != '-') {
            self->_ctx.optvalue = arg + 1;
            except(err, argparse__short_opt(self, self->options))
            {
                // return err;
                // case -1:
                //     break;
                // case -2:
                goto unknown;
            }
            while (self->_ctx.optvalue) {
                except(err, argparse__short_opt(self, self->options))
                {
                    // case -1:
                    //     break;
                    // case -2:
                    goto unknown;
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->_ctx.argc--;
            self->_ctx.argv++;
            break;
        }
        // long option
        except(err, argparse__long_opt(self, self->options))
        {
            // case -1:
            //     break;
            // case -2:
            goto unknown;
        }
        continue;

    unknown:
        fprintf(stdout, "error: unknown option `%s`\n", self->_ctx.argv[0]);
        argparse_usage(self);
        if (!(self->flags & ARGPARSE_IGNORE_UNKNOWN_ARGS)) {
            return Error.argsparse;
        }
    }

end:
    memmove(
        self->_ctx.out + self->_ctx.cpidx,
        self->_ctx.argv,
        self->_ctx.argc * sizeof(*self->_ctx.out)
    );
    self->_ctx.out[self->_ctx.cpidx + self->_ctx.argc] = NULL;

    except(err, argparse__options_check(self, false))
    {
        return err;
    }
    // return self->cpidx + self->argc;
    return Error.ok;
}


static Exception
argparse__help_cb_no_exit(argparse_c* self, const argparse_opt_s* option)
{
    (void)option;
    argparse_usage(self);
    return Error.ok;
}

static Exception
argparse__help_cb(argparse_c* self, const argparse_opt_s* option)
{
    if (argparse__help_cb_no_exit(self, option)) {
        ; // ignore result
    }
    return Error.argsparse;
}
const struct __module__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off
    .usage = argparse_usage,
    .parse = argparse_parse,
    // clang-format on
};