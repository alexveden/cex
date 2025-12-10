#define CEX_LOG_LVL 0 /* 0 (mute all) - 5 (log$trace) */
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

fuzz$setup(void)
{
    uassert(os.path.exists(fuzz$corpus_dir));
    uassert(os.path.exists("../../src") && "expected to be run from fuzz_cex_parser dir");

    mem$scope(tmem$, _)
    {
        for$each (fn, os.fs.find("../../src/*.[hc]", false, _)) {
            e$except (
                err,
                os.fs.copy(fn, str.fmt(_, "%s.out/%S", fuzz$corpus_dir, os.path.split(fn, false)))
            );
        }
    }
}

arr$(cex_token_s) items = NULL;

int
fuzz$case(const u8* data, usize size)
{
    if (size <= 1) { return -1; }

    CexParser_c lx = CexParser.create((char*)data, size, true);
    cex_token_s t;
    while ((t = CexParser.next_token(&lx)).type) {
        if (t.value.buf) {
            uassert(t.type != CexTkn__error);
            uassert(t.value.buf >= (char*)data);
            uassert(t.value.buf <= (char*)data + size);
            uassert(t.value.buf + t.value.len <= (char*)data + size);
        } else {
            uassert(t.type == CexTkn__error);
            uassert(t.value.len == 0);
        }
    }

    // mem$scope(tmem$, _)
    {
        // arr$(cex_token_s) items = arr$new(items, _);
        if (items == NULL) { items = arr$new(items, mem$); }
        CexParser_c lx2 = CexParser.create((char*)data, size, true);
        cex_token_s t;
        u32 n = 0;
        u32 ndecl = 0;
        while ((t = CexParser.next_entity(&lx2, &items)).type) {
            u32 n_items = arr$len(items);
            if (t.value.buf) {
                uassert(t.type != CexTkn__error);
                uassert(t.value.buf >= (char*)data);
                uassert(t.value.buf <= (char*)data + size);
                uassert(t.value.buf + t.value.len <= (char*)data + size);
                for$each (it, items) {
                    uassert(it.type != CexTkn__error);
                    uassert(it.value.buf >= (char*)data);
                    uassert(it.value.buf <= (char*)data + size);
                    uassert(it.value.buf + it.value.len <= (char*)data + size);
                }
                uassert(n_items > 0);
                // uassert(n_items < 10000);
            } else {
                uassert(t.type == CexTkn__error);
                uassert(t.value.len == 0);
                uassert(arr$len(items) == 0);
                break;
            }
            n++;

            cex_decl_s* d = CexParser.decl_parse(&lx2, t, items, NULL, mem$);
            if (d == NULL) { continue; }
            uassert(d->type != CexTkn__eof);
            CexParser.decl_free(d, mem$);
            ndecl++;
        }
        (void)n;
        (void)ndecl;
    }
    return 0;
}

fuzz$main();
