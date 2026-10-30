# Building a C library with CEX

Minimal CEX project that builds a conventional C ABI library (`libcex_demo`) as
both a static archive and a shared library, then links a plain-C consumer
against it.

## API

`lib/cex_demo.h` is the only file consumers include:

- `cex_demo_request_t` — one `int32_t value`.
- `cex_demo_run(cex_demo_request_t* req)` — increments `req->value`; a `NULL`
  request returns `cex_demo_Error.argument`. Returns `cex_demo_Error.ok` (NULL)
  on success.
- `cex_demo_traceback(char* buf, size_t buf_size)` — formats the recorded CEX
  traceback into `buf`. Returns `ok`, `argument` for a `NULL`/zero-size buffer,
  or `overflow` when the traceback does not fit.
- `cex_demo_Error` — exported struct with `ok`, `argument`, and `overflow`
  pointers. Compare errors by pointer; the string is display-only.

## Build and test

```sh
cc ./cex.c -o ./cex      # bootstrap the project CLI
./cex test run all       # native unit tests (ASan/UBSan by default)
./cex build-lib          # static + shared library -> build/lib/
./cex consumer           # build the lib, then compile and run consumer/main.c
```

`./cex build-lib` writes:

- `build/lib/libcex_demo.a` (or `.lib` on Windows)
- `build/lib/libcex_demo.so` (`.dylib` on macOS, `.dll` on Windows)

## Layout

| Path | Purpose |
|---|---|
| `lib/cex_demo.h` | public ABI: standard C types, `extern "C"`, export macro |
| `lib/cex_demo.c` | implementation; embeds CEX via `CEX_IMPLEMENTATION` |
| `tests/test_cex_demo.c` | native tests; unity-includes `lib/cex_demo.c` |
| `consumer/main.c` | plain C consumer linked against the static archive |
| `cex.c` | CLI + custom `build-lib` / `consumer` commands |

## How the ABI works

- The library is compiled with `-fvisibility=hidden`; only `CEX_DEMO_API`
  symbols are exported.
- Errors are `const char*`; `NULL` means success. Compare by pointer against
  `cex_demo_Error.*`, never by string content.
- The traceback is formatted into a caller-owned buffer, so foreign callers do
  not need access to CEX's global traceback ring.
- Public structs are POD and fixed-width, so the layout is the same across
  compilers and language bindings.

## FFI

The shared library can be loaded from any FFI: Python `ctypes`/`cffi`, Node,
Rust, Zig, etc. Compare the returned `const char*` against the exported
`cex_demo_Error` pointers and read the traceback with `cex_demo_traceback()`.

A larger reference with 12 language bindings and zig cross-compilation targets:
<https://github.com/alexveden/cex_lib_ffi_test>.
