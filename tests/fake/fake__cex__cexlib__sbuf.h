// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/sbuf.h>


FAKE_VALUE_FUNC(sbuf_head_s*, sbuf__head, sbuf_c)
FAKE_VALUE_FUNC(size_t, sbuf__alloc_capacity, size_t)
FAKE_VALUE_FUNC(Exc, sbuf__grow_buffer, sbuf_c*, u32)
FAKE_VALUE_FUNC(Exc, sbuf_create, sbuf_c*, u32, const Allocator_c*)
FAKE_VALUE_FUNC(Exc, sbuf_create_static, sbuf_c*, char*, size_t)
FAKE_VALUE_FUNC(Exc, sbuf_grow, sbuf_c*, u32)
FAKE_VALUE_FUNC(Exc, sbuf_append_c, sbuf_c*, char*)
FAKE_VALUE_FUNC(Exc, sbuf_replace, sbuf_c*, const sview_c, const sview_c)
FAKE_VALUE_FUNC(Exc, sbuf_append, sbuf_c*, sview_c)
FAKE_VOID_FUNC(sbuf_clear, sbuf_c*)
FAKE_VALUE_FUNC(u32, sbuf_length, const sbuf_c)
FAKE_VALUE_FUNC(u32, sbuf_capacity, const sbuf_c)
FAKE_VALUE_FUNC(sbuf_c, sbuf_destroy, sbuf_c*)

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
    .length = sbuf_length,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__sbuf__resetall(void) {
    RESET_FAKE(sbuf__head)
    RESET_FAKE(sbuf__alloc_capacity)
    RESET_FAKE(sbuf__grow_buffer)
    RESET_FAKE(sbuf_create)
    RESET_FAKE(sbuf_create_static)
    RESET_FAKE(sbuf_grow)
    RESET_FAKE(sbuf_append_c)
    RESET_FAKE(sbuf_replace)
    RESET_FAKE(sbuf_append)
    RESET_FAKE(sbuf_clear)
    RESET_FAKE(sbuf_length)
    RESET_FAKE(sbuf_capacity)
    RESET_FAKE(sbuf_destroy)
}

