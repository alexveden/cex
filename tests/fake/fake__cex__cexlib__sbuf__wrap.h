// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/sbuf.h>

// IMPORTANT: wrapping works only with gcc  `-Wl,--wrap=Shmem_new,--wrap=Protocol_event_emitter_new`  flag
FAKE_VALUE_FUNC(sbuf_head_s*, __wrap_sbuf__head, sbuf_c)sbuf_head_s* __real_sbuf__head(sbuf_c);

FAKE_VALUE_FUNC(size_t, __wrap_sbuf__alloc_capacity, size_t)size_t __real_sbuf__alloc_capacity(size_t);

FAKE_VALUE_FUNC(Exc, __wrap_sbuf__grow_buffer, sbuf_c*, u32)Exception __real_sbuf__grow_buffer(sbuf_c*, u32);

FAKE_VALUE_FUNC(Exc, __wrap_sbuf_create, sbuf_c*, u32, const Allocator_c*)Exception __real_sbuf_create(sbuf_c*, u32, const Allocator_c*);

FAKE_VALUE_FUNC(Exc, __wrap_sbuf_create_static, sbuf_c*, char*, size_t)Exception __real_sbuf_create_static(sbuf_c*, char*, size_t);

FAKE_VALUE_FUNC(Exc, __wrap_sbuf_grow, sbuf_c*, u32)Exception __real_sbuf_grow(sbuf_c*, u32);

FAKE_VALUE_FUNC(Exc, __wrap_sbuf_append_c, sbuf_c*, char*)Exception __real_sbuf_append_c(sbuf_c*, char*);

FAKE_VALUE_FUNC(Exc, __wrap_sbuf_replace, sbuf_c*, const sview_c, const sview_c)Exception __real_sbuf_replace(sbuf_c*, const sview_c, const sview_c);

FAKE_VALUE_FUNC(Exc, __wrap_sbuf_append, sbuf_c*, sview_c)Exception __real_sbuf_append(sbuf_c*, sview_c);

FAKE_VOID_FUNC(__wrap_sbuf_clear, sbuf_c*)void __real_sbuf_clear(sbuf_c*);

FAKE_VALUE_FUNC(u32, __wrap_sbuf_len, const sbuf_c)u32 __real_sbuf_len(const sbuf_c);

FAKE_VALUE_FUNC(u32, __wrap_sbuf_capacity, const sbuf_c)u32 __real_sbuf_capacity(const sbuf_c);

FAKE_VALUE_FUNC(sbuf_c, __wrap_sbuf_destroy, sbuf_c*)sbuf_c __real_sbuf_destroy(sbuf_c*);

FAKE_VALUE_FUNC_VARARG(Exc, __wrap_sbuf_sprintf, sbuf_c*, const char*, ...)Exception __real_sbuf_sprintf(sbuf_c*, const char*, ...);


const struct __module__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off
    .create = sbuf_create,
    .create_static = sbuf_create_static,
    .grow = sbuf_grow,
    .append_c = sbuf_append_c,
    .replace = sbuf_replace,
    .append = sbuf_append,
    .clear = sbuf_clear,
    .len = sbuf_len,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    .sprintf = sbuf_sprintf,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__sbuf__wrap__resetall(void) {
    RESET_FAKE(__wrap_sbuf__head)
    RESET_FAKE(__wrap_sbuf__alloc_capacity)
    RESET_FAKE(__wrap_sbuf__grow_buffer)
    RESET_FAKE(__wrap_sbuf_create)
    RESET_FAKE(__wrap_sbuf_create_static)
    RESET_FAKE(__wrap_sbuf_grow)
    RESET_FAKE(__wrap_sbuf_append_c)
    RESET_FAKE(__wrap_sbuf_replace)
    RESET_FAKE(__wrap_sbuf_append)
    RESET_FAKE(__wrap_sbuf_clear)
    RESET_FAKE(__wrap_sbuf_len)
    RESET_FAKE(__wrap_sbuf_capacity)
    RESET_FAKE(__wrap_sbuf_destroy)
    RESET_FAKE(__wrap_sbuf_sprintf)
}

