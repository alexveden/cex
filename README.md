# Cex.C - Comprehensively EXtended C Language
**MOCCA - Make Old C Cexy Again!**

![Test suite](https://github.com/alexveden/cex/actions/workflows/main.yml/badge.svg)
![Fuzz Test](https://github.com/alexveden/cex/actions/workflows/fuzzing.yml/badge.svg)
![Multiarch support](https://github.com/alexveden/cex/actions/workflows/multiarch.yml/badge.svg)
![Examples](https://github.com/alexveden/cex/actions/workflows/examples.yml/badge.svg)


Cex.C (officially pronounced /ˈtsɛk.si/ "tsek-see") was born as an alternative answer to a plethora of brand new LLVM based languages which strive to replace old C. Cex.C still remains C language itself, with small, but important tweaks that bring a completely different development experience.

LEGAL NOTICE: Any intentional mispronunciation of Cex.C or cexy$ (build system), officially pronounced /ˈtsɛk.si/ ("tsek-see"), into an incorrect form may be considered intentional tseksual harassment of the project — which identifies itself with the code gender (it/its) — and may be subject to legal action under the MIT License. /LOL/ 

## GETTING STARTED

### Website and documentation

[cex-c.org](https://cex-c.org/)

[Documentation](https://cex-c.org/docs.html)

### Existing project (when cex.c exists in the project root directory)
```
1. > cd project_dir
2. > gcc/clang ./cex.c -o ./cex     (need only once, then cex will rebuild itself)
3. > ./cex --help                   get info about available commands
```

### New project / bare cex.h file

1. download [cex.h](https://raw.githubusercontent.com/alexveden/cex/refs/heads/master/cex.h)
2. Make a project directory 
```
mkdir project_dir
cd project_dir
```
3. Make a seed program (NOTE: passing header file is ok)
```
gcc -D CEX_NEW -x c ./cex.h -o cex
clang -D CEX_NEW -x c ./cex.h -o cex
```
4. Run cex program for project initialization
```
./cex
```
5. Now your project is ready to go 
```
./cex test run all
./cex app run myapp
```

### cex tool usage:
```
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

## About Cex.C
### What is Cex.C
CEX is designed as a standalone, single-header programming language with no dependencies other than the GCC/Clang compiler and libc.

Written as a single-header C11 (GNU C) library (C23 compatible), CEX is specifically tailored for GCC/Clang compilers. Its mission is to improve C without reinventing a new compiler stack. CEX incorporates modern programming trends while remaining fully compatible with all C tooling.

Though CEX remains unsafe, modern compilers and tools (such as address sanitizers combined with unit tests) help mitigate risks. The language draws inspiration from modern languages, blending their best ideas into a C-compatible form.

The philosophy of CEX revolves around independence and self-containment, minimizing external dependencies. Whenever possible, dependencies are included as source code. CEX embraces the 80/20 principle, prioritizing convenience and developer experience for the majority of use cases (80%). For niche scenarios — whether too large, too small, or requiring extreme performance — a custom solution may be more appropriate.

### Key Features
- Cross-platform, multi-architecture support, big/little endian
- No dependencies, single header C programming language less than 20k lines
- Integrated build system - CEX builds itself, no external build system needed!
- New memory management model based on Allocators (temporary memory scopes with auto-free, arenas, etc.)
- New namespacing capabilities for grouping functions / simulating OOP classes
- New error handling model (support for stack traceback on errors, assertions with stack trace (with ASAN))
- Developer experience - unit test runner / code generation / help system included in `cex.h`
- Code distribution system based on Git and managing dependencies (system libs) with `pkgconf`/`vcpkg`
- Simple, but powerful core lib included in `cex.h`:
    * Generic / type-safe dynamic arrays and hashmaps included
    * Strings refactored: safe-string functions (copy/formatting), dynamic string buffer (`sbuf`), string views/slices (`str_s`), simple pattern matching engine (wildcard patterns).
    * `os` namespace - for running commands, filesystem manipulation, environment variables, path manipulation, platform info
    * `io` namespace - cross platform IO support, including helper functions, e.g. `io.file.load/save()`
    * `argparse` - convenient argument parsing for CLI tools with built-in commands support
    * `cexy` - fancy project management tool and build system.
- **cexstd** lib additionally available via single command `./cex libfetch cexstd/`:
    * `cexstd/json/json.h` `json` namespace - for working with JSON, also offers automatic serialization/deserialization code generation for your data structures.

## Code example
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


## Supported compilers / platforms
### Tested compilers / Libc support
- GCC - 10, 11, 12, 13, 14, 15, 16
- Clang - 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
- Fil-C - [Fil-C](https://fil-c.org/) memory safe C/C++ compiler (Clang based, experimental support)
- MSVC - unsupported, probably never will be
- LibC tested - glibc (linux), musl (linux), ucrt/mingw (windows), macos
- Emscripten - for wasm
- Freestanding/embedded - CEX supports optional feature disabling for constrained environments, even without LIBC (see [example](https://github.com/alexveden/cex/tree/master/examples/freestanding))

### Tested platforms / architectures
- Linux - x32 / x64 (glibc, gcc + clang), 
- Alpine linux - (libc musl, gcc) on architectures x86_64, x86, aarch64, armhf, armv7, loongarch64, ppc64le, riscv64, and s390x (big-endian) 
- Windows (via MSYS2 build) - x64 (mingw64 + clang), libc mscrt/ucrt
- Macos - x64 / arm64 (clang)
- WASM - emscripten compiler (due to platform limitations, `os.cmd.` related features are not available)

### Test suite
CEX is tested on various platforms, compiler versions, sanitizers, and optimization flags, ensuring future compatibility and stability. Sanitizers and Valgrind verify the absence of memory leaks, buffer overflows, and undefined behavior. Additionally, tests with release flags confirm that compiler optimizations do not interfere with the code logic.

The CEX codebase uses the `clang-tidy` static analyzer in its continuous integration (CI) pipeline. The build is configured to fail if any warnings are generated.


| OS / Build type   | Valgrind |     UBSAN      |  ASAN | Release -O3 | Release -NDEBUG -O2 |
|:----------|:---------:|:-------------:|:------:| :-----: | :-----: |
| Linux Ubuntu 2204 x64 |   ✅ |✅ | ✅ |✅ |✅ |
| Linux Ubuntu 2404 x64 | ✅|  ✅ | ✅ |✅ |✅ |
| Linux Ubuntu 2404 x32 |✅ |  ✅ | ✅ |✅ |✅ |
| Linux Alpine x86_64 | ✅| ✅  |✅  |✅ |✅ |
| Linux Alpine x86 | ✅  | |  |✅ |✅ |
| Linux Alpine aarch64 | ✅|   |  |✅ |✅ |
| Linux Alpine armhf | |   |  |✅ |✅ |
| Linux Alpine armv7 |  |   |  |✅ |✅ |
| Linux Alpine loongarch64 |  |  |  |✅ |✅ |
| Linux Alpine ppc64le |   |  |  |✅ |✅ |
| Linux Alpine riscv64 |  |  |  |✅ |✅ |
| Linux Alpine s390x |  |  |  |✅ |✅ |
| Windows 2019 (Win10) x64 | |  ✅ | ✅ |✅ |✅ |
| Windows 2022 (Win10) x64 | |  ✅ | ✅ |✅ |✅ |
| Windows 2025 (Win11) x64 | |  ✅ | ✅ |✅ |✅ |
| MacOS 13 x64 |  |  ✅ | ✅ |✅ |✅ |
| MacOS 14 arm64 |  |  ✅ | ✅ |✅ |✅ |
| MacOS 15 arm64 | |  ✅ | ✅ |✅ |✅ |
| WASM | |  ✅ | ✅ |✅ |✅ |


## Examples
* [Building Lua + Lua Module in CEX](https://github.com/alexveden/cex/tree/master/examples/lua_module)
* [Building SQLite Program From Source](https://github.com/alexveden/cex/tree/master/examples/sqlite)
* [Building with system libraries](https://github.com/alexveden/cex/tree/master/examples/libs_sys)

* [Building with vcpkg local repo](https://github.com/alexveden/cex/tree/master/examples/libs_vcpkg)
* [Freestanding / embedded concept](https://github.com/alexveden/cex/tree/master/examples/freestanding)
* [JSON config file / serde code generation](https://github.com/alexveden/cex/tree/master/examples/json_config)

## Credits

CEX contains some code and ideas from the following projects, all of them licensed under MIT license (or Public Domain):

1. [nob.h](https://github.com/tsoding/nob.h) - by Tsoding / Alexey Kutepov, MIT/Public domain, great idea of making self-contained build system, great YouTube channel
2. [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h) - MIT/Public domain, by Sean Barrett, CEX arr$/hm$ are refactored versions of STB data structures, great idea 
3. [stb_sprintf.h](https://github.com/nothings/stb/blob/master/stb_sprintf.h) - MIT/Public domain, by Sean Barrett, I refactored it, fixed all UB warnings from UBSAN, added CEX specific formatting
4. [minirent.h](https://github.com/tsoding/minirent) - Alexey Kutepov, MIT license, WIN32 compatibility lib 
5. [subprocess.h](https://github.com/sheredom/subprocess.h) - by Neil Henning, public domain, used in CEX as a daily driver for `os$cmd` and process communication
6. [utest.h](https://github.com/sheredom/utest.h) - by Neil Henning, public domain, CEX test$ runner borrowed some ideas of macro magic for making declarative test cases
7. [c3-lang](https://github.com/c3lang/c3c) - I borrowed some ideas about language features from C3, especially `mem$scope`, `mem$`/`tmem$` global allocators, scoped macros too.
8. [argparse](https://github.com/cofyc/argparse) - initial CEX `argparse` implementation was a port from this project, both projects have similar approach for argument parsing.


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
