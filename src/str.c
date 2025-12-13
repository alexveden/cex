#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_str)
#    include "_sprintf.h"
#    include "all.h"

static inline int
_cex_str__toupper(int c)
{
    if ((unsigned)c - 'a' < 26) { return c & 0x5f; }
    return c;
}

static inline int
_cex_str__tolower(int c)
{
    if ((unsigned)c - 'A' < 26) { return c | 0x20; }
    return c;
}

static inline bool
_cex_str__isvalid(str_s* s)
{
    return s->buf != NULL;
}

static inline isize
_cex_str__index(str_s* s, char* c, u8 clen)
{
    isize result = -1;

    if (!_cex_str__isvalid(s)) { return -1; }

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) { split_by_idx[(u8)c[i]] = 1; }

    for (usize i = 0; i < s->len; i++) {
        if (split_by_idx[(u8)s->buf[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

/// Creates string slice of input C string (NULL tolerant, (str_s){0} on error)
static str_s
cex_str_sstr(char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) { return (str_s){ 0 }; }

    return (str_s){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}

/// Creates string slice from a buf+len
static str_s
cex_str_sbuf(char* s, usize length)
{
    if (unlikely(s == NULL)) { return (str_s){ 0 }; }

    return (str_s){
        .buf = s,
        .len = strnlen(s, length),
    };
}

/// Compares two null-terminated strings (null tolerant)
static bool
cex_str_eq(char* a, char* b)
{
    if (unlikely(a == NULL || b == NULL)) { return a == b; }
    return strcmp(a, b) == 0;
}

/// Compares two strings, case insensitive, null tolerant
bool
cex_str_eqi(char* a, char* b)
{
    if (unlikely(a == NULL || b == NULL)) { return a == b; }
    while (*a && *b) {
        if (_cex_str__tolower((u8)*a) != _cex_str__tolower((u8)*b)) { return false; }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

/// Compares two string slices, null tolerant
static bool
cex_str__slice__eq(str_s a, str_s b)
{
    if (a.len != b.len) { return false; }
    return str.slice.qscmp(&a, &b) == 0;
}

/// Compares two string slices, null tolerant, case insensitive
static bool
cex_str__slice__eqi(str_s a, str_s b)
{
    if (a.len != b.len) { return false; }
    return str.slice.qscmpi(&a, &b) == 0;
}

/// Makes slices of `s` slice, start/end are indexes, can be negative from the end, if end=0 mean
/// full length of the string. `s` may be not null-terminated. function is NULL tolerant, return
/// (str_s){0} on error
static str_s
cex_str__slice__sub(str_s s, isize start, isize end)
{
    str_s result = { 0 };

    if (s.buf != NULL) {
        isize _len = s.len;
        if (unlikely(start < 0)) { start += _len; }
        if (end == 0) { /* _end=0 equivalent infinity */
            end = _len;
        } else if (unlikely(end < 0)) {
            end += _len;
        }
        end = end < _len ? end : _len;
        start = start > 0 ? start : 0;

        if (start < end) {
            result.buf = &(s.buf[start]);
            result.len = (usize)(end - start);
        }
    }

    return result;
}

/// Makes slices of `s` char* string, start/end are indexes, can be negative from the end, if end=0
/// mean full length of the string. `s` may be not null-terminated. function is NULL tolerant,
/// return (str_s){0} on error
static str_s
cex_str_sub(char* s, isize start, isize end)
{
    str_s slice = cex_str_sstr(s);
    return cex_str__slice__sub(slice, start, end);
}

/// Makes a copy of initial `src`, into `dest` buffer constrained by `destlen`. NULL tolerant,
/// always null-terminated, overflow checked.
static Exception
cex_str_copy(char* dest, char* src, usize destlen)
{
    uassert(dest != src && "buffers overlap");
    if (unlikely(dest == NULL || destlen == 0)) { return Error.argument; }
    dest[0] = '\0'; // If we fail next, it still writes empty string
    if (unlikely(src == NULL)) { return Error.argument; }

    char* d = dest;
    char* s = src;
    size_t n = destlen;

    while (--n != 0) {
        if (unlikely((*d = *s) == '\0')) { break; }
        d++;
        s++;
    }
    *d = '\0'; // always terminate

    if (unlikely(*s != '\0')) { return Error.overflow; }

    return Error.ok;
}

/// Replaces substring occurrence in a string
char*
cex_str_replace(char* s, char* old_sub, char* new_sub, IAllocator allc)
{
    if (s == NULL || old_sub == NULL || new_sub == NULL || old_sub[0] == '\0') { return NULL; }
    size_t str_len = strlen(s);
    size_t old_sub_len = strlen(old_sub);
    size_t new_sub_len = strlen(new_sub);

    size_t count = 0;
    char* pos = s;
    while ((pos = strstr(pos, old_sub)) != NULL) {
        count++;
        pos += old_sub_len;
    }
    size_t new_str_len = str_len + count * (new_sub_len - old_sub_len);
    char* new_str = (char*)mem$malloc(allc, new_str_len + 1); // +1 for the null terminator
    if (new_str == NULL) {
        return NULL; // Memory allocation failed
    }
    new_str[0] = '\0';

    char* current_pos = new_str;
    char* start = s;
    while (count--) {
        char* found = strstr(start, old_sub);
        size_t segment_len = found - start;
        memcpy(current_pos, start, segment_len);
        current_pos += segment_len;
        memcpy(current_pos, new_sub, new_sub_len);
        current_pos += new_sub_len;
        start = found + old_sub_len;
    }
    // NOLINTNEXTLINE
    strcpy(current_pos, start);
    new_str[new_str_len] = '\0';
    return new_str;
}

/// Makes a copy of initial `src` slice, into `dest` buffer constrained by `destlen`. NULL tolerant,
/// always null-terminated, overflow checked.
static Exception
cex_str__slice__copy(char* dest, str_s src, usize destlen)
{
    uassert(dest != src.buf && "buffers overlap");
    if (unlikely(dest == NULL || destlen == 0)) { return Error.argument; }
    dest[0] = '\0';
    if (unlikely(src.buf == NULL)) { return Error.argument; }
    if (src.len >= destlen) { return Error.overflow; }
    memcpy(dest, src.buf, src.len);
    dest[src.len] = '\0';
    dest[destlen - 1] = '\0';
    return Error.ok;
}

/// Analog of vsprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
static Exception
cex_str_vsprintf(char* dest, usize dest_len, char* format, va_list va)
{
    if (unlikely(dest == NULL)) { return Error.argument; }
    if (unlikely(dest_len == 0)) { return Error.argument; }
    uassert(format != NULL);

    dest[dest_len - 1] = '\0'; // always null term at capacity

    int result = cexsp__vsnprintf(dest, dest_len, format, va);

    if (result < 0 || (usize)result >= dest_len) {
        // NOTE: even with overflow, data is truncated and written to the dest + null term
        return Error.overflow;
    }

    return EOK;
}

/// Analog of sprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
static Exc
cex_str_sprintf(char* dest, usize dest_len, char* format, ...)
{
    va_list va;
    va_start(va, format);
    Exc result = cex_str_vsprintf(dest, dest_len, format, va);
    va_end(va);
    return result;
}


/// Calculates string length, NULL tolerant.
static usize
cex_str_len(char* s)
{
    if (s == NULL) { return 0; }
    return strlen(s);
}

/// Find a substring in a string, returns pointer to first element. NULL tolerant, and NULL on err.
static char*
cex_str_find(char* haystack, char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) { return NULL; }
    return strstr(haystack, needle);
}

/// Find substring from the end , NULL tolerant, returns NULL on error.
char*
cex_str_findr(char* haystack, char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) { return NULL; }
    usize haystack_len = strlen(haystack);
    usize needle_len = strlen(needle);
    if (unlikely(needle_len > haystack_len)) { return NULL; }
    for (char* ptr = haystack + haystack_len - needle_len; ptr >= haystack; ptr--) {
        if (unlikely(strncmp(ptr, needle, needle_len) == 0)) {
            uassert(ptr >= haystack);
            uassert(ptr <= haystack + haystack_len);
            return (char*)ptr;
        }
    }
    return NULL;
}

/// Get index of first occurrence of `needle`, returns -1 on error.
static isize
cex_str__slice__index_of(str_s s, str_s needle)
{
    if (unlikely(!s.buf || !needle.buf || needle.len == 0 || needle.len > s.len)) { return -1; }
    if (unlikely(needle.len == 1)) {
        char n = needle.buf[0];
        for (usize i = 0; i < s.len; i++) {
            if (s.buf[i] == n) { return i; }
        }
    } else {
        for (usize i = 0; i <= s.len - needle.len; i++) {
            if (memcmp(&s.buf[i], needle.buf, needle.len) == 0) { return i; }
        }
    }
    return -1;
}

/// Checks if slice starts with prefix, returns (str_s){0} on error, NULL tolerant
static bool
cex_str__slice__starts_with(str_s s, str_s prefix)
{
    if (unlikely(!s.buf || !prefix.buf || prefix.len == 0 || prefix.len > s.len)) { return false; }
    return memcmp(s.buf, prefix.buf, prefix.len) == 0;
}

/// Checks if slice ends with prefix, returns (str_s){0} on error, NULL tolerant
static bool
cex_str__slice__ends_with(str_s s, str_s suffix)
{
    if (unlikely(!s.buf || !suffix.buf || suffix.len == 0 || suffix.len > s.len)) { return false; }
    return s.len >= suffix.len && !memcmp(s.buf + s.len - suffix.len, suffix.buf, suffix.len);
}

/// Checks if string starts with prefix, returns false on error, NULL tolerant
static bool
cex_str_starts_with(char* s, char* prefix)
{
    if (s == NULL || prefix == NULL || prefix[0] == '\0') { return false; }

    while (*prefix && *s == *prefix) { ++s, ++prefix; }
    return *prefix == 0;
}

/// Checks if string ends with prefix, returns false on error, NULL tolerant
static bool
cex_str_ends_with(char* s, char* suffix)
{
    if (s == NULL || suffix == NULL || suffix[0] == '\0') { return false; }
    size_t slen = strlen(s);
    size_t sufflen = strlen(suffix);

    return slen >= sufflen && !memcmp(s + slen - sufflen, suffix, sufflen);
}

/// Replaces slice prefix (start part), or returns the same slice if it's not found
static str_s
cex_str__slice__remove_prefix(str_s s, str_s prefix)
{
    if (!cex_str__slice__starts_with(s, prefix)) { return s; }

    return (str_s){
        .buf = s.buf + prefix.len,
        .len = s.len - prefix.len,
    };
}

/// Replaces slice suffix (end part), or returns the same slice if it's not found
static str_s
cex_str__slice__remove_suffix(str_s s, str_s suffix)
{
    if (!cex_str__slice__ends_with(s, suffix)) { return s; }
    return (str_s){
        .buf = s.buf,
        .len = s.len - suffix.len,
    };
}

static inline void
cex_str__strip_left(str_s* s)
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
cex_str__strip_right(str_s* s)
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


/// Removes white spaces from the beginning of slice
static str_s
cex_str__slice__lstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    cex_str__strip_left(&result);
    return result;
}

/// Removes white spaces from the end of slice
static str_s
cex_str__slice__rstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    cex_str__strip_right(&result);
    return result;
}


/// Removes white spaces from both ends of slice
static str_s
cex_str__slice__strip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    cex_str__strip_left(&result);
    cex_str__strip_right(&result);
    return result;
}

/// libc `qsort()` comparator function for alphabetical sorting of str_s arrays
static int
cex_str__slice__qscmp(const void* a, const void* b)
{
    str_s self = *(str_s*)a;
    str_s other = *(str_s*)b;
    if (unlikely(self.buf == NULL || other.buf == NULL)) {
        return (self.buf < other.buf) - (self.buf > other.buf);
    }

    usize min_len = self.len < other.len ? self.len : other.len;
    int cmp = memcmp(self.buf, other.buf, min_len);

    if (unlikely(cmp == 0 && self.len != other.len)) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

/// libc `qsort()` comparator function for alphabetical case insensitive sorting of str_s arrays
static int
cex_str__slice__qscmpi(const void* a, const void* b)
{
    str_s self = *(str_s*)a;
    str_s other = *(str_s*)b;

    if (unlikely(self.buf == NULL || other.buf == NULL)) {
        return (self.buf < other.buf) - (self.buf > other.buf);
    }

    usize min_len = self.len < other.len ? self.len : other.len;

    int cmp = 0;
    char* s = self.buf;
    char* o = other.buf;
    for (usize i = 0; i < min_len; i++) {
        cmp = _cex_str__tolower(*s) - _cex_str__tolower(*o);
        if (cmp != 0) { return cmp; }
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

/// iterator over slice splits:  for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {}
static str_s
cex_str__slice__iter_split(str_s s, char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        usize cursor;
        usize split_by_len;
        usize str_len;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    static_assert(alignof(struct iter_ctx) <= alignof(usize), "cex_iterator_s _ctx misalign");

    // NOLINTNEXTLINE
    if (unlikely(!iterator->initialized)) {
        iterator->initialized = 1;
        // First run handling
        if (unlikely(!_cex_str__isvalid(&s) || s.len == 0)) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->split_by_len = strlen(split_by);
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        if (ctx->split_by_len == 0) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }

        isize idx = _cex_str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) { idx = s.len; }
        ctx->cursor = idx;
        ctx->str_len = s.len; // this prevents s being changed in a loop
        iterator->idx.i = 0;
        if (idx == 0) {
            // first line is \n
            return (str_s){ .buf = "", .len = 0 };
        } else {
            return str.slice.sub(s, 0, idx);
        }
    } else {
        if (unlikely(ctx->cursor >= ctx->str_len)) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == ctx->str_len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            iterator->idx.i++;
            return (str_s){ .buf = "", .len = 0 };
        }

        // Get remaining string after prev split_by char
        str_s tok = str.slice.sub(s, ctx->cursor, 0);
        isize idx = _cex_str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->cursor = s.len;
            // iterator->stopped = 1;
            return tok;
        } else if (idx == 0) {
            return (str_s){ .buf = "", .len = 0 };
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->cursor += idx;
            return str.slice.sub(tok, 0, idx);
        }
    }
}


static Exception
cex_str__to_signed_num(char* self, usize len, i64* num, i64 num_min, i64 num_max)
{
    static_assert(sizeof(i64) == 8, "unexpected u64 size");
    uassert(num_min < num_max);
    uassert(num_min <= 0);
    uassert(num_max > 0);
    uassert(num_max > 64);
    uassert(num_min == 0 || num_min < -64);
    uassert(num_min >= INT64_MIN + 1 && "try num_min+1, negation overflow");

    if (unlikely(self == NULL)) { return Error.argument; }
    if (unlikely(len > 32)) { return Error.argument; }

    char* s = self;
    if (len == 0) { len = strlen(self); }
    usize i = 0;

    for (; i < len && s[i] == ' '; i++) {}
    if (unlikely(i >= len)) { return Error.argument; }

    u64 neg = 1;
    if (s[i] == '-') {
        neg = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) { return Error.argument; }

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

        if (unlikely(c >= base)) { return Error.argument; }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') { return Error.argument; }
    }

    *num = (i64)acc * neg;

    return Error.ok;
}

static Exception
cex_str__to_unsigned_num(char* s, usize len, u64* num, u64 num_max)
{
    static_assert(sizeof(u64) == 8, "unexpected u64 size");
    uassert(num_max > 0);
    uassert(num_max > 64);

    if (unlikely(s == NULL)) { return Error.argument; }

    if (len == 0) { len = strlen(s); }
    if (unlikely(len > 32)) { return Error.argument; }
    usize i = 0;

    for (; i < len && s[i] == ' '; i++) {}
    if (unlikely(i >= len)) { return Error.argument; }


    if (s[i] == '-') {
        return Error.argument;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) { return Error.argument; }

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

        if (unlikely(c >= base)) { return Error.argument; }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') { return Error.argument; }
    }

    *num = (i64)acc;

    return Error.ok;
}

static Exception
cex_str__to_double(char* self, usize len, double* num, i32 exp_min, i32 exp_max)
{
    static_assert(sizeof(double) == 8, "unexpected double precision");
    if (unlikely(self == NULL)) { return Error.argument; }

    char* s = self;
    if (len == 0) { len = strlen(s); }
    if (unlikely(len > 64)) { return Error.argument; }

    usize i = 0;
    double number = 0.0;

    for (; i < len && s[i] == ' '; i++) {}
    if (unlikely(i >= len)) { return Error.argument; }

    double sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }

    if (unlikely(i >= len)) { return Error.argument; }

    if (unlikely(s[i] == 'n' || s[i] == 'i' || s[i] == 'N' || s[i] == 'I')) {
        if (unlikely(len - i < 3)) { return Error.argument; }
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
            if (s[i] != ' ') { return Error.argument; }
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
        if (unlikely(n > INT32_MAX)) { return Error.overflow; }

        exponent += n * sign;
    }

    if (num_digits == 0) { return Error.argument; }

    if (unlikely(exponent < exp_min || exponent > exp_max)) { return Error.overflow; }

    // Scale the result
    double p10 = 10.;
    i32 n = exponent;
    if (n < 0) { n = -n; }
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

    if (number == HUGE_VAL) { return Error.overflow; }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') { return Error.argument; }
    }

    *num = number;

    return Error.ok;
}

static Exception
cex_str__convert__to_bools(str_s s, bool* num)
{
    if (unlikely(!num)) { return Error.argument; }
    if (str$eq(s, "true")) {
        *num = true;
    } else if (str$eq(s, "false")) {
        *num = false;
    } else {
        return Error.argument;
    }
    return EOK;
}

static Exception
cex_str__convert__to_f32s(str_s s, f32* num)
{
    if (unlikely(!num)) { return Error.argument; }
    f64 res = 0;
    Exc r = cex_str__to_double(s.buf, s.len, &res, -37, 38);
    *num = (f32)res;
    return r;
}

static Exception
cex_str__convert__to_f64s(str_s s, f64* num)
{
    if (unlikely(!num)) { return Error.argument; }
    return cex_str__to_double(s.buf, s.len, num, -307, 308);
}

static Exception
cex_str__convert__to_i8s(str_s s, i8* num)
{
    if (unlikely(!num)) { return Error.argument; }
    i64 res = 0;
    Exc r = cex_str__to_signed_num(s.buf, s.len, &res, INT8_MIN, INT8_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_i16s(str_s s, i16* num)
{
    if (unlikely(!num)) { return Error.argument; }
    i64 res = 0;
    auto r = cex_str__to_signed_num(s.buf, s.len, &res, INT16_MIN, INT16_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_i32s(str_s s, i32* num)
{
    if (unlikely(!num)) { return Error.argument; }
    i64 res = 0;
    auto r = cex_str__to_signed_num(s.buf, s.len, &res, INT32_MIN, INT32_MAX);
    *num = res;
    return r;
}


static Exception
cex_str__convert__to_i64s(str_s s, i64* num)
{
    if (unlikely(!num)) { return Error.argument; }
    i64 res = 0;
    // NOTE:INT64_MIN+1 because negating of INT64_MIN leads to UB!
    auto r = cex_str__to_signed_num(s.buf, s.len, &res, INT64_MIN + 1, INT64_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u8s(str_s s, u8* num)
{
    if (unlikely(!num)) { return Error.argument; }
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT8_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u16s(str_s s, u16* num)
{
    if (unlikely(!num)) { return Error.argument; }
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT16_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u32s(str_s s, u32* num)
{
    if (unlikely(!num)) { return Error.argument; }
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT32_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u64s(str_s s, u64* num)
{
    if (unlikely(!num)) { return Error.argument; }
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT64_MAX);
    *num = res;

    return r;
}

static Exception
cex_str__convert__to_bool(char* s, bool* num)
{
    return cex_str__convert__to_bools(str.sstr(s), num);
}

static Exception
cex_str__convert__to_f32(char* s, f32* num)
{
    return cex_str__convert__to_f32s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_f64(char* s, f64* num)
{
    return cex_str__convert__to_f64s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_i8(char* s, i8* num)
{
    return cex_str__convert__to_i8s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_i16(char* s, i16* num)
{
    return cex_str__convert__to_i16s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_i32(char* s, i32* num)
{
    return cex_str__convert__to_i32s(str.sstr(s), num);
}


static Exception
cex_str__convert__to_i64(char* s, i64* num)
{
    return cex_str__convert__to_i64s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u8(char* s, u8* num)
{
    return cex_str__convert__to_u8s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u16(char* s, u16* num)
{
    return cex_str__convert__to_u16s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u32(char* s, u32* num)
{
    return cex_str__convert__to_u32s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u64(char* s, u64* num)
{
    return cex_str__convert__to_u64s(str.sstr(s), num);
}


static char*
_cex_str__fmt_callback(char* buf, void* user, u32 len)
{
    (void)buf;
    cexsp__context* ctx = user;
    if (unlikely(ctx->has_error)) { return NULL; }

    if (unlikely(
            len >= CEX_SPRINTF_MIN && (ctx->buf == NULL || ctx->length + len >= ctx->capacity)
        )) {

        if (len > INT32_MAX || ctx->length + len > INT32_MAX) {
            ctx->has_error = true;
            return NULL;
        }

        uassert(ctx->allc != NULL);

        if (ctx->buf == NULL) {
            ctx->buf = mem$calloc(ctx->allc, 1, CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity = CEX_SPRINTF_MIN * 2;
        } else {
            ctx->buf = mem$realloc(ctx->allc, ctx->buf, ctx->capacity + CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity += CEX_SPRINTF_MIN * 2;
        }
    }
    ctx->length += len;

    // fprintf(stderr, "len: %d, total_len: %d capacity: %d\n", len, ctx->length, ctx->capacity);
    if (len > 0) {
        if (ctx->buf) {
            if (buf == ctx->tmp) {
                uassert(ctx->length <= CEX_SPRINTF_MIN);
                memcpy(ctx->buf, buf, len);
            }
        }
    }


    return (ctx->buf != NULL) ? &ctx->buf[ctx->length] : ctx->tmp;
}

/// Formats string and allocates it dynamically using allocator, supports CEX format engine
static char*
cex_str_fmt(IAllocator allc, char* format, ...)
{
    va_list va;
    va_start(va, format);

    cexsp__context ctx = {
        .allc = allc,
    };
    // TODO: add optional flag, to check if any va is null?
    cexsp__vsprintfcb(_cex_str__fmt_callback, &ctx, ctx.tmp, format, va);
    va_end(va);

    if (unlikely(ctx.has_error)) {
        mem$free(allc, ctx.buf);
        return NULL;
    }

    if (ctx.buf) {
        uassert(ctx.length < ctx.capacity);
        ctx.buf[ctx.length] = '\0';
        ctx.buf[ctx.capacity - 1] = '\0';
    } else {
        uassert(ctx.length <= arr$len(ctx.tmp) - 1);
        ctx.buf = mem$malloc(allc, ctx.length + 1);
        if (ctx.buf == NULL) { return NULL; }
        memcpy(ctx.buf, ctx.tmp, ctx.length);
        ctx.buf[ctx.length] = '\0';
    }
    return ctx.buf;
}

/// Clone slice into new char* allocated by `allc`, null tolerant, returns NULL on error.
static char*
cex_str__slice__clone(str_s s, IAllocator allc)
{
    if (s.buf == NULL) { return NULL; }
    char* result = mem$malloc(allc, s.len + 1);
    if (result) {
        memcpy(result, s.buf, s.len);
        result[s.len] = '\0';
    }
    return result;
}

/// Clones string using allocator, null tolerant, returns NULL on error.
static char*
cex_str_clone(char* s, IAllocator allc)
{
    if (s == NULL) { return NULL; }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        memcpy(result, s, slen);
        result[slen] = '\0';
    }
    return result;
}

/// Returns new lower case string, returns NULL on error, null tolerant
static char*
cex_str_lower(char* s, IAllocator allc)
{
    if (s == NULL) { return NULL; }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        for (usize i = 0; i < slen; i++) { result[i] = _cex_str__tolower(s[i]); }
        result[slen] = '\0';
    }
    return result;
}

/// Returns new upper case string, returns NULL on error, null tolerant
static char*
cex_str_upper(char* s, IAllocator allc)
{
    if (s == NULL) { return NULL; }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        for (usize i = 0; i < slen; i++) { result[i] = _cex_str__toupper(s[i]); }
        result[slen] = '\0';
    }
    return result;
}

/// Splits string using split_by (allows many) chars, returns new dynamic array of split char*
/// tokens, allocates memory with allc, returns NULL on error. NULL tolerant. Items of array are
/// cloned, so you need free them independently or better use arena or tmem$.
static arr$(char*) cex_str_split(char* s, char* split_by, IAllocator allc)
{
    str_s src = cex_str_sstr(s);
    if (src.buf == NULL || split_by == NULL) { return NULL; }
    arr$(char*) result = arr$new(result, allc);
    if (result == NULL) { return NULL; }

    for$iter (str_s, it, cex_str__slice__iter_split(src, split_by, &it.iterator)) {
        char* tok = cex_str__slice__clone(it.val, allc);
        arr$push(result, tok);
    }

    return result;
}

/// Splits string by lines, result allocated by allc, as dynamic array of cloned lines, Returns NULL
/// on error, NULL tolerant. Items of array are cloned, so you need free them independently or
/// better use arena or tmem$. Supports \n or \r\n.
static arr$(char*) cex_str_split_lines(char* s, IAllocator allc)
{
    uassert(allc != NULL);
    if (s == NULL) { return NULL; }
    arr$(char*) result = arr$new(result, allc);
    if (result == NULL) { return NULL; }
    char c;
    char* line_start = s;
    char* cur = s;
    while ((c = *cur)) {
        switch (c) {
            case '\r':
                if (cur[1] == '\n') { goto default_next; }
                fallthrough();
            case '\n':
            case '\v':
            case '\f': {
                str_s line = { .buf = (char*)line_start, .len = cur - line_start };
                if (line.len > 0 && line.buf[line.len - 1] == '\r') { line.len--; }
                char* tok = cex_str__slice__clone(line, allc);
                arr$push(result, tok);
                line_start = cur + 1;
            }
                fallthrough();
            default:
            default_next:
                cur++;
        }
    }
    if (line_start <= cur) {
        str_s line = { .buf = (char*)line_start, .len = cur - line_start };
        if (line.len > 0) {
            char* tok = cex_str__slice__clone(line, allc);
            arr$push(result, tok);
        }
    }
    return result;
}

/// Joins string using a separator (join_by), NULL tolerant, returns NULL on error.
static char*
cex_str_join(char** str_arr, usize str_arr_len, char* join_by, IAllocator allc)
{
    if (str_arr == NULL || join_by == NULL) { return NULL; }

    usize jlen = strlen(join_by);
    if (jlen == 0) { return NULL; }

    char* result = NULL;
    usize cursor = 0;
    for$each (s, str_arr, str_arr_len) {
        if (s == NULL) {
            if (result != NULL) { mem$free(allc, result); }
            return NULL;
        }
        usize slen = strlen(s);
        if (result == NULL) {
            result = mem$malloc(allc, slen + jlen + 1);
        } else {
            result = mem$realloc(allc, result, cursor + slen + jlen + 1);
        }
        if (result == NULL) {
            return NULL; // memory error
        }

        if (cursor > 0) {
            memcpy(&result[cursor], join_by, jlen);
            cursor += jlen;
        }

        memcpy(&result[cursor], s, slen);
        cursor += slen;
    }
    if (result) { result[cursor] = '\0'; }

    return result;
}


static bool
_cex_str_match(char* str, isize str_len, char* pattern, isize* matched_till_star)
{
    if (unlikely(str == NULL || str_len <= 0)) { return false; }

    uassert(pattern && "null pattern");
    isize start_str_len = str_len;

main_loop_again:
    while (*pattern != '\0') {
        switch (*pattern) {
            case '*':
                if (matched_till_star) {
                    // NOTE: this case only for handling recursive calls (see below in this scope)
                    *matched_till_star = start_str_len - str_len;
                    return true;
                }
                while (*pattern == '*' || *pattern == '?') {
                    if (unlikely(str_len > 0 && *pattern == '?')) {
                        str++;
                        str_len--;
                    }
                    pattern++;
                }

                if (!*pattern) { return true; }

                if (*pattern != '?' && *pattern != '[' && *pattern != '(' && *pattern != '\\') {
                    while (str_len > 0 && *pattern != *str) {
                        str++;
                        str_len--;
                    }
                }

                while (str_len > 0) {
                    isize n_matched = 0;
                    // NOTE: recursive calls when we need pattern match next
                    //  this prevents recursive call explosion when we have * in the pattern later
                    if (unlikely(_cex_str_match(str, str_len, pattern, &n_matched))) {
                        if (n_matched > 0) {
                            str += n_matched;
                            str_len -= n_matched;
                            while (*pattern != '\0' && *pattern != '*') {
                                if (*pattern == '\\') {
                                    if (pattern[1]) { pattern++; }
                                }
                                pattern++;
                            }
                            goto main_loop_again;
                        }
                        return true;
                    }
                    str++;
                    str_len--;
                }
                return false;

            case '?':
                if (str_len == 0) {
                    return false; // '?' requires a character
                }
                str++;
                str_len--;
                pattern++;
                break;

            case '(': {
                char* strstart = str;
                isize str_len_start = str_len;
                if (unlikely(*(pattern + 1) == ')')) {
                    uassertf(false, "Empty '()' group");
                    return false;
                }
                if (unlikely(str_len_start) == 0) { return false; }

                while (str_len_start > 0) {
                    pattern++;
                    str = strstart;
                    str_len = str_len_start;
                    bool matched = false;
                    while (*pattern != '\0') {
                        if (unlikely(*pattern == '\\')) {
                            // Escaped symbol, can be anything
                            pattern++;
                            if (unlikely(*pattern == '\0')) {
                                uassertf(false, "Unterminated \\ sequence inside '()' group");
                                return false;
                            }
                            if (str_len > 0 && *pattern == *str) { matched = true; }
                            goto next;
                        }

                        if (str_len > 0 && *pattern != '|' && *pattern != ')' && *pattern == *str) {
                            matched = true;
                        } else {
                            while (*pattern != '|' && *pattern != ')' && *pattern != '\0') {
                                matched = false;
                                pattern++;
                            }
                            break;
                        }

                    next:
                        pattern++;
                        str++;
                        str_len--;
                    }
                    if (*pattern == '|') {
                        if (!matched) { continue; }
                        // we have found the match, just skip to the end of group
                        while (*pattern != ')' && *pattern != '\0') { pattern++; }
                    }

                    if (unlikely(*pattern != ')')) {
                        uassertf(false, "Invalid pattern - no closing ')'");
                        return false;
                    }

                    pattern++;
                    if (!matched) {
                        return false;
                    } else {
                        // All good find next pattern
                        break; // while (*str != '\0') {
                    }
                }
                break;
            }
            case '[': {
                char* pstart = pattern;
                bool has_previous_match = false;
                while (str_len > 0) {
                    bool negate = false;
                    bool repeating = false;
                    pattern = pstart + 1;

                    if (unlikely(*pattern == '!')) {
                        uassertf(*(pattern + 1) != ']', "expected some chars after [!..]");
                        negate = true;
                        pattern++;
                    }

                    bool matched = false;
                    while (*pattern != ']' && *pattern != '\0') {
                        if (*(pattern + 1) == '-' && *(pattern + 2) != ']' &&
                            *(pattern + 2) != '\0') {
                            // Handle character ranges like a-zA-Z0-9
                            uassertf(
                                *pattern < *(pattern + 2),
                                "pattern [n-m] sequence, n must be less than m: [%c-%c]",
                                *pattern,
                                *(pattern + 2)
                            );
                            if (*str >= *pattern && *str <= *(pattern + 2)) { matched = true; }
                            pattern += 3;
                        } else if (*pattern == '\\') {
                            // Escape sequence
                            pattern++;
                            if (*pattern != '\0') {
                                if (*pattern == *str) { matched = true; }
                                pattern++;
                            }
                        } else {
                            if (unlikely(*pattern == '+')) {
                                // repeating group [a-z+]@, match all cases until @
                                if (unlikely(!(*(pattern + 1) == ']'))) {
                                    uassertf(
                                        false,
                                        "Unescaped '+' literal, or '+' must be last before ]"
                                    );
                                    return false;
                                }
                                repeating = true;
                            } else if (unlikely(*pattern == '*')) {
                                uassertf(
                                    false,
                                    "Invalid pattern, unescaped *, use [...\\*...], or '+' for any modifier"
                                );
                                return false;
                            } else {
                                if (*pattern == *str) { matched = true; }
                            }
                            pattern++;
                        }
                    }

                    if (unlikely(*pattern != ']')) {
                        uassertf(false, "Invalid pattern - no closing ']'");
                        return false;
                    } else {
                        pattern++;

                        if (matched == negate) {
                            if (repeating && has_previous_match) {
                                // We have not matched char, but it may match to next pattern
                                break;
                            }
                            return false;
                        }

                        str++;
                        str_len--;
                        has_previous_match = true;
                        if (!repeating) {
                            break; // while (*str != '\0') {
                        }
                    }
                }

                if (str_len == 0) {
                    // str end reached, pattern also must be at end (null-term)
                    return *pattern == '\0';
                }
                break;
            }

            case '\\':
                // Escape next character
                pattern++;
                if (*pattern == '\0') { return false; }
                fallthrough();

            default:
                if (*pattern && str_len == 0) { return false; }
                if (*pattern != *str) { return false; }
                str++;
                str_len--;
                pattern++;
        }
    }

    return str_len == 0;
}

/// Slice pattern matching check (see ./cex help str$ for examples)
static bool
cex_str__slice__match(str_s s, char* pattern)
{
    return _cex_str_match(s.buf, s.len, pattern, NULL);
}

/// String pattern matching check (see ./cex help str$ for examples)
static bool
cex_str_match(char* s, char* pattern)
{
    return _cex_str_match(s, str.len(s), pattern, NULL);
}

/// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical
static int
cex_str_qscmp(const void* a, const void* b)
{
    const char* _a = *(const char**)a;
    const char* _b = *(const char**)b;

    if (_a == NULL || _b == NULL) { return (_a < _b) - (_a > _b); }
    return strcmp(_a, _b);
}

/// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical case insensitive
static int
cex_str_qscmpi(const void* a, const void* b)
{
    const char* _a = *(const char**)a;
    const char* _b = *(const char**)b;

    if (_a == NULL || _b == NULL) { return (_a < _b) - (_a > _b); }

    while (*_a && *_b) {
        int diff = _cex_str__tolower((unsigned char)*_a) - _cex_str__tolower((unsigned char)*_b);
        if (diff != 0) { return diff; }
        _a++;
        _b++;
    }
    return _cex_str__tolower((unsigned char)*_a) - _cex_str__tolower((unsigned char)*_b);
}

const struct __cex_namespace__str str = {
    // Autogenerated by CEX
    // clang-format off

    .clone = cex_str_clone,
    .copy = cex_str_copy,
    .ends_with = cex_str_ends_with,
    .eq = cex_str_eq,
    .eqi = cex_str_eqi,
    .find = cex_str_find,
    .findr = cex_str_findr,
    .fmt = cex_str_fmt,
    .join = cex_str_join,
    .len = cex_str_len,
    .lower = cex_str_lower,
    .match = cex_str_match,
    .qscmp = cex_str_qscmp,
    .qscmpi = cex_str_qscmpi,
    .replace = cex_str_replace,
    .sbuf = cex_str_sbuf,
    .split = cex_str_split,
    .split_lines = cex_str_split_lines,
    .sprintf = cex_str_sprintf,
    .sstr = cex_str_sstr,
    .starts_with = cex_str_starts_with,
    .sub = cex_str_sub,
    .upper = cex_str_upper,
    .vsprintf = cex_str_vsprintf,

    .convert = {
        .to_bool = cex_str__convert__to_bool,
        .to_bools = cex_str__convert__to_bools,
        .to_f32 = cex_str__convert__to_f32,
        .to_f32s = cex_str__convert__to_f32s,
        .to_f64 = cex_str__convert__to_f64,
        .to_f64s = cex_str__convert__to_f64s,
        .to_i16 = cex_str__convert__to_i16,
        .to_i16s = cex_str__convert__to_i16s,
        .to_i32 = cex_str__convert__to_i32,
        .to_i32s = cex_str__convert__to_i32s,
        .to_i64 = cex_str__convert__to_i64,
        .to_i64s = cex_str__convert__to_i64s,
        .to_i8 = cex_str__convert__to_i8,
        .to_i8s = cex_str__convert__to_i8s,
        .to_u16 = cex_str__convert__to_u16,
        .to_u16s = cex_str__convert__to_u16s,
        .to_u32 = cex_str__convert__to_u32,
        .to_u32s = cex_str__convert__to_u32s,
        .to_u64 = cex_str__convert__to_u64,
        .to_u64s = cex_str__convert__to_u64s,
        .to_u8 = cex_str__convert__to_u8,
        .to_u8s = cex_str__convert__to_u8s,
    },

    .slice = {
        .clone = cex_str__slice__clone,
        .copy = cex_str__slice__copy,
        .ends_with = cex_str__slice__ends_with,
        .eq = cex_str__slice__eq,
        .eqi = cex_str__slice__eqi,
        .index_of = cex_str__slice__index_of,
        .iter_split = cex_str__slice__iter_split,
        .lstrip = cex_str__slice__lstrip,
        .match = cex_str__slice__match,
        .qscmp = cex_str__slice__qscmp,
        .qscmpi = cex_str__slice__qscmpi,
        .remove_prefix = cex_str__slice__remove_prefix,
        .remove_suffix = cex_str__slice__remove_suffix,
        .rstrip = cex_str__slice__rstrip,
        .starts_with = cex_str__slice__starts_with,
        .strip = cex_str__slice__strip,
        .sub = cex_str__slice__sub,
    },

    // clang-format on
};
#endif
