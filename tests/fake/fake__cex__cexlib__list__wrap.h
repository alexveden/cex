// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/list.h>

// IMPORTANT: wrapping works only with gcc  `-Wl,--wrap=Shmem_new,--wrap=Protocol_event_emitter_new`  flag
FAKE_VALUE_FUNC(Exc, __wrap_list_create, list_c*, size_t, size_t, size_t, const Allocator_c*)Exception __real_list_create(list_c*, size_t, size_t, size_t, const Allocator_c*);

FAKE_VALUE_FUNC(Exc, __wrap_list_create_static, list_c*, void*, size_t, size_t, size_t)Exception __real_list_create_static(list_c*, void*, size_t, size_t, size_t);

FAKE_VALUE_FUNC(Exc, __wrap_list_insert, void*, void*, size_t)Exception __real_list_insert(void*, void*, size_t);

FAKE_VALUE_FUNC(Exc, __wrap_list_del, void*, size_t)Exception __real_list_del(void*, size_t);

FAKE_VOID_FUNC(__wrap_list_sort, void*, int (*comp)(const, const)void __real_list_sort(void*, int (*comp)(const, const);

FAKE_VALUE_FUNC(Exc, __wrap_list_append, void*, void*)Exception __real_list_append(void*, void*);

FAKE_VOID_FUNC(__wrap_list_clear, void*)void __real_list_clear(void*);

FAKE_VALUE_FUNC(Exc, __wrap_list_extend, void*, void*, size_t)Exception __real_list_extend(void*, void*, size_t);

FAKE_VALUE_FUNC(size_t, __wrap_list_len, void*)size_t __real_list_len(void*);

FAKE_VALUE_FUNC(size_t, __wrap_list_capacity, void*)size_t __real_list_capacity(void*);

FAKE_VALUE_FUNC(void*, __wrap_list_destroy, void*)void* __real_list_destroy(void*);

FAKE_VALUE_FUNC(void*, __wrap_list_iter, void*, cex_iterator_s*)void* __real_list_iter(void*, cex_iterator_s*);


const struct __module__list list = {
    // Autogenerated by CEX
    // clang-format off
    .create = list_create,
    .create_static = list_create_static,
    .insert = list_insert,
    .del = list_del,
    .sort = list_sort,
    .append = list_append,
    .clear = list_clear,
    .extend = list_extend,
    .len = list_len,
    .capacity = list_capacity,
    .destroy = list_destroy,
    .iter = list_iter,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__list__wrap__resetall(void) {
    RESET_FAKE(__wrap_list_create)
    RESET_FAKE(__wrap_list_create_static)
    RESET_FAKE(__wrap_list_insert)
    RESET_FAKE(__wrap_list_del)
    RESET_FAKE(__wrap_list_sort)
    RESET_FAKE(__wrap_list_append)
    RESET_FAKE(__wrap_list_clear)
    RESET_FAKE(__wrap_list_extend)
    RESET_FAKE(__wrap_list_len)
    RESET_FAKE(__wrap_list_capacity)
    RESET_FAKE(__wrap_list_destroy)
    RESET_FAKE(__wrap_list_iter)
}

