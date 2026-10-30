/*
 * Plain-C consumer of the cex_demo library.
 *
 * This file intentionally does not include cex.h. It only sees the public
 * header, exactly like any external C/C++/FFI caller would. It is linked
 * against the static archive built by `./cex build-lib`.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cex_demo.h"

int
main(void)
{
    /* Happy path: pass a request by pointer and check the mutated value. */
    cex_demo_request_t req = { .value = 41 };
    cex_demo_exc_t exc = cex_demo_run(&req);
    if (exc != cex_demo_Error.ok || req.value != 42) {
        fprintf(stderr, "run failed: exc=%s value=%d\n", exc ? exc : "NULL", req.value);
        return 1;
    }
    printf("value=%d\n", req.value);

    /*
     * Error path: NULL is invalid. The returned pointer must be compared by
     * address against the exported error table, not by string content.
     */
    exc = cex_demo_run(NULL);
    if (exc != cex_demo_Error.argument) {
        fprintf(stderr, "NULL request returned unexpected error\n");
        return 1;
    }
    printf("error=%s\n", exc);

    /*
     * The traceback is formatted into a buffer owned by this consumer. The
     * library never allocates on the consumer's behalf and never prints.
     */
    char traceback[512];
    if (cex_demo_traceback(traceback, sizeof traceback) != cex_demo_Error.ok) {
        fprintf(stderr, "traceback failed\n");
        return 1;
    }
    if (!strstr(traceback, "cex_demo__validate_request") || !strstr(traceback, "cex_demo_run")) {
        fprintf(stderr, "traceback missing frames: %s", traceback);
        return 1;
    }
    printf("traceback=%s", traceback);
    return 0;
}
