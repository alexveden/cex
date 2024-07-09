// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/list.h>


FAKE_VALUE_FUNC(list_head_s*, list__head, list_c*)
FAKE_VALUE_FUNC(size_t, list__alloc_capacity, size_t)
FAKE_VALUE_FUNC(void*, list__elidx, list_head_s*, size_t)
FAKE_VALUE_FUNC(list_head_s*, list__realloc, list_head_s*, size_t)
FAKE_VALUE_FUNC(size_t, list__alloc_size, size_t, size_t, size_t)
FAKE_VALUE_FUNC(Exc, list_create, list_c*, size_t, size_t, size_t, const Allocator_c*)
FAKE_VALUE_FUNC(Exc, list_append, void*, void*)
FAKE_VALUE_FUNC(Exc, list_extend, void*, void*, size_t)
FAKE_VALUE_FUNC(size_t, list_len, void*)
FAKE_VALUE_FUNC(void*, list_destroy, void*)
FAKE_VALUE_FUNC(void*, list_iter, void*, cex_iterator_s*)

const struct __module__list list = {
    // Autogenerated by CEX
    // clang-format off
    .create = list_create,
    .append = list_append,
    .extend = list_extend,
    .len = list_len,
    .destroy = list_destroy,
    .iter = list_iter,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__list__resetall(void) {
    RESET_FAKE(list__head)
    RESET_FAKE(list__alloc_capacity)
    RESET_FAKE(list__elidx)
    RESET_FAKE(list__realloc)
    RESET_FAKE(list__alloc_size)
    RESET_FAKE(list_create)
    RESET_FAKE(list_append)
    RESET_FAKE(list_extend)
    RESET_FAKE(list_len)
    RESET_FAKE(list_destroy)
    RESET_FAKE(list_iter)
}

