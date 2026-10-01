#define CEX_LOG_LVL 0 /* 0 (mute all) - 5 (log$trace) */
#define CEX_TEST
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define OOM_MAX_CALL 8

int
fuzz$case(const u8* data, usize size)
{
    if (size < 10) { return -1; }
    fuzz$dnew(data, size);

    // OOM paths intentionally assert on grow failures; disabling uassert lets them run
    // while ASAN/UBSan still catch memory-safety bugs
    uassert_disable();

    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 1024, .disable_scopes = true }
    );

    hm$(u64, u64) imap = NULL;
    hm$(char*, u64) smap = NULL;
    hm$(char*, u64) amap = NULL;

    if (arena != NULL) {
        imap = hm$new(imap, arena);
        smap = hm$new(smap, arena, .copy_keys = true);
        amap = hm$new(amap, arena, .copy_keys = true, .copy_keys_arena_pgsize = 1024);

        u8 op = 0;
        char keybuf[24];
        while (fuzz$dget(&op)) {
            u64 k = 0;
            if (!fuzz$dget(&k)) { break; }
            u8 oom = 0;
            if (!fuzz$dget(&oom)) { break; }

            snprintf(keybuf, sizeof(keybuf), "k%llu", (unsigned long long)(k % 64));
            f32 threshold = (f32)(oom % (OOM_MAX_CALL + 1));

            if ((op & 7) == 7 && fuzz$dprob(0.2)) {
                ((AllocatorArena_c*)arena)->test_oom_threshold = threshold;
                hm$(u64, u64) tmp = hm$new(tmp, arena);
                hm$free(tmp);
                ((AllocatorArena_c*)arena)->test_oom_threshold = 0.0f;
                continue;
            }

            ((AllocatorArena_c*)arena)->test_oom_threshold = threshold;
            if (amap != NULL && _cexds__header(amap)->_hash_table != NULL) {
                AllocatorArena_c* key_arena =
                    (AllocatorArena_c*)_cexds__header(amap)->_hash_table->key_arena;
                if (key_arena != NULL) { key_arena->test_oom_threshold = threshold; }
            }

            switch (op & 7) {
                case 0:
                case 1:
                    if (imap != NULL) { hm$set(imap, k, k); }
                    break;
                case 2:
                    if (imap != NULL) { (void)hm$get(imap, k, 0); }
                    break;
                case 3:
                    if (imap != NULL) { hm$del(imap, k); }
                    break;
                case 4:
                    if (smap != NULL) { hm$set(smap, keybuf, k); }
                    break;
                case 5:
                    if (smap != NULL) { (void)hm$get(smap, keybuf, 0); }
                    break;
                case 6:
                    if (smap != NULL) { hm$del(smap, keybuf); }
                    break;
                case 7:
                    if (amap != NULL && fuzz$dprob(0.5)) {
                        hm$set(amap, keybuf, k);
                    } else if (imap != NULL && fuzz$dprob(0.5)) {
                        hm$clear(imap);
                    } else if (imap != NULL) {
                        for$each (it, imap) { (void)it; }
                    }
                    break;
            }

            ((AllocatorArena_c*)arena)->test_oom_threshold = 0.0f;
            if (amap != NULL && _cexds__header(amap)->_hash_table != NULL) {
                AllocatorArena_c* key_arena =
                    (AllocatorArena_c*)_cexds__header(amap)->_hash_table->key_arena;
                if (key_arena != NULL) { key_arena->test_oom_threshold = 0.0f; }
            }
        }
    }

    hm$free(imap);
    hm$free(smap);
    hm$free(amap);
    if (arena != NULL) { AllocatorArena_destroy(arena); }

    uassert_enable();
    return 0;
}

fuzz$setup()
{
    // the panic handler prints disabled uasserts to stdout; mute it to keep fuzz output clean
    freopen("/dev/null", "w", stdout);

    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    u8 ops[] = { 1, 4, 7, 1, 4, 3, 6, 0 };
    u8 oom_vals[] = { 2, 1, 3 };
    u8 buf[10 * 48];

    mem$scope(tmem$, _)
    {
        for (u32 s = 0; s < arr$len(oom_vals); s++) {
            usize n = 0;
            for (u64 i = 0; i < 48; i++) {
                u8 op = ops[i % arr$len(ops)];
                u64 key = i;
                buf[n++] = op;
                memcpy(buf + n, &key, sizeof(key));
                n += sizeof(key);
                buf[n++] = oom_vals[s];
            }
            char* fn = str.fmt(_, "%s/%03u", fuzz$corpus_dir, s);
            FILE* f = NULL;
            e$except(err, io.fopen(&f, fn, "wb")) { uassert(false && "fopen"); }
            e$except(err, io.fwrite(f, buf, n)) { uassert(false && "fwrite"); }
            io.fclose(&f);
        }
    }
}

fuzz$main();
