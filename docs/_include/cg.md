

### Code generation

Code generation macros for emitting formatted source code into an `sbuf_c`.

`cg$init()` declares the shared `cg$var` in the current scope; `cg$init_scope()` keeps it local to a block.

#### Example

```c
Exception
generate_my_code(sbuf_c* b)
{
    // NOTE: cg$ macros must be used inside a cg$init() scope, or cg$var must be in scope
    cg$init(b);

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

    if (!cg$is_valid()) { e$raise(cg$var->error, "code generation failed"); }

    return EOK;
}
```

```c
sbuf_c b = sbuf.create(1024, mem$);
e$ret(generate_my_code(&b));

io.printf("result: \n%s\n", b);

sbuf.destroy(&b);
```



```c
/// add case in switch() statement
#define cg$case(format, ...)

/// decrease code indent by 4
#define cg$dedent()

/// add default in switch() statement
#define cg$default()

/// add else
#define cg$else()

/// add else if
#define cg$elseif(format, ...)

/// add for loop
#define cg$for(format, ...)

/// add CEX for$each loop
#define cg$foreach(format, ...)

/// add new function     cg$func("void my_func(int arg_%d)", 2)
#define cg$func(format, ...)

/// add if statement
#define cg$if(format, ...)

/// increase code indent by 4
#define cg$indent()

/// Initializes new code generator (uses sbuf instance as backing buffer)
#define cg$init(out_sbuf)

/// Initializes new code generator within a scope (auto cleanup at scope exit)
#define cg$init_scope(out_sbuf)

/// false if any cg$ operation failed, use cg$var->error to get Exception type of error
#define cg$is_valid()

/// append code at the current line without "\n"
#define cg$pa(format, ...)

/// add new line of code with formatting
#define cg$pf(format, ...)

/// add new line of code
#define cg$pn(text)

#define cg$printva(cg)

/// add new code scope with indent (use for low-level stuff)
#define cg$scope(format, ...)

/// add switch() statement
#define cg$switch(format, ...)

/// Common code gen buffer variable (all cg$ macros use it under the hood)
#define cg$var

/// add while loop
#define cg$while(format, ...)




```
