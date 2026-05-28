# CEX.C — AGENTS.md

## Build system (no Make/CMake)

```sh
cc -g ./cex.c -o ./cex          # one-time bootstrap
./cex -DCEX_DEBUG config        # select a preset
./cex test run all              # full test suite
```

Presets: `CEX_DEBUG`, `CEX_RELEASE`, `CEX_NDEBUG`, `CEX_VALGRIND`, `CEX_C23`,
`CEX_WASM_DEBUG`, `CEX_WASM_RELEASE`, `CEX_WINE`, `CEX_FUZZ_AFL`, `CEX_X32`.
Set via `./cex -D<PRESET> config`.

## Commands

| Command | Usage |
|---|---|
| test | `./cex test run all` / `run tests/file.c` / `debug file.c` / `bench file.c` / `clean all` |
| fuzz | `./cex fuzz run all` |
| app | `./cex app run myapp` / `build` / `debug` |
| process | `./cex process lib/mylib.c` — generate CEX namespaces for code |
| new | `./cex new my_proj` — scaffold a new project |
| config | `./cex config` / `./cex -D<KEY> config` |
| libfetch | `./cex libfetch cexstd/` — git-based dependency fetch |
| help | `./cex help` / `help str.` / `help --source u8` |
| stats | `./cex stats -v` |
| build-docs | `./cex build-docs` — builds docs via quarto (requires quarto CLI) |

`./cex test` subcommands: `run` (build+run), `build`, `create`, `clean`, `debug` (runs via `cexy$debug_cmd` — gdb on linux), `bench`.

## Architecture

- **cex.h** — single-header "standard library" (~20k lines). **Everything lives here** (build system, test runner, core lib, namespaces). DO NOT edit it directly, it will be re-build by first `./cex` CLI call.
- **cex.c** — project-specific entrypoint that bundles `src/*.[ch]` into `cex.h` via `cex_bundle()`.
- **src/** — 21 `.h` + 21 `.c` files bundled into `cex.h` in deterministic order (defined in `cex.c:cex_bundle`). Uses `#pragma once`/`#include` stripping during bundling.
- **tests/** — each test file `#include "src/all.c"` (accesses CEX internals). Each test file ends with `test$main();`.
- **cexstd/** — optional std lib (json, random, fsm, testing), fetched via `./cex libfetch cexstd/`.
- C11 (with GNU C extensions) library (C23 compatible), CEX is specifically tailored for GCC/Clang compilers

## Namespace convention
- CEX introduces namespace concept, which requires special treatment
- Each namespace takes place in `src/<namespace>.c` / `src/<namespace>.h`
- Each namespace macro will use namespace prefix `<namespace>$<macro_name>` and it should be defined in `src/<namespace>.h`
- Namespace is defined in `.h` file as `struct __cex_namespace__<namespace>`
- Each namespace may have corresponding test case `tests/test_<namespace>.c` or `tests/test_<namespace>_<subnamespace>.c`
- `./cex` CLI has auto-generating capabilities, you should run `./cex process src/<namespace>.c`
- Namespace function implementation should have the following prefix `cex_<namespace>_<func>()`
- Sub-namespace function use the following naming convention `cex_<namespace>__<subnamespace>__<func>()`
- There is a difference at call site for namespaces' functions for example `os` namespace has `cex_os__cmd__run` function definition, but outside code will call it as `os.cmd.run()`. Keep this in mind when you need to figure out logic of a called function.

## Testing conventions

- Test files must be in `tests/` and start with `test_` prefix (e.g. `tests/test_foo.c`).
- Tests are **unity builds**: `#include` sources directly instead of linking. Each test file
  `#include "src/all.c"` to access internal CEX state (or `#include "cex.h"` for public API).
- Each test file ends with `test$main();` — this macro generates the `main()` entry point.
- Test cases: `test$case(name) { ... return EOK; }`.
- Hooks: `test$setup_case()`, `test$teardown_case()`, `test$setup_suite()`, `test$teardown_suite()`.
- Assertions: `tassert()`, `tassert_eq(a, b)` (type-generic), `tassert_ne`, `tassert_eq_almost`,
  `tassert_eq_arr`, `tassert_eq_ptr`, `tassert_eq_mem`, `tassert_er`, `tassertf`.
- `uassert()` = hard assertion (aborts), `tassert()` = soft (fails test, continues).
- Test output is captured by default (stdout suppressed), but all failed tests will show their captured stdout and error messages.
- Test cases for running may be filtered when you work on specific feature; use `./cex test run <file> --filter='my_full_case_name'`  runs only `test$case(my_full_case_name)`, you can use wildcards `--filter='my_full_case*'` or list of cases `--filter='(my_full_case_name|another_case|case3)'`.

## Benchmarking conventions
- Benchmarks: `test$bench(name) { ... return EOK; }` — runs only with `./cex test bench <file>`.
- Each case `test$bench(name)` - has `__attribute__((optimize("O0")))` no need to do tricks to keep this code from eliminating by compiler, but consider only calling other functions from there, loops are also fine
- DO NOT do any data initialization logic in `test$bench(name)`, use `test$setup_case()`, `test$teardown_case()`, `test$setup_suite()`, `test$teardown_suite()` and global state variables for data bootstrapping for benchmarks.
- Use this file as example: `tests/hash/test_bench_hash.c`
- DO NOT generate benchmark cases with X-macros / use strait code generation for every function

## Code style & tooling

- `.clang-format`: Mozilla-based, 100 col, 4-space indent, `InsertBraces: true`.
- `.clang-tidy`: warnings-as-errors, skips `DeprecatedOrUnsafeBufferHandling` and `DeadStores`.
- `compile_flags.txt` exists for clangd LSP.
- CI clang-tidy: `clang-tidy $(find ./src -name "*.c" -o -name "*.h")`.
- CEX namespace convention: `$` in macro identifiers (e.g. `test$case`, `mem$scope`, `e$ret`, `for$each`).

## Key conventions

- `$` in identifiers indicates it's a macro, typically `<namespace>$<name>`, `$<name>` allowed for private use and must be `#undef $<name>` in the same file or function (depends on scope)
- `mem$scope(tmem$, _) { }` — temp allocator scope, auto-frees on scope exit.
- `e$ret(func())` — return-with-traceback on error. `e$except(err, func()) { }` — catch error scope.
- `Exception` return type forces caller to check (uses `warn_unused_result`).
- DO NOT edit `./cex.h` directly, work on `src/*` and then run `./cex test <test file>` command, this will ensure `./cex.h` reassembly
- All CEX `printf`-like functions and macros use custom printing format, which is mostly posix complient, with some differences (**platform independent**): `i64` type is handled by `%ld` format, `u64` is `%lu` format, `str_s` variables formatted by `%S`.

## Development workflow
- Typically you should work on `src/*.[ch]` files
- You MUST always run test for a specific namespace
- When changing signatures of the namespace' functions or adding new, run `./cex process src/<namespace>.c`, do not alter contents of `struct __cex_namespace__*` it's fully auto-generated
