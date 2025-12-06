#if !defined(cex$enable_minimal)

#pragma once
#include "all.h"

typedef struct cex_codegen_s
{
    sbuf_c* buf;
    u32 indent;
    Exc error;
} cex_codegen_s;

/*
 *                  CODE GEN MACROS
 */

/**
* Code generation module 

```c

test$case(test_codegen_test)
{
    sbuf_c b = sbuf.create(1024, mem$);
    // NOTE: cg$ macros should be working within cg$init() scope or make sure cg$var is available
    cg$init(&b);

    tassert(cg$var->buf == &b);
    tassert(cg$var->indent == 0);

    cg$pn("printf(\"hello world\");");
    cg$pn("#define GOO");
    cg$pn("// this is empty scope");
    cg$scope("", "")
    {
        cg$pf("printf(\"hello world: %d\");", 2);
    }

    cg$func("void my_func(int arg_%d)", 2)
    {
        cg$scope("var my_var = (mytype)", "")
        {
            cg$pf(".arg1 = %d,", 1);
            cg$pf(".arg2 = %d,", 2);
        }
        cg$pa(";\n", "");

        cg$if("foo == %d", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
        }
        cg$elseif("bar == foo + %d", 7)
        {
            cg$pn("// else if scope");
        }
        cg$else()
        {
            cg$pn("// else scope");
        }

        cg$while("foo == %d", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
        }

        cg$for("u32 i = 0; i < %d; i++", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
            cg$foreach("it, my_var", "")
            {
                cg$pn("printf(\"Hello: %d\", foo);");
            }
        }

        cg$scope("do ", "")
        {
            cg$pf("// do while", 1);
        }
        cg$pa(" while(0);\n", "");
    }

    cg$switch("foo", "")
    {
        cg$case("'%c'", 'a')
        {
            cg$pn("// case scope");
        }
        cg$scope("case '%c': ", 'b')
        {
            cg$pn("fallthrough();");
        }
        cg$default()
        {
            cg$pn("// default scope");
        }
    }

    tassert(cg$is_valid());

    printf("result: \n%s\n", b);


    sbuf.destroy(&b);
    return EOK;
}

```

*/
#define __cg$

/// Common code gen buffer variable (all cg$ macros use it under the hood)
#    define cg$var __cex_code_gen

/// Initializes new code generator (uses sbuf instance as backing buffer)
#    define cg$init(out_sbuf)                                                                      \
        cex_codegen_s cex$tmpname(code_gen) = { .buf = (out_sbuf) };                             \
        cex_codegen_s* cg$var = &cex$tmpname(code_gen)

#    define cg$init_scope(out_sbuf)                                                                      \
    cex_codegen_s cex$tmpname(code_gen) = { .buf = (out_sbuf) };                             \
    for (cex_codegen_s* cg$var = &cex$tmpname(code_gen), \
             *cex$tmpname(cg_scope_init_sentinel) = cg$var;                                        \
         cex$tmpname(cg_scope_init_sentinel) && cg$var != NULL;                                    \
         cex$tmpname(cg_scope_init_sentinel) = NULL)

/// false if any cg$ operation failed, use cg$var->error to get Exception type of error
#    define cg$is_valid() (cg$var != NULL && cg$var->buf != NULL && cg$var->error == EOK)

/// increase code indent by 4
#    define cg$indent() ({ cg$var->indent += 4; })

/// decrease code indent by 4
#    define cg$dedent()                                                                            \
        ({                                                                                         \
            if (cg$var->indent >= 4) cg$var->indent -= 4;                                          \
        })

/*
 *                  CODE MACROS
 */
/// add new line of code
#    define cg$pn(text)                                                                            \
        ((text && text[0] == '\0') ? _cex__codegen_print_line(cg$var, "\n")                        \
                                   : _cex__codegen_print_line(cg$var, "%s\n", text))

/// add new line of code with formatting
#    define cg$pf(format, ...) _cex__codegen_print_line(cg$var, format "\n", ##__VA_ARGS__)

/// append code at the current line without "\n"
#    define cg$pa(format, ...) _cex__codegen_print(cg$var, true, format, ##__VA_ARGS__)

// clang-format off

/// add new code scope with indent (use for low-level stuff)
#define cg$scope(format, ...) \
    for (cex_codegen_s* cex$tmpname(codegen_scope)  \
                __attribute__ ((__cleanup__(_cex__codegen_print_scope_exit))) =     \
                _cex__codegen_print_scope_enter(cg$var, format, ##__VA_ARGS__),       \
        *cex$tmpname(codegen_sentinel) = cg$var;                                    \
        cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;        \
        cex$tmpname(codegen_sentinel) = NULL)
// clang-format on


/// add new function     cg$func("void my_func(int arg_%d)", 2)
#    define cg$func(format, ...) cg$scope(format, ##__VA_ARGS__)

/// add if statement
#    define cg$if(format, ...) cg$scope("if (" format ") ", ##__VA_ARGS__)

/// add else if
#    define cg$elseif(format, ...)                                                                   \
        cg$pa(" else ", "");                                                                       \
        cg$if(format, ##__VA_ARGS__)

/// add else 
#    define cg$else()                                                                                \
        cg$pa(" else", "");                                                                        \
        cg$scope(" ", "")

/// add while loop
#    define cg$while(format, ...) cg$scope("while (" format ") ", ##__VA_ARGS__)

/// add for loop
#    define cg$for(format, ...) cg$scope("for (" format ") ", ##__VA_ARGS__)

/// add CEX for$each loop
#    define cg$foreach(format, ...) cg$scope("for$each (" format ") ", ##__VA_ARGS__)

/// add switch() statement
#    define cg$switch(format, ...) cg$scope("switch (" format ") ", ##__VA_ARGS__)

/// add case in switch() statement
#    define cg$case(format, ...)                                                                     \
        for (cex_codegen_s * cex$tmpname(codegen_scope)                                          \
                                   __attribute__((__cleanup__(_cex__codegen_print_case_exit))) =   \
                 _cex__codegen_print_case_enter(cg$var, "case " format, ##__VA_ARGS__),              \
                                   *cex$tmpname(codegen_sentinel) = cg$var;                        \
             cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;                  \
             cex$tmpname(codegen_sentinel) = NULL)

/// add default in switch() statement
#    define cg$default()                                                                             \
        for (cex_codegen_s * cex$tmpname(codegen_scope)                                          \
                                   __attribute__((__cleanup__(_cex__codegen_print_case_exit))) =   \
                 _cex__codegen_print_case_enter(cg$var, "default", NULL),                          \
                                   *cex$tmpname(codegen_sentinel) = cg$var;                        \
             cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;                  \
             cex$tmpname(codegen_sentinel) = NULL)


void _cex__codegen_print_line(cex_codegen_s* cg, char* format, ...);
void _cex__codegen_print(cex_codegen_s* cg, bool rep_new_line, char* format, ...);
cex_codegen_s* _cex__codegen_print_scope_enter(cex_codegen_s* cg, char* format, ...);
void _cex__codegen_print_scope_exit(cex_codegen_s** cgptr);
cex_codegen_s* _cex__codegen_print_case_enter(cex_codegen_s* cg, char* format, ...);
void _cex__codegen_print_case_exit(cex_codegen_s** cgptr);
void _cex__codegen_indent(cex_codegen_s* cg);

#endif
