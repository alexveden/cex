# Traceback backend experiments

Exploratory programs that compare stack-trace backends for a future
`cex$platform_panic()` traceback. They answer: which backend works on which
platform, what it costs in build/link flags, and whether it is safe in a crash
signal handler.

Run locally:

```sh
bash run.sh              # full matrix for current platform
CC=clang bash run.sh     # override compiler
```

Outputs land in `build/traceback_out/` (`_summary.txt` + one log per case).
Crash cases are expected; the runner records exit codes but always exits 0.

CI: `.github/workflows/traceback.yml` runs the matrix on Linux (gcc, clang),
Windows MSYS2 (mingw64, clang64) and macOS (intel, arm64), and uploads the logs
as artifacts. Trigger manually with `workflow_dispatch`.

## Programs

| File | Backend |
|---|---|
| `tb_execinfo.c` | `backtrace()` + `backtrace_symbols_fd()` / `backtrace_symbols()` |
| `tb_unwind.c` | `_Unwind_Backtrace` + `dladdr` |
| `tb_unwind_raw.c` | `_Unwind_Backtrace` raw IPs for offline `addr2line`/`atos` |
| `tb_asan.c` | `__sanitizer_print_stack_trace()` |
| `tb_retaddr.c` | `__builtin_return_address(n)` walk |
| `tb_signals.c` | SIGSEGV / SIGABRT / SIGFPE / SIGILL handler traceback |
| `tb_win32.c` | `CaptureStackBackTrace` (+ optional dbghelp) |

## Results (Linux x86-64, gcc 14)

### ASAN — best output

```
#0 __sanitizer_print_stack_trace
#1 tb_level3 tb_asan.c:31
#2 tb_level2 tb_asan.c:41
#3 tb_level1 tb_asan.c:47
#4 main      tb_asan.c:53
```

- Resolves static functions, file:line, and libc BuildIds. No extra flags.
- **Conflict:** installing a custom SIGSEGV handler replaces ASAN's handler and
  suppresses its report. Signal handlers must be skipped when ASAN is enabled.

### execinfo — `-rdynamic` only helps exported symbols

- Without `-rdynamic`: `libc.so(__libc_start_main)` plus `binary(+0xoffset)`.
- With `-rdynamic`: `main` and `_start` resolve, **static functions stay offsets**.
- `backtrace_symbols_fd` is `write()`-based; `backtrace_symbols` mallocs.
- POSIX only.

### `_Unwind_Backtrace` — portable, raw IPs

- Accurate frame count (`-O0`: 7, `-O2`: 5; inlining collapses frames).
- `dladdr` resolves exported symbols only (`main`, `__libc_start_main`, `_start`).
- No extra lib on modern glibc; available on Linux, macOS, MinGW.

### Raw IPs + offline `addr2line` — full names, no runtime cost

```
tb_level3 tb_unwind_raw.c:32
tb_level2 tb_unwind_raw.c:40
tb_level1 tb_unwind_raw.c:46
main      tb_unwind_raw.c:52
```

- Needs `-g`, and `-no-pie` on Linux so runtime address == file address.
- macOS uses `atos`; Windows uses `llvm-symbolizer` or dbghelp.

### `__builtin_return_address` — unusable at `-O2`

- `-O0`: four sane frames (asking beyond real depth is UB).
- `-O2`: frame 0 already points into libc, frame 1 segfaults. Inlining and tail
  calls destroy the walk; `-fno-omit-frame-pointer` does not fix it.

### Crash signal handlers — works

- `sigaction` + `backtrace_symbols_fd` produced tracebacks for
  SIGSEGV(11), SIGABRT(6), SIGFPE(8), SIGILL(4).
- `_exit(128 + sig)` keeps conventional exit codes.
- Warm up `backtrace()` before installing the handler so it never mallocs inside
  the handler.

## Platform support

| Backend | Linux | macOS | Windows |
|---|---|---|---|
| ASAN | yes | yes (clang) | yes (clang) |
| execinfo | yes | yes | no |
| `_Unwind_Backtrace` | yes | yes | yes (MinGW) |
| `CaptureStackBackTrace` | no | no | yes |
| signal handlers | `sigaction` | `sigaction` | `SetUnhandledExceptionFilter` |

## Recommendation

1. ASAN first: keep `__sanitizer_print_stack_trace()` when `mem$asan_enabled()`.
2. Otherwise `_Unwind_Backtrace()` + `dladdr` where available, raw IPs elsewhere.
3. Always-on for hosted builds; no-op when `cex$is_freestanding`.
4. Do not install signal handlers when ASAN is enabled; otherwise install
   POSIX `sigaction` / Windows `SetUnhandledExceptionFilter` with a re-entrancy
   guard so `abort()` does not print twice.
5. Do not force `-rdynamic`: it resolves only exported symbols. Document
   `addr2line`/`atos`/`llvm-symbolizer` for full names on non-ASAN builds.
6. Do not use `__builtin_return_address`.

## Windows / macOS notes

- `tb_win32.c` uses `<windows.h>` directly. The real `cex$platform_*`
  implementation would go through CEX's own `platform_win32.h` declarations.
- Windows symbol names need dbghelp + PDB (`-DCEX_TB_DBGHELP -ldbghelp`).
- macOS `-rdynamic` is unsupported; use `atos` for offline symbolization.
