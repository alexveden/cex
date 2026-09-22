// Traceback experiment: AddressSanitizer stack trace
//
// __sanitizer_print_stack_trace() is provided by the ASAN runtime and yields the
// richest output (function, file:line, offset) on Linux, macOS and Windows.
// Must be compiled with -fsanitize=address.
#if defined(__has_feature)
#    if __has_feature(address_sanitizer)
#        define TB_HAS_ASAN 1
#    endif
#endif

#ifndef TB_HAS_ASAN
#    if defined(__SANITIZE_ADDRESS__)
#        define TB_HAS_ASAN 1
#    else
#        define TB_HAS_ASAN 0
#    endif
#endif

#include <stdio.h>

#if TB_HAS_ASAN
extern void __sanitizer_print_stack_trace(void);
#endif

static int
tb_level3(void)
{
#if TB_HAS_ASAN
    fprintf(stderr, "--- __sanitizer_print_stack_trace ---\n");
    __sanitizer_print_stack_trace();
#else
    fprintf(stderr, "SKIP: not compiled with -fsanitize=address\n");
#endif
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
