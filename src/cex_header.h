/*
# CEX.C - Comprehensively EXtended C Language (cex-c.org)
                                                                MOCCA - Make Old C Cexy Again!

>    MIT License 2023-2025 (c) Alex Veden (see license information at the end of this file)
>    https://github.com/alexveden/cex/

CEX is self-contained C language extension, the only dependency is one of gcc/clang compilers.
cex.h contains build system, unit test runner, small standard lib and help system.

Visit https://cex-c.org for more information

## GETTING STARTED
(existing project, when cex.c exists in the project root directory)
```
1. > cd project_dir
2. > gcc/clang ./cex.c -o ./cex     (need only once, then cex will rebuil itself)
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

/*
 *                  GLOBAL CEX VARS / DEFINES
 *
 * NOTE: run `cex config --help` for more information about configuration
 */

/// disables all asserts and safety checks (fast release mode)
// #define NDEBUG

/// custom fprintf() function for asserts/logs/etc
// #define __cex__fprintf(stream, prefix, filename, line, func, format, ...)

/// customize abort() behavior
// #define __cex__abort()

// customize uassert() behavior
// #define __cex__assert()

// you may override this level to manage log$* verbosity
// #define CEX_LOG_LVL 5

/// disable ASAN memory poisoning and mem$asan_poison*
// #define CEX_DISABLE_POISON 1

/// size of stack based buffer for small strings
// #define CEX_SPRINTF_MIN 512

/// disables float printing for io.printf/et al functions (code size reduction)
// #define CEX_SPRINTF_NOFLOAT

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
#define cex$version_minor 19
#define cex$version_patch 1
#define cex$version_date "{date}"
