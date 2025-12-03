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
    if (json.reader.create(&js, (char*)data, size, false)) { return 0; }
    while (json.reader.next(&js)) {}

    // strict mode
    if (json.reader.create(&js, (char*)data, size, true)) { return 0; }
    while (json.reader.next(&js)) {}

    return 0;
}

fuzz$main();
