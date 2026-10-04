#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "src/all.c"
#include "src/all.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "vendor/subprocess.h"
#pragma GCC diagnostic pop

// CEX vendors sheredom/subprocess.h privately as _cex_subprocess_*. This test
// pulls in the real upstream subprocess.h too and proves both APIs coexist.
test$case(subprocess_upstream_and_cex_coexist)
{
    (void)subprocess_create;
    (void)subprocess_create_ex;
    (void)subprocess_join;
    (void)subprocess_destroy;
    (void)subprocess_terminate;
    (void)subprocess_alive;
    (void)subprocess_stdin;
    (void)subprocess_stdout;
    (void)subprocess_stderr;
    (void)subprocess_read_stdout;
    (void)subprocess_read_stderr;

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    struct subprocess_s p = { 0 };
    const char* cmd[] = { "true", NULL };
    tassert_eq(0, subprocess_create(cmd, subprocess_option_search_user_path, &p));
    int rc = -1;
    tassert_eq(0, subprocess_join(&p, &rc));
    tassert_eq(0, rc);
    subprocess_destroy(&p);

    tassert(os.cmd.exists("true"));
#endif

    return EOK;
}

test$main();
