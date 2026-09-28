/**
CEX.C — Comprehensively EXtended C Language (https://cex-c.org)

A self-contained C language extension. Its only dependency is a C compiler
(`cc`, or gcc/clang); `cex.h` bundles the build system, unit test runner, small
standard library, and the `./cex` CLI. MIT License 2023-2026 (c) Alex Veden.

## Getting started

New project (bare `cex.h`):

```sh
cc -D CEX_NEW -x c ./cex.h -o ./cex   # prime ./cex + scaffold project
./cex                                 # create boilerplate project
./cex test run all                    # run sample unit tests
./cex app run myapp                   # run sample app
```

Existing project (already has `cex.c` in the root):

```sh
cc ./cex.c -o ./cex                   # bootstrap once; cex then rebuilds itself
./cex --help                          # list available commands
```

## Common commands

| Command | Purpose |
|---|---|
| `./cex test run all` | build + run the whole test suite |
| `./cex test run tests/test_foo.c` | run one test file (`--filter='case*'` to pick cases) |
| `./cex test debug tests/test_foo.c` | run one test under `cexy$debug_cmd` (gdb) |
| `./cex test bench tests/test_foo.c` | run `test$bench()` cases |
| `./cex test clean all` | remove built test binaries (run before a full suite) |
| `./cex test create tests/test_foo.c` | scaffold a test file from template |
| `./cex app run myapp` | build + run an app |
| `./cex app debug myapp` | run an app under the debugger |
| `./cex fuzz --max-time=60 run fuzz/some/fuzz_file.c` | bounded fuzz run |
| `./cex process src/foo.c` | (re)generate the `foo` namespace after signature changes |
| `./cex new myproj` | scaffold a new project |
| `./cex config` | show project/toolchain config (`./cex -DKEY config` sets flags) |
| `./cex libfetch cexstd/` | fetch CEX std-lib dependencies |


## CEX namespaces

CEX namespaces, each responds to `./cex help <ns>$`:

| Namespace | Purpose |
|---|---|
| `cex$` | language overview, global config defines and macros |
| `e$` | Exception-based error handling (`e$ret`, `e$except`, `e$raise`) |
| `mem$` | allocators (`mem$`, `tmem$`), `mem$scope`, arena helpers |
| `arr$` | dynamic arrays |
| `hm$` | hashmaps |
| `for$` | unified iteration (`for$each`, `for$eachp`, `for$iter`) |
| `str` | C strings and `str_s` slices |
| `sbuf` | dynamic string builder |
| `io` | printf/scan and file IO |
| `os` | OS, paths, processes, env, time, random |
| `log$` | logging macros (`log$error`, `log$warn`, ...) |
| `argparse` | command-line argument parsing |
| `test$` | unit test runner (`test$case`, `test$bench`, assertions) |
| `fuzz$` | libFuzzer / AFL harness |
| `cg$` | code generation helpers |
| `cexy` | build system config (`cexy$*`) and CLI API |

Help commands:

* `./cex help <symbol>` — find any symbol containing `<symbol>`
* `./cex help str.find` — docs for an exact match
* `./cex help <ns>$` — namespace cheat-sheet (docs, macros, types, examples)

## Agentic workflow

* `./cex help --list` — list all namespaces in the project (CEX + your own)
* `./cex help --brief str$ [os$ e$ ...]` — compact namespace outline, one line per member
* `./cex help --idioms str$ [os$ e$ ...]` — print only the namespace idioms/docs block
* `./cex help --brief --idioms str$ [os$ e$ ...]` — namespace idioms + compact API
* `./cex help --source 'str$s'` — print source of a function or macro
* `./cex help --example str.find` — up to 3 real usages (file:line + code)
* `./cex help --agents` — dump the key CEX namespace idioms (AGENTS.md content)
* `./cex help --agents --out <file>` — write that content to an agent instruction file

Prefer `./cex help` over grepping — it returns signatures, usage idioms, and
real examples pulled from the codebase.

If the project has no agent instruction file (`AGENTS.md`, `CLAUDE.md`,
`.cursorrules`, or similar), propose creating one and offer to generate it with
`./cex help --agents --out <file>`.
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

/// CEX major version
#define cex$version_major 0
/// CEX minor version
#define cex$version_minor 22
/// CEX patch version
#define cex$version_patch 0
/// CEX build date (substituted at bundle time)
#define cex$version_date "{date}"
