#!/usr/bin/zsh

rpl 'void my_test_shutdown_func\(void\)\{' 'test$teardown(){' **/test_*.c **/*.h
rpl 'ATEST_SETUP_F\(void\)' 'test$setup()' **/test_*.c **/*.h
rpl 'ATEST_F\(' 'test$case(' **/test_*.c **/*.h
rpl 'atassert' 'tassert' **/test_*.c
rpl '^\s+return\s+NULL; // Every.*' '    return EOK;\n' **/test_*.c
rpl 'ATEST_PARSE_MAINARGS' 'test$args_parse' **/test_*.c
rpl 'ATEST_PRINT_HEAD' 'test$print_header' **/test_*.c
rpl 'ATEST_RUN' 'test$run' **/test_*.c
rpl 'ATEST_PRINT_FOOTER' 'test$print_footer' **/test_*.c
rpl 'ATEST_EXITCODE' 'test$exit_code' **/test_*.c

