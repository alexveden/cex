// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <_cexlib/allocators.h>



const struct __module__allocators allocators = {
    // Autogenerated by CEX
    // clang-format off

    .heap = {  // sub-module .heap >>>
        .create = allocators__heap__create,
        .destroy = allocators__heap__destroy,
    },  // sub-module .heap <<<

    .staticarena = {  // sub-module .staticarena >>>
        .create = allocators__staticarena__create,
        .destroy = allocators__staticarena__destroy,
    },  // sub-module .staticarena <<<
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__allocators__resetall(void) {
}

