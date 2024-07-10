// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/dict.h>

// IMPORTANT: wrapping works only with gcc  `-Wl,--wrap=Shmem_new,--wrap=Protocol_event_emitter_new`  flag
FAKE_VALUE_FUNC(u64, __wrap_hm_int_hash_simple, u64)u64 __real_hm_int_hash_simple(u64);

FAKE_VALUE_FUNC(int, __wrap_hm_int_compare, const void*, const void*, void*)int __real_hm_int_compare(const void*, const void*, void*);

FAKE_VALUE_FUNC(u64, __wrap_hm_int_hash, const void*, u64, u64)u64 __real_hm_int_hash(const void*, u64, u64);

FAKE_VALUE_FUNC(int, __wrap_hm_str_compare, const void*, const void*, void*)int __real_hm_str_compare(const void*, const void*, void*);

FAKE_VALUE_FUNC(int, __wrap_hm_str_static_compare, const void*, const void*, void*)int __real_hm_str_static_compare(const void*, const void*, void*);

FAKE_VALUE_FUNC(u64, __wrap_hm_str_hash, const void*, u64, u64)u64 __real_hm_str_hash(const void*, u64, u64);

FAKE_VALUE_FUNC(u64, __wrap_hm_str_static_hash, const void*, u64, u64)u64 __real_hm_str_static_hash(const void*, u64, u64);

FAKE_VALUE_FUNC(dict_c*, __wrap_dict_create_u64, size_t, u32)dict_c* __real_dict_create_u64(size_t, u32);

FAKE_VALUE_FUNC(Exc, __wrap_dict_create, dict_c**, size_t, size_t, size_t, size_t, u64 (*hash_func)(const void*, u64, u64, i32 (*compare_func)(const void*, const void*, void*, const Allocator_c*, void (*elfree)(void*, void*)Exception __real_dict_create(dict_c**, size_t, size_t, size_t, size_t, u64 (*hash_func)(const void*, u64, u64, i32 (*compare_func)(const void*, const void*, void*, const Allocator_c*, void (*elfree)(void*, void*);

FAKE_VALUE_FUNC(dict_c*, __wrap_dict_create_str, size_t, u32)dict_c* __real_dict_create_str(size_t, u32);

FAKE_VALUE_FUNC(Exc, __wrap_dict_set, dict_c*, const void*)Exception __real_dict_set(dict_c*, const void*);

FAKE_VALUE_FUNC(void*, __wrap_dict_geti, dict_c*, u64)void* __real_dict_geti(dict_c*, u64);

FAKE_VALUE_FUNC(void*, __wrap_dict_gets, dict_c*, const char*)void* __real_dict_gets(dict_c*, const char*);

FAKE_VALUE_FUNC(void*, __wrap_dict_get, dict_c*, const void*)void* __real_dict_get(dict_c*, const void*);

FAKE_VALUE_FUNC(size_t, __wrap_dict_len, dict_c*)size_t __real_dict_len(dict_c*);

FAKE_VALUE_FUNC(dict_c*, __wrap_dict_destroy, dict_c**)dict_c* __real_dict_destroy(dict_c**);

FAKE_VOID_FUNC(__wrap_dict_clear, dict_c*)void __real_dict_clear(dict_c*);


const struct __module__dict dict = {
    // Autogenerated by CEX
    // clang-format off
    .create_u64 = dict_create_u64,
    .create = dict_create,
    .create_str = dict_create_str,
    .set = dict_set,
    .geti = dict_geti,
    .gets = dict_gets,
    .get = dict_get,
    .len = dict_len,
    .destroy = dict_destroy,
    .clear = dict_clear,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__dict__wrap__resetall(void) {
    RESET_FAKE(__wrap_hm_int_hash_simple)
    RESET_FAKE(__wrap_hm_int_compare)
    RESET_FAKE(__wrap_hm_int_hash)
    RESET_FAKE(__wrap_hm_str_compare)
    RESET_FAKE(__wrap_hm_str_static_compare)
    RESET_FAKE(__wrap_hm_str_hash)
    RESET_FAKE(__wrap_hm_str_static_hash)
    RESET_FAKE(__wrap_dict_create_u64)
    RESET_FAKE(__wrap_dict_create)
    RESET_FAKE(__wrap_dict_create_str)
    RESET_FAKE(__wrap_dict_set)
    RESET_FAKE(__wrap_dict_geti)
    RESET_FAKE(__wrap_dict_gets)
    RESET_FAKE(__wrap_dict_get)
    RESET_FAKE(__wrap_dict_len)
    RESET_FAKE(__wrap_dict_destroy)
    RESET_FAKE(__wrap_dict_clear)
}

