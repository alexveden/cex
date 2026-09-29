#pragma once
#if !defined(cex$enable_minimal)
#include "all.h"

/// Fuzz case: ``int fuzz$case(const u8* data, usize size) { return 0;}
#define fuzz$case test$noopt LLVMFuzzerTestOneInput

/// Fuzz test constructor (for building corpus seeds programmatically)
#define fuzz$setup __attribute__((constructor)) void _cex_fuzzer_setup_constructor

/// Special fuzz variable used by all fuzz$ macros
#define fuzz$dvar _cex_fuz_dvar

/// Initialize fuzz$ helper macros
#define fuzz$dnew(data, size) cex_fuzz_s fuzz$dvar = fuzz.create((data), (size))

/// Load random data into variable by pointer from random fuzz data
#define fuzz$dget(out_result_ptr) fuzz.dget(&(fuzz$dvar), (out_result_ptr), sizeof(*(out_result_ptr)))

/// Get deterministic probability based on fuzz data
#define fuzz$dprob(prob_threshold) fuzz.dprob(&(fuzz$dvar), prob_threshold)

/// Current fuzz_ file corpus directory relative to calling source file
#define fuzz$corpus_dir fuzz.corpus_dir(__FILE_NAME__)                                                                          \

/// Fuzz data fetcher (makes C-type payloads from random fuzz$case data)
typedef struct cex_fuzz_s
{
    const u8* cur;
    const u8* end;
} cex_fuzz_s;

#ifndef CEX_FUZZ_MAX_BUF
/// Max fuzz input size (stack buffer), default 1024000
#    define CEX_FUZZ_MAX_BUF 1024000
#endif

#ifndef CEX_FUZZ_AFL
/// Fuzz main function
#    define fuzz$main()
#else
#    ifndef __AFL_INIT
void __AFL_INIT(void);
#    endif
#    ifndef __AFL_LOOP
int __AFL_LOOP(unsigned int N);
#    endif
#    define fuzz$main()                                                                            \
        test$noopt int main(int argc, char** argv)                                                 \
        {                                                                                          \
            (void)argc;                                                                            \
            (void)argv;                                                                            \
            isize len = 0;                                                                         \
            char buf[CEX_FUZZ_MAX_BUF];                                                            \
            __AFL_INIT();                                                                          \
            while (__AFL_LOOP(UINT_MAX)) {                                                         \
                len = read(0, buf, sizeof(buf));                                                   \
                if (len < 0) {                                                                     \
                    log$error("ERROR: reading AFL input: %s\n", strerror(errno));                  \
                    return errno;                                                                  \
                }                                                                                  \
                if (len == 0) {                                                                    \
                    log$info("Nothing to read, breaking?\n");                                      \
                    break;                                                                         \
                }                                                                                  \
                u8* test_data = malloc(len);                                                       \
                uassert(test_data != NULL && "memory error");                                      \
                memcpy(test_data, buf, len);                                                       \
                LLVMFuzzerTestOneInput(test_data, (usize)len);                                     \
                free(test_data);                                                                   \
            }                                                                                      \
            return 0;                                                                              \
        }

#endif

/**

## Fuzzing

libFuzzer (clang) and AFL++ harnesses with sanitizers enabled by default. The goal is
to trigger `uassert()` / ASAN / UBSan failures, not to validate the target's error codes.

- `fuzz$case()` is the fuzzer entry point (`LLVMFuzzerTestOneInput`)
- `fuzz$main()` emits the `main()` required by AFL++ (no-op for libFuzzer)
- `fuzz$setup()` seeds the corpus at load time (constructor), optional
- the `fuzz` namespace / `fuzz$` macros turn raw input bytes into typed values
- return `-1` from `fuzz$case()` to skip an input (wrong size / uninteresting)

### CLI

| Command | Purpose |
| --- | --- |
| `./cex fuzz create fuzz/myapp/fuzz_bar.c` | scaffold a new `fuzz_` file |
| `./cex fuzz run fuzz/some/fuzz_file.c` | build + run one harness |
| `./cex fuzz run all` | build + run every `fuzz/fuzz_*.c` (60s each) |
| `./cex fuzz debug fuzz/some/fuzz_file.c` | run one harness under `cexy$debug_cmd` |

Options:

- `--max-time=N` — seconds per harness (60 for `all`, unlimited for a single target)
- `--timeout=N` — seconds before one input is treated as a hang (default 10)
- trailing arguments are forwarded to the fuzzer (e.g. `-runs=1000`, `-seed=1`)

`cexy$fuzzer` selects the toolchain — clang libFuzzer by default; set it to an `afl-*`
command to build AFL++ mode (adds `-DCEX_FUZZ_AFL`). AFL++ runs use `fuzz$main()` and
cannot be launched with `fuzz debug`.

### Corpus layout

Corpus directories are named after the harness file and live next to it:

- `<harness>_corpus` — seed inputs read by the fuzzer (write them in `fuzz$setup()`)
- `<harness>_corpus.out` — new coverage inputs produced by libFuzzer
- `<harness>_corpus.afl` — AFL++ output directory
- `<harness>.dict` — optional dictionary, picked up automatically when present

On a crash, copy the offending case from `<harness>_corpus.out` into
`<harness>_corpus` so it is kept as a regression seed.

### Writing a case

```c
int
fuzz$case(const u8* data, usize size)
{
    if (size < 4 || size > 256) { return -1; }  // skip uninteresting sizes

    // raw bytes are always available
    my_parser((char*)data, size);

    return 0;
}
```

### Consuming typed values

`fuzz.create()` wraps the raw input; `fuzz.dget()` copies the next `sizeof(*ptr)` bytes
and returns `false` once the input is exhausted. `fuzz.dprob()` consumes one byte as a
probability (threshold must be `> 1/255` and `< 1.0`). The `fuzz$` macros are shortcuts
that drop the `fz` variable and `sizeof` bookkeeping:

```c
int
fuzz$case(const u8* data, usize size)
{
    fuzz$dnew(data, size);

    u16 val = 0;
    my_struct_s st = { 0 };

    while (fuzz$dget(&val)) {
        my_func(val);
        if (fuzz$dprob(0.2)) { my_func(val * 10); }
        if (fuzz$dget(&st)) { my_func_struct(&st); }
    }

    return 0;
}
```

### Seeding a corpus

`fuzz$setup()` runs before `main()` and writes seed files into `fuzz$corpus_dir`
(derived from the calling source file). It is optional; skip it if random input is fine.

```c
fuzz$setup()
{
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    char* seeds[] = { "123", "1e999", "-inf", "0.0001" };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(seeds); i++) {
            char* fn = str.fmt(_, "%s/%03d", fuzz$corpus_dir, i);
            if (io.file.save(fn, seeds[i])) { uassert(false && "seed write failed"); }
        }
    }
}
```

*/
struct __cex_namespace__fuzz {
    // Autogenerated by CEX
    // clang-format off

    /// Get current corpus dir relative tho the `this_file_name`
    char*           (*corpus_dir)(char* this_file_name);
    /// Creates new fuzz data generator, for fuzz-driven randomization
    cex_fuzz_s      (*create)(const u8* data, usize size);
    /// Get result from random data into buffer (returns false if not enough data)
    bool            (*dget)(cex_fuzz_s* fz, void* out_result, usize result_size);
    /// Get probability using fuzz data, based on threshold
    bool            (*dprob)(cex_fuzz_s* fz, double threshold);

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__fuzz fuzz;

#endif
