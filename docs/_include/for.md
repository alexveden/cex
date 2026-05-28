

Unified array / hashmap / slice iteration framework.

`for$` macros provide a single syntax for looping over any iterable data in CEX:

| Variant                              | Copies elements?          | Use case                                         |
|--------------------------------------|---------------------------|--------------------------------------------------|
| `for$each(it, array, len?)`          | By value (≤ 64 B)         | Small types / copy iteration / slices            |
| `for$eachp(it, array, len?)`         | By pointer (no copy)      | Large structs / avoid copy overhead / slices     |
| `for$iter(T, it, iter_func)`         | Custom (cex_iterator_s)   | Tokenizers, generators, splitters                |

All three work identically on `arr$`, `hm$`, static C arrays, and pointer+length slices.

- Using for$ as unified array iterator
```c

test$case(test_array_iteration)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    for$each(it, array) {
        io.printf("el=%d\n", it);
    }
    // Prints:
    // el=1
    // el=2
    // el=3

    // NOTE: prefer this when you work with bigger structs to avoid extra memory copying
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;

        // NOTE: it now is a pointer
        io.printf("el[%zu]=%d\n", i, *it);
    }
    // Prints:
    // el[0]=1
    // el[1]=2
    // el[2]=3

    // Static arrays work as well (arr$len inferred)
    i32 arr_int[] = {1, 2, 3, 4, 5};
    for$each(it, arr_int) {
        io.printf("static=%d\n", it);
    }
    // Prints:
    // static=1
    // static=2
    // static=3
    // static=4
    // static=5


    // Simple pointer+length also works (let's do a slice)
    i32* slice = &arr_int[2];
    for$each(it, slice, 2) {
        io.printf("slice=%d\n", it);
    }
    // Prints:
    // slice=3
    // slice=4


    // it is type of cex_iterator_s
    // NOTE: run in shell: ➜ ./cex help cex_iterator_s
    s = str.sstr("123,456");
    for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
        io.printf("it.value = %S\n", it.val);
    }
    // Prints:
    // it.value = 123
    // it.value = 456

    arr$free(array);
    return EOK;
}

```



```c
/// Iterates over arrays by **value** (copies each element into `it`). Works on arr$, hm$, static arrays, and pointer+len. Capped at `CEX_FOREACH_MAX_COPY_SIZE` (64 B) per element.
#define for$each(it, array, array_len...)

/// Iterates over arrays by **pointer** (no copy — `it` is `T*`). Best for large structs. Works on arr$, hm$, static arrays, and pointer+len.
#define for$eachp(it, array, array_len...)

/// Iterates via a custom iterator function.
#define for$iter(it_val_type, it, iter_func)




```
