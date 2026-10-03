
CEX.C — Comprehensively EXtended C Language (https://cex-c.org)

A self-contained C language extension. Its only dependency is a C compiler
(`cc`, or gcc/clang); `cex.h` bundles the build system, unit test runner, small
standard library, and the `./cex` CLI. MIT License 2023-2026 (c) Alex Veden.

### Getting started

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

### Common commands

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


### Basics

#### Code Style Guidelines

| Convention | Example | Meaning |
|---|---|---|
| `dollar$means_macros` | `e$ret`, `arr$push`, `test$case` | `$` marks a macro; the `first$` part links to its namespace |
| `functions_are_snake_case()` | `os.fs.mkpath()` | functions and methods are lower case |
| `*_c` suffix | `sbuf_c`, `KeyMap_c`, `AllocatorArena_c` | struct with a code namespace attached (container/object) |
| `*_s` suffix | `str_s`, `cex_iterator_s` | plain data container, no attached logic |
| `MyObj.method()` | `KeyMap.create()` | object-specific namespace mirrors the type name |
| `namespace.func()` | `str.find()` | namespace names are lower case |
| `Enums__double_underscore` | `MyEnum_e`, `MyEnum__foo` | enum type `_e`; elements use `Type__value` |
| `CONSTANTS_ARE_UPPER` | `NAMESPACE_MY_CONST` / `namespace$CONST_NAME` | two constant notations |

#### Types

CEX provides short aliases for primitive types and a few extra types covering gaps in C.

| Type | Description |
|---|---|
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
| sbuf_c | dynamic string builder; always null-terminated and compatible with native `char*` |
| str_s | string slice (buf + len) |
| Exc / Exception | error type in CEX |
| Error.<some> | generic error collection |
| IAllocator | memory allocator interface type |
| arr$(T) | generic type dynamic array |
| hm$(K, V) | generic type hashmap |

#### Utility macros

| Name | Description |
|---|---|
| uassert() | General purpose assert with tracebacks |
| uassert_always() | Assert with tracebacks that is not stripped by `NDEBUG` |
| unlikely() | Branch predictor management for unexpected conditions |
| likely() | Branch predictor management for expected conditions |
| breakpoint() | Cross-platform debugger breakpoint |
| fallthrough() | Explicit fallthrough to the next switch case |
| tassert_* | Unit-test assertions see `./cex help test$` |

### CEX Core Namespaces

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

### Namespaces

CEX namespaces group functions under one global name, reducing C name collisions and keeping
project structure navigable. They also add OOP-ish behavior to structs.

#### Key features

* generated from the `.c` file — no `.h` upkeep on signature changes
* `.c`/`.h` filename must match the namespace name
* only the namespace name is exposed globally (smaller collision surface)
* sub-namespaces give a decision tree in LSP completion
* `.` separator reads better and highlights namespace vs function

#### In a nutshell

A namespace is a global `const` struct of function pointers:

```c
#define CEX_NAMESPACE __attribute__((visibility("hidden"))) extern const

struct __cex_namespace__KeyMap {
    Exception (*create)(KeyMap_c* self, char* input_dev_or_name);
    void      (*destroy)(KeyMap_c* self);
    Exception (*handle_events)(KeyMap_c* self);
};

CEX_NAMESPACE struct __cex_namespace__KeyMap KeyMap;
```

Usage:

```c
KeyMap_c keymap = { 0 };
e$goto(KeyMap.create(&keymap, file), end);
e$goto(KeyMap.handle_events(&keymap), end);
```

`_c` on the struct hints it has a code namespace (a class/object); `_s` means plain data.

#### Sub-namespaces

One extra level groups large namespaces, e.g. `str.slice.*` for `str_s` operations,
`str.convert.*` for conversions, and `str.find()` at the root. They let you type the
function name as a decision tree (`str` → `slice` → function).

#### How to make a namespace

1. create `src/foo.c` and `src/foo.h`
2. write `foo_fun1()`, `foo_fun2()`, `foo__bar__fun3()` — they become `foo.fun1()`,
   `foo.fun2()`, `foo.bar.fun3()`
3. run `./cex process src/foo.c`

Caveats:

* `.c` and `.h` must share the folder and the `foo` prefix
* namespaced functions start with `foo_`; sub-namespace uses `foo__subname__`
* only one level of sub-namespace is allowed
* signatures need not be declared in the header — `.c` statics are enough
* `static inline` functions are excluded
* `foo__some` is treated as internal and excluded
* use the exact `src/foo.c` argument to create a namespace; `all` only updates existing ones
* the parser also accepts `cex_foo_fun1()` and strips the `cex_` prefix when building names

CLI:

```sh
./cex process src/foo.c   # create or update one namespace
./cex process all         # update all after signature changes
```

Note: function-pointer calls may be slower without optimization; `-O1` turns them into direct
calls. For shared libraries, prefer plain C functions.

### Agentic workflow

* `./cex help --list` — list all namespaces in the project (CEX + your own)
* `./cex help --brief str$ [os$ e$ ...]` — compact namespace outline, one line per member
* `./cex help --idioms str$ [os$ e$ ...]` — print only the namespace idioms/docs block
* `./cex help --brief --idioms str$ [os$ e$ ...]` — namespace idioms + compact API
* `./cex help --source 'str$s'` — print source of a function or macro
* `./cex help --example str.find` — up to 3 real usages (file:line + code)
* `./cex help --agents` — dump the key CEX namespace idioms (AGENTS.md content)
* `./cex help --agents --out <file>` — write that content to an agent instruction file
* Use CEX types instead of C primitives — `u64` not `unsigned long`, `u8` not
  `unsigned char`, `i32` not `int`, `f64` not `double`, `usize` not `size_t`
* Never edit `cex.h` directly — it is a generated single-header artifact; treat it
  as read-only and regenerate it from source instead
* When a task involves any of the following, fetch the docs first with
  `./cex help --idioms --brief <ns>$`:
  * build process / `./cex.c` → `./cex help --idioms --brief cexy$`
  * fuzzing → `./cex help --idioms --brief fuzz$`
  * files/paths, processes, env, time, random → `./cex help --idioms --brief os$`
  * file IO → `./cex help --idioms --brief io$`
  * dynamic string building → `./cex help --idioms --brief sbuf$`
  * CLI args → `./cex help --idioms --brief argparse$`
  * logging → `./cex help --idioms --brief log$`
  * code generation → `./cex help --idioms --brief cg$`

Prefer `./cex help` over grepping — it returns signatures, usage idioms, and
real examples pulled from the codebase.

If the project has no agent instruction file (`AGENTS.md`, `CLAUDE.md`,
`.cursorrules`, or similar), propose creating one and offer to generate it with
`./cex help --agents --out <file>`.


```c
/// Arena allocator block magic marker
#define CEX_ALLOCATOR_ARENA_MAGIC

/// Heap allocator block magic marker
#define CEX_ALLOCATOR_HEAP_MAGIC

/// Maximum nesting depth for mem$scope() calls
#define CEX_ALLOCATOR_MAX_SCOPE_STACK

/// Temp allocator block magic marker
#define CEX_ALLOCATOR_TEMP_MAGIC

/// Default page size (256 KB) for the temp allocator arena
#define CEX_ALLOCATOR_TEMP_PAGE_SIZE

/// Max alignment supported by the arena allocator
#define CEX_ARENA_MAX_ALIGN

/// Max single arena allocation / page size. Bounded by mem$MAX (signed-negative sizes
/// are rejected, size arithmetic cannot overflow) and by the record's 40-bit size field,
/// whichever is smaller.
#define CEX_ARENA_MAX_ALLOC

#define CEX_DISABLE_POISON

/// Max element size (bytes) copied by for$each(), default 64
#define CEX_FOREACH_MAX_COPY_SIZE

/// Max fuzz input size (stack buffer), default 1024000
#define CEX_FUZZ_MAX_BUF

#define CEX_HEADER_H

/// Compile-time log level (0 mute .. 5 trace), default 4
#define CEX_LOG_LVL

/// Marks a variable as a CEX namespace struct (visibility("hidden") on non-Win32)
#define CEX_NAMESPACE

#define CEX_NAMESPACE_DEF

/// uassert() panic detail: 0 silent trap, 1 file:line, 2 expression (default 1)
#define CEX_PANIC_VERBOSITY

#define CEX_PLATFORM_WIN32_H

#define CEX_SPRINTF_MIN

/// Max captured test assertion message length, default 512
#define CEX_TEST_AMSG_MAX_LEN

/// Max recorded traceback frames (buffered levels)
#define CEX_TRACEBACK_CAP

/// Traceback capture: 0 off, 1-2 buffered ring, 3 immediate logging (default 2)
#define CEX_TRACEBACK_VERBOSITY

/// Concatenate textually a##b
#define cex$concat(a, b)

/// Concatenate textually c##a##b
#define cex$concat3(c, a, b)

/// Enables CEX data structures: dynamic arrays, hashmaps, arr$* macros, for$each
#define cex$enable_ds

/// Enables CEX IO operations
#define cex$enable_io

/// Enables CEX memory allocators: tmem$, mem$
#define cex$enable_mem

/// Disables all key CEX capabilities, except core types, and macros, other functionality must be
/// re-enabled via `#define cex$enable_*`
#define cex$enable_minimal

/// Enables CEX os namespace + argparse
#define cex$enable_os

/// Enables CEX string operations
#define cex$enable_str

/// Set to 1 if current platform is freestanding (no OS, or libc)
#define cex$is_freestanding

/// Macro for redefining default platform calloc()
#define cex$platform_calloc

/// Macro for redefining default platform free()
#define cex$platform_free

/// Macro for redefining default platform malloc()
#define cex$platform_malloc

/// Macro for redefining memory-failure panic; define it as an empty function-like macro
///  (e.g. `#define cex$platform_mem_panic(...)`) to restore NULL returns
#define cex$platform_mem_panic(...)

/// Macro for redefining panic function (used in assertions, and other CEX stuff)
#define cex$platform_panic

/// Macro for redefining default platform realloc()
#define cex$platform_realloc

/// Produces a literal string of any text inside the (...)
#define cex$stringize(...)

/// cex$tmpname - internal macro for generating temporary variable names (unique__line_num)
#define cex$tmpname(base)

/// makes a new variable with __cex__ prefix
#define cex$varname(a, b)

/// CEX build date (substituted at bundle time)
#define cex$version_date

/// CEX major version
#define cex$version_major

/// CEX minor version
#define cex$version_minor

/// CEX patch version
#define cex$version_patch

/// Code generator state: sbuf backing buffer, current indent level, and error state
typedef struct cex_codegen_s

/// Parsed declaration: name, docs, body, refined type/args, source location, and attributes
typedef struct cex_decl_s

/// Fuzz data fetcher (makes C-type payloads from random fuzz$case data)
typedef struct cex_fuzz_s

/// Generic iterator state (≤ 64 bytes). Used by `for$iter()` and custom iterator functions.
typedef struct cex_iterator_s

/// Token produced by the parser: a type tag + a slice into source content
typedef struct cex_token_s




```
