#define CEX_LOG_LVL 0 /* 0 (mute all) - 5 (log$trace) */
#define CEX_TEST
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int
fuzz$case(const u8* data, usize size)
{
    if (size < 9) { return -1; }
    fuzz$dnew(data, size);

    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }
    );
    hm$(u64, u64) imap = NULL;
    hm$(char*, u64) smap = NULL;
    hm$(char*, u64) amap = NULL;

    if (arena != NULL) {
        imap = hm$new(imap, arena);
        smap = hm$new(smap, arena, .copy_keys = true);
        amap = hm$new(amap, arena, .copy_keys = true, .copy_keys_arena_pgsize = 1024);

        if (imap != NULL && smap != NULL && amap != NULL) {
            u8 op = 0;
            char keybuf[24];
            while (fuzz$dget(&op)) {
                u64 k = 0;
                if (!fuzz$dget(&k)) { break; }

                snprintf(keybuf, sizeof(keybuf), "k%llu", (unsigned long long)(k % 64));
                switch (op & 7) {
                    case 0:
                    case 1: hm$set(imap, k, k); break;
                    case 2: (void)hm$get(imap, k, 0); break;
                    case 3: hm$del(imap, k); break;
                    case 4: hm$set(smap, keybuf, k); break;
                    case 5: (void)hm$get(smap, keybuf, 0); break;
                    case 6: hm$del(smap, keybuf); break;
                    case 7:
                        if (fuzz$dprob(0.33)) {
                            hm$clear(imap);
                        } else if (fuzz$dprob(0.5)) {
                            hm$set(amap, keybuf, k);
                        } else {
                            for$each (it, imap) { (void)it; }
                        }
                        break;
                }
            }
        }
    }

    hm$free(imap);
    hm$free(smap);
    hm$free(amap);
    if (arena != NULL) { AllocatorArena_destroy(arena); }

    return 0;
}

fuzz$setup()
{
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    struct
    {
        u8 data[45];
        usize len;
    } seeds[] = {
        // imap: set 1, set 2, get 1, del 1, get 2
        { { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
            3, 1, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0 },
          45 },
        // string maps: set/get/del/arena-set on key 'k'
        { { 4, 'k', 0, 0, 0, 0, 0, 0, 0, 4, 'k', 0, 0, 0, 0, 0, 0, 0, 5, 'k', 0, 0, 0, 0, 0, 0, 0,
            6, 'k', 0, 0, 0, 0, 0, 0, 0, 7, 'k', 0, 0, 0, 0, 0, 0, 0 },
          45 },
    };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(seeds); i++) {
            char* fn = str.fmt(_, "%s/%03d", fuzz$corpus_dir, i);
            FILE* f;
            e$except(err, io.fopen(&f, fn, "wb")) { uassert(false && "fopen"); }
            e$except(err, io.fwrite(f, seeds[i].data, seeds[i].len)) { uassert(false && "fwrite"); }
            io.fclose(&f);
        }
    }
}

fuzz$main();
