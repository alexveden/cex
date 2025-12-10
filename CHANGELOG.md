# CEX Release Notes

## 0.18.0
2025-11-25

### Changes / improvements
- feat: Added static code analyzer support in CI (clang-tidy) for better code quality
- feat: cexy build system generates `compile_flags.txt` unless it explicitly disabled `#define cexy$create_compile_flags 0`


### Fixes
- Fixed some possible null pointer dereferences
- Fixed `clangd` "Too many errors emitted" diagnostic at the beginning of the code file
- Fixed `clang-tidy` warnings

## 0.17.0
2025-10-15


### Changes / improvements
- Added WASM support via emscripten compiler 
* Added - `#define cex$enable_minimal` allowing to use only bare minimum on CEX functionality, and selectively re-enable some of the parts. This opens opportunities for CEX embedded portability: [freestanding example](https://github.com/alexveden/cex/tree/master/examples/freestanding).
- Removed: (breaking) `io.fileno` - removed, it's non-standard and platform specific function
- Added `cex$is_freestanding` macro

### Fixes
- Fixed memory alignment for HeapAllocator bug when system malloc() returns non 16-byte aligned ptr

## 0.16.0
2025-09-09

> [!NOTE]
>
> This release contains many breaking changes, however I've probably polished core namespaces enough and expect them to be more stable in the future.

### Changes / improvements
- refactor: (breaking) renamed test$NOOPT to test$noopt
- refactor: (breaking) sbuf - removed redundant code grow() and update_len(), shrink() refactored
- refactor: (breaking) - removed sbuf.create_temp() - just use sbuf.create(128, tmem$)
- refactor: (breaking) refactored sbuf - made exc checking optional + added sbuf.validate() + sizes now are usize (size_t)
- example: added CEX pre-compilation example
- refactor: (breaking) io.fread/io.fwrite - changed API for convenience
- refactor: (breaking) C23 compatible unreachable() + C23 tests passed
- refactor: (breaking) moved json namespace to cexstd/json/ - currently it's never used in CEX core
- refactor: _Static_asserts are renamed to static_asserts for C23 compatibility
- refactor: (breaking) renamed `var` into `auto` for C23 compatibility
- refactor: (breaking) retired arr$slice functionality
- refactor: (breaking) made all macros for code generation with cg$ prefix
- doc: added ./cex build-docs command + full documentation in a single HTML
- feat: ./cex help -o file.txt foo$ - export full output to file
- feat: ./cex help foo$ - added __foo$ document placeholder if there is no `foo` namespace struct
- ci: windows 2019 retired

### Fixes
- fix: cexy process fixed multiline docstrings
- fix: dup() function case on windows in test$ running module
- fix: sbuf added more NULL resilience checks
- fix: CexParser - broken CEX_NAMESPACE handling after refactoring + incorrect skipping of private entities with `_`
- fix: ./cex help --filter - always using default
- fix: ./cex stats still use cex.h for statistics outside cex.h project
- fix: ./cex stats - cex.h still included in LOC stats for side projects

## 0.15.0
2025-08-03
### Changes / improvements
- (breaking) e$except_errno() - triggered on any negative value returned (previous behavior only -1)

## 0.14.0
2025-06-05
### Changes / improvements
- Added fuzzer support (LibFuzzer + AFL++)
- Added `./cex fuzz run ...` command
- uassert() now emits __builtin_trap() instead of abort() for better call stack
- Added fuzz tests for core elements of CEX
- `./cex test run test/file.c` - test always rebuilt when called as single file

### Fixes
- CexParser - various fuzzer driven fixes
- str.match - various fuzzer driven fixes
- json - various fuzzer driven fixes


## 0.13.0
2025-05-16

### Changes / improvements
- (breaking) `cexy.src_changed()` - refactored with array length argument
- (breaking) `os.cmd.create()` - refactored arguments supporting pointer/length instead of arr$(char*)
- Examples: new `Building Lua + Lua Module in CEX` example
- Examples: new `Building SQLite Program From Source` example
- `e$except_true` added handler when OK state is 0, and other is error
- (breaking) `os.fm.remove_tree()` - attempt for removing non existing path will lead to `Error.not_found`
- `cexy app create` - refactored structure of new app (added argparse)
- `cexy.app.find_app_target_src` - refactored arguments
- Examples: added building/linking with system libs
- `cexy` - Added support of vcpkg libs
- Examples: added building/linking with vcpkg libs
- refactor!: removed all const qualifiers from the cex project
- feat: added __builtin_unreachable() after assert/panic

### Fixes
- `os.match` - fixed `[A-Z+]*` pattern invalid match at the beginning
- `./cex help` - now includes `./cex.h` if its a symlink
- fix(cexy): fix cexy.src_include_changed() was not searching relative to src file
- fix(cexy): pkgconf fixed include dirs flags before linker
- fix: mingw warning visibility attribute not supported in this configuration
- fix(cexy): compile flags failed if pkgconf failed (now just prints an error)
- fix(cexy): help skipping build/ and tests/ dirs entirely
- fix(cexy): help incorrectly handled forward-declared types
- fix(cexy): ./cex process all - ignores build/ and tests/ now


## 0.12.0
2025-05-12

### Changes / improvements
- Code base cleanup
- refactor(cexy)!: removed `cexy$cc_args_release/debug` - now it's `cexy$cc_args` (controlled by cex -DSOME_CONF config)
- Added Valgrind to CI (extra memory leak check, uninitialized variables check, file handle leaks checks) + available on non x86 architectures
- `os.path.abs()` - getting absolute path from any other path
- `cex help` - added code syntax colors in terminal
- `json` - added new JSON parser/builder into cex.h core
- `str$convert()` - typesafe generic macro for converting char*/str_s to any basic numeric type
- `cex stats` - added new command for calculating project lines of code + quality stats
- `cexy$pkgconf_libs` - new universal way of using dependencies in CEXY build system (tests and apps). Uses system (or custom) `pkgconf` command for resolving library settings and compiler args.
- `cexy$pkgconf_libs` - required libs now checked in `cex config`, user will receive diagnostic error if anything is missing
- `./cex` - added `compile_flags.txt` generation for clangd LSP/tooling (if that file exists)
- `./cex help <cex_namspace>` - added macro constants + alphabetical sorting
- `os.timer()` - implemented high-performance timer (cross-platform)

### Fixes
- Fixed memleaks after program destruction - hanging tmem$ last page (Valgrind issue)
- Fixed memleaks for test runner - list of tests were not cleaned up (Valgrind issue)
- `AllocatorArena` - assertion in scope exit with some nested scopes pattern
- `cex test create` - new tests now include `#define CEX_TEST` + compiler arg removed


## 0.11.0 Change list
2025-05-03

### Changes / improvements
- `os.cmd.exists()` - added function for checking if command exists in PATH
- `os.fs.copy_tree(src_dir, dst_dir)` - added recursive copy of files between folders
- `cexy.utils.git_lib_fetch()` - fetching/updating arbitrary Git lib (e.g. single header lib) from git
- `cexy.utils.git_hash()` - getting current git hash of the current repo
- `cex` - added `cex libfetch` command 
- Added Alpine Linux support (multiarch + libc musl) + CI
- Added tests for multiple architectures (including big endian): x86_64 (native), x86 (native), aarch64, armhf, armv7, loongarch64, ppc64le, riscv64, and s390x
- `cexy.utils.pkgconf() / cexy$pkgconf` - system dependency resolving utility function
- Added automatic timestamp generation in cex version when bundling
- Removed redundant `cexy$` vars, renamed `cexy$cc_args_test`

### Fixes
- `str.match()` - fixed `str.match(s, "*(abc|def)")` pattern handling
- `str.match()` - fixed `str.slice.match(s, "[a-Z]")` when using on slice view
- `cex.h` - bare project fixed windows specific lock/initialization issue when building new proj

## 0.10.0 Change list
2025-04-27

### Changes / improvements
- Added 32-bit support + CI tests
- Added MacOS support + CI tests
- Added Windows support + CI tests
- `os.get_last_error()` - unified Win32/POSIX string-line error (CEX Exception format)
- CEX `sprintf` family added more resilient error handling for `%s` / `%S`
- `io.file.size()` - reimplemented, more cross-platform compatibility
- `hm$` - implemented `char*` key copy mode + arena mode
- Implemented `cexy` build system (self rebuild, test runner, code process, help)
- `os.platform...` - new namespace added for platform specific actions (OS Type, Arch Type)
- `argparse` - refactored core, added support for commands
- Memory management refactored - AllocatorArena / AllocatorHeap added, `mem$scope()` added
- `os.fs...` - OS file system capabilities full refactoring
- `os.cmd...` - OS commands/subprocess running capabilities (subprocess launch, getting output of process, in-screen interactive command launching)
- `CexParser` - implemented C code parser (code gen, help retrieval, build system helper)
- `cg$` - code generation tools added 
- `str.slice...` - CEX string refactoring (added str_s slice, view only strings with length)
- `str.match()` - simple but powerful pattern matching for strings (used in os/file wildcards also).
- `os.fs.find()` - recursive file search with pattern matching
- `io` - fully refactored io namespace (more compatible with C now + added helpers)
- CEX test engine - fully refactored test suite/generation/runner
- Refactored dynamic arrays / hashmaps - type safe + generic, based on STB DS + Allocators


### Fixes
- `str.copy()` - reimplemented in safer (simpler BSD style) `strlcpy()`
- `AllocatorHeap` - fixed aligned realloc

