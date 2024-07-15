/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#ifndef ARGPARSE_H
#define ARGPARSE_H

#include "cex.h"
#include <stdint.h>

struct argparse_c;
struct argparse_option_s;

typedef Exception argparse_callback(struct argparse_c* self, const struct argparse_option_s* option);


enum argparse_flag
{
    ARGPARSE_STOP_AT_NON_OPTION = 1 << 0,
    ARGPARSE_IGNORE_UNKNOWN_ARGS = 1 << 1,
};

enum argparse_option_type
{
    /* special */
    ARGPARSE_OPT_END,
    ARGPARSE_OPT_GROUP,
    /* options with no arguments */
    ARGPARSE_OPT_BOOLEAN,
    ARGPARSE_OPT_BIT,
    /* options with arguments (optional or required) */
    ARGPARSE_OPT_INTEGER,
    ARGPARSE_OPT_FLOAT,
    ARGPARSE_OPT_STRING,
};

enum argparse_option_flags
{
    OPT_NONEG = 1, /* disable negation */
};
/**
 *  argparse option
 *
 *  `type`:
 *    holds the type of the option, you must have an ARGPARSE_OPT_END last in your
 *    array.
 *
 *  `short_name`:
 *    the character to use as a short option name, '\0' if none.
 *
 *  `long_name`:
 *    the long option name, without the leading dash, NULL if none.
 *
 *  `value`:
 *    stores pointer to the value to be filled.
 *
 *  `help`:
 *    the short help message associated to what the option does.
 *    Must never be NULL (except for ARGPARSE_OPT_END).
 *
 *  `callback`:
 *    function is called when corresponding argument is parsed.
 *
 *  `data`:
 *    associated data. Callbacks can use it like they want.
 *
 *  `flags`:
 *    option flags.
 */
typedef struct argparse_option_s
{
    enum argparse_option_type type;
    const char short_name;
    const char* long_name;
    void* value;
    const char* help;
    argparse_callback* callback;
    intptr_t data;
    int flags;
} argparse_option_s;

/**
 * argpparse
 */
typedef struct argparse_c
{
    // user supplied
    const argparse_option_s* options;
    const char* const* usages;
    int flags;
    const char* description; // a description after usage
    const char* epilog;      // a description at the end
    // internal context
    int argc;
    char** argv;
    char** out;
    int cpidx;
    const char* optvalue; // current option value
} argparse_c;


// built-in option macros
// clang-format off
#define argparse$opt_end()        { ARGPARSE_OPT_END, 0, NULL, NULL, 0, NULL, 0, 0 }
#define argparse$opt_bool(...) { ARGPARSE_OPT_BOOLEAN, __VA_ARGS__ }
#define argparse$opt_bit(...)     { ARGPARSE_OPT_BIT, __VA_ARGS__ }
#define argparse$opt_integer(...) { ARGPARSE_OPT_INTEGER, __VA_ARGS__ }
#define argparse$opt_float(...)   { ARGPARSE_OPT_FLOAT, __VA_ARGS__ }
#define argparse$opt_string(...)  { ARGPARSE_OPT_STRING, __VA_ARGS__ }
#define argparse$opt_group(h)     { ARGPARSE_OPT_GROUP, 0, NULL, NULL, h, NULL, 0, 0 }
#define argparse$opt_help()       argparse$opt_bool('h', "help", NULL,                 \
                                     "show this help message and exit", \
                                     argparse__help_cb, 0, OPT_NONEG)
// clang-format on

#endif
