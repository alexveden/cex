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
    if (size < 10) { return -1; }
    fuzz$dnew(data, size);

    IAllocator arena = AllocatorArena.create(
        &(AllocatorArena_kw){ .page_size = 1024, .disable_scopes = true }
    );
    arr$(u64) arr = NULL;

    if (arena != NULL) {
        u8 op = 0;
        while (fuzz$dget(&op)) {
            u64 v = 0;
            if (!fuzz$dget(&v)) { break; }
            u8 oom = 0;
            if (!fuzz$dget(&oom)) { break; }

            // arr$push()/arr$pusha() abort on OOM via uassert_always, so growth is
            // exercised through the graceful arr$new()/arr$setcap()/arr$grow() API
            ((AllocatorArena_c*)arena)->test_oom_threshold = 0.0f;

            switch (op % 6) {
                case 0:
                    if (arr == NULL) {
                        ((AllocatorArena_c*)arena)->test_oom_threshold = (f32)(oom % 4);
                        arr$new(arr, arena, .capacity = v % 64);
                        ((AllocatorArena_c*)arena)->test_oom_threshold = 0.0f;
                    }
                    break;
                case 1:
                    if (arr != NULL) {
                        ((AllocatorArena_c*)arena)->test_oom_threshold = (f32)(oom % 4);
                        arr$setcap(arr, v % 256);
                        ((AllocatorArena_c*)arena)->test_oom_threshold = 0.0f;
                    }
                    break;
                case 2:
                    if (arr != NULL) { arr$push(arr, v); }
                    break;
                case 3:
                    if (arr != NULL && arr$len(arr) > 0) { (void)arr$pop(arr); }
                    break;
                case 4:
                    if (arr != NULL) {
                        if (arr$len(arr) > 0) {
                            (void)arr$del(arr, v % arr$len(arr));
                        } else {
                            arr$clear(arr);
                        }
                    }
                    break;
                case 5:
                    if (arr != NULL && (v & 1)) {
                        arr$free(arr);
                    } else if (arr != NULL) {
                        for$each (it, arr) { (void)it; }
                    }
                    break;
            }
        }
    }

    arr$free(arr);
    if (arena != NULL) { AllocatorArena_destroy(arena); }

    return 0;
}

fuzz$setup()
{
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    u8 ops[] = { 0, 1, 2, 2, 1, 3, 4, 5 };
    u8 oom_vals[] = { 1, 2, 3 };
    u8 buf[10 * 48];

    mem$scope(tmem$, _)
    {
        for (u32 s = 0; s < arr$len(oom_vals); s++) {
            usize n = 0;
            for (u64 i = 0; i < 48; i++) {
                u8 op = ops[i % arr$len(ops)];
                u64 v = i;
                buf[n++] = op;
                memcpy(buf + n, &v, sizeof(v));
                n += sizeof(v);
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
