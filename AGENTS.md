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

## Documentation conventions
- `./cex help` a special command for getting help, it automatically parses full project files to search usage
- `///` brief docs comments are placed before each function in the namespace
- Large `/** */` doc comment is placed in namespace header file right before `struct __cex_namespace__<namespace>` definition
- Sometimes there is no `struct __cex_namespace__<namespace>` definition so large `/** */` doc comment is placed before `#define __<namespace>$`
- All files in `docs/_include/*.md` are auto generated, do not edit them directly, edit `/** */` in headers before namespace definition
- If you need to use markdown tables in doc-strings, make sure white space alignment with max 120 width, because they will be displayed in terminal 

## Key conventions, principles & unconventional features

These conventions are the most critical to understand before writing or modifying any CEX code. They depart significantly from standard C idioms.

### 1. `$` in identifiers = macro marker

A `$` in a symbol means it's a macro, not a function. This is the single most important visual cue in CEX code.

| Pattern | Meaning |
|---|---|
| `<namespace>$<name>` | Macro belonging to a namespace (e.g. `e$ret`, `arr$push`, `test$case`) |
| `$<name>` | Private/local macro — must be `#undef`'d in the same file or function scope |
| `namespace$CONST` | Namespaced constant |

Related naming conventions from CEX code style:

| Convention | Example | Meaning |
|---|---|---|
| `PascalCase_s` | `str_s`, `MyStruct_s` | Simple data container, no special logic |
| `PascalCase_c` | `sbuf_c`, `AllocatorArena_c` | Container/object with methods |
| `Enums__double_underscore` | `LogLvl__debug`, `LogLvl__error` | Enum type + elements |
| `snake_case_func()` | `os.fs.mkpath()` | All functions and methods |

### 2. Error handling model (`Exception`, `e$*`)

CEX replaces C's scattered error conventions (`-1`, `NULL`, `errno`, custom enums) with a single unified `Exception` type.

#### Core mechanism

- `Exception = char*` with `__attribute__((warn_unused_result))` — the compiler **forces** you to check it.
- `EOK` (alias for `NULL`) means success. Any non-NULL pointer is an error.
- Error identity uses **pointer comparison**, not `strcmp`: `if (err == Error.argument)` compares addresses, not string contents.
- Standard errors live in a global `Error` struct (const pointers): `Error.ok`, `Error.memory`, `Error.io`, `Error.argument`, `Error.integrity`, `Error.not_found`, `Error.runtime`, `Error.assert`, etc.
- Custom errors: define a `const struct` with `Exc` fields, each initialized to a unique string literal address.

#### Macro toolbox

| Macro | Purpose |
|---|---|
| `e$ret(func())` | Call `func()`, on error log traceback + return the same error |
| `e$except(err, func()) { }` | Call `func()`, bind result to `err`, log traceback, run handler block |
| `e$raise(Error.xxx, "fmt", ...)` | Return error with file:line logging and formatted message |
| `e$assert(cond)` | Check condition, return `Error.assert` with logging if false (persists in release) |
| `e$goto(func(), label)` | On error, `goto label` (for resource cleanup patterns) |
| `e$except_errno(syscall()) { }` | Wrap `-1`+`errno` system calls |
| `e$except_null(ptr()) { }` | Wrap `NULL`-on-error functions |
| `e$except_true(func()) { }` | Wrap non-zero-on-error functions |

#### Tracebacks

Each `e$` macro logs `file:line func()` at the error site. When errors propagate through `e$ret`, every call frame is logged, building a full stack trace without external tools.

#### Silent vs loud

- **Loud** (default with `e$except`/`e$ret`): logs error location for debugging.
- **Silent** (`e$except_silent`): suppresses logging for performance-critical paths or tight loops. Avoid `e$raise()` in callees when using silent handling.

### 3. Memory management model (`IAllocator`, `mem$scope`)

CEX adopts an allocator-centric paradigm inspired by Zig/C3: all allocation is explicit via an `IAllocator` interface.

#### Allocator interface

`IAllocator = const struct Allocator_i*` — a vtable of `malloc`/`realloc`/`calloc`/`free` + `scope_enter`/`scope_exit` for arena support.

#### Two global allocators

| Allocator | Type | Lifetime |
|---|---|---|
| `mem$` | Heap (backed by `malloc`/`free`) | Manual — call `mem$free()` |
| `tmem$` | Temp arena (~256KB pages) | Auto-freed at `mem$scope` exit |

#### Scoped allocation (`mem$scope`)

```c
mem$scope(tmem$, _)    // _ is the convention for temp allocator variable
{
    arr$(char*) paths = arr$new(paths, _);  // allocated on arena
    // ... use paths ...
}  // all arena memory freed here, up to 32 levels of nesting
```

- Arena allocator works like a stack: allocation moves a pointer, scope exit rewinds.
- Arena pages are reused across scopes, keeping CPU cache hot.
- `mem$arena(page_size) { }` creates a standalone arena scope.

#### Critical pitfalls

- **Never return a pointer from a `mem$scope`** — memory is freed at scope exit (use-after-free).
- **Never `realloc` across scope levels** — reallocating a pointer from one scope inside a nested scope triggers an assertion in test mode.
- **`break`/`continue` inside `mem$scope` breaks the scope** (backed by a `for` loop), not an outer loop.
- **Arena allocator never reuses freed chunks** — heavy reallocation on arenas may cause page bloat. Pre-allocate capacity when possible.

#### Unit test behavior

- New allocations filled with `0xf7` poison pattern.
- `mem$` tracks allocation count vs free count for leak detection.
- ASAN integration: poisoned regions around arena allocations trigger use-after-poison on OOB access.
- Substitute `tmem$` → `mem$` temporarily to get more precise ASAN diagnostics.

### 4. Namespace system (`namespace.function()`)

Namespaces give CEX function-call syntax with LSP-driven discovery and no runtime overhead.

- Each namespace is a `struct __cex_namespace__<name>` containing function pointers.
- Code calls `<ns>.<func>()`, e.g. `os.cmd.run()`, `str.slice.starts_with()`.
- Implementation uses the real C-linkage name: `cex_<ns>__<sub>__<func>()` → `<ns>.<sub>.<func>()`.
- First-level: `cex_<ns>_<func>()` → `<ns>.<func>()`.
- Sub-namespaces form a **decision tree** in LSP suggestions: typing `str.` shows `.slice`, `.convert`, etc., each branching to their own method list. This reduces the mental burden of remembering flat prefix-based APIs.
- The struct and function-pointer wiring is **auto-generated** by `./cex process src/<namespace>.c`. Never hand-edit `struct __cex_namespace__*` contents.

### 5. Core types and unified operations

CEX provides a set of foundational types that don't exist in standard C, with cross-type unified operations.

#### Dynamic arrays (`arr$(T)`)

- Backed by a plain `T*` pointer with metadata (length, capacity) stored at a negative offset before the pointer.
- Compatible with any C function expecting `T* + length`: just pass `arr, arr$len(arr)`.
- Access: natural `arr[i]` indexing, or `arr$at(arr, i)` for bounds-checked access.

| Macro | Operation |
|---|---|
| `arr$new(arr, alloc, .capacity=N)` | Initialize |
| `arr$push(arr, item)` | Append one |
| `arr$pushm(arr, a, b, c)` | Append many |
| `arr$pusha(arr, other, [len])` | Append another array |
| `arr$pop(arr)` | Remove and return last |
| `arr$del(arr, i)` / `arr$delswap(arr, i)` | Remove at index |
| `arr$ins(arr, i, val)` | Insert at index |
| `arr$free(arr)` | Deallocate |
| `arr$len(arr)` | Get length (see below) |
| `arr$sort(arr, cmp)` | Qsort in-place |

#### Hashmaps (`hm$(K, V)`)

- Backed by dynamic arrays of `{key, value}` structs, plus a hash table overlay.
- `arr$len()` and `for$each` work on hashmaps directly (they are iterable/indexable).
- String keys stored by reference by default; use `.copy_keys=true` + `.copy_keys_arena_pgsize=N` for owning mode.

#### Unified `arr$len()`

`arr$len(x)` works on all of these — NULL input returns 0:

- CEX dynamic arrays (`arr$(T)`) and hashmaps (`hm$(K,V)`)
- Static C arrays (`int a[10]`) and char buffers (`char buf[64]`)
- String literals (`"hello"` → length 6, includes null terminator)

#### Unified `for$each` / `for$eachp`

`for$each(val, arr)` and `for$eachp(ptr, arr)` iterate over any of:

- CEX dynamic arrays, hashmaps, static C arrays, pointer+length pairs

#### Three string tiers

| Type | Description | Key namespace |
|---|---|---|
| `char*` | Null-terminated, allocator-explicit, NULL-tolerant | `str` |
| `str_s` | Slice `{buf, len}` (16 bytes, pass by value) | `str.slice` |
| `sbuf_c` | Dynamic string builder (alias for `char*`) | `sbuf` |

- `str.fmt(allocator, ...)` — one-shot formatted string on any allocator.
- All `str` functions return NULL on error, allowing chaining with a final NULL check.
- `str.match(s, "cmd_*_(insert|delete)")` — built-in glob-like pattern matching.

### 6. Format specifier differences

All CEX `printf`-like functions (`io.printf`, `log$error`, `str.fmt`, `sbuf.appendf`) use a custom formatting engine, not libc `sprintf`. Key differences from POSIX:

| Specifier | CEX behavior |
|---|---|
| `%ld` / `%lu` | Always 64-bit (`i64`/`u64`), **platform independent** |
| `%d` / `%u` | Always 32-bit (`i32`/`u32`), **platform independent** |
| `%S` | Prints `str_s` slice (`.buf`, `.len`) — crashes with a diagnostic if a plain `char*` is passed |
| `%s` | Standard null-terminated `char*` (unchanged) |

### 7. Build system and development workflow

- CEX is its own build system. The `./cex` binary is compiled from `cex.c` and serves as both build orchestrator and CLI.
- **Always edit `src/*.[ch]`**, never `cex.h`. Running `./cex test <file>` or `./cex process <file>` automatically triggers `cex_bundle()` which reassembles `cex.h` from the source files.
- `cexy$` namespace holds build config: `cexy$cc`, `cexy$build_dir`, `cexy$debug_cmd`, etc.
- Running `./cex process src/<namespace>.c` is **required** when adding new functions or changing signatures — it regenerates the namespace struct and function-pointer wiring.
- After modifying a namespace, **always run its tests**: `./cex test run tests/test_<namespace>.c` (optionally with `--filter` for specific cases).
