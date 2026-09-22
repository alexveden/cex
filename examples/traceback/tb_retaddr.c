// Traceback experiment: __builtin_return_address(n) frame walking
//
// Shows how frame-pointer omission (-fomit-frame-pointer, default at -O2)
// breaks naive return-address walking. The depth is capped at 4 on purpose:
// __builtin_return_address() beyond the real stack depth is undefined, and the
// argument must be a compile-time constant.
#include <stdio.h>

static int
tb_level3(void)
{
    fprintf(stderr, "--- __builtin_return_address (max 4) ---\n");
    fprintf(stderr, "%2d: %p\n", 0, __builtin_return_address(0));
    fprintf(stderr, "%2d: %p\n", 1, __builtin_return_address(1));
    fprintf(stderr, "%2d: %p\n", 2, __builtin_return_address(2));
    fprintf(stderr, "%2d: %p\n", 3, __builtin_return_address(3));
    return 0;
}

static int
tb_level2(void)
{
    return tb_level3();
}

static int
tb_level1(void)
{
    return tb_level2();
}

int
main(void)
{
    return tb_level1();
}
