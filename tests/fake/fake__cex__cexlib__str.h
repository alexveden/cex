// Autogenerated <fff.h> mocks
// NOTE: Do not edit, all changes can be lost

// clang-format off
#include <cex/cextest/fff.h>
#include <cex/cexlib/str.h>


FAKE_VALUE_FUNC(str_c, str_cstr, const char*)
FAKE_VALUE_FUNC(str_c, str_cbuf, char*, size_t)
FAKE_VALUE_FUNC(str_c, str_sub, str_c, ssize_t, ssize_t)
FAKE_VALUE_FUNC(Exc, str_copy, str_c, char*, size_t)
FAKE_VALUE_FUNC(size_t, str_len, str_c)
FAKE_VALUE_FUNC(bool, str_is_valid, str_c)
FAKE_VALUE_FUNC(char*, str_iter, str_c, cex_iterator_s*)
FAKE_VALUE_FUNC(ssize_t, str_find, str_c, str_c, size_t, size_t)
FAKE_VALUE_FUNC(ssize_t, str_rfind, str_c, str_c, size_t, size_t)
FAKE_VALUE_FUNC(bool, str_contains, str_c, str_c)
FAKE_VALUE_FUNC(bool, str_starts_with, str_c, str_c)
FAKE_VALUE_FUNC(bool, str_ends_with, str_c, str_c)
FAKE_VALUE_FUNC(str_c, str_remove_prefix, str_c, str_c)
FAKE_VALUE_FUNC(str_c, str_remove_suffix, str_c, str_c)
FAKE_VALUE_FUNC(str_c, str_lstrip, str_c)
FAKE_VALUE_FUNC(str_c, str_rstrip, str_c)
FAKE_VALUE_FUNC(str_c, str_strip, str_c)
FAKE_VALUE_FUNC(int, str_cmp, str_c, str_c)
FAKE_VALUE_FUNC(int, str_cmpi, str_c, str_c)
FAKE_VALUE_FUNC(str_c*, str_iter_split, str_c, const char*, cex_iterator_s*)

const struct __module__str str = {
    // Autogenerated by CEX
    // clang-format off
    .cstr = str_cstr,
    .cbuf = str_cbuf,
    .sub = str_sub,
    .copy = str_copy,
    .len = str_len,
    .is_valid = str_is_valid,
    .iter = str_iter,
    .find = str_find,
    .rfind = str_rfind,
    .contains = str_contains,
    .starts_with = str_starts_with,
    .ends_with = str_ends_with,
    .remove_prefix = str_remove_prefix,
    .remove_suffix = str_remove_suffix,
    .lstrip = str_lstrip,
    .rstrip = str_rstrip,
    .strip = str_strip,
    .cmp = str_cmp,
    .cmpi = str_cmpi,
    .iter_split = str_iter_split,
    // clang-format on
};
// clang-format off


static void fake__cex__cexlib__str__resetall(void) {
    RESET_FAKE(str_cstr)
    RESET_FAKE(str_cbuf)
    RESET_FAKE(str_sub)
    RESET_FAKE(str_copy)
    RESET_FAKE(str_len)
    RESET_FAKE(str_is_valid)
    RESET_FAKE(str_iter)
    RESET_FAKE(str_find)
    RESET_FAKE(str_rfind)
    RESET_FAKE(str_contains)
    RESET_FAKE(str_starts_with)
    RESET_FAKE(str_ends_with)
    RESET_FAKE(str_remove_prefix)
    RESET_FAKE(str_remove_suffix)
    RESET_FAKE(str_lstrip)
    RESET_FAKE(str_rstrip)
    RESET_FAKE(str_strip)
    RESET_FAKE(str_cmp)
    RESET_FAKE(str_cmpi)
    RESET_FAKE(str_iter_split)
}

