#include "src/all.c"

static bool
fail_hook(char* name)
{
    char* v = os.env.get("CEX_SUITE_FAIL", NULL);
    return v != NULL && str.eq(v, name);
}

static bool
mock_isatty(FILE* file)
{
    (void)file;
    return true;
}

__attribute__((constructor)) static void
force_ansi_when_requested(void)
{
    if (os.env.get("CEX_SUITE_ANSI", NULL) != NULL) { io.isatty = mock_isatty; }
}

static Exception
err_ret(int i)
{
    if (i) { e$ret(e$raise(Error.io, "raise io")); }
    return EOK;
}

test$bench(bench_case)
{
    return EOK;
}

test$setup_suite()
{
    if (fail_hook("setup_suite")) { return e$raise(Error.io, "setup_suite boom"); }
    return EOK;
}

test$setup_case()
{
    if (fail_hook("setup_case")) { return e$raise(Error.io, "setup_case boom"); }
    return EOK;
}

test$teardown_case()
{
    if (fail_hook("teardown_case")) { return e$raise(Error.io, "teardown_case boom"); }
    return EOK;
}

test$teardown_suite()
{
    if (fail_hook("teardown_suite")) { return e$raise(Error.io, "teardown_suite boom"); }
    return EOK;
}

test$case(regular_fail)
{
    if (fail_hook("case")) {
        (void)os.random.next();
        return e$raise(Error.io, "case boom");
    }
    if (fail_hook("tassert")) {
        Exc e = err_ret(1);
        tassert_eq(e, Error.io);
        tassert_eq(0, 1);
    }
    if (fail_hook("tassert_er")) { tassert_er(Error.argument, err_ret(1)); }
    return EOK;
}

test$case(replace_global_mem_allocator)
{
    if (fail_hook("alloc_replace")) {
        AllocatorHeap_c* custom =
            mem$malloc(test$alloc, sizeof(AllocatorHeap_c), alignof(AllocatorHeap_c));
        memcpy(custom, &_cex__default_global__allocator_heap, sizeof(*custom));
        memset(&custom->stats, 0, sizeof(custom->stats));
        // NOTE: deliberately not restored here - the runner must restore it
        mem$ = &custom->alloc;

        u8* p = mem$malloc(mem$, 64);
        if (p == NULL) { return e$raise(Error.runtime, "custom alloc failed"); }
        mem$free(mem$, p);
        fprintf(stderr, "MEM_REPLACED\n");
    }
    return EOK;
}

test$case(global_mem_allocator_restored)
{
    if (fail_hook("alloc_replace")) {
        if (mem$ != &_cex__default_global__allocator_heap.alloc) {
            return e$raise(Error.runtime, "mem$ not restored by the runner");
        }
        fprintf(stderr, "MEM_RESTORED\n");
    }
    return EOK;
}

test$case(leak_memory)
{
    if (fail_hook("leak")) {
        u8* p = mem$malloc(mem$, 64);
        if (p == NULL) { return e$raise(Error.runtime, "leak alloc failed"); }
        (void)p;
    }
    return EOK;
}

test$main();
