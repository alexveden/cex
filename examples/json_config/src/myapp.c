#define CEX_IMPLEMENTATION
#include "cex.h"
#include "lib/mylib.c"  /* NOTE: include .c to make unity build! */
#include "src/App.c"
#include "cexstd/json/json.c"
#include "src/serde.c"

int main(int argc, char** argv){
    (void)argc;
    (void)argv;

    App_c app = {0};
    e$except(err, App.create(&app, argc, argv)) {
        io.printf("Exc = %s\n", err);
        return 1;
    }

    App.destroy(&app);

    return 0;
}
