#include "src/all.c"
#include "lib/json/json.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


int
fuzz$case(const u8* data, usize size)
{
    if (size == 0) { return -1; }
    jr_c js;

    // permissive mode
    if (jr$new(&js, (char*)data, size, .strict_mode = false)) { return 0; }
    while(_cex_json__reader__next(&js)) {}

    // strict mode
    if (jr$new(&js, (char*)data, size, .strict_mode = true)) { return 0; }
    while(_cex_json__reader__next(&js)) {}

    return 0;
}

fuzz$main();
