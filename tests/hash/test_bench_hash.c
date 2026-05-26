#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "komihash_port.h"
//#include "komihash.h"
#include "a5hash.h"

#define G_BUF_LEN 1 * 1024 * 1024
#define G_WORDS_LEN 100 * 1024

arr$(str_s) g_words;
arr$(u64) g_nums;
char* g_buf;

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
test$setup_suite()
{
    g_buf = mem$malloc(mem$, G_BUF_LEN);
    uassert(g_buf);
    for (u32 i = 0; i < G_BUF_LEN; i++) { g_buf[i] = i % 254; }

    g_words = arr$new(g_words, mem$, .capacity = G_WORDS_LEN);
    g_nums = arr$new(g_nums, mem$, .capacity = G_WORDS_LEN);

    for (u32 i = 0; i < G_WORDS_LEN; i++) {
        usize wlen = i % 4 * 4 + 4;
        char* w = mem$malloc(mem$, wlen + 1);

        char c = i % 254;
        if (c == 0) c = 1;
        if ((usize)c + wlen > 254) {
            c -= wlen;
        }

        for(u32 j = 0; j < wlen; j++){
            w[j] = c;
            c++;
        }
        w[wlen] = '\0';
        arr$push(g_words, (str_s){.buf = w, .len = wlen});
        arr$push(g_nums, i);
    }

    return EOK;
}
test$teardown_suite()
{
    mem$free(mem$, g_buf);

    for$each (it, g_words) { mem$free(mem$, it.buf); }

    arr$free(g_words);
    arr$free(g_nums);

    return EOK;
}

test$bench(raw_buffer_cexds_hash)
{
    _cexds__hash_bytes(g_buf, G_BUF_LEN, 0);
    return EOK;
}

test$bench(raw_buffer_komihash)
{
    komihash(g_buf, G_BUF_LEN, 0);
    return EOK;
}

test$bench(raw_buffer_a5hash)
{
    a5hash(g_buf, G_BUF_LEN, 0);
    return EOK;
}

test$bench(words_list_nolen__cexds_hash)
{
    for$eachp(it, g_words) {
        _cexds__hash_string(it->buf, 20000, 0);
    }
    return EOK;
}

test$bench(words_list_nolen__komihash)
{
    for$eachp(it, g_words) {
        komihash(it->buf, strlen(it->buf), 0);
    }
    return EOK;
}

test$bench(words_list_nolen__a5hash)
{
    for$eachp(it, g_words) {
        a5hash(it->buf, strlen(it->buf), 0);
    }
    return EOK;
}

test$bench(words_list_withlen__cexds_hash)
{
    for$eachp(it, g_words) {
        _cexds__hash_string(it->buf, it->len, 0);
    }
    return EOK;
}

test$bench(words_list_withlen__komihash)
{
    for$eachp(it, g_words) {
        komihash(it->buf, it->len, 0);
    }
    return EOK;
}

test$bench(words_list_withlen__a5hash)
{
    for$eachp(it, g_words) {
        a5hash(it->buf, it->len, 0);
    }
    return EOK;
}

test$bench(numbers_cexds)
{
    for$eachp(it, g_nums) {
        _cexds__hash_bytes(it, sizeof(*it), 0);
    }
    return EOK;
}

test$bench(numbers_komihash)
{
    for$eachp(it, g_nums) {
        komihash(it, sizeof(*it), 0);
    }
    return EOK;
}

test$bench(numbers_a5hash)
{
    for$eachp(it, g_nums) {
        a5hash(it, sizeof(*it), 0);
    }
    return EOK;
}

test$case(komihash_stability){
    char buf[] = {"0123456789"}; 

    u64 h = komihash(buf, 10, 0);
    tassert_eq(h, 4432705459570477571L);

    /*
    komihash_stream_t ctx;
    komihash_stream_init( &ctx, 0 );
    komihash_stream_update( &ctx, buf, 5);
    komihash_stream_update( &ctx, buf + 5, 5);
    uint64_t stream_h = komihash_stream_final( &ctx );
    tassert_eq(stream_h, 4432705459570477571L);
    */

    h = komihash(buf, 5, 0);
    h = komihash(buf + 5, 5, h);
    tassert_eq(h, 10550884172113008973LU);
    return EOK;
}

test$case(cexds_hash_stability){
    char buf[] = {"01234567890123456789"}; 

    u64 h = _cexds__hash_bytes(buf, 20, 0);
    tassert_eq(h, 5429886828806815719L);

    h = _cexds__hash_bytes(buf, 10, 0);
    tassert_eq(h, 2473161072300887698L);

    h = _cexds__hash_bytes(buf + 10, 10, h);
    tassert_eq(h, 9777486606392370344UL);

    // WARNING: string hash uses different algo 
    h = _cexds__hash_string(buf, 1000, 0);
    tassert_eq(h, 9355514798287909589UL);

    return EOK;
}

test$case(cexds_hash_stability_4or8){
    char buf[] = {"01230123"}; 

    u64 h = _cexds__hash_bytes(buf, 4, 0);
    tassert_eq(h, 14843858323657194021UL);

    h = _cexds__hash_bytes(buf + 4, 4, 0);
    tassert_eq(h, 14843858323657194021UL);

    h = _cexds__hash_bytes(buf + 4, 4, h);
    tassert_eq(h, 9392861481796581258UL);

    h = _cexds__hash_bytes(buf, 8, 0);
    tassert_eq(h, 11948792088630395043UL);

    return EOK;
}

test$main();
