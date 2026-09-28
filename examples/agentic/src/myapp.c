#define CEX_IMPLEMENTATION
#include "cex.h"
#include "lib/mylib.c"  /* NOTE: include .c to make unity build! */

int main(int argc, char** argv){
    (void)argc;
    (void)argv;
    io.printf("MOCCA - Make Old C Cexy Again!\n");
    io.printf("1 + 2 = %d\n", mylib_add(1, 2));
    return 0;
}
