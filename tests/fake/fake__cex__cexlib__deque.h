// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/deque.h>


FAKE_VALUE_FUNC(deque_head_s*, deque__head, deque_c)
FAKE_VALUE_FUNC(size_t, deque__alloc_capacity, size_t)
FAKE_VALUE_FUNC(void*, deque__get_byindex, deque_head_s*, size_t)
FAKE_VALUE_FUNC(size_t, deque__alloc_size, size_t, size_t, size_t)
FAKE_VALUE_FUNC(Exc, deque_validate, deque_c)
FAKE_VALUE_FUNC(Exc, deque_create, deque_c*, size_t, bool, size_t, size_t, const Allocator_c*)
FAKE_VALUE_FUNC(Exc, deque_create_static, deque_c*, void*, size_t, bool, size_t, size_t)
FAKE_VALUE_FUNC(Exc, deque_append, deque_c*, const void*)
FAKE_VALUE_FUNC(Exc, deque_enqueue, deque_c*, const void*)
FAKE_VALUE_FUNC(Exc, deque_push, deque_c*, const void*)
FAKE_VALUE_FUNC(void*, deque_dequeue, deque_c*)
FAKE_VALUE_FUNC(void*, deque_pop, deque_c*)
FAKE_VALUE_FUNC(void*, deque_get, deque_c*, size_t)
FAKE_VALUE_FUNC(size_t, deque_count, const deque_c*)
FAKE_VOID_FUNC(deque_clear, deque_c*)
FAKE_VALUE_FUNC(void*, deque_destroy, deque_c*)
FAKE_VALUE_FUNC(void*, deque_iter_get, deque_c*, i32, cex_iterator_s*)
FAKE_VALUE_FUNC(void*, deque_iter_fetch, deque_c*, i32, cex_iterator_s*)

const struct __module__deque deque = {
    // Autogenerated by CEX
    // clang-format off
    .validate = deque_validate,
    .create = deque_create,
    .create_static = deque_create_static,
    .append = deque_append,
    .enqueue = deque_enqueue,
    .push = deque_push,
    .dequeue = deque_dequeue,
    .pop = deque_pop,
    .get = deque_get,
    .count = deque_count,
    .clear = deque_clear,
    .destroy = deque_destroy,
    .iter_get = deque_iter_get,
    .iter_fetch = deque_iter_fetch,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__deque__resetall(void) {
    RESET_FAKE(deque__head)
    RESET_FAKE(deque__alloc_capacity)
    RESET_FAKE(deque__get_byindex)
    RESET_FAKE(deque__alloc_size)
    RESET_FAKE(deque_validate)
    RESET_FAKE(deque_create)
    RESET_FAKE(deque_create_static)
    RESET_FAKE(deque_append)
    RESET_FAKE(deque_enqueue)
    RESET_FAKE(deque_push)
    RESET_FAKE(deque_dequeue)
    RESET_FAKE(deque_pop)
    RESET_FAKE(deque_get)
    RESET_FAKE(deque_count)
    RESET_FAKE(deque_clear)
    RESET_FAKE(deque_destroy)
    RESET_FAKE(deque_iter_get)
    RESET_FAKE(deque_iter_fetch)
}

