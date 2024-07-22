#include "str.h"
#include "_stb_sprintf.h"
#include <ctype.h>
#include <float.h>
#include <math.h>


static inline bool
str__isvalid(const str_c* s)
{
    return s->buf != NULL;
}

static inline ssize_t
str__index(str_c* s, const char* c, u8 clen)
{
    ssize_t result = -1;

    if (!str__isvalid(s)) {
        return -1;
    }

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) {
        split_by_idx[(u8)c[i]] = 1;
    }

    for (size_t i = 0; i < s->len; i++) {
        if (split_by_idx[(u8)s->buf[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

str_c
str_cstr(const char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}

str_c
str_cbuf(char* s, size_t length)
{
    if (unlikely(s == NULL)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = s,
        .len = strnlen(s, length),
    };
}

str_c
str_sub(str_c s, ssize_t start, ssize_t end)
{
    if (unlikely(s.len == 0)) {
        if (start == 0 && end == 0) {
            return s;
        } else {
            return (str_c){ 0 };
        }
    }

    if (start < 0) {
        // negative start uses offset from the end (-1 - last char)
        start += s.len;
    }

    if (end == 0) {
        // if end is zero, uses remainder
        end = s.len;
    } else if (end < 0) {
        // negative end, uses offset from the end
        end += s.len;
    }

    if (unlikely((size_t)start >= s.len || end > (ssize_t)s.len || end < start)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = s.buf + start,
        .len = (size_t)(end - start),
    };
}

Exception
str_copy(str_c s, char* dest, size_t destlen)
{
    uassert(dest != s.buf && "self copy is not allowed");

    if (unlikely(dest == NULL || destlen == 0)) {
        return Error.argument;
    }

    // If we fail next, it still writes empty string
    dest[0] = '\0';

    if (unlikely(!str__isvalid(&s))) {
        return Error.argument;
    }

    size_t slen = s.len;
    size_t len_to_copy = MIN(destlen - 1, slen);
    memcpy(dest, s.buf, len_to_copy);

    // Null terminate end of buffer
    dest[len_to_copy] = '\0';

    if (unlikely(slen >= destlen)) {
        return Error.overflow;
    }

    return Error.ok;
}

Exception
str_sprintf(char* dest, size_t dest_len, const char* format, ...) {

    va_list va;
    va_start(va, format);

    int result = STB_SPRINTF_DECORATE(vsnprintf)(dest, dest_len, format, va);
    va_end(va);
    if (result >= 0){
        if((unsigned)result >= dest_len)
            return Error.overflow;
        return Error.ok;
    } else {
        return Error.argument;
    }
}

size_t
str_len(str_c s)
{
    return s.len;
}

bool
str_is_valid(str_c s)
{
    return str__isvalid(&s);
}

char*
str_iter(str_c s, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == alignof(size_t), "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        // Iterator is not initialized set to 1st item
        if (unlikely(!str__isvalid(&s) || s.len == 0)) {
            return NULL;
        }
        iterator->val = s.buf;
        iterator->idx.i = 0;
        return &s.buf[0];
    }

    if (ctx->cursor >= s.len - 1) {
        return NULL;
    }

    ctx->cursor++;
    iterator->idx.i++;
    iterator->val = &s.buf[iterator->idx.i];
    return iterator->val;
}

ssize_t
str_find(str_c s, str_c needle, size_t start, size_t end)
{
    if (needle.len == 0 || needle.len > s.len) {
        return -1;
    }
    if (start >= s.len) {
        return -1;
    }

    if (end == 0 || end > s.len) {
        end = s.len;
    }

    for (size_t i = start; i < end - needle.len + 1; i++) {
        // check the 1st letter
        if (s.buf[i] == needle.buf[0]) {
            // 1 char
            if (needle.len == 1) {
                return i;
            }
            // check whole word
            if (memcmp(&s.buf[i], needle.buf, needle.len) == 0) {
                return i;
            }
        }
    }

    return -1;
}

ssize_t
str_rfind(str_c s, str_c needle, size_t start, size_t end)
{
    if (needle.len == 0 || needle.len > s.len) {
        return -1;
    }
    if (start >= s.len) {
        return -1;
    }

    if (end == 0 || end > s.len) {
        end = s.len;
    }

    for (size_t i = end - needle.len + 1; i-- > start;) {
        // check the 1st letter
        if (s.buf[i] == needle.buf[0]) {
            // 1 char
            if (needle.len == 1) {
                return i;
            }
            // check whole word
            if (memcmp(&s.buf[i], needle.buf, needle.len) == 0) {
                return i;
            }
        }
    }

    return -1;
}

bool
str_contains(str_c s, str_c needle)
{
    return str_find(s, needle, 0, 0) != -1;
}

bool
str_starts_with(str_c s, str_c needle)
{
    return str_find(s, needle, 0, needle.len) != -1;
}

bool
str_ends_with(str_c s, str_c needle)
{
    if (needle.len > s.len) {
        return false;
    }

    return str_find(s, needle, s.len - needle.len, 0) != -1;
}

str_c
str_remove_prefix(str_c s, str_c prefix)
{
    ssize_t idx = str_find(s, prefix, 0, prefix.len);
    if (idx == -1) {
        return s;
    }

    return (str_c){
        .buf = s.buf + prefix.len,
        .len = s.len - prefix.len,
    };
}

str_c
str_remove_suffix(str_c s, str_c suffix)
{
    if (suffix.len > s.len) {
        return s;
    }

    ssize_t idx = str_find(s, suffix, s.len - suffix.len, 0);

    if (idx == -1) {
        return s;
    }

    return (str_c){
        .buf = s.buf,
        .len = s.len - suffix.len,
    };
}


static inline void
str__strip_left(str_c* s)
{
    char* cend = s->buf + s->len;

    while (s->buf < cend) {
        switch (*s->buf) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->buf++;
                s->len--;
                break;
            default:
                return;
        }
    }
}

static inline void
str__strip_right(str_c* s)
{
    while (s->len > 0) {
        switch (s->buf[s->len - 1]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->len--;
                break;
            default:
                return;
        }
    }
}


str_c
str_lstrip(str_c s)
{
    if (s.buf == NULL) {
        return (str_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_c){
            .buf = "",
            .len = 0,
        };
    }

    str_c result = (str_c){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    return result;
}

str_c
str_rstrip(str_c s)
{
    if (s.buf == NULL) {
        return (str_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_c){
            .buf = "",
            .len = 0,
        };
    }

    str_c result = (str_c){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_right(&result);
    return result;
}

str_c
str_strip(str_c s)
{
    if (s.buf == NULL) {
        return (str_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_c){
            .buf = "",
            .len = 0,
        };
    }

    str_c result = (str_c){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    str__strip_right(&result);
    return result;
}

int
str_cmp(str_c self, str_c other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    size_t min_len = MIN(self.len, other.len);
    int cmp = memcmp(self.buf, other.buf, min_len);

    if (cmp == 0 && self.len != other.len) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

int
str_cmpi(str_c self, str_c other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    size_t min_len = MIN(self.len, other.len);

    int cmp = 0;
    char* s = self.buf;
    char* o = other.buf;
    for (size_t i = 0; i < min_len; i++) {
        cmp = tolower(*s) - tolower(*o);
        s++;
        o++;
    }

    if (cmp == 0 && self.len != other.len) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

str_c*
str_iter_split(str_c s, const char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
        size_t split_by_len;
        str_c str;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) <= alignof(size_t), "cex_iterator_s _ctx misalign");

    if (unlikely(iterator->val == NULL)) {
        // First run handling
        if (unlikely(!str__isvalid(&s))) {
            return NULL;
        }
        ctx->split_by_len = strlen(split_by);

        if (ctx->split_by_len == 0) {
            return NULL;
        }
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        ssize_t idx = str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) {
            idx = s.len;
        }
        ctx->cursor = idx;
        ctx->str = str.sub(s, 0, idx);

        iterator->val = &ctx->str;
        iterator->idx.i = 0;
        return iterator->val;
    } else {
        uassert(iterator->val == &ctx->str);

        if (ctx->cursor >= s.len) {
            return NULL; // reached the end stops
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == s.len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            iterator->idx.i++;
            ctx->str = (str_c){ .buf = "", .len = 0 };
            return iterator->val;
        }

        // Get remaining string after prev split_by char
        str_c tok = str.sub(s, ctx->cursor, 0);
        ssize_t idx = str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->str = tok;
            ctx->cursor = s.len;
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->str = str.sub(tok, 0, idx);
            ctx->cursor += idx;
        }

        return iterator->val;
    }
}


Exception
str__to_signed_num(str_c self, i64* num, i64 num_min, i64 num_max)
{
    _Static_assert(sizeof(i64) == 8, "unexpected u64 size");
    uassert(num_min < num_max);
    uassert(num_min <= 0);
    uassert(num_max > 0);
    uassert(num_max > 64);
    uassert(num_min == 0 || num_min < -64);
    uassert(num_min >= INT64_MIN + 1 && "try num_min+1, negation overflow");

    if (unlikely(self.len == 0)) {
        return Error.argument;
    }

    char* s = self.buf;
    size_t len = self.len;
    size_t i = 0;

    for (; s[i] == ' ' && i < self.len; i++) {
    }

    u64 neg = 1;
    if (s[i] == '-') {
        neg = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;
        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = (u64)(neg == 1 ? (u64)num_max : (u64)-num_min);
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) {
            return Error.argument;
        }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = (i64)acc * neg;

    return Error.ok;
}

Exception
str__to_unsigned_num(str_c self, u64* num, u64 num_max)
{
    _Static_assert(sizeof(u64) == 8, "unexpected u64 size");
    uassert(num_max > 0);
    uassert(num_max > 64);

    if (unlikely(self.len == 0)) {
        return Error.argument;
    }

    char* s = self.buf;
    size_t len = self.len;
    size_t i = 0;

    for (; s[i] == ' ' && i < self.len; i++) {
    }

    if (s[i] == '-') {
        return Error.argument;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;

        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = num_max;
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) {
            return Error.argument;
        }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = (i64)acc;

    return Error.ok;
}

Exception
str__to_double(str_c self, double* num, i32 exp_min, i32 exp_max)
{
    _Static_assert(sizeof(double) == 8, "unexpected double precision");
    if (unlikely(self.len == 0)) {
        return Error.argument;
    }

    char* s = self.buf;
    size_t len = self.len;
    size_t i = 0;
    double number = 0.0;

    for (; s[i] == ' ' && i < self.len; i++) {
    }

    double sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if (unlikely(s[i] == 'n' || s[i] == 'i' || s[i] == 'N' || s[i] == 'I')) {
        if (unlikely(len - i < 3)) {
            return Error.argument;
        }
        if (s[i] == 'n' || s[i] == 'N') {
            if ((s[i + 1] == 'a' || s[i + 1] == 'A') && (s[i + 2] == 'n' || s[i + 2] == 'N')) {
                number = NAN;
                i += 3;
            }
        } else {
            // s[i] = 'i'
            if ((s[i + 1] == 'n' || s[i + 1] == 'N') && (s[i + 2] == 'f' || s[i + 2] == 'F')) {
                number = (double)INFINITY * sign;
                i += 3;
            }
            // INF 'INITY' part (optional but still valid)
            // clang-format off
            if (unlikely(len - i >= 5)) {
                if ((s[i + 0] == 'i' || s[i + 0] == 'I') && 
                    (s[i + 1] == 'n' || s[i + 1] == 'N') &&
                    (s[i + 2] == 'i' || s[i + 2] == 'I') &&
                    (s[i + 3] == 't' || s[i + 3] == 'T') &&
                    (s[i + 4] == 'y' || s[i + 4] == 'Y')) {
                    i += 5;
                }
            }
            // clang-format on
        }

        // Allow trailing spaces, but no other character allowed
        for (; i < len; i++) {
            if (s[i] != ' ') {
                return Error.argument;
            }
        }

        *num = number;
        return Error.ok;
    }

    i32 exponent = 0;
    u32 num_decimals = 0;
    u32 num_digits = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c == 'e' || c == 'E' || c == '.') {
            break;
        } else {
            return Error.argument;
        }

        number = number * 10. + c;
        num_digits++;
    }
    // Process decimal part
    if (i < len && s[i] == '.') {
        i++;

        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }
            number = number * 10. + c;
            num_decimals++;
            num_digits++;
        }
        exponent -= num_decimals;
    }


    number *= sign;

    if (i < len - 1 && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        sign = 1;
        if (s[i] == '-') {
            sign = -1;
            i++;
        } else if (s[i] == '+') {
            i++;
        }

        u32 n = 0;
        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }

            n = n * 10 + c;
        }

        exponent += n * sign;
    }

    if (num_digits == 0) {
        return Error.argument;
    }

    if (exponent < exp_min || exponent > exp_max) {
        return Error.overflow;
    }

    // Scale the result
    double p10 = 10.;
    i32 n = exponent;
    if (n < 0) {
        n = -n;
    }
    while (n) {
        if (n & 1) {
            if (exponent < 0) {
                number /= p10;
            } else {
                number *= p10;
            }
        }
        n >>= 1;
        p10 *= p10;
    }

    if (number == HUGE_VAL) {
        return Error.overflow;
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = number;

    return Error.ok;
}

Exception str_to_f32(str_c self, f32* num){
    f64 res = 0;
    Exc r = str__to_double(self, &res, -37, 38);
    *num = (f32)res;
    return r;
}

Exception str_to_f64(str_c self, f64* num){
    return str__to_double(self, num, -307, 308);
}

Exception
str_to_i8(str_c self, i8* num)
{
    i64 res = 0;
    Exc r = str__to_signed_num(self, &res, INT8_MIN, INT8_MAX);
    *num = res;
    return r;
}

Exception
str_to_i16(str_c self, i16* num)
{
    i64 res = 0;
    var r = str__to_signed_num(self, &res, INT16_MIN, INT16_MAX);
    *num = res;
    return r;
}

Exception
str_to_i32(str_c self, i32* num)
{
    i64 res = 0;
    var r = str__to_signed_num(self, &res, INT32_MIN, INT32_MAX);
    *num = res;
    return r;
}


Exception
str_to_i64(str_c self, i64* num)
{
    i64 res = 0;
    // NOTE:INT64_MIN+1 because negating of INT64_MIN leads to UB!
    var r = str__to_signed_num(self, &res, INT64_MIN + 1, INT64_MAX);
    *num = res;
    return r;
}

Exception
str_to_u8(str_c self, u8* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT8_MAX);
    *num = res;
    return r;
}

Exception
str_to_u16(str_c self, u16* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT16_MAX);
    *num = res;
    return r;
}

Exception
str_to_u32(str_c self, u32* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT32_MAX);
    *num = res;
    return r;
}

Exception
str_to_u64(str_c self, u64* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT64_MAX);
    *num = res;

    return r;
}
const struct __module__str str = {
    // Autogenerated by CEX
    // clang-format off
    .cstr = str_cstr,
    .cbuf = str_cbuf,
    .sub = str_sub,
    .copy = str_copy,
    .sprintf = str_sprintf,
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
    .to_f32 = str_to_f32,
    .to_f64 = str_to_f64,
    .to_i8 = str_to_i8,
    .to_i16 = str_to_i16,
    .to_i32 = str_to_i32,
    .to_i64 = str_to_i64,
    .to_u8 = str_to_u8,
    .to_u16 = str_to_u16,
    .to_u32 = str_to_u32,
    .to_u64 = str_to_u64,
    // clang-format on
};