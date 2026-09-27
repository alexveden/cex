#define TBUILDDIR "tests/build/cexytest/"
#define CEX_LOG_LVL 4
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#include "src/all.c"

test$setup_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    e$assert(!os.path.exists(TBUILDDIR) && "must not exist!");
    e$ret(os.fs.mkpath(TBUILDDIR));
    e$assert(os.path.exists(TBUILDDIR) && "must exist!");
    return EOK;
}
test$teardown_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    return EOK;
}

#if !defined(__EMSCRIPTEN__)

test$case(test_namespace_entities)
{
#define $file "src/str"
    mem$scope(tmem$, _)
    {

        arr$(cex_token_s) items = arr$new(items, _);
        char* code_c = io.file.load($file ".c", _);
        char* code_h = io.file.load($file ".h", _);
        tassert(code_c);
        tassert(code_h);

        log$debug("Header symbols: %s\n", $file ".h");
        CexParser_c lx = CexParser.create(code_h, 0, true);
        cex_token_s t;
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            log$debug(
                "Decl: type: '%s' name: %S doc_len: %d body_len: %d ret: %s args: %s\n",
                CexTkn_str[d->type],
                d->name,
                d->docs.len,
                d->body.len,
                d->ret_type,
                d->args
            );
        }

        log$debug("\nSource symbols: %s\n", $file ".c");
        lx = CexParser.create(code_c, 0, true);
        while ((t = CexParser.next_entity(&lx, &items)).type) {
            cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
            if (d == NULL) { continue; }
            log$debug(
                "Decl: type: '%s' name: %S doc_len: %d body_len: %d ret: %s args: %s\n",
                CexTkn_str[t.type],
                d->name,
                d->docs.len,
                d->body.len,
                d->ret_type,
                d->args
            );
        }
        // tassert(false);
    }
#undef $file
    return EOK;
}

#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
