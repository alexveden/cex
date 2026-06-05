---
title: "CEX.C Language Documentation"
format:
  html:
    toc: true
    toc-location: left-body
    toc-depth: 4
    embed-resources: true
    theme: darkly
    page-layout: full
    html-math-method: plain
    highlight-style: github-dark
    css: _quarto/styles.css
    syntax-definitions:
      - _quarto/cex_syntax.xml
    filters:
      - _quarto/callouts.lua

    other-links:
      - text: cex-c.org
        href: https://cex-c.org/
      - text: Download cex
        icon: download
        href: https://cex-c.org/cex.zip
      - text: GitHub Repo
        icon: github
        href: https://github.com/alexveden/cex
      - text: Found an error (please propose edit)?
        icon: pencil
        href: https://github.com/alexveden/cex/edit/master/docs/README.md
---

## Getting started with CEX.C
### What is CEX
Cex.C (officially pronounced /ˈtsɛk.si/ "tsek-see") is Comprehensively EXtended C Language. CEX was born as an alternative answer to a plethora of brand new LLVM based languages which strive to replace old C. CEX still remains C language itself, with small but important tweaks that makes CEX a completely different development experience.

I tried to bring the best ideas from modern languages while maintaining a smooth developer experience for writing C code. The main goal of CEX is to provide tools for developers and help them write high quality C code in general.

#### Core features

- Single header, cross-platform, drop-in C language extension
- No dependencies except C compiler
- Self-contained build system: CMake/Make/Ninja no more
- Modern memory management model
- New error handling model
- New strings
- Namespaces
- Code quality oriented tools
- New dynamic arrays and hashmaps with seamless C compatibility


#### Solving old C problems

CEX is another attempt to make old C a little bit better. Unlike other new system languages like Rust, Zig, C3 which tend to start from scratch, CEX focuses on evolution process and leverages existing tools provided by modern compilers to make code safer, easy to write and debug.

| C Problem | CEX Solution |
| -------------- | --------------- |
| Bug-prone memory management | CEX provides allocator centric and scoped memory allocation. It uses ArenaAllocators and Temporary allocator in `mem$scope()` which decrease probability of memory bugs.  |
| Unsafe arrays |  Address sanitizers are enabled by default, so you'll get your crashes as in other languages. |
| 3rd party build system  |  Integrated build system, eliminates flame wars about what is better. Now you can use Cex to run your build scripts, like in `Zig`  |
| Rudimentary error handling | CEX introduces `Exception` type and compiler forces you to check it. New error handling approach makes error checking easy and opens cool possibilities like stack traces in C. |
| C is unsafe | Yeah, and it's a cool feature! On other hand, CEX provides unit testing engine and fuzz tester support out of the box.  |
| Bad string support | String operations in CEX are safe, NULL and buffer overflow resilient. CEX has dynamic string builder, slices and C compatible strings. |
| No data structures |  CEX has type-safe generic dynamic array and hashmap types, they cover 80% of all use cases. |
| No namespaces |  It's more about LSP, developer experience and readability. It's a much better experience to type and read `str.slice.starts_with` than `str_slice_starts_with`. |


### Making new CEX project

You can initialize a working boilerplate project just using a C compiler and the `cex.h` file.

> [!NOTE]
>
> Make sure that you have a C compiler installed, we use `cc` command as a default compiler. You may replace it with gcc or clang.

1. Make a project directory
```sh
mkdir project_dir
cd project_dir
```
2. Download [cex.h](https://raw.githubusercontent.com/alexveden/cex/refs/heads/master/cex.h)
3. Make a seed program

At this step we are compiling a special pre-seed program that will create a template project at the first run
```sh
cc -D CEX_NEW -x c ./cex.h -o ./cex
```
4. Run cex program for project initialization

Cex program automatically creates a project structure with sample app and unit tests. It also recompiles itself to become universal build system for the project. You may change its logic inside `cex.c` file, this is your build script now.
```sh
./cex
```
5. Now your project is ready to go

Now you can launch a sample program or run its unit tests.
```sh
./cex test run all
./cex app run myapp
```

6. This is how to check your environment and build variables
```sh
> ./cex config

cexy$* variables used in build system, see `cex help 'cexy$cc'` for more info
* CEX_LOG_LVL               4
* cexy$build_dir            ./build
* cexy$src_dir              ./examples
* cexy$cc                   cc
* cexy$cc_include           "-I."
* cexy$cc_args_sanitizer    "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined", "-fsanitize=leak", "-fstack-protector-strong"
* cexy$cc_args              "-Wall", "-Wextra", "-Werror", "-g3", "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined", "-fsanitize=leak", "-fstack-protector-strong"
* cexy$cc_args_test         "-Wall", "-Wextra", "-Werror", "-g3", "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined", "-fsanitize=leak", "-fstack-protector-strong", "-Wno-unused-function", "-Itests/"
* cexy$ld_args
* cexy$fuzzer               "clang", "-O0", "-Wall", "-Wextra", "-Werror", "-g", "-Wno-unused-function", "-fsanitize=address,fuzzer,undefined", "-fsanitize-undefined-trap-on-error"
* cexy$debug_cmd            "gdb", "-q", "--args"
* cexy$pkgconf_cmd          "pkgconf"
* cexy$pkgconf_libs
* cexy$process_ignore_kw    ""
* cexy$cex_self_args
* cexy$cex_self_cc          cc

Tools installed (optional):
* git                       OK
* cexy$pkgconf_cmd          OK ("pkgconf")
* cexy$vcpkg_root           Not set
* cexy$vcpkg_triplet        Not set

Global environment:
* Cex Version               0.14.0 (2025-06-05)
* Git Hash                  07aa036d9094bc15eac8637786df0776ca010a33
* os.platform.current()     linux
* ./cex -D<ARGS> config     ""
```

### Meet Cexy build system
`cexy$` is a build system integrated with Cex, which helps to manage your project, run tests, find symbols and get help.


```sh
> ./cex --help
Usage:
cex  [-D] [-D<ARG1>] [-D<ARG2>] command [options] [args]

CEX language (cexy$) build and project management system

help                Search cex.h and project symbols and extract help
process             Create CEX namespaces from project source code
new                 Create new CEX project
stats               Calculate project lines of code and quality stats
config              Check project and system environment and config
libfetch            Get 3rd party source code via git or install CEX libs
test                Test running
fuzz                Generic fuzz tester
app                 Generic app build/run/debug

You may try to get help for commands as well, try `cex process --help`
Use `cex -DFOO -DBAR config` to set project config flags
Use `cex -D config` to reset all project config flags to defaults
```


### Code example
#### Hello world in CEX
```c
#define CEX_IMPLEMENTATION
#include "cex.h"

int
main(int argc, char** argv)
{
    io.printf("MOCCA - Make Old C Cexy Again!\n");
    return 0;
}
```

#### Holistic function
```c
// CEX has special exception return type that forces the caller to check return type of calling
//   function, also it provides support of call stack printing on errors in vanilla C
Exception
cmd_custom_test(u32 argc, char** argv, void* user_ctx)
{
    // Let's open temporary memory allocator scope (var name is `_`)
    //  it will free all allocated memory after any exit from scope (including return or goto)
    mem$scope(tmem$, _)
    { 
        e$ret(os.fs.mkpath("tests/build/")); // make directory or return error with traceback
        e$assert(os.path.exists("tests/build/")); // evergreen assertion or error with traceback

        // auto type variables
        auto search_pattern = "tests/os_test/*.c";

        // Trace with file:<line> + formatting
        log$trace("Finding/building simple os apps in %s\n", search_pattern);

        // Search all files in the directory by wildcard pattern
        //   allocate the results (strings) on temp allocator arena `_`
        //   return dynamic array items type of `char*`
        arr$(char*) test_app_src = os.fs.find(search_pattern, false, _);

        // for$each works on dynamic, static arrays, and pointer+length
        for$each(src, test_app_src)
        {
            char* tgt_ext = NULL;
            char* test_launcher[] = { cexy$debug_cmd }; // CEX macros contain $ in their names

            // arr$len() - universal array length getter 
            //  it supports dynamic CEX arrays and static C arrays (i.e. sizeof(arr)/sizeof(arr[0]))
            if (arr$len(test_launcher) > 0 && str.eq(test_launcher[0], "wine")) {
                // str.fmt() - using allocator to sprintf() format and return new char*
                tgt_ext = str.fmt(_, ".%s", "win");
            } else {
                tgt_ext = str.fmt(_, ".%s", os.platform.to_str(os.platform.current()));
            }

            // NOTE: cexy is a build system for CEX, it contains utilities for building code
            // cexy.target_make() - makes target executable name based on source
            char* target = cexy.target_make(src, cexy$build_dir, tgt_ext, _);

            // cexy.src_include_changed - parses `src` .c/.h file, finds #include "some.h",
            //   and checks also if "some.h" is modified
            if (!cexy.src_include_changed(target, src, NULL)) {
                continue; // target is actual, source is not modified
            }

            // Launch OS command and get interactive shell
            // os.cmd. provides more capabilities for launching subprocesses and grabbing stdout
            e$ret(os$cmd(cexy$cc, "-g", "-Wall", "-Wextra", "-o", target, src));
        }
    }

    // CEX provides capabilities for generating namespaces (for user's code too!)
    // For example, cexy namespace contains
    // cexy.src_changed() - 1st level function
    // cexy.app.run() - sub-level function
    // cexy.cmd.help() - sub-level function
    // cexy.test.create() - sub-level function
    return cexy.cmd.simple_test(argc, argv, user_ctx);
}
```


### Supported compilers/platforms

#### Tested compilers / Libc support

* GCC - 10, 11, 12, 13, 14, 15
* Clang - 13, 14, 15, 16, 17, 18, 19, 20
* MSVC - unsupported, probably never will
* LibC tested - glibc (linux), musl (linux), ucrt/mingw (windows), macos
- Emscripten - for wasm

#### Tested platforms / architectures
* Linux - x32 / x64 (glibc, gcc + clang),
* Alpine linux - (libc musl, gcc) on architectures x86_64, x86, aarch64, armhf, armv7, loongarch64, ppc64le, riscv64, and s390x (big-endian)
* Windows (via MSYS2 build) - x64 (mingw64 + clang), libc mscrt/ucrt
* Macos - x64 / arm64 (clang)
- WASM - emscripten compiler

### Resources
* [GitHub Repo](https://github.com/alexveden/cex)
* [Ask a question on GitHub](https://github.com/alexveden/cex/discussions)

## CEX philosophy

### Main purpose of CEX

Cex was designed as a thin base layer above core C language, with the followings goals in mind:

* **Enhancing developer experience.** Most common things should be seamless as possible, without changing core language mechanics. Reducing boilerplate code, with improving readability and debuggability.
* **Eliminating dependencies.** Cex is a single header, all-in-one language, with core tools for building, testing and debugging your project. You need only C compiler (clang or gcc), that's it. Build system is included, you can write your build logic in C.
* **Cross-platform.** All Cex capabilities are cross-platform tested, and you don't need to figure out nuances of behavior of system API for different platforms.
* **Self-sufficient build system.** CMake/Make/ShellScripts are dependencies, they are essentially separate programming languages, it's a burden. Cex itself is a build system, with simple CLI and supports cross-platform builds, persistent configuration, build logic written in C (like in Zig).
* **Scripting-flavor.** CEX is designed to make common code patterns easy to work with, it combats with extra complexity, reducing mental overhead for writing code. New memory management and error handling make daily life way easier.
* **Less is more and enough is enough.** Cex tries to add just enough new entities (types, namespaces, functions) to original C to make life easier, extend C functionality only when needed. Cex embraces conservatism of C, and its goal is to be a stable base layer for projects years ahead.
* **Code quality tools.** Cex leverages existing compiler capabilities for making C code better. It includes sanitizers, lib fuzzers, and unit tests out of the box, letting you focus on development.
* **Long-term lifetime.** When a project is built with CEX, it carries everything needed inside its repo, CEX header itself after 1.0 release will be maintained for ultimate backward compatibility. Ideally, it has to be API stable at SQLite project.

### Simplicity as a virtue
C was the least common denominator for legacy and modern system software for decades. It's a simple language, but very hard to master. Cex tries to add a thin layer of things, for making life a little bit easier, but without bringing too much complexity to the code.

It's challenging to make something simple by adding more stuff, which by definition adds complexity. However, by adding stuff Cex reduces decision-making burden, providing common code patterns, utility functions, and rethinking C experience.

For example:

* Cex errors make vanilla C error handling obsolete. It's just one type with two states (error / no error), with unlimited options for errors variants. No more special enums, no more `-1 and errno`. Cex errors are easy to throw, easy to log, easy to handle.
* Memory handling via allocators makes all allocating functions explicit. It's easier to reason about the code, easier to track lifetimes, easier to clean up when using arenas and scopes. Also having standard allocators allows you to write reusable code or use allocators for memory leaks debugging.
* Namespaces mitigate the remembering burden when we have dozens of functions with the same prefix, it's easier to type and follow by LSP suggestions. Using sub-namespaces helps reduce mental overhead of picking the right one. For example, `str.convert.` and `str.slice.` expand to specific sub-namespaces in LSP suggestions in CEX. Using namespaces feels like a decision tree, you branch step by step from upper namespace to sub-namespace to end function. It feels much easier to work with `str.slice.remove_prefix` typing than remembering full function name `str_slice_remove_prefix.`
* Commonality of data collections. Cex has dynamic arrays and hashmaps, which can be handled as any other C array. Inspired by Python approach of applying `len` and `for` to anything iterable, Cex also has `arr$len` and `for$each` which can be commonly used for any C/Cex static or dynamic array, hashmap or pointer+length data.
* CEX build system might look like overkill (~~just use CMake~~) however, with it you still practice C/Cex, no extra dependency needed for building your project. Cex on the other hand does its best to provide utility tools for building code, working with files, strings, and OS.

### Making C cexy again

Old kind C99 might look too outdated for the modern times, but we have C23 compilers nowadays with brilliant tooling like sanitizers and fuzzers included. This opens a new era of C, safer C.

C looks like it's a perfect fit for unsafe low-level applications like OS kernels, drivers, math libraries. Doing higher level stuff was always miserable in C in different reasons. Cex is an attempt of bringing C on a little bit higher level.

#### Joy of C

In my opinion, the unsafety of C is really fun to work with, everything is under your control, everything is your responsibility. You can do wild stuff without any complaints from the compiler, ultimate freedom of code with ultimate responsibility.

This freedom of code is not nearly achievable with any modern language, they tend to set unlimited guardrails and protect us from any possible issues. This comes hand in hand with language complexity, unlimited struggle with compiler warnings, and adding new abstraction levels over everything which may hurt you. We end up with a sterile world of safe computer science, without any chance to touch and understand how machine works on low level.

Mission of CEX is to bring joy of the programming in C on the new level, to help in making C code safer and easier to write and understand.

#### Shooting in the foot

What if self shooting in the foot is not a bad idea? Before you start imagining pistol or shotgun, hold off, let's start small... What if we could pick a toy gun with plastic bullets? What if we could stress test our code under different conditions and see what's happened? What happens when we pass NULL to that argument? What about buffer overflow?

> [!CAUTION]
>
> CEX Principle: making by shaking.

For making safe C code we must have tools for that, fortunately modern compilers already have them:

1. Address sanitizers - for clang/gcc catch variety of bugs (buffer overflows, use after free, memory leaks, etc.). We only need to help them to trigger, by shaking the code via unit tests or fuzzers.
2. Unit Tests - C is one of the languages which require 3x more testing efforts than any other programming language.
3. Fuzzers - for some cases the variety of inputs is too large, so we could not cover all of them via unit testing. Fuzzers come to help with this issue, but also can be used in Deterministic Simulation Testing, or randomized testing. LibFuzzer is included in clang, or you can use AFL++ if you want.
4. Assertions - placing asserts everywhere in your code is a big deal for a code quality, and long term early warning about possible system inconsistencies. They can be used not only at checking input of a function, but validating results, or even at intermediate stages.

All the tools above are available with `cex.h` out of the box, so adding new test via CLI never has been easier:

```sh
# New test boilerplate
./cex test create tests/test_my_stuff.c

# Running a test
./cex test run tests/test_my_stuff.c

# Running all tests in a project
./cex test run all

```

#### Problems and solutions of C

Every programming language has its own quirks, sharp edges, and workarounds. C is not an exception here. However, in my opinion, sanitizers made a revolution in C development. With modern compilers C is much safer than it used to be even 10 years ago. Fuzzers made another step above, especially if your program works a lot with user input.


| Problem | Solution |
| -------------- | --------------- |
| Memory safety | We can use sanitizers to catch most of the cases, more unit tests! |
| Memory leaks | Memory scopes in CEX, sanitizers for checking |
| Manual memory management | Temp allocator in CEX, memory scopes, arenas |
| Name conflicts | Hard to solve, CEX mitigates it by introducing namespace generation |
| Error handling is inconsistent | CEX introduces new unified error handling | 
| Type overflows | Unit testing, fuzzing |
| Unsafe type casting | It hurts especially at refactoring, but unit testing will catch everything. |
| Undefined behavior | Use UB sanitizer, fix all compiler warnings |
| Macros | People hate macros, I don't know why, just use in moderation |
| No generic types | CEX dynamic arrays and hashmaps are fully generic, solvable with macros and `_Generic` |
| No tracebacks | Use sanitizers, also Cex `uassert` prints tracebacks, Cex errors handling can generate tracebacks |
| Poor core types (strings, dynamic arrays, hashmaps) | CEX introducing general purpose core types |


#### When C shines


| What | Why |
| -------------- | --------------- |
| Simple semantics | It's a good thing to have less cryptic combinations of special characters and keywords in the language. C is simple, and it doesn't mean it's easy. |
| Full control | We have full control over everything what's happening in our program: how memory is aligned, how control flow is aligned in assembly, how memory is allocated. |
| Tooling | C has enormous amount of development tools: testers, fuzzers, debuggers, coverage, performance, etc... |
| Language stability | It's cool to have a project that compiles and works after 5-10 years, with minimal changes. I would call C is an anti-language to modern NodeJS world. |
| Knowledge base | Probably it's a most diverse and stable knowledge base of all languages. |
| Works everywhere | Anybody tried to run doom on a toaster? |
| Performance | It feels good when you beat blazingly fast javascript by a factor of 100x with moderate C code |
| Compatibility | C is a lowest common denominator of all languages, anything can wrap and call C code |


#### How to improve C

Cex was initially inspired by my Python experience, especially how a very limited set of built-in types (str, list, dict, tuple, set + primitives) and simple semantics were able to produce huge ecosystem of Python nowadays. In my opinion, we don't need to have every hyped programming paradigm to be added to the C language to make it better.

However, we need some things to be productive in C that CEX tries to implement:

1. New error handling system - which makes error handling seamless and easy to work with.
2. New memory management model - for making memory management more transparent, tracking lifetimes of objects more clear.
3. Better strings - because modern computing became string-centric, strings are everywhere, we need better tool set in C.
4. Better arrays - there are no built-in dynamic arrays in C, but it's the most used data structure of all times.
5. Hashmaps / sets - the second most used data structure, without C coverage.
6. Build system - current build system situation is endless source of dependency conflicts, cross-platform headaches, and other issues that steals our mental energy.
7. Code quality tools - we should lower barriers for running unit tests, fuzzers, coverage, benchmarks, etc.

Cex also includes some things for IO, OS/file system operations, JSON lib for fueling cross-platform build system and configuration. However, the goal of CEX core is to remain thin layer above original C, adding just enough.

### Why not just use R\*\*t, Z\*g or C\@\$ ?

There is something appealing in C simplicity, it shines when we need full control over the code and assembly. Maybe it's not for everyone, and maybe it's a bad idea to use C for web-backends. But modern languages often affected by rush for adding new things,  new paradigms, piling a complexity of semantics and dependencies.

C brings stability to the table, if something is written in C there is a chance that this project will be compilable after 5 years from now. Very few modern languages have this paradigm in mind. Most keep rushing to make changes, adding new features.



## Basics

### Code Style Guidelines

* `dollar$means_macros`. CEX style uses `$` delimiter as a macro marker, if you see it anywhere in the code this means you are dealing with some sort of macro. `first$` part of name usually linked to a namespace of a macro, so you may expect other macros, type names or functions with that prefix.

* `functions_are_snake_case()`. Lower case for functions

* `MyStruct_c` or `my_struct_s`. Struct types typically expected to be a `PascalCase` with suffix, `_c` suffix indicates there is a code namespace with the same name (i.e. `_c` hints it's a container or kind of object), `_s` suffix means simple data-container without special logic.

* `MyObj.method()` or `namespace.func()`. Namespace names typically lower case, and object specific namespace names reflect type name, e.g. `MyObj_c` has `MyObj.method()`.

* `Enums__double_underscore`. Enum types are defined as `MyEnum_e` and each element looks like `MyEnum__foo`, `MyEnum__bar`.

* `CONSTANTS_ARE_UPPER`. Two notations of constants: `UPPER_CASE_CONST` or `namespace$CONST_NAME`

### Types
CEX provides several short aliases for primitive types and some extra types for covering blank spots in C.

| Type | Description |
| -------------- | --------------- |
| auto | automatically inferred variable type |
| bool | boolean type |
| u8/i8 | 8-bit integer |
| u16/i16 | 16-bit integer |
| u32/i32 | 32-bit integer |
| u64/i64 | 64-bit integer |
| f32 | 32-bit floating point number (float) |
| f64 | 64-bit floating point number (double) |
| usize | maximum array size (size_t) |
| isize | signed array size (ptrdiff_t) |
| char* | core type for null-term strings |
| sbuf_c | dynamic string builder type |
| str_s | string slice (buf + len) |
| Exc / Exception | error type in CEX |
| Error.\<some\> | generic error collection |
| IAllocator | memory allocator interface type |
| arr\$(T) | generic type dynamic array |
| hm\$(T) | generic type hashmap|

### CEX Core Namespaces
> [!NOTE]
>
> You can get cheat-sheet with `./cex help <namespace>$` command (ending `$` is important!). For example, `./cex help io$`, `./cex help e$`.


| Namespace | Description |
| -------------- | --------------- |
| e$ | CEX Exception handling |
| for$ | CEX array looping (for\$each, etc.) |
| log$ | Logging system |
| mem\$\* | Memory management and allocators |
| mem$ | Global variable for general purpose heap allocator |
| tmem$ | Global variable for temporary arena allocator |
| str | General purpose string / slice namespace | 
| sbuf | String builder class |
| arr$ | Type-safe, generic, dynamic array |
| hm$ | Type-safe, generic hashmap |
| io | Cross-platform IO namespace |
| test$ | Unit-test namespace: test$case, test$mock_scope, tassert_* assertions |
| fuzz$ | Fuzz-test interface |
| argparse | Command line argument parsing class |
| cg$ | Code generation namespace |
| cexy$ | CEX Build system config vars and interface |


### Utility macros

| Name | Description |
| -------------- | --------------- |
| uassert() | General purpose assert with tracebacks |
| uassertf() | General purpose assert with formatting  |
| unlikely() | Branch predictor management for unexpected conditions |
| likely() | Branch predictor management for expected conditions |
| breakpoint() | Cross-platform debugger breakpoint |
| fallthrough() | Explicit fallthrough to the next switch case |
| unreachable() | Panics in debug mode, __builtin_unreachable() `#ifdef NDEBUG` mode |
| tassert_* | Unit-test assertions see `./cex help tassert_` |

## Error handling

### The problem of error handling in C

C errors have always been a mess due to historical reasons and because of ABI specifics. The main curse of C errors is mixing values with errors, for example system specific calls return `-1` and set `errno` variable. Some return 0 on error, some NULL, sometimes it's an enum, or `MAP_FAILED (which is (void*)-1)`.

This convention on errors drains a lot of developer energy making him keep searching docs and figuring out which return values of a function are considered errors.

C error handling makes code cluttered with endless `if (ret_code == -1)`pattern.


The code below is a typical error handling pattern in C, however it's illustration for a specific issues:
```c
isize read_file(char* filename, char* buf, usize buf_size) {
    if (buf == NULL || filename == NULL) {
        errno = EINVAL;       /* <1> */
        return -1;
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));  /* <2> */
        return -1;
    }
    isize bytes_read = read(fd, buf, buf_size);
    if (bytes_read == -1) {
        perror("Error reading"); /* <3> */
        return -1;
    }
    return bytes_read; /* <4> */
}
```
1. `errno` is set, but it hard to distinguish by which API call or function argument is failed.
2. Error message line is located not at the same place as it was reported, so the developer must go through code to check.
3. `errno` is too broad and ambiguous for describing exact reason of failure.
4. `foo` return value is mixing error `-1` and legitimate value of `bytes_read`. The situation gets worse if we need to use non integer return type of a function.

### CEX Error handling goals
CEX made an attempt to re-think general purpose error handling in applications, with the following goals:

* **Errors should be unambiguous** - detaching errors from valid result of a function, there are only 2 states: OK or an error.
* **Error handling should be general purpose** - providing generic code patterns for error handling
* **Error should be easy to report** - avoiding error code to string mapping
* **Error should be bubbling up** - code can pass the same error to the upper caller
* **Error should extendable** - allowing unique error identification
* **Error should be passed as values** - low overhead, error handling
* **Error handling should be natural** - no special constructs required to handle error in C code
* **Error should be forced to check** - no occasional error check skips

### How error handling is implemented
CEX has a special `Exception` type which is essentially alias for `char*`, and yes all error handling in CEX is based on `char*`. Before you start laughing and rolling on the floor, let me explain the most important part of the `Exception` type, this little `*` part. Exception in CEX is a **pointer** (an address, a number) to a some arbitrary char array on memory.

What if the returned pointer could be always some constant area indicating an error? With that rule, we don't have to match error (string) content, but we can compare only address of the error.

#### CEX Error in a nutshell
```c
// NOTE: excerpt from cex.h

/// Generic CEX error is a char*, where NULL means success(no error)
typedef char* Exc;

/// Equivalent of Error.ok, execution success
#define EOK (Exc) NULL

/// Use `Exception` in function signatures, to force developer to check return value
/// of the function.
#define Exception Exc __attribute__((warn_unused_result))


/**
 * @brief Generic errors list, used as constant pointers, errors must be checked as
 * pointer comparison, not as strcmp() !!!
 */
extern const struct _CEX_Error_struct
{
    Exc ok; // Success no error
    Exc argument;
    // ... cut ....
    Exc os;
    Exc integrity;
} Error;


// NOTE: user code

Exception
remove_file(char* path)
{
    if (path == NULL || path[0] == '\0') { 
        return Error.argument;  // Empty of null file
    }
    if (!os.path.exists(path)) {
        return "Not exists" // literal error are allowed, but must be handled as strcmp()
    }
    if (str.eq(path, "magic.file")) {
        // Returns an Error.integrity and logs error at current line to stdout
        return e$raise(Error.integrity, "Removing magic file is not allowed!");
    }
    if (remove(path) < 0) { 
        return strerror(errno); // using system error text (arbitrary!)
    }
    return EOK;
}

Exception
main(char* path)
{
    // Method 1: low level handling (no re-throw)
    if (remove_file(path)) { return Error.os; }
    if (remove_file(path) != EOK) { return "bad stuff"; }
    if (remove_file(path) != Error.ok) { return EOK; }

    // Method 2: handling specific errors
    Exc err = remove_file(path);
    if (err == Error.argument) { // <<< NOTE: comparing address not a string contents!
        io.printf("Some weird things happened with path: %s, error: %s\n", path, err);
        return err;
    }
    // Method 3: helper macros + handling with traceback
    e$except(err, remove_file(path)) { // NOTE: this call automatically prints a traceback
        if (err == Error.integrity) { /* TODO: do special case handling */  }
    }

    // Method 4: helper macros + handling unhandled
    e$ret(remove_file(path)); // NOTE: on error, prints traceback and returns error to the caller

    remove_file(path);  // <<< OOPS compiler error, return value of this function unchecked

    return 0;
}


```

#### Error tracebacks and custom errors in CEX
CEX error system was designed to help in debugging, this is a simple example of deep call stack printing in CEX.

```c

#define CEX_IMPLEMENTATION
#include "cex.h"

const struct _MyCustomError
{
    Exc why_arg_is_one;
} MyError = { .why_arg_is_one = "WhyArgIsOneError" };

Exception
baz(int argc)
{
    if (argc == 1) { return e$raise(MyError.why_arg_is_one, "Why argc is 1, argc = %d?", argc); }
    return EOK;
}

Exception
bar(int argc)
{
    e$ret(baz(argc));
    return EOK;
}

Exception
foo2(int argc)
{
    io.printf("MOCCA - Make Old C Cexy Again!\n");
    e$ret(bar(argc));
    return EOK;
}

int
main(int argc, char** argv)
{
    (void)argv;
    e$except (err, foo2(argc)) { 
        if (err == MyError.why_arg_is_one) {
            io.printf("We need moar args!\n");
        }
        return 1; 
    }
    return 0;
}
```

```sh
MOCCA - Make Old C Cexy Again!
[ERROR]   ( main.c:12 baz() ) [WhyArgIsOneError] Why argc is 1, argc = 1?
[^STCK]   ( main.c:19 bar() ) ^^^^^ [WhyArgIsOneError] in function call `baz(argc)`
[^STCK]   ( main.c:27 foo2() ) ^^^^^ [WhyArgIsOneError] in function call `bar(argc)`
[^STCK]   ( main.c:35 main() ) ^^^^^ [WhyArgIsOneError] in function call `foo2(argc)`
We need moar args!
```

### Rewriting initial C example to CEX

Main benefits of using CEX error handling system:

1. Error messages come with `source_file.c:line` and `function()` for easier to debugging
2. Easier to do quick checks with `e$assert`
3. Easier to re-throw generic unhandled errors inside code
4. Unambiguous return values: OK or error.
5. Unlimited variants of returning different types of errors (`Error.argument`, `"literals"`, `strerror(errno)`, `MyCustom.error`)
6. Easy to log - Exceptions are just `char*` strings
7. Traceback support when chained via multiple functions

::: {.panel-tabset}
#### CEX
```c
Exception read_file(char* filename, char* buf, isize* out_buf_size) {
    e$assert(buf != NULL);  /* <1> */
    e$assert(filename != NULL && "invalid filename");

    int fd = 0;
    e$except_errno(fd = open(filename, O_RDONLY)) { return Error.os; } /* <2> */
    e$except_errno(*out_buf_size = read(fd, buf, *out_buf_size)) { return Error.io; } /* <3> */
    return EOK; /* <4> */
}
```
1. Returns error with printing out internal expression: `[ASSERT]  ( main.c:26 read_file() ) buf != NULL`. `e$assert` is an Exception returning assert, it doesn't abort your program, and these asserts are not stripped in release builds.
2. Handles typical `-1 + errno` check with print: `[ERROR]   ( main.c:27 read_file() ) fd = open("foo.txt", O_RDONLY) failed errno: 2, msg: No such file or directory`
3. Result of a function returned by reference to the `out` parameter.
4. Unambiguous return code for success.


#### C
```c
isize read_file(char* filename, char* buf, usize buf_size) {
    if (buf == NULL || filename == NULL) {
        errno = EINVAL;
        return -1;
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
        return -1;
    }
    isize bytes_read = read(fd, buf, buf_size);
    if (bytes_read == -1) {
        perror("Error reading");
        return -1;
    }
    return bytes_read;
}
```
:::

### Helper macros `e$...`
CEX has a toolbox of macros with `e$` prefix, which are dedicated to the `Exception` specific tasks. However, it's not mandatory to use them, and you can stick to regular control flow constructs from C.

In general, `e$` macros provide location logging (source file, line, function), which is a building block for error traceback mechanism in CEX.

`e$` macros mostly designed to work with functions that return `Exception` type.

#### Returning the `Exc[eption]`
Errors in CEX are just plain string pointers. If the `Exception` function returns `NULL` or `EOK` or `Error.ok` this is indication of successful execution, otherwise any other value is an error.

Also, you may return with `e$raise(error_to_return, format, ...)` macro, which prints location of the error in the code with message formatting.

```c
Exception error_sample1(int a) {
    if (a == 0) return Error.argument; // standard set of errors in CEX
    if (a == -1) return "Negative one";   // error literal also works, but harder to handle
    if (a == -2) return UserError.neg_two; // user error
    if (a == 7) return e$raise(Error.argument, "Bad a=%d", a); // error with logging
    
    return EOK; // success
    // return Error.ok; // success
    // return NULL; // success
}
```

#### Handling errors
Error handling in CEX supports two ways:

* Silent handling - which suppresses error location logging, this might be useful for performance critical code, or tight loops. Also, this is a general way of returning errors for CEX standard lib.
* Loud handling with logging - this way is useful for one shot complex functions which may return multiple types of errors for different reasons. This is the way if you wanted to incorporate tracebacks for your errors.

##### Silent handling example
> [!NOTE]
>
> Avoid using e$raise() in called functions if you need silent error handling, use plain `return Error.*`

```c
Exception foo_silent(void) {
    // Method 1: quick and dirty checks
    if (error_sample1(0)) { return "Error"; /* Silent handling without logic */ }
    if (error_sample1(0)) { /* Discarding error of a call */ }

    // Method 2: silent error condition
    Exc err = error_sample1(0);
    if (err) {
        if (err == Error.argument) {
            /* Handling specific error here */
        }
        return err;
    }

    // Method 3: silent macro, with temp error value
    e$except_silent(err, error_sample1(0)) {
        // NOTE: nesting is allowed!
        e$except_silent(err, error_sample1(-2)) {
            return err; // err = UserError.neg_two
        }

        // err = Error.argument now
        if (err == Error.argument) {
            /* Handling specific error here */
        }
        // break; // BAD! See caveats section below
    }
    return EOK;
}

```

> [!NOTE]
>
> `e$except_silent` will print error log when code runs under unit test or inside CEX build system, this helps a lot with debugging.

##### Loud handling with logging

If you write some general purpose code with debuggability in mind, the logged error handling can be a breeze. It allows traceback error logging, therefore deep stack errors now easier to track and reason about.

There are special error handling macros for this purpose:

1. `e$except(err, func_call()) { ... }` - error handling scope which initialize temporary variable `err` and logs if there was an error returned by `func_call()`. `func_call()` must return `Exception` type for this macro.
2. `e$except_errno(sys_func()) { ... }` - error handling for system functions, returning `-1` and setting `errno`.
3. `e$except_null(ptr_func()) { ... }` - error handling for `NULL` on error functions.
4. `e$except_true(func()) { ... }` - error handling for functions returning non-zero code on error.
5. `e$ret(func_call());` - runs the `Exception` type returning function `func_call()`,  and on error it logs the traceback and re-return the same return value. This is a main code shortcut and driver for all CEX tracebacks. Use it if you don't care about precise error handling and fine to return immediately on error.
6. `e$goto(func_call(), goto_err_label);` - runs the `Exception` type function, and does `goto goto_err_label;`. This macro is useful for resource deallocation logic, and intended to use for typical C error handling pattern `goto fail`.
7. `e$assert(condition)` or `e$assert(condition && "What's wrong")` or `e$assertf(condition, format, ...)`  - quick condition checking inside `Exception` functions, logs an error location + returns `Error.assert`. These asserts remain in release builds and are not affected by the `NDEBUG` flag.

```c
Exception foo_loud(int a) {
    e$assert(a != 0);
    e$assert(a != 11 && "a is suspicious");
    e$assertf(a != 22, "a=%d is something bad", a);

    char* m = malloc(20);
    e$assert(m != NULL && "memory error"); // evergreen assert

    e$ret(error_sample1(9)); // Re-return on error

    e$goto(error_sample1(0), fail); // goto fail and free the resource

    e$except(err, error_sample1(0)) {
        // NOTE: nesting is allowed!
        e$except(err, error_sample1(-2)) {
            return err; // err = UserError.neg_two
        }
        // err = Error.argument now
        if (err == Error.argument) {
            /* Handling specific error here */
        }

        // continue; // BAD! See caveats section below
    }

    // For these e$except_*() macros you can use assignment expression
    // e$except_errno(fd = open(..))
    // e$except_null(f = malloc(..))
    // e$except_true (sqlite3_open(db_path, &db))
    FILE* f;
    e$except_null(f = fopen("foo.txt", "r")) {
        return Error.io;
    }

    return EOK;
    
fail:
    free(m);
    return Error.runtime;
}

```

#### Caveats

Most of `e$excep_*` macros are backed by `for()` loop, so you have to be careful when you nest them inside outer loops and try to `break`/`continue` outer loop on error.

In my opinion using `e$except_` inside loops is generally bad idea, and you should consider:

1. Factoring error emitting code into a separate function
2. Using `if(error_sample(i))` instead of `e$except`

##### Bad example!

```c
Exception foo_err_loop(int a) {
    for (int i = 0; i < 10; i++) {
        e$except(err, error_sample1(i)) {
            break; // OOPS: `break` stops `e$except`, not outer for loop
        }
    }
    return EOK;
}
```


### Standard `Error`
CEX implements a standard `Error` namespace, which typical for most common situations if you might need to handle them.

```c
const struct _CEX_Error_struct Error = {
    .ok = EOK,                       // Success
    .memory = "MemoryError",         // memory allocation error
    .io = "IOError",                 // IO error
    .overflow = "OverflowError",     // buffer overflow
    .argument = "ArgumentError",     // function argument error
    .integrity = "IntegrityError",   // data integrity error
    .exists = "ExistsError",         // entity or key already exists
    .not_found = "NotFoundError",    // entity or key not found
    .skip = "ShouldBeSkipped",       // NOT an error, function result must be skipped
    .empty = "EmptyError",           // resource is empty
    .eof = "EOF",                    // end of file reached
    .argsparse = "ProgramArgsError", // program arguments empty or incorrect
    .runtime = "RuntimeError",       // generic runtime error
    .assert = "AssertError",         // generic runtime check
    .os = "OSError",                 // generic OS check
    .timeout = "TimeoutError",       // await interval timeout
    .permission = "PermissionError", // Permission denied
    .try_again = "TryAgainError",    // EAGAIN / EWOULDBLOCK errno analog for async operations
}

Exception foo(int a) {
    e$except(err, error_sample1(0)) {
        if (err == Error.argument) {
            return Error.runtime; // Return another error
        }
    }
    return Error.ok; // success
}
```

### Making custom user exceptions
#### Extending with existing functionality

Probably you only need to make custom errors for specific handling needs, which is a rare case. In common case you might need to report details of the error and forget about it. Before we dive into customized error structs, let's consider what simple instruments do we have for error customization without making another entity in the code:

1. You may try to return string literals as a custom error, these errors are convenient options when you don't need to handle them (e.g. for rare/weird edge cases)
```c
Exception foo_literal(int a) {
    if (a == 777999) return "a is a duplicate of magic number";
    return EOK;
}
```
2. You may try to return standard error + log something with `e$raise()` which support location logging and custom formatting.
```c
Exception foo_ret(int a) {
    if (a == 777999) return e$raise(Error.argument, "a=%d looks weird", a);
    return EOK;
}
```

#### Custom error structs
If you need custom handling, you might need to create a new dedicated structure for errors.

Here are some requirements for a custom error structure:

1. It has to be a constant global variable
2. All fields must be initialized, uninitialized fields are NULL therefore they are **success** code.

```c
// myerr.h
extern const struct _MyError_struct
{
    Exc foo;
    Exc bar;
    Exc baz;
} MyError;

// myerr.c
const struct _MyError_struct MyError = {
    .foo = "FooError",
    .bar = "BarError",
    // WARNING: missing .baz - which will be set to NULL => EOK
};

// other.c
#include "cex.h"
#include "myerr.h"

Exception foo(int a) {
    e$except(err, error_sample1(0)) {
        if (err == Error.argument) {
            return MyError.foo;
        }
    }
    return Error.ok; // success
}

```

### Advanced topics
#### Performance
##### Errors are pointers
Using strings as error value carrier may look controversial at the first glance. However, let's remember that strings in C are `char*`, and essentially `*` part means that it's a `size_t` integer value of a memory address. Therefore, CEX approach is to have set of pre-defined and constant memory addresses that hold standard error values (see `Standard Error` section above).

So for error handling we need to compare return value with `EOK|NULL|Error.ok` to check if error was returned or not. Then we check address of returned error and compare it with the address of the standard error.

With this being said, performance of typical error handling in CEX is one assembly instruction that compares a register with `NULL` and one instruction for comparing **address** of an error with some other constant address when handling returned error type.

> [!NOTE]
>
> CEX uses direct pointer comparison `if (err == Error.argument)`, instead of string content comparison `if(strcmp(err, "ArgumentError") == 0) /* << BAD */`

##### Branch predictor control
All CEX `e$` macros uses `unlikely` a.k.a. `__builtin_expect` to shape assembly code in the way of favoring happy path, for example this is a `e$assert` source snippet:
```c
#    define e$assert(A)                                                                             \
        ({                                                                                          \
            if (unlikely(!((A)))) {                                                                 \
                __cex__fprintf(stdout, "[ASSERT] ", __FILE_NAME__, __LINE__, __func__, "%s\n", #A); \
                return Error.assert;                                                                \
            }                                                                                       \
        })
```

The `unlikely(!(A))` hints the compiler to place assembly instructions in a way of favoring happy path of the `e$assert`, which is a performance gain when you have multiple error handling checks and/or big blocks for error handling.

#### Compatibility
Be careful if you need to expose CEX exception returning functions to an API. Sometimes, if you are working with different shared libraries, the addresses of the same errors might be different. If user code is intended to check and handle API errors, maybe it's better to stick to C-compatible approach instead of CEX errors.

CEX Exceptions work best when you use them in single address space of an app or a library. If you need to cross this boundary, do your best assessment for pros and cons.

### Useful code patterns

#### Escape `main()` when possible

CEX approach is to keep `main()` function separated and as short as possible. This opens capabilities for full code unit testing, unity builds, and tracebacks. This is a typical example app:
```c
// app_main.c file
#include "cex.h"
Exception
app_main(int argc, char** argv)
{
    bool my_flag = false;
    argparse_c args = {
        .description = "New CEX App",
        argparse$opt_list(
            argparse$opt_help(),
            argparse$opt(&my_flag, 'c', "ctf", .help = "Capture the flag"),
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return Error.argsparse; }
    io.printf("MOCCA - Make Old C Cexy Again!\n");
    io.printf("%s\n", (my_flag) ? "Flag is captured" : "Pass --ctf to capture the flag");
    return EOK;
}

// main.c file
#define CEX_IMPLEMENTATION   // this only appears in main file, before #include "cex.h"
#include "cex.h"
#include "app_main.c"  // NOTE: include .c, using unity build approach

int
main(int argc, char** argv)
{
    if(app_main(argc, argv)) { return 1; }
    return 0;
}
```

#### Inversion of error checking
Instead of doing `if` nesting, try an opposite approach, check an error and exit. In CEX you can also use `e$assert()` for a quick and dirty checking with one line.

::: {.panel-tabset}
##### CEX
```c
Exception
app_main(int argc, char** argv)
{
    e$assert(argc == 2);  // assert shortcut
    if (str.eq(argv[1], "MOCCA")) { return Error.integrity; }

    io.printf("MOCCA - Make Old C Cexy Again!\n");
    return EOK;
}
```
##### Nested errors
```c
Exception
app_main(int argc, char** argv)
{
    if (argc > 1) {
        if (str.eq(argv[1], "MOCCA")) {
            io.printf("MOCCA - Make Old C Cexy Again!\n");
        } else {
            return Error.integrity;
        }
    } else {
        return Error.argument;
    }
    return EOK;
}
```
:::


#### Resource cleanup
Sometimes you need to open resources, manage your memory, and carry error code. Or maybe we have to use legacy API inside function, with some incompatible error code calls. Here is a CEX flavored implementation of common `goto fail` C code pattern.

::: {.panel-tabset}
##### Cleanup
```c

Exception
print_zip(char* zip_path, char* extract_dir)
{
    Exc result = Error.runtime; // NOTE: default error code, setting to error by default

    // Open the ZIP archive
    int err;
    struct zip* archive = NULL;
    e$except_null (archive = zip_open(zip_path, 0, &err)) { goto end; }

    i32 num_files = zip_get_num_entries(archive, 0);

    for (i32 i = 0; i < num_files; i++) {
        struct zip_stat stat;
        if (zip_stat_index(archive, i, 0, &stat) != 0) {
            result = Error.integrity;  // NOTE: we can substitute error code if needed
            goto end;
        }

        // NOTE: next may return error on buffer overflow -> goto end then
        char output_path[64];
        e$goto(str.sprintf(output_path, sizeof(output_path), "%s/%s", extract_dir, stat.name), end);

        io.printf("Element: %s\n", output_path);
    }

    // NOTE: success when no `goto end` happened, only one happy outcome
    result = EOK;

end:
    // Cleanup and result
    zip_close(archive);
    return result;
}
```

##### On fail
```c
MyObj
MyObj_create(char* path, usize buf_size)
{
    MyObj self = {0};

    e$except_null (self.file = fopen(path, "r")) { goto fail; }

    self.buf = malloc(buf_size);
    if (self.buf == NULL) { goto fail; }

    e$goto(fetch_data(&self.data), fail);
    
    // MyObj was initialized and in consistent state
    return self;

fail:
    // On error - do a cleanup of initialized stuff
    if (self.file) { fclose(self.file); }
    if (self.buf) { free(self.buf); }
    memset(&self, 0, sizeof(MyObj));
    return self;
}
```

:::


<a id="lang-memory-management"></a>

## Memory management

### The problem of memory management in C
C has a long-lasting history of memory management issues. Many modern languages proposed multiple solutions for these issues: RAII, borrow checkers, garbage collection, allocators, etc. All of them work and solve the memory problem to some extent, but sometimes adding new sets of problems in different places.

From my perspective, the root cause of the C memory problem is hidden memory allocation. When developer works with a function which does memory allocation, it's hard to remember its behavior without looking into source code or documentation. Absence of explicit indication of memory allocation leads to flaws with memory handling, for example: memory leaks, use after free, or performance issues.

While C remains a system and low-level language, it's important to have precise control over code behavior and memory allocations. So in my opinion, RAII and garbage collection are alien approaches to C philosophy, but on the other hand modern languages like `Zig` or `C3` have allocator centric approach, which is more explicit and suitable for C.

### Modern way of memory management in CEX
#### Allocator-centric approach
CEX tries to adopt allocator-centric approach to memory management, which helps to follow these principles:

* **Explicit memory allocation.** Each object (class) or function that may allocate memory has to have an allocator parameter. This requirement, adds explicit API signature hints, and communicates about memory implications of a function without deep dive into documentation or source code.
* **Transparent memory management.** All memory operations are provided by `IAllocator` interface, which can be an interchangeable allocator object of different type.
* **Memory scoping**. When possible memory usage should be limited by scope, which naturally regulates lifetimes of allocated memory and automatically free it after exiting scope.
* **Unit test friendly**. Allocators allow implementation of additional levels of memory safety when run in unit test environment. For example, CEX allocators add special poisoned areas around allocated blocks, which trigger address sanitizer when this region accesses with user code. Allocators open the door for memory leak checks, or extra memory error simulations for better out-of-memory error handling.
* **Standard and Temporary allocators**. Sometimes it's useful to have initialized allocator under your belt for short-lived temporary operations. CEX provides two global allocators by default: `mem$` - is a standard heap allocator using `malloc/realloc/free`, and `tmem$` - is dynamic arena allocator of small size (about 256k of per page).

#### Example
This is a small example of key memory management concepts in CEX:

```c
mem$scope(tmem$, _) /* <1> */
{
    arr$(char*) incl_path = arr$new(incl_path, _); /* <2> */
    for$each (p, alt_include_path) {
        arr$push(incl_path, p);  /* <3> */
        if (!os.path.exists(p)) { log$warn("alt_include_path not exists: %s\n", p); }
    }
} /* <4> */
```
1. Initializes a temporary allocator (`tmem$`) scope in `mem$scope(tmem$, _) {...}` and assigns it as a variable `_` (you can use any name).
2. Initializes dynamic array with the scoped allocator variable `_`, allocates new memory.
3. May allocate memory
4. All memory will be freed at exit from this scope

#### Lifetimes and scopes
Use of memory scopes naturally regulates lifetime of initialized memory. From the example above you can't use `incl_path` variable outside of `mem$scope`. And more to say, that memory will be automatically freed after exiting scope. This design approach significantly reduces surface for use after free errors in general.


#### Temporary memory allocator
Dealing with lots of small memory allocations always was a pain in C, because we need to deallocate them at the end, also because of potential overhead each individual memory allocation might have. Temporary allocator in CEX works as a small-page (around 256kb) memory arena, which can be dynamically resized when needed. The most important feature of temporary arena allocator it does the full cleanup at the `mem$scope` exit automatically.

Temporary allocator is always available via `tmem$` global variable and can be used anytime at the program lifetime. It allowed to be used only inside `mem$scope`, with support of up to 32 levels of `mem$scope` nesting. At the end of the program, CEX will automatically finalize and free all allocated memory.

You can find more technical details about implementation below in this article.

### Memory management in CEX
#### Allocators
Allocators add many benefits into codebase design and development experience:

* All memory allocating functions or objects become explicit, because they require `IAllocator` argument
* Logic of the code become detached from memory model, the same dynamic array can be backed by heap, arena, or stack based static char buffer with the same allocator interface. The same piece of code may work on Linux OS or embedded device without changes to memory allocation model.
* Allocators may add testing capabilities, i.e. simulating out-of-mem errors in unit tests, or adding memory checks or extra integrity checks of memory allocations
* There are multiple memory allocation models (heap, arenas, temp allocation), so you can find the best type of allocator for your needs and use case.
* It's easier to trace and doing memory benchmarks with allocators.
* Automatic garbage collection with `mem$scope` and arena allocators - you'll get everything freed on scope exit

#### Allocator interface
The allocator interface is represented by `IAllocator` type, which is an interface structure of function pointers for generic operations. Allocators in CEX support `malloc/realloc/calloc/free` functions similar to their analogs in C, the only optional parameter is alignment for requested memory region.


```c
#define IAllocator const struct Allocator_i* 

typedef struct Allocator_i
{
    // >>> cacheline
    alignas(64) void* (*const malloc)(IAllocator self, usize size, usize alignment);
    void* (*const calloc)(IAllocator self, usize nmemb, usize size, usize alignment);
    void* (*const realloc)(IAllocator self, void* ptr, usize new_size, usize alignment);
    void* (*const free)(IAllocator self, void* ptr);
    const struct Allocator_i* (*const scope_enter)(IAllocator self);   /* Only for arenas/temp alloc! */
    void (*const scope_exit)(IAllocator self);    /* Only for arenas/temp alloc! */
    u32 (*const scope_depth)(IAllocator self);  /* Current mem$scope depth */
    struct {
        u32 magic_id;
        bool is_arena;
        bool is_temp;
    } meta;
    //<<< 64 byte cacheline
} Allocator_i;

```

#### `mem$` API

You shouldn't use allocator interface directly (it's less convenient), so it's better to use memory specific macros:

* `mem$malloc(allocator, size, [alignment])` - allocates uninitialized memory with `allocator`, `size` in bytes, `alignment` parameter is optional, by default it's system specific alignment (up to 64 byte alignment is supported)
* `mem$calloc(allocator, nmemb, size, [alignment])` - allocates zero-initialized memory with `allocator`, `nbemb` elements of `size` each, `alignment` parameter is optional, by default it's system specific alignment (up to 64 byte alignment is supported)
* `mem$realloc(allocator, old_ptr, size, [alignment])` - reallocates previously initialized `old_ptr` with `allocator`, `alignment` parameter is optional and must match initial alignment of `old_ptr`
* `mem$free(allocator, old_ptr)` - frees `old_ptr` and implicitly sets it to `NULL` to avoid use-after-free issues.
* `mem$new(allocator, T)` - generic allocation of new instance of `T` (type), with respect of its size and alignment.

Allocator scoping:

* `mem$arena_scope(page_size, allc_var) { ... }` - enters new instance of allocator arena with the `page_size`. Also supports `AllocatorArena_kw*` pointer form for `disable_scopes` etc.
* `mem$scope(arena_or_tmem$, scope_var) { ... }` - opens new memory scope (works only with arena allocators or temp allocator)


#### Dynamic arenas
Dynamic arenas use an array of dynamically allocated pages, each page has static size and is allocated on heap. When you allocate memory on arena and there is enough room on page, the arena allocates this chunk of memory inside page (simply moving a pointer without real allocation). If your memory request is big enough, the arena creates new page while keeping all old pages untouched and manages new allocation on the new page.

Arenas are designed to work with `mem$scope()`, allowing you to create temporary memory allocations, without worrying about cleanup. Once scope is left, the arena will deallocate all memory and return to the initial state. This approach allows using up to 32 levels of `mem$scope()` nesting. Essentially it is the exact mechanism that fuels `tmem$` - temporary allocator in CEX.

Working with arenas:

::: {.panel-tabset}

##### Direct initialization
```c
IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 }); /*<1>*/
u8* p = mem$malloc(arena, 100);  /* <2> */

mem$scope(arena, tal) /*<3>*/
{
    u8* p2 = mem$malloc(tal, 100000);  /*<4>*/

    mem$scope(arena, tal)
    {
        u8* p3 = mem$malloc(tal, 100);
    } /*<5>*/
} /*<6>*/

AllocatorArena.destroy(arena); /*<7>*/
```
1. New arena with 4096 byte page via `AllocatorArena_kw` kwargs struct
2. Allocating some memory from arena
3. Entering new memory scope
4. Allocation size exceeds page size, new page will be allocated then. `p` address remain the same!
5. At scope exit `p3` will be freed, `p2` and `p` remain
6. At scope exit `p2` will be freed, excess pages will be freed, `p` remains
7. Arena destruction, all pages are freed, `p` is invalid now.

##### Arena scope
```c
    mem$arena_scope(4096, arena)
    {
        // This needs an extra page
        u8* p2 = mem$malloc(arena, 10040);
        mem$scope(arena, tal)
        {
            u8* p3 = mem$malloc(tal, 100);
        }
    }
```

##### Temp allocator
```c
mem$scope(tmem$, _) /* <1> */
{
    u8* p2 = mem$malloc(_, 110241024); /* <2> */

    mem$scope(tmem$, _) /* <3> */
    {
        u8* p3 = mem$malloc(_, 100);
    } /* <4>*/
} /* <5> */
```
1. Initializes a temporary allocator (`tmem$`) scope in `mem$scope(tmem$, _) {...}` and assigns it as a variable `_` (you can use any name). `_` is a pattern for temp allocator in CEX.
2. New page for temp allocator is created, because size exceeds existing page size
3. Nested scope is allowed
4. Scope exit: `p3` is automatically cleaned up
5. Scope exit: `p2` is cleaned up + extra page freed.

:::


#### Standard allocators

There are two general purpose allocators globally available out of the box for CEX:

* `mem$` - is a heap allocator, the same old `malloc/free` type of allocation, with extra alignment support. In unit tests this allocator provides simple memory leak checks even without address sanitizer enabled.
* `tmem$` - dynamic arena, with 256kb page size, used for short-lived temporary operations, cleans up pages automatically at program exit. Does page allocation only at the first allocation, otherwise remain global static struct instance (about 128 bytes size). Thread safe, uses `thread_local`.


#### Caveats
##### Do cross-scope memory access carefully
Never reallocate memory from one scope, in the nested scope, which will automatically lead to use-after-free issue. This is a bad example:
```c
// BAD!
mem$scope(tmem$, _)
{
    u8* p2 = mem$malloc(_, 100); /* <1> */

    mem$scope(tmem$, _)
    {
        p2 = mem$realloc(_, p2, 110241024); /* <2> */
    } /* <3>*/

    if(p2[128] == '0') { /* OOPS */} /* <4> */
} 
```
1. Initially allocation at first scope
2. `realloc` uses different scope depth, this might lead to assertion in CEX unit test
3. `p2` automatically freed, because now it belongs to different scope
4. You'll face use-after-free, which typically expressed use-after-poison in temp allocator.

> [!TIP]
>
> CEX does its best to catch these cases in unit test mode, it will raise an assertion at the `mem$realloc` line with some meaningful error about this. Standard CEX collections like dynamic arrays `arr$` and hashmap `hm$` also get triggered when they need to resize in a different level of `mem$scope`.

##### Be careful with reallocations on arenas
CEX arenas are designed to be always growing, if your code pattern is based on heavily reallocating memory, the arena growth may lead to performance issues, because each reallocation may trigger memory copy with new page creation. Consider pre-allocate some reasonable capacity for your data when working with arenas (including temp allocator). However, if you're reallocating the **exact** last pointer, the arena might do it in place on the same page.

##### Unit Test specific behavior
When run in test mode (or specifically `#ifdef CEX_TEST` is true) the memory allocation model in CEX includes some extra safety capabilities:

1. Heap based allocator (`mem$`) starts tracking memory leaks, comparing number of allocations and frees.
2. `mem$malloc()` - return uninitialized memory with `0xf7` byte pattern
3. If Address Sanitizer is available all allocations for arenas and heap will be surrounded by poisoned areas. If you see use-after-poison errors, it's likely a sign of use-after-free or out of bounds access in `tmem$`. Try to switch your code to the `mem$` allocator if possible to triage the exact reason of the error.
4. Allocators do sanity checks at the end of each unit test case

> [!NOTE]
>
> A full reference of all test-mode side effects is available in the
> [Test-Mode Sanity Checks](#test-mode-sanity-checks-and-side-effects) section.

##### Be careful with break/continue
`mem$scope/mem$arena_scope` are macros backed by `for` loop, be careful when you use them inside loops and trying to `break/continue` outer loop.
```c
// BAD!
for(u32 i = 0; i < 10; i++){
{
    mem$scope(tmem$, _)
    {
        u8* p2 = mem$malloc(_, 100);
        if(p2[1] == '0') { 
            break; // OOPS, this will break mem$scope not a outer for loop
        }
    }
} 
```
##### Never return pointers from scope
Function exit will lead to memory cleanup after memory scope `p2` address now is invalid. You might get use-after-poison or use-after-free ASAN crash. Or `0xf7` pattern of data when running in test environment without ASAN.

```c
// BAD!
mem$scope(tmem$, _)
{
    u8* p2 = mem$malloc(_, 100);
    return p2; // BAD! This address will be freed at scope exit
}
```


### Advanced topics
#### Performance tips
##### TempAllocator makes CPU cache hot
If we use `mem$scope(tmem$)` a lot, the ArenaAllocator re-uses same memory pages, therefore these memory areas will be prefetched by CPU cache, which will be beneficial for performance. The ArenaAllocator in general works like a stack, with automatic memory cleanup at the scope exit.

##### Arena allocation is cheap
ArenaAllocator implements memory allocation by moving a memory pointer back and forth, it doesn't take much for allocating small chunks if there is no need for requesting memory from the OS for the new arena page.

##### Be careful with ArenaAllocator when you need to reallocate a lot
AllocatorArena and temporary allocator do not reuse blank chunks of the freed memory in pages, they simply allocate new memory. This might be a problem when you try to dynamically resize some container (e.g. dynamic array `arr$`), which could lead to uncontrollable growth of arena pages and therefore performance degradation.

On the other hand, it's totally fine to pre-allocate some capacity for your needs upfront. Just try to be mindful about you memory allocation and usage patterns.

#### When to use arena or heap allocator

##### ArenaAllocator and `tmem$` use cases
ArenaAllocator works great when you need disposable memory for a temporary needs, or you have limited boundaries in time and space for a program operation. For example, read file, process it, calculate stuff, close it, done.

Another great benefit of arenas is stability of memory pointers, once memory is allocated it sits there at the same place.

Arenas simplifies managing memory for small objects, so you don't need to write extra memory management logic for each small allocation, everything will be cleared at scope exit.

##### HeapAllocator (`mem$`) use cases
HeapAllocator is simply system allocator, backed by malloc/free. You can use it for long living or frequently resizable objects (e.g. dynamic arrays or hashmaps). Works best for bigger allocations with longer lives.


#### UnitTesting and memory leaks
When you run CEX allocators in unit test code, they apply extra sanity check logic for helping you to debug memory related issues:

* New `mem$malloc` allocations for `mem$/tmem$` are filled by `0xf7` byte pattern, which indicates uninitialized memory.
* `mem$` allocator tracks number of allocations and deallocations and emits unit test `[LEAK]` warning (in the case if ASAN is disabled)
* There are some small poisoned areas around allocations by `mem$/tmem$` which trigger ASAN `use-after-poison` crash (read/write), or check validity of poison pattern inside these areas when ASAN is disabled.
* After each test CEX automatically performs `tmem$` sanity checks in order to find memory corruption

> [!TIP]
>
> If you need to debug memory leaks for your code consider to use `mem$` (heap based) allocation, which utilizes ASAN memory leak tracking mechanisms.

#### Out-of-bounds access and poisoning
CEX encourages using ASAN everywhere for debug needs. ASAN works great for handling out-of-bounds access for heap allocated memory. It's a little bit difficult for arenas, because they use big pages of memory (we own it), therefore no complaints from the ASAN. In order to fix this `tmem$` and AllocatorArena add poison areas around each allocation which triggers `use-after-poison` crash. If you face it, make sure that your program doesn't read/write out of bounds, try to temporarily substitute `tmem$` with `mem$` to get more precise error information.

### Code patterns
#### Using temporary memory scope

```c
mem$scope(tmem$, _) 
{
    u8* p2 = mem$malloc(_, 100);
} 
```

> [!NOTE]
>
> CEX convention to use `_` variable as temp allocator.

#### Using heap allocator
```c
u8* p2 = mem$malloc(mem$, 100); // mem$ is a global variable for HeapAllocator
mem$free(mem$, p2); // we must manually free it
uassert(p2 == NULL); // p2 set to NULL by mem$free()
```


#### Opening new ArenaAllocator scope

```c
mem$arena_scope(4096, arena)
{
    u8* p2 = mem$malloc(arena, 10040);
}
```

#### Mixing ArenaAllocator and temp allocator

```c
mem$arena_scope(4096, arena)
{
    // We will store result in the arena
    u8* result = mem$malloc(arena, 10040);

    mem$scope(tmem$, _) 
    {
        // Do a temporary calculations with tmem$
        u8* p2 = mem$malloc(_, 100);

        // Copy persistent results here
        result[0] = p2[0];
    }  // NOTE: p2 and all temp data freed
    
    // result remains
} 
// result freed
```

<a id="lang-strings"></a>

## Strings

### Problems with strings in C

Strings in C are historically an endless source of problems, bugs and vulnerabilities. String manipulation in standard C library is very low level and sometimes confusing. But in my opinion, the most of the problems with string in C is a result of poor code practices, rather than language issues itself.

With modern tooling like Address Sanitizer it's much easier to catch these bugs, so we are starting to face developer experience issues rather than security complications.

Problems with C `char*` strings:

* No length information included, which leads to performance issues with overuse of `strlen`
* Null terminator is critical for security, but not all libc functions handle strings securely
* String slicing is impossible without copy and setting null-terminator at the end of slice
* libc string functions behavior sometimes is implementation specific and insecure

### Strings in CEX

There are 3 key string manipulation routines in general:

1. General purpose string manipulation - uses vanilla `char*` type, with null-terminator, with dedicated `str` namespace. The main purpose is to make strings easy to work with, and keeping them C compatible. `str` namespace uses allocators for all memory allocating operations, which allows us to use temporary allocations with `tmem$`.
2. String slicing - sometimes we need to obtain and work with a part of existing string, so CEX use `str_s` type for defining slices. There is dedicated sub-namespace `str.slice` which is specially designed for working with slices. Slices may or may not be null-terminated, they carry pointer and length. Typically is a quick and non-allocating way of working of string view representation.
3. String builder - in the case if we need to build string dynamically we may use `sbuf_c` type and `sbuf` namespace in CEX. This type is dedicated for dynamically growing strings backed by allocator, that are always null-terminated and compatible with `char*` without casting.

Cex strings follow these principles:

* Security first - all strings are null-terminated, all buffer related operations always checking bounds.
* NULL-tolerant - all strings may accept NULL pointers and return NULL result on error. This significantly reduces the count of `if(s == NULL)` error checks after each function, allowing to chain string operations and check `NULL` at the last step.
* Memory allocations are explicit - if string function accepts `IAllocator` this is indication of allocating behavior.
* Developer convenience - sometimes it's easier to allocate and make new formatted string on `tmem$` for example `str.fmt(_, "Hello: %s", "CEX")`, or use builtin pattern matching engine `str.match(arg[1], "command_*_(insert|delete|update))")`, or work with read-only slice representation of constant strings.


> [!TIP]
>
> To get brief cheat sheet on functions list via Cex CLI type `./cex help str$` or `./cex help sbuf$`

### General purpose strings

Use `str` for general purpose string manipulation, this namespace typically returns `char*` or NULL on error, all functions are tolerant to NULL arguments of `char*` type and re-return NULL in this case. Each allocating function must have `IAllocator` argument, and will return NULL on memory errors.

```c
    char*           str.clone(char* s, IAllocator allc);
    Exception       str.copy(char* dest, char* src, usize destlen);
    bool            str.ends_with(char* s, char* suffix);
    bool            str.eq(char* a, char* b);
    bool            str.eqi(char* a, char* b);
    char*           str.find(char* haystack, char* needle);
    char*           str.findr(char* haystack, char* needle);
    char*           str.fmt(IAllocator allc, char* format,...);
    char*           str.join(char** str_arr, usize str_arr_len, char* join_by, IAllocator allc);
    usize           str.len(char* s);
    char*           str.lower(char* s, IAllocator allc);
    bool            str.match(char* s, char* pattern);
    int             str.qscmp(const void* a, const void* b);
    int             str.qscmpi(const void* a, const void* b);
    char*           str.replace(char* s, char* old_sub, char* new_sub, IAllocator allc);
    str_s           str.sbuf(char* s, usize length);
    arr$(char*)     str.split(char* s, char* split_by, IAllocator allc);
    arr$(char*)     str.split_lines(char* s, IAllocator allc);
    Exc             str.sprintf(char* dest, usize dest_len, char* format,...);
    str_s           str.sstr(char* ccharptr);
    bool            str.starts_with(char* s, char* prefix);
    str_s           str.sub(char* s, isize start, isize end);
    char*           str.upper(char* s, IAllocator allc);
    Exception       str.vsprintf(char* dest, usize dest_len, char* format, va_list va);

```

### String slices

CEX has a special type and namespace for slices, which is a dedicated struct of `(len, char*)` fields, intended for working with parts of other strings, or can be a representation of a null-terminated string of full length.

#### Creating string slices

```c
char* my_cstring = "Hello CEX";

// Getting a sub-string of a C string
str_s my_cstring_sub = str.sub(my_cstring, -3, 0); // Value: CEX, -3 means from end of my_cstring

// Created from any other null-terminated C string
str_s my_slice = str.sstr(my_cstring);

// Statically initialized slice with compile time known length
str_s compile_time_slice = str$s("Length of this slice created compile time"); 

// Making slice from a buffer (may not be null-terminated)
char buf[100] = {"foo bar"}; 
str_s my_slice_buf = str.sbuf(buf, arr$len(buf));

```

> [!NOTE]
>
> `str_s` types are always passed by value, it's a 16-byte struct, which fits 2 CPU registers on x64


#### Using slices

Once slice is created and you see `str_s` type, it's only safe to use special functions which work only with slices, because null-termination is not guaranteed anymore.

There are plenty of operations which can be made only on string view, without touching underlying string data.

```c

char*           str.slice.clone(str_s s, IAllocator allc);
Exception       str.slice.copy(char* dest, str_s src, usize destlen);
bool            str.slice.ends_with(str_s s, str_s suffix);
bool            str.slice.eq(str_s a, str_s b);
bool            str.slice.eqi(str_s a, str_s b);
isize           str.slice.index_of(str_s s, str_s needle);
str_s           str.slice.iter_split(str_s s, char* split_by, cex_iterator_s* iterator);
str_s           str.slice.lstrip(str_s s);
bool            str.slice.match(str_s s, char* pattern);
int             str.slice.qscmp(const void* a, const void* b);
int             str.slice.qscmpi(const void* a, const void* b);
str_s           str.slice.remove_prefix(str_s s, str_s prefix);
str_s           str.slice.remove_suffix(str_s s, str_s suffix);
str_s           str.slice.rstrip(str_s s);
bool            str.slice.starts_with(str_s s, str_s prefix);
str_s           str.slice.strip(str_s s);
str_s           str.slice.sub(str_s s, isize start, isize end);
```

> [!NOTE]
>
> All Cex formatting functions (e.g. `io.printf()`, `str.fmt()`) support special format `%S` dedicated for string slices, allowing to work with slices naturally.

```c
char* my_cstring = "Hello CEX";
str_s my_slice = str.sstr(my_cstring);
str_s my_sub = str.slice.sub(my_slice, -3, 0);

io.printf("%S - Making Old C Cexy Again\n", my_sub);
io.printf("buf: %c %c %c len: %zu", my_sub.buf[0], my_sub.buf[1], my_sub.buf[2], my_sub.len);
```

#### Error handling
On error all slice related routines return empty `(str_s){.buf = NULL, .len = 0}`, all routines check if `.buf == NULL` therefore it's safe to pass empty/error slice multiple times without need for checking errors after each call. This allows operations chaining like this:

```c
str_s my_sub = str.slice.sub(my_slice, -3, 0);
my_sub = str.slice.remove_prefix(my_sub, str$s("pref"));
my_sub = str.slice.strip(my_sub);
if (!my_sub.buf) {/* OOPS error */}
```

### String conversions

When working with strings, conversion from string into numerical types becomes very useful. Libc conversion functions are messy and error-prone, CEX uses own implementation, with support for both `char*` and slices `str_s`.

You may use one of the functions above or pick type-safe/generic macro `str$convert(str_or_slice, out_var_pointer)`

```c
Exception       str.convert.to_f32(char* s, f32* num);
Exception       str.convert.to_f32s(str_s s, f32* num);
Exception       str.convert.to_f64(char* s, f64* num);
Exception       str.convert.to_f64s(str_s s, f64* num);
Exception       str.convert.to_i16(char* s, i16* num);
Exception       str.convert.to_i16s(str_s s, i16* num);
Exception       str.convert.to_i32(char* s, i32* num);
Exception       str.convert.to_i32s(str_s s, i32* num);
Exception       str.convert.to_i64(char* s, i64* num);
Exception       str.convert.to_i64s(str_s s, i64* num);
Exception       str.convert.to_i8(char* s, i8* num);
Exception       str.convert.to_i8s(str_s s, i8* num);
Exception       str.convert.to_u16(char* s, u16* num);
Exception       str.convert.to_u16s(str_s s, u16* num);
Exception       str.convert.to_u32(char* s, u32* num);
Exception       str.convert.to_u32s(str_s s, u32* num);
Exception       str.convert.to_u64(char* s, u64* num);
Exception       str.convert.to_u64s(str_s s, u64* num);
Exception       str.convert.to_u8(char* s, u8* num);
Exception       str.convert.to_u8s(str_s s, u8* num);

```

For example:
```c
i32 num = 0;
s = "-2147483648";

// Both are equivalent
e$ret(str.convert.to_i32(s, &num));
e$ret(str$convert(s, &num));
```

### Dynamic strings / string builder

If you need to build string dynamically you can use `sbuf_c` type, which is simple alias for `char*`, but with special logic attached. This type implements dynamic growing / shrinking, and formatting of strings with null-terminator.


#### Example
```c
sbuf_c s = sbuf.create(5, mem$); /* <1> */

char* cex = "CEX";
e$ret(sbuf.appendf(&s, "Hello %s", cex)); /* <2> */
e$assert(str.ends_with(s, "CEX")); /* <3> */

sbuf.destroy(&s);
```
1. Creates new dynamic string on heap, with 5 bytes initial capacity
2. Appends text to string with automatic resize (memory reallocation)
3. `s` variable of type `sbuf_c` is compatible with any `char*` routines, because it's an alias of `char*`

> [!TIP]
>
> If you need one-shot format for string try to use `str.fmt(allocator, format, ...)` inside temporary allocator `mem$scope(tmem$, _)`

#### `sbuf` namespace

```c
    /// Append string to the builder
    Exc             sbuf.append(sbuf_c* self, char* s);
    /// Append format (using CEX formatting engine)
    Exc             sbuf.appendf(sbuf_c* self, char* format,...);
    /// Append format va (using CEX formatting engine), always null-terminating
    Exc             sbuf.appendfva(sbuf_c* self, char* format, va_list va);
    /// Returns string capacity from its metadata
    u32             sbuf.capacity(sbuf_c* self);
    /// Clears string
    void            sbuf.clear(sbuf_c* self);
    /// Creates new dynamic string builder backed by allocator
    sbuf_c          sbuf.create(usize capacity, IAllocator allocator);
    /// Creates dynamic string backed by static array
    sbuf_c          sbuf.create_static(char* buf, usize buf_size);
    /// Destroys the string, deallocates the memory, or nullify static buffer.
    sbuf_c          sbuf.destroy(sbuf_c* self);
    /// Returns false if string invalid
    bool            sbuf.isvalid(sbuf_c* self);
    /// Returns string length from its metadata
    u32             sbuf.len(sbuf_c* self);
    /// Sets the length of a string to any value, if new_length greater than capacity, re-allocates more
    /// space, always null-terminating. Newly allocated space is not ZII'ed, you must fill it yourself.
    Exc             sbuf.set_len(sbuf_c* self, usize new_length);
    /// Validate dynamic string state, with detailed Exception
    Exception       sbuf.validate(sbuf_c* self);
```

### String formatting in CEX
All CEX routines with format strings (e.g. `io.printf()`/`log$error()`/`str.fmt()`) use CEX special formatting engine with extended features:

* `%S` format specifier is used for printing string slices of `str_s` type
* `%S` format has sanity checks in case a simple string is passed to its place, it will print `(%S-bad/overflow)` in the text. However, it's not guaranteed behavior, and depends on platform.
* `%lu`/`%ld` - formats are dedicated for printing 64-bit integers, they are not platform specific
* `%u`/`%d` - formats are dedicated for printing 32-bit integers, they are not platform specific
* Other formats should be compatible with vanilla libC.


<a id="lang-data-structures"></a>

## Data structures and arrays


### Data structures in CEX
There is a lack of support for data structures in C, typically it's up to the developer to decide what to do. However, I noticed that many other C projects tend to reimplement over and over again two core data structures, which are used in 90% of cases: dynamic arrays and hashmaps.

Key requirements of the CEX data structures:

* Allocator based memory management - allowing you to decide memory model and tweak it anytime.
* Type safety and LSP support - each DS must have a specific type and support LSP suggestions.
* Generic types - DS must be generic.
* Seamless C compatibility - allowing accessing CEX DS as plain C arrays and pass them as pointers.
* Supports any item type including overaligned.

### Dynamic arrays
Dynamic arrays (a.k.a. vectors or lists) are designed specifically for developer convenience and based on ideas of Sean Barrett's STB DS.

#### What is dynamic array in CEX
Technically speaking it's a simple C pointer `T*`, where `T` is any generic type. The memory for that pointer is allocated by allocator, and its length is stored at some byte offset before the address of the dynamic array head.

With this type representation we can get some useful benefits:

* Array access with simple indexing, i.e. `arr[i]` instead of `dynamic_arr_get_at(arr, i)`
* Passing by pointer into vanilla C code. For example, a function signature `my_func(int* arr, usize arr_len)`  is compatible with `arr$(int*)`, so we can call it as `my_func(arr, arr$len(arr))`
* Passing length information integrated into single pointer, `arr$len(arr)` extracts length from dynamic array pointer
* Type safety out of the box and full LSP support without dealing with `void*`


#### `arr$` namespace
`arr$` is completely macro-driven namespace, with generic type support and safety checks.

`arr$` API:

| Macro | Description                                                             |
| -------------- |-------------------------------------------------------------------------|
| arr\$(T) | Macro type definition, just for indication that it's a dynamic array    |
| arr\$new(arr, allocator, kwargs...) | Initialization of the new instance of dynamic array                     | 
| arr\$free(arr) | Dynamic array cleanup (if HeapAllocator was used)                       |
| arr\$clear(arr) | Clearing dynamic array contents                                         |
| arr\$push(arr, item) | Adding new item to the end of array                                     |
| arr\$pushm(arr, item, item1, itemN) | Adding many new items to the end of array                               |
| arr\$pusha(arr, other_arr, \[other_arr_len\]) | Adding another array to the end of array                                |
| arr\$pop(arr) | Returns last element and removes it                                     |
| arr\$at(arr, i) | Returns element at index with boundary checks for i                     |
| arr\$last(arr) | Returns last element                                                    |
| arr\$del(arr, i) | Removes element at index (following data is moved at the i-th position) |
| arr\$delswap(arr, i) | Removes element at index, the removed element is replaced by last one   |
| arr\$ins(arr, i, value) | Inserts element at index                                                |
| arr\$grow_check(arr, add_len) | Grows array by `add_len` if needed                                      |
| arr\$sort(arr, qsort_cmp) | Sorting array with `qsort` function                                     |


#### Examples

::: {.panel-tabset}
##### Initialization
```c
arr$(int) arr = arr$new(arr, mem$); /* <1> */
arr$push(arr, 1);  /* <2> */
arr$pushm(arr, 2, 3, 4); /* <3> */
int static_arr[] = { 5, 6 };
arr$pusha(arr, static_arr /*, array_len (optional) */); /* <4> */

io.printf("arr[0]=%d\n", arr[0]); // prints arr[0]=1

// Iterate over array: prints lines 1 ... 6
for$each (v, arr) { /* <5> */
    io.printf("%d\n", v); 
}

arr$free(arr); /* <6> */
```

1. Initialization and allocator
2. Adding single element
3. Adding multiple elements via vargs.
4. Adding arbitrary array, supports static arrays, dynamic CEX arrays or `int*`+arr_len
5. Array iteration via `for$each` is common and compatible with all arrays in Cex (dynamic, static, pointer+len)
6. Deallocating memory (only needed when HeapAllocator is used)

##### Overaligned structs
```c

test$case(test_overaligned_struct)
{
    struct test32_s
    {
        alignas(32) usize s;
    };

    arr$(struct test32_s) arr = arr$new(arr, mem$);
    struct test32_s f = { .s = 100 };
    tassert(mem$aligned_pointer(arr, 32) == arr);

    for (u32 i = 0; i < 1000; i++) {
        f.s = i;
        arr$push(arr, f);

        tassert_eq(arr$len(arr), i + 1);
    }
    tassert_eq(arr$len(arr), 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eq(arr[i].s, i);
        tassert(mem$aligned_pointer(&arr[i], 32) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}
```

##### Array of strings

```c
test$case(test_array_char_ptr)
{
    arr$(char*) array = arr$new(array, mem$);
    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$pushm(array, "baz", "CEX", "is", "cool");
    for (usize i = 0; i < arr$len(array); ++i) { io.printf("%s \n", array[i]); }
    arr$free(array);

    return EOK;
}

```

##### Array inside `tmem$`
```c
mem$scope(tmem$, _) /* <1> */
{
    arr$(char*) incl_path = arr$new(incl_path, _, .capacity = 128); /* <2> */
    for$each (p, alt_include_path) {
        arr$push(incl_path, p);  /* <3> */
        if (!os.path.exists(p)) { log$warn("alt_include_path not exists: %s\n", p); }
    }
} /* <4> */
```
1. Initializes a temporary allocator (`tmem$`) scope in `mem$scope(tmem$, _) {...}` and assigns it as a variable `_` (you can use any name).
2. Initializes dynamic array with the scoped allocator variable `_`, allocates with specific capacity argument.
3. May allocate memory
4. All memory will be freed at exit from this scope


:::

### Hashmaps

Hashmaps (`hm$`) in CEX are backed by structs with `key` and `value` fields, essentially they are backed by plain dynamic arrays of structs (iterable values) with hash table part for implementing keys hashing.

Hashmaps in CEX are also generic, you may use any type of keys or values. However, there are special handling for string keys (`char*`, or `str_s` CEX slices). Typically, string keys are not copied by hashmap by default, and stored by reference, so you'll have to keep their allocation stable.

Hashmap initialization is similar to the dynamic arrays, you should define a type and call `hm$new`.

#### Array compatibility
Hashmaps in CEX are backed by dynamic arrays, which leads to the following developer experience enhancements:

* `arr$len` can be applied to hashmaps for checking number of available elements
* `for$each/for$eachp` can be used for iteration over hashmap key/values pairs
* Hashmap items can be accessed as arrays with index

#### Initialization
There are several ways for declaring hashmap types:

1. Local function hashmap variables
```c
    hm$(char*, int) intmap = hm$new(intmap, mem$);
    hm$(const char*, int) map = hm$new(map, mem$);
    hm$(struct my_struct, int) map = hm$new(map, mem$);

```

2. Global hashmaps with special types
```c

// NOTE: struct must have the .key and .value fields
typedef struct
{
    int key;
    float my_val;
    char* my_string;
    int value;
} my_hm_struct;

void foo(void) {
    // NOTE: this is equivalent of my_hm_struct* map = ...
    hm$s(my_hm_struct) map = hm$new(map, mem$);
}

void my_func(hm$s(my_hm_struct)* map) {
    // NOTE: passing hashmap type, parameter
    int v = hm$get(*map, 1);

    // NOTE: hm$set() may resize map, because of this we use `* map` argument, for keeping the pointer valid!
    hm$set(*map, 3, 4);

    // Setting entire structure
    hm$sets(*map, (my_hm_struct){ .key = 5, .my_val = 3.14, .my_string = "cexy", .value = 98 }));
}

```

3. Declaring hashmap as a type

```c
typedef hm$(char*, int) MyHashMap;

struct my_hm_struct {
    MyHashmap hm;
};

void foo(void) {
    // Initializing a new variable
    MyHashMap map = hm$new(map, mem$);
    
    // Initialing hashmap as a member of struct
    struct my_hm_struct hs = {0};
    hm$new(hs.hm, mem$);

}
```



#### Hashmap API
| Macro | Description |
| --------------- | --------------- | --------------- |
| hm\$new(hm, allocator, kwargs...) |  Initialization of hashmap |
| hm\$set(hm, key, value) | Set element |
| hm\$setp(hm, key, value) | Set element and return pointed to the newly added item inside hashmap |
| hm\$sets(hm, struct_value...) | Set entire element as backing struct |
| hm\$get(hm, key) | Get a value by key (as a copy) |
| hm\$getp(hm, key) | Get a value by key as a pointer to hashmap value |
| hm\$gets(hm, key) | Get a value by key as a pointer to a backing struct |
| hm\$clear(hm) | Clears contents of hashmap |
| hm\$del(hm, key) | Delete element by key |
| hm\$len(hm) | Number of elements in hashmap / `arr$len()` also works |

#### Initialization params
`hm$new` accepts optional params which may help you to adjust hashmap key behavior:

* `.capacity=16` - initial capacity of the hashmap, will be rounded to the closest power of 2 number
* `.seed=` - initial seed for hashing algorithm
* `.copy_keys=false` - enabling copy of `char*` keys and storing them specifically in hashmap
* `.copy_keys_arena_pgsize=0` - enabling using arena for `copy_keys` mode

Example:
```c
test$case(test_hashmap_string_copy_arena)
{
    hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true, .copy_keys_arena_pgsize = 1024);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");

    // Initial buffer gets destroyed, but hashmap keys remain the same
    memset(key2, 0, sizeof(key2));

    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$get(smap, "foo"), 3);

    hm$free(smap);
    return EOK;
}

```

#### Examples
::: {.panel-tabset}
##### Hashmap basic use
```c
hm$(char*, int) smap = hm$new(smap, mem$);
hm$set(smap, "foo", 3);
hm$get(smap, "foo");
hm$len(smap);
hm$del(smap, "foo");
hm$free(smap);

```

##### String key hashmap
```c
test$case(test_hashmap_string)
{
    char key_buf[10] = "foobar";

    hm$(char*, int) smap = hm$new(smap, mem$);

    char* k = "foo";
    char* k2 = "baz";

    char key_buf2[10] = "foo";
    char* k3 = key_buf2;
    hm$set(smap, "foo", 3);

    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, k), 3);
    tassert_eq(hm$get(smap, key_buf2), 3);
    tassert_eq(hm$get(smap, k3), 3);

    tassert_eq(hm$get(smap, "bar"), 0);
    tassert_eq(hm$get(smap, k2), 0);
    tassert_eq(hm$get(smap, key_buf), 0);

    tassert_eq(hm$del(smap, key_buf2), 1);
    tassert_eq(hm$len(smap), 0);

    hm$free(smap);
    return EOK;
}

```

##### Iterating elements
```c
test$case(test_hashmap_basic_iteration)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$set(intmap, 1, 10);
    hm$set(intmap, 2, 20);
    hm$set(intmap, 3, 30);

    tassert_eq(hm$len(intmap), 3);  // special len
    tassert_eq(arr$len(intmap), 3); // NOTE: arr$len is compatible

    // Iterating by value (data is copied)
    u32 nit = 1;
    for$each (it, intmap) {
        tassert_eq(it.key, nit);
        tassert_eq(it.value, nit * 10);
        nit++;
    }

    // Iterating by pointers (data by reference)
    for$eachp(it, intmap)
    {
        isize _nit = intmap - it; // deriving index from pointers
        tassert_eq(it->key, _nit);
        tassert_eq(it->value, _nit * 10);
    }

    hm$free(intmap);

    return EOK;
}

```

:::

### Working with arrays

Arrays are probably the most used concept in any language, C arrays may have many different forms. Unfortunately, the main problem of working with arrays in C is a specialization of methods and operations, each type of array may require special iteration macro, or function for getting array length or element.

Collection types in C:

* Static arrays `i32 arr[10]`
* Dynamic arrays as pointers `(i32* arr, usize arr_len)`
* Custom dynamic arrays `dynamic_array_push_back(&int_array, &i);`
* Char buffers `char buf[1024]`
* Null-terminated strings and slices
* Hashmaps

Cex tries to solve this by unification of all arrays operations around standard design principles, without getting too far away from standard C.

#### `arr$len` unified length

`arr$len(array)` macro is an ultimate tool for getting lengths of arrays in CEX. It supports: static arrays, char buffers, string literals, dynamic arrays of CEX `arr$` and hashmaps of CEX `hm$`. Also, it's a NULL-resilient macro, which returns 0 if `array` argument is NULL.

> [!NOTE]
>
> Not all array pointers are supports by `arr$len` (only dynamic arrays or hashmaps are valid), however in debug mode `arr$len` will raise an assertion/ASAN crash if you passed wrong pointer type there.

Example:
```c
test$case(test_array_len)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    // Works with CEX dynamic arrays
    tassert_eq(arr$len(array), 3);

    // NULL is supported, and emits 0 length
    arr$free(array);
    tassert(array == NULL); 
    tassert_eq(arr$len(array), 0); // NOTE: NULL array - len = 0

    // Works with static arrays
    char buf[] = {"hello"}; 
    tassert_eq(arr$len(buf), 6); // NOTE: includes null term

    // Works with arrays of given capacity
    char buf2[10] = {0};
    tassert_eq(arr$len(buf2), 10);

    // Type doesn't matter
    i32 a[7] = {0};
    tassert_eq(arr$len(a), 7);

    // Works with string literals
    tassert_eq(arr$len("CEX"), 4); // NOTE: includes null term

    // Works with CEX hashmap
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$set(intmap, 1, 3);
    tassert_eq(arr$len(intmap), 1);

    hm$free(intmap);

    return EOK;
}

```

#### Accessing elements of array is unified

```c
test$case(test_array_access)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    // Dynamic array access is natural C index
    tassert_eq(array[2], 3);
    // tassert_eq(arr$at(array, 3), 3); // NOTE: this is bounds checking access, with assertion 
    arr$free(array);

    // Works with static arrays
    char buf[] = {"hello"}; 
    tassert_eq(buf[1], 'e'); 

    // Works with CEX hashmap
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$set(intmap, 1, 3);
    hm$set(intmap, 2, 5);
    tassert_eq(arr$len(intmap), 2);

    // Accessing hashmap as array
    // NOTE: hashmap elements are ordered until first deletion
    tassert_eq(intmap[0].key, 1);
    tassert_eq(intmap[0].value, 3);

    tassert_eq(intmap[1].key, 2);
    tassert_eq(intmap[1].value, 5);

    hm$free(intmap);

    return EOK;
}
```

#### CEX way of iteration over arrays

CEX introduces a unified `for$*` macros which helps with dealing with looping, these are typical patters for iteration:

* `for$each(it, array, [array_len])` - iterates over array, `it` represents value of array item. `array_len` is optional and uses `arr$len(array)` by default, or you might explicitly set it for iterating over arbitrary C pointer+len arrays.
* `for$eachp(it, array, [array_len])` - iterates over array, `it` represent a pointer to array item. `array_len` is inferred by default.
* `for$iter(it_val_type, it, iter_funct)` - a special iterator for non-indexable collections or function based iteration, tailored for customized iteration of unknown length.
* `for(usize i = 0; i < arr$len(array); i++)` - the classic also works :)

```c
test$case(test_array_iteration)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    i32 nit = 0; // it's only for testing
    for$each(it, array) {
        tassert_eq(it, ++nit);
        io.printf("el=%d\n", it);
    }
    // Prints: 
    // el=1
    // el=2
    // el=3

    nit = 0;
    // NOTE: prefer this when you work with bigger structs to avoid extra memory copying
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;
        tassert_eq(i, nit);

        // NOTE: it now is a pointer
        tassert_eq(*it, ++nit);
        io.printf("el[%zu]=%d\n", i, *it);
    }
    // Prints: 
    // el[0]=1
    // el[1]=2
    // el[2]=3

    // Static arrays work as well (arr$len inferred)
    i32 arr_int[] = {1, 2, 3, 4, 5};
    for$each(it, arr_int) {
        io.printf("static=%d\n", it);
    }
    // Prints:
    // static=1
    // static=2
    // static=3
    // static=4
    // static=5


    // Simple pointer+length also works (let's do a slice)
    i32* slice = &arr_int[2];
    for$each(it, slice, 2) {
        io.printf("slice=%d\n", it);
    }
    // Prints:
    // slice=3
    // slice=4

    arr$free(array);
    return EOK;
}

```

### Making custom collection iterators
It's possible to make custom iterator, specifically for unbounded collections or sparse data structures. However, this iteration has higher overhead than simple `for$each` loop, but sometimes it's necessary.

> [!NOTE]
>
> Consider using `iter_` prefix of the function name, by convention, it's a good indicator of using `for$iter()`

Example, of how `str.slice.iter_split()` was implemented:

```c


typedef struct
{
    struct
    {
        union
        {
            usize i;
            char* skey;
            void* pkey;
        };
    } idx;
    char _ctx[47]; // <<< use this buffer to store iterator state, it's usize aligned
    u8 stopped;
    u8 initialized;
} cex_iterator_s;
static_assert(sizeof(cex_iterator_s) <= 64, "cex size");

static str_s
cex_str__slice__iter_split(str_s s, char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        usize cursor;
        usize split_by_len;
        usize str_len;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    static_assert(alignof(struct iter_ctx) <= alignof(usize), "cex_iterator_s _ctx misalign");

    if (unlikely(!iterator->initialized)) {
        // First run handling

        iterator->initialized = 1;
        if (unlikely(!_cex_str__isvalid(&s) || s.len == 0)) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->split_by_len = strlen(split_by);
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        if (ctx->split_by_len == 0) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }

        isize idx = _cex_str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) { idx = s.len; }
        ctx->cursor = idx;
        ctx->str_len = s.len; // this prevents s being changed in a loop
        iterator->idx.i = 0;
        if (idx == 0) {
            // first line is \n
            return (str_s){ .buf = "", .len = 0 };
        } else {
            return str.slice.sub(s, 0, idx);
        }
    } else {
        if (unlikely(ctx->cursor >= ctx->str_len)) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == ctx->str_len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            iterator->idx.i++;
            return (str_s){ .buf = "", .len = 0 };
        }

        // Get remaining string after prev split_by char
        str_s tok = str.slice.sub(s, ctx->cursor, 0);
        isize idx = _cex_str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->cursor = s.len;
            // iterator->stopped = 1;
            return tok;
        } else if (idx == 0) {
            return (str_s){ .buf = "", .len = 0 };
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->cursor += idx;
            return str.slice.sub(tok, 0, idx);
        }
    }
}
```

## Namespaces
Naming collisions will always remain a problem of C language. However, we could try our best to reduce surface of conflict, by aggregating functions with prefixes to nice-looking namespace symbols. But the primary role of CEX namespacing approach is to keep project structure organized, easier to navigate and understand. Another beneficial effect of using namespaces is reduction of cognitive work when we try to recall the function name when typing with LSP, we'll see this effect below. At last, using namespacing we can add OOP-ish flavor to our structures, which could behave as classes.

### Key features of namespaces
* They can be automatically generated from `.c` file, **no need** for maintaining changes in `.h` for every function signature change.
* Helping to maintain naming conventions name of the `.c` file must be the same as namespace
* Reducing surface for name collisions, only global namespace name is exposed.
* Allowing support of sub-namespaces, easier to remember and type with LSP
* Less symbols in LSP suggestions
* Better readability with `.` separator, and color highlighting of different parts of the function call
* Namespace structure combines function signatures closely in one place, so it's easier to figure out what functions are available.

### CEX Namespaces in the nutshell

CEX namespace is a global **const** struct, with function pointers in it.

```c

#define CEX_NAMESPACE __attribute__((visibility("hidden"))) extern const

typedef struct  {...} KeyMap_c;

struct __cex_namespace__KeyMap {
    // Autogenerated by CEX
    // clang-format off

    /// NOTE: Cex may generate brief doc string here, if was added prior function implementation in .c file
    Exception       (*create)(KeyMap_c* self, char* input_dev_or_name);
    // Destroys KeyMap instance
    void            (*destroy)(KeyMap_c* self);
    Exception       (*find_mapped_keyboard)(KeyMap_c* self, char* keyboard_name);
    Exception       (*handle_events)(KeyMap_c* self);
    Exception       (*handle_key)(KeyMap_c* self, struct input_event* ev);
    Exception       (*handle_mouse_move)(KeyMap_c* self);

    // clang-format on
};

CEX_NAMESPACE struct __cex_namespace__KeyMap KeyMap;

```


So the `KeyMap` namespace allowing the following usage:
```c

KeyMap_c keymap = { 0 }; /* <1> */
e$goto(KeyMap.create(&keymap, file), end); /* <2> */
e$goto(KeyMap.handle_events(&keymap), end); /* <3> */


```
1. `_c` suffix of `KeyMap_c` is an indication of namespace, it can be interpreted as `class` or `has code` conceptually.
2. Functions of `KeyMap` namespace are separated by dots, it's easier to read, and type with LSP, because it filters only relevant information. See pictures below.
3. Dotted notation may get distinct color highlighting which help to distinguish `namespace` and its `function`

### LSP Suggestions are much better

1. If you start typing conventional `KeyMap_create()` function name, the LSP suggestions will get cluttered, fuzzy typing may return not what you want
2. With CEX namespace you get only list of `KeyMap` functions, and fuzzy typing works way better because you have limited options

::: {#fig-namespaces layout-ncol=2}

![Vanilla](_include/namespace_keymap_vanilla.png){#fig-vanilla}

![KeyMap](_include/namespace_keymap_lsp.png){#fig-lsp}

LSP Suggestions
:::


### Sub-namespaces

Sometimes libraries or namespaces can have dozens of functions, so it's more convenient to add extra level of namespacing. For example, CEX `str` namespace has many functions which are grouped by functionality. `str.slice.` works with `str_s` types, `str.convert.` dealing with conversions, some functions take place in the root namespace, for example `str.find()`.

Sub-namespaces allow building a mental model of code, and help write function names as a decision tree. For example, if I need `str`, `.`, then I need deal with slice `slice`, `.`, then I have to find exact thing what I need.


::: {#fig-namespaces layout-ncol=3}

![Start](_include/namespace_str_1.png){#fig-str-1 fig-pos='b'}

![Slice](_include/namespace_str_2.png){#fig-str-2 fig-pos='b'}

![Function](_include/namespace_str_3.png){#fig-str-3 fig-pos='b'}

LSP For sub-namespaces
:::

> [!NOTE]
>
> Check full `str` namespace options with `./cex help str$`

### How to make a namespace

#### Making code
For example:  you need to add new `foo` namespace

1. Create a pair of files with name prefix `src/foo.c` and `src/foo.h`
2. You can create `static` functions `foo_fun1()`, `foo_fun2()`, `foo__bar__fun3()`, `foo__bar__fun4()`. These functions will be processed and wrapped into a `foo` namespace so you can
   access them via `foo.fun1()`, `foo.fun2()`, `foo.bar.fun3()`, `foo.bar.fun4()`
3. Run `./cex process src/foo.c`

#### Requirements / caveats:

1. You must have `foo.c` and `foo.h` in the same folder
2. Filename must start with `foo` - namespace prefix
3. Each function in `foo.c` that you'd like to add to namespace must start with `foo_`
4. For adding sub-namespace use `foo__subname__` prefix
5. Only one level of sub-namespace is allowed
6. You may not declare function signature in header, and only use .c static functions
7. Functions with `static inline` are not included into namespace
8. Functions with prefix `foo__some` are considered internal and not included
9. New namespace is created when you use exact `src/foo.c` argument, `all` just for updates

#### Style conventions
In my experience, it's helpful to distinguish between type of code (namespace) we are dealing with, nothing strict, just guidelines:

1. Sometimes we need OOP-ish / object / class behavior, which wraps a `typedef struct MyClass_c`, with constructor `MyClass.create()` and destructor `MyClass.destroy(MyClass_c* self)`. This type of code should be placed into `MyClass.c/MyClass.h` files.
2. Sometimes we need just a bunch of functions logically combined and dealing with different set of types, then we should use lower case name `foo`, or `my_namespace`. For example, CEX `str` namespace. Also, you may want to add `_c` suffix to the `typedef struct my_type_c` to indicate that it has a namespace code attached `my_type.some_func()`.

#### CLI Commands

```sh
# More help
➜ ./cex process --help

# Creates new `foo` namespace or update existing one 
➜ ./cex process src/foo.c 

# Update all existing namespaces in the project
#   use it after you change signatures of your functions
➜ ./cex process all

```


### Special notes

#### Performance questions
While namespaces are the static structures with pointers in them, they may be a cause of performance hit for calling functions without compiler optimization enabled (the same as C++ virtual functions hit). However, modern compilers are smart enough to replace function pointer dereferencing call with direct function call when `-O1` optimization is enabled.

#### Getting LSP help / goto definition
CEX Namespaces work with `clangd` LSP server pretty well. However, LSP help functionality is limited, we can get only list of parameters for completion.

Go to definition works, but with some caveats. If you place cursor (`|`) at `KeyMap.cre|ate`  and do go to definition in LSP, it will jump onto `KeyMap` structure type. If you need to goto implementation place cursor like this `KeyM|ap.create`, and goto definition, it will jump on the `KeyMap` struct implementation inside `KeyMap.c` file. Then find `KeyMap_create` record and jump at it once again.

#### Sharing namespaces as library
If you are going to make shared library (.so or .dll) probably CEX namespaces are not the best fit for this. They should work, but you probably get performance hit of indirect function calls. In this particular case, it's better to use vanilla C functions.

## Build system
CEX has an integrated build system `cexy$`, inspired by Zig-build and Tsoding's `nob.h`. It allows you to build your project without dealing with CMake/Make/Ninja/Meson dependencies. For small projects cexy has simplified mode when build is config-driven. For complex or cross-platform projects `cexy` enables low-level tools for running the compiler and building specific project assembly logic.

### How it works
1. You need to create `cex.c` file, which is entry point for all building process and cexy tools. For the newer projects, if `cex.c` is not there, run the bootstrapping routine:

```sh
cc -D CEX_NEW -x c ./cex.h -o ./cex
```

2. Then you should compile `cex` CLI, simply using following command:
```sh
cc ./cex.c -o ./cex
```

3. Afterwards you should have the `./cex` executable in project directory. It's your main entry point for CEX project management and your project is ready to go.

Now you can launch a sample program or run its unit tests.
```sh
./cex test run all
./cex app run myapp
```

### Unity build principles

CEX uses the **unity build** approach — instead of separate compilation and linking, source files are included directly via `#include`. This avoids symbol visibility issues, simplifies build configuration, and enables whole-program optimization.

Each app or test file includes the specific source files it needs:

```c
// tests/test_foo.c — include internal sources directly
#include "src/module1.c"
#include "src/module2.c"
// #include "src/all.c"   // or include everything at once
// #include "cex.h"       // or public API only
```

The key principle: **your app `.c` and test `.c` files are the compilation units**. They `#include` the sources they depend on, and the compiler produces a single executable in one pass. No separate compilation, no linker scripts.

Benefits:

- **No linker errors** from missing symbols or ODR violations
- **Whole-program optimization** — all code visible to the optimizer at once
- **Simplified build logic** — no Makefile/CMake for small-to-medium projects
- **White-box testing** — direct access to `static` functions for mocking and inspection
- **Mocks are trivial** — `#define` a symbol before including the unit under test to replace it
- **Isolated unit testing** — include only the specific modules under test rather than the whole project, allowing you to test small, isolated parts of a larger codebase without pulling in unrelated dependencies or side effects
- **Faster full builds** — a single compiler invocation eliminates object file I/O and linker overhead, making full rebuilds faster than traditional separate-compilation setups for small-to-medium projects

In simple mode, `./cex app build myapp` and `./cex test run tests/test_file.c` handle all the `#include` plumbing automatically — the build system tracks modification times of included files and recompiles only when a dependency has changed. You just follow the convention of placing your app's `main()` in `src/<name>.c` and tests in `tests/test_<name>.c`.

### Key-features of cexy$ CLI tool

- Main project management CLI: building, running unit tests, fuzzer, stats, etc
- Generates new apps or projects
- Generates CEX namespaces for user code
- Fuzzy search for help in user codebase
- Supports custom command runner
- Supports build-mode configuration
- Supports OS related operations with files, paths, command launching, etc
- Adds support for external dependencies via pkg-config and vcpkg
- UnitTest and Fuzzer runner
- Fetches 3rd party code, updates `cex.h` itself or `cex lib` via git

### Simple-mode
`cexy$` has built-in build routine for building/running/debugging apps, running unit tests and fuzzers. It can be configured using `#    define cexy$<config-constant-here>` in your `cex.c` file.

> [!TIP] Getting cexy$ help about config variables
> ```sh
> # full list of cexy API namespace and cexy$ variables
> ./cex help cexy$
> # list of actual values for cexy$ vars in current project
> ./cex config
> ```

When you run `./cex app run|test|fuzz myapp` it uses `cexy$` config vars internally, and runs build routine which may cover 80% of generic project needs.

Simple mode add several project structure constraints:

1. Source code should be in `src/` directory
2. If you have `myapp` application its `main()` function should be located at `src/myapp.c` or `src/myapp/main.c`
3. Simple-mode uses unity build approach, so all your sources have to be included as `#include "src/foo.c"` in `src/myapp.c`.
4. Simple-mode does not produce object files and does not do extra linking stage. It's intentional, and in my opinion is better for smaller/medium (<100k LOC) projects.

### Fast debug builds

When compiling in debug mode (`cexy$cc_args` without any `-O` flag), simple mode automatically pre-compiles `cex.h` into a shared object file. This avoids re-parsing the ~700 KB header on every build, significantly speeding up incremental `./cex app` and `./cex test` commands.

The workflow:

1. **Hash-based caching** — compiler arguments are hashed to produce a unique `.obj` filename (`build/cex.<hash>.obj`).
2. **Staleness check** — the cached `.obj` is reused unless `cex.h` has been modified.
3. **Auto-link** — the precompiled object is linked via `-DCEX_PREBUILT` + the `.obj` path.

```c
// Example: debug build skips cex.h recompilation on subsequent runs
./cex app build myapp
// first run: compiles cex.h -> build/cex.a1b2c3.obj, then links myapp
// second run (no cex.h change): links directly from cached .obj
```

Key points:

- Only active in **simple mode** for `./cex app` and `./cex test` commands.
- Automatically skipped when any optimizer flag (`-O1`, `-O2`, `-O3`, `-Os`) is present — release builds always do a full unity compile.
- Disable with `#define cexy$disable_cex_precompiling` in your `cex.c`.

### Project configuration
`cexy$` is configured via setting constants in header files, which can be directly compiled as C code in your project as well. Use `./cex config` for checking current project configuration. Configuration can optionally be included as `cex_config.h` (or any other name), or directly set in `cex.c` file.

You can change pre-defined cexy config with `./cex -D<YOUR_VAR> config`, it will recompile cex CLI with new settings and all subsequent `./cex` call will be using new settings. You may reset to defaults with `./cex -D config`.

::: {.panel-tabset}
#### Step 1: Sample config

```c
// file: cex.c

#if __has_include("cex_config.h")
// Custom config file
#    include "cex_config.h"
#else
// Overriding config values
#    if defined(CEX_DEBUG)
#        define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#    else
#        define cexy$cc_args "-Wall", "-Wextra", "-Werror", "-g", "-O3", "-fwhole-program"
#    endif
#endif
```

#### Step 2: apply new preset
```sh
# Check current config (CEX_DEBUG not set, using -O3 gcc/clang argument)
./cex config
>>>
* cexy$cc_args              "-Wall", "-Wextra", "-Werror", "-g", "-O3", "-fwhole-program"
* ./cex -D<ARGS> config     ""
<<<

# Using CEX_DEBUG from `cex.c` (step 1 tab), you may use any name
./cex -DCEX_DEBUG config

# Check what's changed
./cex config
>>>
* cexy$cc_args              "-Wall", "-Wextra", "-Werror", "-g3", "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined", "-fsanitize=leak", "-fstack-protector-strong"
* ./cex -D<ARGS> config     "-DCEX_DEBUG "
<<<

# Revert previous config back
./cex -D config

```

:::

### Minimalist cexy build system
If you wish you could build using your own logic, let's make a simple custom build command, without utilizing cexy machinery.

::: {.panel-tabset}
#### Step 1: `cex.c`
```c
// file: cex.c

#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

Exception cmd_mybuild(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{

    cexy$initialize(); // cex self rebuild and init
    argparse_c args = {
        .description = cexy$description,
        .epilog = cexy$epilog,
        .usage = cexy$usage,
        argparse$cmd_list(
            cexy$cmd_all,
            // cexy$cmd_fuzz, /* disable built-in commands */
            // cexy$cmd_test, /* disable built-in commands */
            // cexy$cmd_app,  /* disable built-in commands */
            { .name = "my-build", .func = cmd_mybuild, .help = "My Custom build" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    void* my_user_ctx = NULL; // passed as `user_ctx` to command
    if (argparse.run_command(&args, my_user_ctx)) { return 1; }
    return 0;
}

Exception cmd_mybuild(int argc, char** argv, void* user_ctx) {
    log$info("Launching my-build command\n");
    e$ret(os$cmd("gcc", "-Wall", "-Wextra", "hello.c", "-o", "hello"));
    return EOK;
}
```

#### Step 2: `hello.c`

```c
// file: hello.c

#define CEX_IMPLEMENTATION
#include "cex.h"

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    io.printf("Hello from CEX\n");
    return 0;
}
```

#### Step 3: Build / Run

```sh

~ ➜ ./cex my-build
[INFO]    ( cex.c:50 cmd_mybuild() ) Launching my-build command
[DEBUG]   ( cex.c:51 cmd_mybuild() ) CMD: gcc -Wall -Wextra hello.c -o hello
~ ➜ ./hello
Hello from CEX
```

:::

> [!TIP] Getting cexy logic
>
> You can use cexy build source directly and adjust if needed, just use this command to extract source code from `./cex help --source cexy.cmd.simple_app`


### Dependency management

Dependencies are always pain-points, it's against CEX philosophy, but sometimes it's necessary evil.
CEX has capabilities for using `pkgconf` compatible-utilities, and `vcpkg` framework. You may check `examples/` folder in `cex` Git repo, it contains couple sample projects with dependencies. Windows OS dependencies is a hell, try to use MSYS2 or vcpkg.

Currently `pkgconf/vcpkg` dependencies are supported in simple mode, or figure out how to integrate `cexy$pkgconf()` macro into your custom build yourself.

Here is excerpt of `libcurl+libzip` build for Linux+MacOS+windows:

```c
// file: cex.c
#    define cexy$pkgconf_libs "libcurl", "libzip"
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */

#    if _WIN32
// using mingw libs .a
#        define cexy$build_ext_lib_stat ".a"
// NOTE: windows is a special case, the best way to manage dependencies to have vcpkg
//       you have to manually install vcpkg and configure paths. Currently it uses static
//       environment and mingw because it was tested under MSYS2
//
//  Also install the following in `classic` mode:
//  > vcpkg install --triplet=x64-mingw-static curl
//  > vcpkg install --triplet=x64-mingw-static libzip

#        define cexy$vcpkg_triplet "x64-mingw-static"
#        define cexy$vcpkg_root "c:/vcpkg/"
#    else
// NOTE: linux / macos will use system wide libs
//       make sure you installed libcurl-dev libzip-dev via package manager
//       names of packages will depend on linux distro and macos home brew.
#    endif
#endif


```

### Cross-platform builds
For compile time you may use platform specific constants, for example `#ifdef _WIN32` or you can set arbitrary config define that switching to platform logic (compile time). Also, cex has `os.platform.` sub-namespace for runtime platform checks:

::: {.panel-tabset}

#### Compile-time checks

```c
#    if _WIN32
// using mingw libs .a
#        define cexy$build_ext_lib_stat ".a"
#        define cexy$vcpkg_triplet "x64-mingw-static"
#elif defined(__APPLE__) || defined(__MACH__)
#        define cexy$vcpkg_triplet "arm64-osx"
#    else
#        define cexy$vcpkg_triplet "x64-linux"
#    endif
#endif

```

#### Explicit config

```c
// NOTE: activate with the following command
// ./cex -DCEX_WIN config

// file: cex.c
#ifdef CEX_WIN
#    define cexy$cc "x86_64-w64-mingw32-gcc"
#    define cexy$cc_args_sanitizer "-g3"
#    define cexy$debug_cmd "wine"
#    define cexy$build_ext_exe ".exe"
#endif

```

#### Run-time checks

```c
// platform-dependent compilation flags (runtime)
// file: cex.c (as a part of custom build command)

arr$(char*) args = arr$new(args, _);
arr$pushm(args, cexy$cc, "shell.c", "../sqlite3.o", "-o", "../sqlite3");
if (os.platform.current() == OSPlatform__win) {
    arr$pushm(args, "-lpthread", "-lm");
} else {

    arr$pushm(args, "-lpthread", "-ldl", "-lm");
}
arr$push(args, NULL);
e$ret(os$cmda(args));

```

:::


> [!TIP] Getting example for arbitrary function use in CEX
>
> You can get example source code with highlighting if any function is used in the project, use shell command: `./cex help --example os.platform.current`

## Developer Tools

CEX language is designed for improving developer experience with C, `./cex` CLI contains key tools for managing project, running apps, debugging, unit testing and fuzzing.

### Sanitizers
CEX enables sanitizers by default if they are supported by your OS and compiler. ASAN/UBSAN are extremely useful for catching bugs. Also, CEX uses sanitizers for call stack printouts for `uassert()`. `clang` has the best sanitizer support across many platforms, `gcc` sanitizers are supported on Linux.

Default sanitizer arguments:
```c
// file cex.c
#define cexy$cc_args_sanitizer    "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined", "-fsanitize=leak", "-fstack-protector-strong"

```

### Asserts
I'm a big fan of "asserts everywhere" code style, which is also known as design by contract, or TigerBeetle style, it has many names. Apparently, C asserts kinda work, but are huge pain for debugging without live debugger session.

So `cex.h` has 2 types of asserts:

- `uassert*()` family work like vanilla assertion and lead to abortion at failure (but they print tracebacks with call stack and line numbers). These asserts are stripped when `NDEBUG` is defined.
- `e$assert()` returns `Error.assert` and only intended for usage in function with `Exception` return type. These asserts remain in place even when `NDEBUG` is defined.

```c
// Raises abort
uassert(a == 4); // vanilla
uassert(b == a && "Oops it's a message"); // with static message
uassertf(b == 2, "b[%d] != 2", b); // with formatting

// Disabling uassert() - only for unit test mode
uassert_disable();
run_bad_stuff(NULL);
uassert_enable();

// Returns Error.assert on failure + prints [ASSERT] file:line in the stdout
Exception read_file(char* filename, char* buf, isize* out_buf_size) {
    e$assert(buff != NULL); // vanilla
    e$assert(filename != NULL && "invalid filename"); // with static message
    e$assertf(filename == NULL, "filename: %s", filename); // with formatting
    return EOK;
}

```

> [!NOTE]
>
> uassert() tracebacks only available if program was compiled with ASAN flags.


### Unit Testing Tool
Each CEX test file is compiled as a standalone executable, this allows making specialized tests with mocks, experiment with parts of bigger project without fixing plethora of compiler errors, and do test-driven development and debugging.

Create new test with: `./cex test create tests/test_file.c `, run it `./cex test run tests/test_file.c` or `./cex test run all`.

```sh
# Getting built-in help

➜ ./cex test

Usage:
cex test [options] {run,build,create,clean,debug} all|tests/test_file.c [--test-options]
CEX built-in simple test runner

Each cexy test is self-sufficient and unity build, which allows you to test
static functions, apply mocks to some selected modules and functions, have more
control over your code. See `cex config --help` for customization/config info.

CEX test runner keeps checking include modified time to track changes in the
source files. It expects that each #include "myfile.c" has "myfile.h" in
the same folder. Test runner uses cexy$cc_include for searching.

CEX is a test-centric language, it enables additional sanity checks when in
test suite, all warnings are enabled -Wall -Wextra. Sanitizers are enabled by
default.

Code requirements:
1. You should include all your source files directly using #include "path/src.c"
2. If needed provide linker options via cexy$ld_libs / cexy$ld_args
3. If needed provide compiler options via cexy$cc_args_test
4. All tests have to be in tests/ folder, and start with `test_` prefix
5. Only #include with "" checked for modification

Test suite setup/teardown:
// setup before every case
test$setup_case() {return EOK;}
// teardown after every case
test$setup_case() {return EOK;}
// setup before suite (only once)
test$setup_suite() {return EOK;}
// teardown after suite (only once)
test$setup_suite() {return EOK;}

Test case:

test$case(my_test_case_name) {
    // run `cex help tassert_` / `cex help tassert_eq` to get more info
    tassert(0 == 1);
    tassertf(0 == 1, "this is a failure msg: %d", 3);
    tassert_eq(buf, "foo");
    tassert_eq(1, true);
    tassert_eq(str.sstr("bar"), str$s("bar"));
    tassert_ne(1, 0);
    tassert_le(0, 1);
    tassert_lt(0, 1);
    return EOK;
}

If you need more control you can build your own test runner. Just use cex help
and get source code `./cex help --source cexy.cmd.simple_test`


    -h, --help    show this help message and exit

Test running examples:
cex test create tests/test_file.c        - creates new test file from template
cex test build all                       - build all tests
cex test run all                         - build and run all tests
cex test run tests/test_file.c           - run test by path
cex test debug tests/test_file.c         - run test via `cexy$debug_cmd` program
cex test clean all                       - delete all test executables in `cexy$build_dir`
cex test clean test/test_file.c          - delete specific test executable
cex test run tests/test_file.c [--help]  - run test with passing arguments to the test runner program

```

#### Test-Mode Sanity Checks and Side Effects

When compiled with `CEX_TEST` enabled (automatic for simple cexy builds), the following
additional safety mechanisms are activated:


| # | Mechanism | What it does |
|---|-----------|--------------|
| 1 | **Namespace mutability** | Namespace structs become non-const allowing function-pointer mocking (e.g. `os.timer = my_mock`) |
| 2 | **`e$except_silent` → loud** | Silent error handlers print tracebacks, aiding debugging of error paths |
| 3 | **`uassert` disable/enable** | `uassert_disable()` redirects failures to stdout + skips abort, letting the runner capture output |
| 4 | **`0xf7` memory poison** *(ASAN fallback)* | All `mem$`/arena allocations filled with `0xf7` to expose uninitialized reads. Duplicates ASAN's detection when the sanitizer is unavailable. |
| 5 | **Heap allocator stats** | `mem$` tracks `n_allocs`, `n_reallocs`, `n_free` for leak detection |
| 6 | **Post-case leak detection** | Runner compares allocs vs frees after each case, emits `[LEAK]` on mismatch |
| 7 | **Arena sanitize on scope exit** | Full heap integrity walk at every `mem$scope` exit |
| 8 | **Arena sanitize on destroy** | Same walk + `bytes_alloc == bytes_free` check before arena destruction |
| 9 | **Per-case arena isolation** | Each test case gets a fresh `test$alloc` arena, destroyed afterward |
| 10 | **Stdout capture** | stdout → temp file; replayed only on test failure with `>>>TEST OUTPUT<<<` markers |
| 11 | **Breakpoint on assert** | `--breakpoint` (`-b`) triggers debugger on `tassert_*` failure |
| 12 | **ASAN poison regions** *(recommended)* | Poison padding surrounds every heap & arena allocation — OOB access triggers a `use-after-poison` crash with precise stack trace |


**Breakpoint example:**
```sh
./cex test run tests/test_file.c -- --breakpoint
./cex test run tests/test_file.c -- -b
```

**Memory poisoning and ASAN** work together as a two-layer defense:

- **With ASAN** (the usual/recommended practice): `-fsanitize=address` is enabled by the test runner (`-Wall -Wextra -fsanitize=address`). `__asan_poison_memory_region` is called around every allocation's padding bytes in both heap and arena allocators. Any read/write past the allocation boundaries immediately crashes with a `use-after-poison` error and a precise stack trace. This is the primary tool for catching buffer overruns and use-after-free bugs.

- **Without ASAN** (but `CEX_TEST`): the same padding areas are filled with `0xf7` and verified on every free / arena scope exit. If the `0xf7` pattern is broken, a `uassert` fires — a best-effort approximation of ASAN's detection.

> [!TIP]
>
> If you hit a `use-after-poison` ASAN crash in `tmem$`, temporarily switch to `mem$` to get more precise diagnostics (arena pages are large, making ASAN's default report less specific).

#### Namespace Mutability and Mocks

In test mode namespace function pointers become writable, enabling simple mocking:

```c
f64 timer_mock(void) { return 777888.9; }

test$case(my_test_mocking_capabilities) {
    f64 orig_time = os.timer();

    os.timer = timer_mock;
    tassert_eq(777888.9, os.timer());

    os.timer = cex_os_timer;
    tassert_ne(777888.9, os.timer());
    tassert_le(os.timer(), orig_time + 1.0);

    return EOK;
}
```

> [!WARNING]
>
> **Restore or go out of scope**: If you don't restore the original pointer, other
> tests in the same run will use the mock. Use `test$setup_case()` / `test$teardown_case()`
> hooks to automate save/restore.

#### `test$mock_scope` — Scoped Namespace Mocking

`test$mock_scope` is a scope-guard macro that saves the full state of one or more
CEX namespaces before entering the body, then automatically restores them on
any exit path (`return`, `break`, `goto`, or normal fallthrough) via
`__attribute__((cleanup(...)))`. No manual save/restore needed.

Works with any CEX namespace — the save/restore is a byte-level `memcpy` of the
namespace struct, making it fully generic. Accepts 1–8 namespace arguments;
each is saved before the body executes and restored in reverse order on scope
exit.

```c
test$case(mock_single_ns) {
    test$mock_scope(os) {
        os.timer = my_timer_mock;      // mock timer
        tassert_eq(777888.9, os.timer());
    }
    // os.timer is auto-restored here
    tassert_ne(777888.9, os.timer());
    return EOK;
}

test$case(mock_two_ns) {
    test$mock_scope(os, io) {
        os.timer = my_timer_mock;
        io.printf = my_printf_mock;
        // ...
    }
    // both restored
    return EOK;
}
```

#### `test$alloc` — Per-Case Arena

`test$alloc` is a dedicated arena created fresh before each test case and destroyed
afterward:

```c
test$alloc = AllocatorArena.create(&(AllocatorArena_kw){
    .page_size = 1024 * 1024,
    .disable_scopes = true,
});
```

| Property | Detail |
|---|---|
| **Always growing** | Bump allocator — never frees mid-case |
| **`mem$scope` ignored** | `mem$scope(test$alloc, _)` is a no-op. All allocations survive until the case ends. |
| **Self-cleanup** | Destroyed after every case (pass or fail), then `test$alloc = NULL`. Next case gets a fresh arena. |
| **`0xf7` poisoning** | All `test$alloc` allocations filled with `0xf7` in test mode |

Usage:

```c
test$case(uses_test_alloc) {
    int* buf = mem$malloc(test$alloc, 256 * sizeof(int));
    // use buf freely — no manual free needed
    return EOK;
}
```

### Benchmarking

CEX benchmarking integrates directly with the test framework — reuse the same test file structure, setup hooks, and assertion macros. Test cases can be mixed with benchmarks in the same test file.

Define benchmarks with `test$bench(name)`:

```c
// Global state for benchmark data
static hash_table_s g_table;

test$setup_case()
{
    // Initialize benchmark data once before each bench case
    // Do NOT init data inside the bench body itself
    g_table = hash_table_create(1000);
    for (u32 i = 0; i < 1000; i++) {
        hash_table_insert(&g_table, i);
    }
    return EOK;
}

test$teardown_case()
{
    hash_table_destroy(&g_table);
    return EOK;
}

test$bench(my_bench)
{
    some_function_of_interest(&g_table);
    return EOK;
}
```

> [!CAUTION]
>
> **Keep data initialization in setup hooks, not in the bench body.** The bench function runs with `__attribute__((optimize("O0")))` to prevent dead-code elimination — any significant allocation or setup inside the bench loop would pollute timing measurements. Use `test$setup_case()`/`test$teardown_case()` (or `test$setup_suite()`/`test$teardown_suite()`) with global state instead.

Key points:

- Benchmarks compile with **`-O3`**, not the debug flags used by regular tests.
- All setup/teardown hooks work identically to regular test cases.

Run benchmarks:

```sh
./cex test bench tests/my_bench.c
```

Output shows **cold** (first iteration) and **hot** (subsequent/cached) timings:

```sh
my_bench................................ cold:  282.000ns  hot:   30.000ns  [PASS]
```

#### Quick code timing with `os$time_scope()`

For ad-hoc timing without writing a full benchmark, use `os$time_scope()` — it measures a block of code and prints the duration via `log$debug()` on scope exit:

```c
os$time_scope()
{
    expensive_function();
    another_expensive_call();
}
// output: [DEBUG] os$time_scope() took: 1.234s
```

Useful for:

- Quick performance sanity checks during development
- Comparing two approaches side-by-side in the same function
- Instrumenting hot paths without external profilers

The macro is backed by a monotonic high-resolution timer and uses GCC/Clang `cleanup` attribute, so timing is reported even on early `return`, `goto`, or `break` from the scope.

### Fuzzers

CEX has a fuzzer back-end, currently `libfuzzer` - built-in in `clang` is preferable, but `AFL++` also works. CEX fuzzers are designed to hit directly in heart of the code, therefore it's easier to use `clang`, however CEX fuzzer API remains compatible with AFL as well.

> [!NOTE]
>
> Try to split functionality across many small fuzz files for different aspects of your program. This will help to hit specific pain points easier.  Look into fuzz examples in CEX Git repo in `fuzz/` folder.

#### Making new fuzzer test
```sh
# Placing into fuzz/ directory is mandatory
./cex fuzz create fuzz/myapp/fuzz_bar.c
./cex fuzz create fuzz/mymodule/fuzz_foo.c
```

#### Sample fuzz file
```c
// file: fuzz/myapp/fuzz_bar.c

#define CEX_IMPLEMENTATION
#include "cex.h"

/*
// setup is not mandatory, but useful for establishing corpus
fuzz$setup(void){
    // This function allows programmatically seed new corpus for fuzzer
    io.printf("CORPUS: %s\n", fuzz$corpus_dir);
    mem$scope(tmem$, _){
        char* fn = str.fmt(_, "%s/my_case", fuzz$corpus_dir);
        (void)fn;
        // io.file.save(fn, "my seed data");
    }
}
*/

int
fuzz$case(const u8* data, usize size){
    // TODO: do your stuff based on input data and size
    if (size > 2 && data[0] == 'C' && data[1] == 'E' && data[2] == 'X') {
        __builtin_trap();
    }
    return 0;
}

fuzz$main();

```

#### Running fuzzer case
```sh
# Run specific test (infinite timeout)
./cex fuzz run fuzz/myapp/fuzz_bar.c

# Run all with time limit per test
./cex fuzz run all

>> (output of fuzzer)

SUMMARY: libFuzzer: deadly signal
MS: 4 PersAutoDict-ChangeBit-ShuffleBytes-CMP- DE: "E\000"-"X\000"-; base unit: a04ab19fbcf9e6dd3b7f1b71cb156335556f3507
0x43,0x45,0x58,0x0,0x3e,
CEX\000>
artifact_prefix='fuzz_file.'; Test unit written to fuzz_file.crash-88777
Base64: Q0VYAD4=

>> cat fuzz_file.crash-88777
CEX >
```

#### Running / debugging crash file
```sh
# Run single artifact file caused crash
# NOTE: fuzz_file.crash-88777 must be located at fuzz/myapp/ 
./cex fuzz run fuzz/myapp/fuzz_bar.c fuzz_file.crash-88777

# run in gdb (see cexy$debug_cmd )
./cex fuzz debug fuzz/myapp/fuzz_bar.c fuzz_file.crash-88777

```

### Lines Of Code stats
`./cex stats` calculates `.c/.h` lines of code and estimates assertion percentage as a code quality metric.

```sh

~ ➜ ./cex stats 'src/*.c' 'tests/*.c'
Project stats (parsed in 0.020sec)
--------------------------------------------------------
Metric                   |     Code     |    Tests     |
--------------------------------------------------------
Files                    |          27  |          30  |
Asserts                  |         361  |        4230  |
Lines of code            |       11494  |       12261  |
Lines of comments        |         725  |         606  |
Asserts per LOC          |        3.14% |       34.50% |
Total asserts per LOC    |       39.94% |         <<<  |
--------------------------------------------------------


```

### Fetching libraries and CEX updates

`./cex libfetch` command is a simple `git` wrapper for retrieving/updating `cexstd/` library files or updating `cex.h` itself. This command can be used with any git repo, for getting single-header files.

```sh

~ ➜ ./cex libfetch --help
Usage:
cex libfetch [options]
Fetching 3rd party libraries via git (by default it uses cex git repo as source)

    -h, --help            show this help message and exit
    -u, --git-url         Git URL of the repository (default: 'https://github.com/alexveden/cex.git')
    -l, --git-label       Git label (default: 'HEAD')
    -o, --out-dir         Output directory relative to project root (default: './')
    -U, --update          Force replacing existing code with repository files (default: N)
    -p, --preserve-dirs   Preserve directory structure as in repo (default: Y)

Command examples:
cex libfetch cexstd/testing/fff.h                      - fetch single header lib from CEX repo
cex libfetch -U cex.h                                  - update cex.h to most recent version
cex libfetch cexstd/random/                            - fetch whole directory recursively from CEX lib
cex libfetch --git-label=v2.0 file.h                   - fetch using specific label or commit
cex libfetch -u https://github.com/m/lib.git file.h    - fetch from arbitrary repo
cex help --example cexy.utils.git_lib_fetch            - you can call it from your cex.c (see example)

```

### Getting help for project

`./cex help` is CLI command for getting help for your project, it works for CEX and your project as well. You can use it as symbol search: types, functions, files, examples and source code.  Also, it supports CEX namespaces as struct interfaces and macro$namespaces as well.

#### Help command

```sh
~ ➜ ./cex help --help
Usage:
cex help [options] [query]
Symbol / documentation search tool for C projects


Options
    -h, --help        show this help message and exit
    -f, --filter      file pattern for searching (default: './*.[hc]')
    -s, --source      show full source on match (default: N)
    -e, --example     finds random example in source base (default: N)
    -o, --out         write output of command to file (default: '')

Query examples:
cex help                     - list all namespaces in project directory
cex help foo                 - find any symbol containing 'foo' (case sensitive)
cex help foo.                - find namespace prefix: foo$, Foo_func(), FOO_CONST, etc
cex help os$                 - find CEX namespace help (docs, macros, functions, types)
cex help 'foo_*_bar'         - find using pattern search for symbols (see 'cex help str.match')
cex help '*_(bar|foo)'       - find any symbol ending with '_bar' or '_foo'
cex help str.find            - display function documentation if exactly matched
cex help 'os$PATH_SEP'       - display macro constant value if exactly matched
cex help str_s               - display type info and documentation if exactly matched
cex help --source str.find   - display function source if exactly matched
cex help --example str.find  - display random function use in codebase if exactly matched

```

#### Getting language cheat-sheets

All core namespaces in CEX have cheat-sheets with full members reference and examples.

Just type:
```sh
# Cheat-sheet for os namespace
./cex help os$
# Help for CEX errors
./cex help e$
# Help for some parts of the language for$ / arr$ / hm$
./cex help arr$ 
# Make your own cheat-sheet!
./cex help myproj_namespace$
```

> [!TIP]
>
> [Core namespace reference](#lang-core-namespace-reference) was fully generated from CEX build-in cheat-sheets and docstrings!

> [!NOTE]
>
> For making your own cheat-sheet place doxygen comment (`/** multiline help */`) right before struct definition in the `mynamespace.h` (assuming `mynamespace`). Or add one before `#define __mynamespace$` if you use macro-only namespace.

#### Real project search
```c
// NOTE: you may get help for any available code in your project, not limited by cex.h
// >> ./cex help arr

macro_func           _arr$slice_get                 ./fuzz/CexParser/fuzz_cex_parser_corpus.out/cex_base.h:455
macro_func           arr$                           ./src/ds.h:140
macro_func           arr$at                         ./src/ds.h:201
macro_func           arr$cap                        ./src/ds.h:172
macro_func           arr$clear                      ./src/ds.h:169
macro_func           arr$del                        ./src/ds.h:175
macro_func           arr$delswap                    ./src/ds.h:184
macro_func           arr$free                       ./src/ds.h:163
macro_func           arr$grow                       ./src/ds.h:283
macro_func           arr$grow_check                 ./src/ds.h:276
macro_func           arr$ins                        ./src/ds.h:263
macro_func           arr$last                       ./src/ds.h:193
macro_func           arr$len                        ./src/ds.h:301
macro_func           arr$new                        ./src/ds.h:147
macro_func           arr$pop                        ./src/ds.h:209
macro_func           arr$push                       ./src/ds.h:217
macro_func           arr$pusha                      ./src/ds.h:237
macro_func           arr$pushm                      ./src/ds.h:227
macro_func           arr$setcap                     ./src/ds.h:166
macro_func           arr$slice                      ./fuzz/CexParser/fuzz_cex_parser_corpus.out/cex_base.h:491
macro_func           arr$sort                       ./src/ds.h:255
macro_func           curlcheck_arr                  ./examples/libs_vcpkg/build/vcpkg/packages/curl_x64-linux/include/curl/typecheck-gcc.h:472
macro_func           luaC_barrier                   ./examples/lua_module/build/lua/src/lgc.h:118
macro_func           luaC_barrierback               ./examples/lua_module/build/lua/src/lgc.h:122
macro_func           luaC_objbarrier                ./examples/lua_module/build/lua/src/lgc.h:126
macro_func           luaC_upvalbarrier              ./examples/lua_module/build/lua/src/lgc.h:130
macro_func           luaM_freearray                 ./examples/lua_module/build/lua/src/lmem.h:43
macro_func           tassert_eq_arr                 ./src/test.h:245
macro_const          __arr$                         ./src/ds.h:73
macro_const          unixShmBarrier                 ./examples/libs_vcpkg/build/sqlite-amalgamation-3490200/sqlite3.c:43856
func_def             apndShmBarrier                 ./examples/libs_vcpkg/build/sqlite-amalgamation-3490200/shell.c:9917
func_def             arrayindex                     ./examples/lua_module/build/lua/src/ltable.c:144
func_def             numusearray                    ./examples/lua_module/build/lua/src/ltable.c:259
func_def             recoverVfsShmBarrier           ./examples/libs_vcpkg/build/sqlite-amalgamation-3490200/shell.c:21195
func_def             setarrayvector                 ./examples/lua_module/build/lua/src/ltable.c:301
func_def             sqlite3MemoryBarrier           ./examples/libs_vcpkg/build/sqlite-amalgamation-3490200/sqlite3.c:30252
func_def             sqlite3OsShmBarrier            ./examples/libs_vcpkg/build/sqlite-amalgamation-3490200/sqlite3.c:26551
func_def             str_in_array                   ./examples/libs_vcpkg/build/sqlite-amalgamation-3490200/shell.c:23732
func_def             vfstraceShmBarrier             ./examples/libs_vcpkg/build/sqlite-amalgamation-3490200/shell.c:17030
func_decl            _cexds__arr_integrity          ./src/ds.h:99
func_decl            _cexds__arr_len                ./src/ds.h:100
func_decl            _cexds__arrfreef               ./src/ds.h:98
func_decl            _cexds__arrgrowf               ./src/ds.h:97
func_decl            luaC_barrier_                  ./examples/lua_module/build/lua/src/lgc.h:140
func_decl            luaC_barrierback_              ./examples/lua_module/build/lua/src/lgc.h:141
func_decl            luaC_upvalbarrier_             ./examples/lua_module/build/lua/src/lgc.h:142
func_decl            luaH_resizearray               ./examples/lua_module/build/lua/src/ltable.h:54
typedef              _cex_arr_slice                 ./fuzz/CexParser/fuzz_cex_parser_corpus.out/cex_base.h:448
typedef              _cexds__arr_new_kwargs_s       ./src/ds.h:142
typedef              _cexds__array_header           ./src/ds.h:130


```

#### Example roulette
If you need some use-case example for some function/symbol in your project you can test your luck and find random use-case of that function with the following:

```c
// NOTE: in example mode you must provide full symbol name without wildcards
~ ➜ ./cex help --example str.find

Found at ./cex.h:13704
Exception cexy__test__create(char* target, bool include_sample)
{
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Test file already exists: %s", target);
    }
    if (str.eq(target, "all") || str.find(target, "*")) {
        return e$raise(
            Error.argument,
            "You must pass exact file path, not pattern, got: %s",
            target
        );
    }

    ...
}

```

### Project management
Using `./cex` CLI you could seed new project or add/run/debug/clean apps/fuzz/tests in existing project.

> [!NOTE]
>
> Try `./cex app --help`, `./cex test --help`, `./cex fuzz --help` for more info. Also, this functionality may not work properly if you use custom build routines.

#### Creating
```sh
# Create new project from scratch + bootstraps ./cex cli + sample hello world project structure
./cex new new_dir_path

# Create new app for existing project as src/myapp/main.c
./cex app create myapp

# Create new unit test for existing project
./cex test create tests/test_file.c  

# Create new fuzz test
./cex fuzz create fuzz/myapp/fuzz_bar.c

```

#### Running
```sh
./cex app run myapp --app-opt=1 app_arg1 app_arg2

./cex test run tests/test_file.c  

./cex fuzz run fuzz/myapp/fuzz_bar.c

```

#### Debugging
```sh
./cex app debug myapp

./cex test debug tests/test_file.c  

./cex fuzz debug fuzz/myapp/fuzz_bar.c

```


<a id="lang-core-namespace-reference"></a>

## Core Namespace Reference

### Getting help

All core namespaces in CEX have cheat-sheets with full members reference and examples.

Just type:
```sh
# Cheat-sheet for os namespace
./cex help os$
# Help for CEX errors
./cex help e$
# Help for some parts of the language for$ / arr$ / hm$
./cex help arr$ 
# Make your own cheat-sheet!
./cex help myproj_namespace$
```

### `argparse`

{{< include _include/argparse.md >}}

### `arr$`
{{< include _include/arr.md >}}

### `cex$`
{{< include _include/cex.md >}}

### `cexy`

{{< include _include/cexy.md >}}

### `cg$`
{{< include _include/cg.md >}}

### `e$`
{{< include _include/e.md >}}

### `for$`
{{< include _include/for.md >}}

### `fuzz$`
{{< include _include/fuzz.md >}}

### `hm$`
{{< include _include/hm.md >}}

### `io`
{{< include _include/io.md >}}

### `log$`
{{< include _include/log.md >}}

### `mem$`
{{< include _include/mem.md >}}

### `os`
{{< include _include/os.md >}}

### `sbuf`
{{< include _include/sbuf.md >}}

### `str`
{{< include _include/str.md >}}

### `test$`
{{< include _include/test.md >}}



## CEX lib

### Role of CEX std lib
CEX std lib (see `cexstd/` folder in repo) is designed to be a collection of random tools and libraries that are not so frequently used. Currently, it's in early alpha stage, API stability is not guaranteed, backward compatibility is not guaranteed. Feel free to contribute your ideas, if you think it could be useful.

### Installing libraries
Installing and updating libs from the main CEX repo is pretty straightforward, and you can use:

```sh
cex libfetch cexstd/testing/fff.h                      - fetch single header lib from CEX repo
cex libfetch -U cex.h                                  - update cex.h to most recent version
cex libfetch cexstd/random/                            - fetch whole directory recursively from CEX lib
cex libfetch cexstd/                                   - fetch everything available in CEX std lib
cex libfetch --git-label=v2.0 file.h                   - fetch using specific label or commit
cex libfetch -u https://github.com/m/lib.git file.h    - fetch from arbitrary repo
cex help --example cexy.utils.git_lib_fetch            - you can call it from your cex.c
cex libfetch --help                                    - more help
```

## Credits

CEX contains some code and ideas from the following projects, all of them licensed under MIT license (or Public Domain):

1. [nob.h](https://github.com/tsoding/nob.h) - by Tsoding / Alexey Kutepov, MIT/Public domain, great idea of making self-contained build system, great YouTube channel btw
2. [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h) - MIT/Public domain, by Sean Barrett, CEX arr$/hm$ are refactored versions of STB data structures, great idea
3. [stb_sprintf.h](https://github.com/nothings/stb/blob/master/stb_sprintf.h) - MIT/Public domain, by Sean Barrett, I refactored it, fixed all UB warnings from UBSAN, added CEX specific formatting
4. [minirent.h](https://github.com/tsoding/minirent) - Alexey Kutepov, MIT license, WIN32 compatibility lib
5. [subprocess.h](https://github.com/sheredom/subprocess.h) - by Neil Henning, public domain, used in CEX as a daily driver for `os$cmd` and process communication
6. [utest.h](https://github.com/sheredom/utest.h) - by Neil Henning, public domain, CEX test$ runner borrowed some ideas of macro magic for making declarative test cases
7. [c3-lang](https://github.com/c3lang/c3c) - I borrowed some ideas about language features from C3, especially `mem$scope`, `mem$`/`tmem$` global allocators, scoped macros too.


## License

```

MIT License

Copyright (c) 2024-2026 Aleksandr Vedeneev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

```
