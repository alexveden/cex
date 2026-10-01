#if !defined(cex$enable_minimal) || defined(cex$enable_mem)

#include "all.h"

void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}

void
_cex_allocator_arena_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    AllocatorArena.destroy(*allc);
}

__attribute__((destructor)) void
_cex_global_allocators_destructor()
{
    AllocatorArena_c* allc = (AllocatorArena_c*)tmem$;
    allocator_arena_page_s* page = allc->last_page;
    while (page) {
        auto tpage = page->prev_page;
        mem$free(allc->backing_alloc, page);
        page = tpage;
    }
}

#endif
