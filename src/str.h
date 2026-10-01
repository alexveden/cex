/**
 * @file
 * @brief
 */

#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_str)

#if defined(cex$enable_minimal) && !defined(cex$enable_ds)
#error "CEX str namespace depends on `#define cex$enable_ds`"
#endif

#include "all.h"

/// Compares str_s (slice) with literal in performance efficient way
#define str$eq(str_s_slice, compare_to_literal)                                                    \
    ((str_s_slice).buf && (str_s_slice).len == sizeof(compare_to_literal) - 1 &&                   \
     memcmp((str_s_slice).buf, compare_to_literal, sizeof(compare_to_literal) - 1) == 0)

/// Joins parts of strings using a separator str$join(allc, ",", "a", "b", "c") -> "a,b,c"
#define str$join(allocator, str_join_by, str_parts...)                                             \
    ({                                                                                             \
        char* _args[] = { str_parts };                                                             \
        usize _args_len = arr$len(_args);                                                          \
        str.join(_args, _args_len, str_join_by, allocator);                                        \
    })

/// Parses string contents as value type based on generic numeric type of out_var_ptr
#define str$convert(str_or_slice, out_var_ptr)                                                     \
    _Generic((str_or_slice), \
    char*: _Generic((out_var_ptr), \
        bool*:  str.convert.to_bool, \
        i8*:  str.convert.to_i8, \
        u8*:  str.convert.to_u8, \
        i16*:  str.convert.to_i16, \
        u16*:  str.convert.to_u16, \
        i32*:  str.convert.to_i32, \
        u32*:  str.convert.to_u32, \
        i64*:  str.convert.to_i64, \
        u64*:  str.convert.to_u64, \
        f32*:  str.convert.to_f32, \
        f64*:  str.convert.to_f64 \
    ), \
    str_s: _Generic((out_var_ptr), \
        bool*:  str.convert.to_bools, \
        i8*:  str.convert.to_i8s, \
        u8*:  str.convert.to_u8s, \
        i16*:  str.convert.to_i16s, \
        u16*:  str.convert.to_u16s, \
        i32*:  str.convert.to_i32s, \
        u32*:  str.convert.to_u32s, \
        i64*:  str.convert.to_i64s, \
        u64*:  str.convert.to_u64s, \
        f32*:  str.convert.to_f32s, \
        f64*:  str.convert.to_f64s \
    ) \
)(str_or_slice, out_var_ptr)

/**

## Strings

### Principles

- `str` namespace is built for compatibility with C strings
- all string functions are NULL resilient
- all string functions can return NULL on error
- you don't have to check every operation for NULL every time, just at the end
- all string format operations support CEX-specific specifiers (see below)

### String slices

- Slices are backed by `(str_s){.buf = s, .len = NNN}` struct
- Slices are passed by value and allocated on stack
- Slices can be made from null-terminated strings, or buffers, or literals
- str$s("hello") - use this for compile time defined slices/constants
- Slices are not guaranteed to be null-terminated
- Slices support operations which are allowed by read-only string view representation
- CEX formatting uses `%S` for slices: `io.printf("Hello %S\n", str$s("world"))`

### String macros

- `str$s("hello")` - compile-time `str_s` from a string literal (literals only, not `char*`)
- `str$eq(slice, "literal")` - fast slice-vs-literal comparison (no `strcmp`)
- `str$join(alloc, ",", "a", "b", "c")` - join parts into a new string
- `str$convert(str_or_slice, &out_var)` - parse a string/slice into a numeric or bool out variable
- Prefer `str$convert()` / `str.convert.*` over libc `atoi`/`atof`/`strtol`/`strtod`:
  type-safe, overflow-checked, NULL resilient, works on both `char*` and `str_s`

### Dynamic strings

For mutable, growing strings use the `sbuf` namespace (`sbuf_c` is a `char*` alias, always
null-terminated):

- `sbuf.create(cap, alloc)` / `sbuf.create_static(buf, n)` - allocator- or stack-backed builder (static buffer is aligned up and cannot grow)
- `sbuf.appendf(&s, "%s: %S", "x", slice)` / `sbuf.append(&s, "text")` - append
- `sbuf.len(&s)` / `sbuf.capacity(&s)` / `sbuf.clear(&s)` - inspect/reset
- `sbuf.destroy(&s)` - free (sets `s` to NULL)

See `./cex help sbuf$` for the full API.

### String formatting in CEX

All CEX routines with format strings (`io.printf()`, `log$error()`, `str.fmt()`,
`sbuf.appendf()`) use the CEX formatting engine with extended features:

* `%S` prints a `str_s` slice — `io.printf("%S\n", str$s("world"))`
* `%S` has sanity checks: a plain `char*` in a `%S` slot prints `(%S-bad/overflow)`;
  behavior is platform-dependent, do not rely on it
* `%lu` / `%ld` — 64-bit integers, platform independent
* `%u` / `%d` — 32-bit integers, platform independent
* `%s` — standard null-terminated `char*`
* other formats are compatible with vanilla libC

### Examples

- Working with slices
```c
char* cstr = "hello";
str_s s = str.sstr(cstr); // (str_s){.buf = "hello", .len = 5}
usize n = str.len(cstr);  // 5
```

- Getting substring as slices
```c
str.sub("123456", 1, -1);     // slice: 2345
str.sub("123456", -3, -1);    // slice: 45
str.sub("123456", -30, 2000); // slice: 123456 (out-of-range clamps, no crash)

str_s s = str.sstr("123456");
str_s sub = str.slice.sub(s, 1, 2); // slice: 2
```

- Splitting / iterating via tokens
```c
// zero-allocation iteration (it.val is a non-null-terminated slice — use %S)
str_s s = str.sstr("123,456");
for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
    io.printf("%S\n", it.val);
}

// mem-allocating split: each item is a cloned C-string
mem$scope(tmem$, _)
{
    arr$(char*) res = str.split("123,456,789", ",", _); // NULL on error
    for$each (v, res) {
        io.printf("%s\n", v);
    }
}
```

- Chaining string operations
```c
mem$scope(tmem$, _)
{
    // each op is NULL tolerant and returns NULL on error, so check once at the end
    char* s = str.fmt(_, "hi there");
    s = str.replace(s, "hi", "hello", _);
    s = str.fmt(_, "result is: %s", s); // "result is: hello there"
    if (s == NULL) { return; } // handle error
}
```

- Pattern matching
```c
// * zero+ chars | ? one char | [abc] one of | [!abc] none of | [abc+] one+
// [a-c0-9] range | \\* literal | (abc|def|xyz) alternatives

str.match("test.txt", "*?txt");                      // true
str.match("image.png", "image.[jp][pn]g");           // true
str.match("backup.txt", "[!a]*.txt");                // true
str.match("create", "(run|build|create|clean)");     // true

str_s src = str$s("my_test __String.txt");
str.slice.match(src, "my_test*.txt"); // true
```

*/
struct __cex_namespace__str {
    // Autogenerated by CEX
    // clang-format off

    /// Clones string using allocator, null tolerant, returns NULL on error.
    char*           (*clone)(char* s, IAllocator allc);
    /// Makes a copy of initial `src`, into `dest` buffer constrained by `destlen`. NULL tolerant,
    /// always null-terminated, overflow checked.
    Exception       (*copy)(char* dest, char* src, usize destlen);
    /// Checks if string ends with prefix, returns false on error, NULL tolerant
    bool            (*ends_with)(char* s, char* suffix);
    /// Compares two null-terminated strings (null tolerant)
    bool            (*eq)(char* a, char* b);
    /// Compares two strings, case insensitive, null tolerant
    bool            (*eqi)(char* a, char* b);
    /// Find a substring in a string, returns pointer to first element. NULL tolerant, and NULL on err.
    char*           (*find)(char* haystack, char* needle);
    /// Find substring from the end , NULL tolerant, returns NULL on error.
    char*           (*findr)(char* haystack, char* needle);
    /// Formats string and allocates it dynamically using allocator, supports CEX format engine
    char*           (*fmt)(IAllocator allc, char* format,...);
    /// Computes string hash, seed can be null, or previous hash value for hash stacking  (null or empty
    /// string returns 0 hash)
    u64             (*hash)(char* a, u64 seed);
    /// Joins string using a separator (join_by), NULL tolerant, returns NULL on error.
    char*           (*join)(char** str_arr, usize str_arr_len, char* join_by, IAllocator allc);
    /// Calculates string length, NULL tolerant.
    usize           (*len)(char* s);
    /// Returns new lower case string, returns NULL on error, null tolerant
    char*           (*lower)(char* s, IAllocator allc);
    /// String pattern matching check (see ./cex help str$ for examples)
    bool            (*match)(char* s, char* pattern);
    /// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical
    int             (*qscmp)(const void* a, const void* b);
    /// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical case insensitive
    int             (*qscmpi)(const void* a, const void* b);
    /// Replaces substring occurrence in a string
    char*           (*replace)(char* s, char* old_sub, char* new_sub, IAllocator allc);
    /// Creates string slice from a buf+len
    str_s           (*sbuf)(char* s, usize length);
    /// Splits string using split_by (allows many) chars, returns new dynamic array of split char*
    /// tokens, allocates memory with allc, returns NULL on error. NULL tolerant. Items of array are
    /// cloned, so you need free them independently or better use arena or tmem$.
    arr$(char*)     (*split)(char* s, char* split_by, IAllocator allc);
    /// Splits string by lines, result allocated by allc, as dynamic array of cloned lines, Returns NULL
    /// on error, NULL tolerant. Items of array are cloned, so you need free them independently or
    /// better use arena or tmem$. Supports \n or \r\n.
    arr$(char*)     (*split_lines)(char* s, IAllocator allc);
    /// Analog of sprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
    Exc             (*sprintf)(char* dest, usize dest_len, char* format,...);
    /// Creates string slice of input C string (NULL tolerant, (str_s){0} on error)
    str_s           (*sstr)(char* ccharptr);
    /// Checks if string starts with prefix, returns false on error, NULL tolerant
    bool            (*starts_with)(char* s, char* prefix);
    /// Makes slices of `s` char* string, start/end are indexes, can be negative from the end, if end=0
    /// mean full length of the string. `s` may be not null-terminated. function is NULL tolerant,
    /// return (str_s){0} on error
    str_s           (*sub)(char* s, isize start, isize end);
    /// Returns new upper case string, returns NULL on error, null tolerant
    char*           (*upper)(char* s, IAllocator allc);
    /// Analog of vsprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
    Exception       (*vsprintf)(char* dest, usize dest_len, char* format, va_list va);

    struct {
        Exception       (*to_bool)(char* s, bool* num);
        Exception       (*to_bools)(str_s s, bool* num);
        Exception       (*to_f32)(char* s, f32* num);
        Exception       (*to_f32s)(str_s s, f32* num);
        Exception       (*to_f64)(char* s, f64* num);
        Exception       (*to_f64s)(str_s s, f64* num);
        Exception       (*to_i16)(char* s, i16* num);
        Exception       (*to_i16s)(str_s s, i16* num);
        Exception       (*to_i32)(char* s, i32* num);
        Exception       (*to_i32s)(str_s s, i32* num);
        Exception       (*to_i64)(char* s, i64* num);
        Exception       (*to_i64s)(str_s s, i64* num);
        Exception       (*to_i8)(char* s, i8* num);
        Exception       (*to_i8s)(str_s s, i8* num);
        Exception       (*to_u16)(char* s, u16* num);
        Exception       (*to_u16s)(str_s s, u16* num);
        Exception       (*to_u32)(char* s, u32* num);
        Exception       (*to_u32s)(str_s s, u32* num);
        Exception       (*to_u64)(char* s, u64* num);
        Exception       (*to_u64s)(str_s s, u64* num);
        Exception       (*to_u8)(char* s, u8* num);
        Exception       (*to_u8s)(str_s s, u8* num);
    } convert;

    struct {
        /// Clone slice into new char* allocated by `allc`, null tolerant, returns NULL on error.
        char*           (*clone)(str_s s, IAllocator allc);
        /// Makes a copy of initial `src` slice, into `dest` buffer constrained by `destlen`. NULL tolerant,
        /// always null-terminated, overflow checked.
        Exception       (*copy)(char* dest, str_s src, usize destlen);
        /// Checks if slice ends with prefix, returns (str_s){0} on error, NULL tolerant
        bool            (*ends_with)(str_s s, str_s suffix);
        /// Compares two string slices, null tolerant
        bool            (*eq)(str_s a, str_s b);
        /// Compares two string slices, null tolerant, case insensitive
        bool            (*eqi)(str_s a, str_s b);
        /// Computes string hash, seed can be null, or previous hash value for hash stacking  (null or empty
        /// string returns 0 hash)
        u64             (*hash)(str_s a, u64 seed);
        /// Get index of first occurrence of `needle`, returns -1 on error.
        isize           (*index_of)(str_s s, str_s needle);
        /// iterator over slice splits:  for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {}
        str_s           (*iter_split)(str_s s, char* split_by, cex_iterator_s* iterator);
        /// Removes white spaces from the beginning of slice
        str_s           (*lstrip)(str_s s);
        /// Slice pattern matching check (see ./cex help str$ for examples)
        bool            (*match)(str_s s, char* pattern);
        /// libc `qsort()` comparator function for alphabetical sorting of str_s arrays
        int             (*qscmp)(const void* a, const void* b);
        /// libc `qsort()` comparator function for alphabetical case insensitive sorting of str_s arrays
        int             (*qscmpi)(const void* a, const void* b);
        /// Replaces slice prefix (start part), or returns the same slice if it's not found
        str_s           (*remove_prefix)(str_s s, str_s prefix);
        /// Replaces slice suffix (end part), or returns the same slice if it's not found
        str_s           (*remove_suffix)(str_s s, str_s suffix);
        /// Removes white spaces from the end of slice
        str_s           (*rstrip)(str_s s);
        /// Checks if slice starts with prefix, returns (str_s){0} on error, NULL tolerant
        bool            (*starts_with)(str_s s, str_s prefix);
        /// Removes white spaces from both ends of slice
        str_s           (*strip)(str_s s);
        /// Makes slices of `s` slice, start/end are indexes, can be negative from the end, if end=0 mean
        /// full length of the string. `s` may be not null-terminated. function is NULL tolerant, return
        /// (str_s){0} on error
        str_s           (*sub)(str_s s, isize start, isize end);
    } slice;

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__str str;

#endif
