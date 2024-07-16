#!/usr/bin/zsh

rpl 'void my_test_shutdown_func\(void\)\{' 'cextest$teardown(){' **/test_*.c **/*.h
rpl 'ATEST_SETUP_F\(void\)' 'cextest$setup()' **/test_*.c **/*.h
rpl 'ATEST_F\(' 'cextest$case(' **/test_*.c **/*.h
rpl 'atassert' 'tassert' **/test_*.c
rpl '^\s+return\s+NULL; // Every.*' '    return EOK;\n' **/test_*.c
rpl 'ATEST_PARSE_MAINARGS' 'cextest$args_parse' **/test_*.c
rpl 'ATEST_PRINT_HEAD' 'cextest$print_header' **/test_*.c
rpl 'ATEST_RUN' 'cextest$run' **/test_*.c
rpl 'ATEST_PRINT_FOOTER' 'cextest$print_footer' **/test_*.c
rpl 'ATEST_EXITCODE' 'cextest$exit_code' **/test_*.c

