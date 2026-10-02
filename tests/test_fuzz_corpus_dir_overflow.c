#include "src/all.c"

test$case(test_fuzz_corpus_dir_overflow)
{
    // 251 chars ending with ".c" -> "<name>_corpus" needs 257 bytes, overflows 256-byte buf
    char name[252] = { 0 };
    memset(name, 'a', 251);
    name[249] = '.';
    name[250] = 'c';

    tassert_eq((int)str.len(name), 251);
    tassert(fuzz.corpus_dir(name) == NULL);

    return EOK;
}

test$main();
