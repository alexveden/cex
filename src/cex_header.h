/**
# CEX.C - Comprehensively EXtended C Language (cex-c.org)
                                                                MOCCA - Make Old C Cexy Again!

>    MIT License 2023-2026 (c) Alex Veden (see license information at the end of this file)
>    https://github.com/alexveden/cex/

CEX is self-contained C language extension, the only dependency is one of gcc/clang compilers.
cex.h contains build system, unit test runner, small standard lib and help system.

Visit https://cex-c.org for more information

## GETTING STARTED
(existing project, when cex.c exists in the project root directory)
```
1. > cd project_dir
2. > gcc/clang ./cex.c -o ./cex     (need only once, then cex will rebuild itself)
3. > ./cex --help                   get info about available commands
```

## GETTING STARTED
(bare cex.h file, and nothing else)
```
1. > download https://cex-c.org/cex.h or copy existing one
2. > mkdir project_dir
3. > cd project_dir
4. > gcc/clang -D CEX_NEW -x c ./cex.h    prime cex.c and build system
5. > ./cex                                creates boilerplate project
6. > ./cex test run all                   runs sample unit tests
7. > ./cex app run myapp                  runs sample app
```

## cex tool usage:
```
> ./cex --help
Usage:
cex  [-D] [-D<ARG1>] [-D<ARG2>] command [options] [args]

CEX language (cexy$) build and project management system

help                Search cex.h and project symbols and extract help
process             Create CEX namespaces from project source code
new                 Create new CEX project
config              Check project and system environment and config
test                Test running
app                 App runner

You may try to get help for commands as well, try `cex process --help`
Use `cex -DFOO -DBAR config` to set project config flags
Use `cex -D config` to reset all project config flags to defaults
```
*/

/**

- **GLOBAL CEX VARS / DEFINES / internals**

**NOTE**: run `cex config --help` for more information about configuration

```c

/// disables all asserts and safety checks (fast release mode)
#define NDEBUG

/// custom fprintf() function for asserts/logs/etc
#define __cex__fprintf(stream, prefix, filename, line, func, format, ...)

/// customize abort() behavior
#define __cex__abort()

/// customize uassert() behavior
#define __cex__assert()

/// Log verbosity level: 0=mute, 1=error, 2=warn, 3=info, 4=debug (default), 5=trace
#define CEX_LOG_LVL 4

/// disable ASAN memory poisoning and mem$asan_poison*
#define CEX_DISABLE_POISON 1

/// size of stack based buffer for small strings
#define CEX_SPRINTF_MIN 512

/// disables float printing for io.printf/et al functions (code size reduction)
#define CEX_SPRINTF_NOFLOAT

/// max AFL fuzzer input buffer size (default: 1024000)
#define CEX_FUZZ_MAX_BUF 1024000

/// enables AFL fuzzing mode (instead of libFuzzer)
#define CEX_FUZZ_AFL

/// max element byte-size for for$each value iteration (default: 64)
#define CEX_FOREACH_MAX_COPY_SIZE 64

/// temp allocator arena page size in bytes (default: 256 KB)
#define CEX_ALLOCATOR_TEMP_PAGE_SIZE 1024 * 256

/// max nesting depth for mem$scope() (default: 16)
#define CEX_ALLOCATOR_MAX_SCOPE_STACK 16

/// build-system mode flag (set automatically by cex.c)
#define CEX_BUILD

/// unit-test mode flag (enables extra checks and poison patterns)
#define CEX_TEST

/// new-project scaffolding mode (generates boilerplate)
#define CEX_NEW

/// single-header implementation mode (expands cex.h contents)
#define CEX_IMPLEMENTATION

/// skip CEX's own Win32 type/function declarations; let system <windows.h> provide them instead
/// use when your project includes <windows.h> (directly or via third-party libs like curl)
/// to avoid conflicts with CEX's predeclared Win32 types (e.g. _LARGE_INTEGER, _FILETIME)
#define CEX_NO_WIN32_TYPES

```

*/

#define __cex$


#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__APPLE__) || defined(__MACH__)
// NOTE: Apple SDK defines sprintf as a macro, this messes str.sprintf() calls, because
//      sprintf() part is expanded as macro.
#    ifdef sprintf
#        undef sprintf
#    endif
#    ifdef vsprintf
#        undef vsprintf
#    endif
#endif

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__) || defined(__GNUG__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define cex$version_major 0
#define cex$version_minor 21
#define cex$version_patch 0
#define cex$version_date "{date}"
