# Coverage

`./cex coverage run|report|export|clean` — test coverage build-system command with two engines:

- `llvm`: clang only, source-based coverage via `llvm-profdata` + `llvm-cov` (`.profraw`)
- `lcov`: compiler `--coverage` + `lcov`/`genhtml` (`.gcno`/`.gcda`), gcc or clang

Engine defaults to `auto` (llvm for clang, lcov for gcc); force it with `--engine=llvm|lcov`.

## Wiring into `cex.c`

Fetch the module, then add three pieces to your `cex.c`:

```sh
./cex libfetch cexstd/
```

```c
#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"
#include "cexstd/testing/coverage/coverage.c"   /* 1. include the module */

Exception cmd_custom_coverage(int argc, char** argv, void* user_ctx); /* 2. declare */

int
main(int argc, char** argv)
{
    cexy$initialize();
    argparse_c args = {
        .usage = cexy$usage,
        argparse$cmd_list(
            cexy$cmd_all,
            cexy$cmd_test,
            { .name = "coverage",                                            /* 3. register */
              .func = cmd_custom_coverage,
              .help = "Test coverage run/report/export/clean" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    e$except (err, argparse.run_command(&args, NULL)) { return 1; }
    return 0;
}

Exception
cmd_custom_coverage(int argc, char** argv, void* user_ctx)
{
    return coverage.cmd(argc, argv, user_ctx);
}
```

## Usage

```sh
# whole suite
./cex coverage run all                       # build+run all tests instrumented
./cex coverage report all                    # aggregate + print text report
./cex coverage report --format=json all      # machine-readable JSON report
./cex coverage report --format=html all      # HTML report -> cexy$build_dir/coverage
./cex coverage report --file 'src/foo*.c' all # only sources matching the glob
./cex coverage export -o coverage.info all   # lcov .info tracefile for external tools
./cex coverage clean all                     # remove raw coverage artifacts

# one test file (target is any tests/test_*.c)
./cex coverage run tests/test_foo.c          # instrument+run just this file
./cex coverage report tests/test_foo.c       # report for that target
./cex coverage export -o foo.info tests/test_foo.c
```

Raw data lives in `cexy$build_dir`. `run all` wipes all raw data; `run tests/test_foo.c` resets
only that target's counters, so one test can be re-run without losing the rest. llvm scopes the
report to the target's binaries, lcov aggregates every `.gcda` in `cexy$build_dir`. `export`
writes an lcov tracefile (merge several with `lcov -a a.info -o merged.info`). The lower-level
`./cex test --coverage run all` only leaves raw data (no report). Not supported with `bench`.

JSON fields: `total{lines_hit,lines_found,funcs_hit,funcs_found}`,
`files[]{path,lines_hit,lines_found,funcs_hit,funcs_found,missed_lines,uncovered_funcs,fully_uncovered}`.

## Tool dependencies

- llvm: `llvm-profdata`, `llvm-cov` in `PATH`
- lcov: `lcov`, `genhtml` in `PATH` (`llvm-cov gcov` is used as the gcov tool under clang)
