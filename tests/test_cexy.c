#define CEX_LOG_LVL 5 /* 0 (mute all) - 5 (log$trace) */
#define TBUILDDIR "tests/build/cexytest/"
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#define cexy$build_dir TBUILDDIR
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

test$case(test_target_make)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR "my_tgt_dir/my_tgt";
        char* src = TBUILDDIR "my_src.c";
        e$assert(!os.path.exists(TBUILDDIR "my_tgt_dir/") && "must not exist!");

        e$ret(io.file.save(src, "#include <my_src2.c>"));
        char* tgt_file = cexy.target_make(src, TBUILDDIR "my_tgt_dir", "my_tgt", _);
        e$assert(os.path.exists(TBUILDDIR "my_tgt_dir/") && "must exist!");
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(tgt_file, TBUILDDIR "my_tgt_dir\\my_tgt.exe");
        } else {
            tassert_eq(tgt_file, tgt);
        }
        e$assert(!os.path.exists(tgt_file) && "must not exist!");
    }
    return EOK;
}

test$case(test_target_make_with_ext)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "my_src/my_src.c";
        e$assert(!os.path.exists(TBUILDDIR "my_src/") && "must not exist!");
        e$assert(!os.path.exists(TBUILDDIR "my_tgt/") && "must not exist!");
        e$ret(os.fs.mkpath(src));


        e$ret(io.file.save(src, "#include <my_src2.c>"));
        tassert(!os.path.exists(TBUILDDIR "my_tgt_dir/"));
        char* tgt_file = cexy.target_make(src, TBUILDDIR "my_tgt_dir", ".test", _);
        tassert(tgt_file != NULL);
        tassert(os.path.exists(TBUILDDIR "my_tgt_dir/") && "must exist!");
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(tgt_file, TBUILDDIR "my_tgt_dir\\" TBUILDDIR "my_src/my_src.c.test.exe");
        } else {
            tassert_eq(tgt_file, TBUILDDIR "my_tgt_dir/" TBUILDDIR "my_src/my_src.c.test");
        }
        e$assert(!os.path.exists(tgt_file) && "must not exist!");
    }
    return EOK;
}

test$case(test_target_git_hash)
{
    mem$scope(tmem$, _)
    {
        char* gh = cexy.utils.git_hash(_);
        tassert_ne(gh, NULL);
        tassert_ne(gh, "");
        tassert_ge(str.len(gh), 20);
    }
    return EOK;
}

test$case(test_needs_build_invalid)
{
    mem$scope(tmem$, _)
    {
        tassert_eq(0, cexy.src_changed(NULL, NULL, 0));
        tassert_eq(0, cexy.src_changed("", NULL, 0));

        char* tgt = TBUILDDIR " my_tgt";
        arr$(char*) src = arr$new(src, _);
        arr$pushm(src, TBUILDDIR "my_tgt.c");
        tassert_eq(0, cexy.src_changed("", src, arr$len(src)));

        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));
    }
    return EOK;
}
test$case(test_needs_build)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR " my_tgt";
        arr$(char*) src = arr$new(src, _);

        char* src_file = TBUILDDIR "my_tgt.c";
        arr$pushm(src, src_file);

        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        tassert_eq(1, cexy.src_changed(tgt, src, arr$len(src)));

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));
        tassert_eq(0, cexy.src_changed(tgt, &src_file, 1));
        os.sleep(1.5);
        tassert_eq(0, cexy.src_changed(tgt, src, arr$len(src)));
        e$ret(io.file.save(src[0], "// world"));
        tassert_eq(1, cexy.src_changed(tgt, src, arr$len(src)));
    }
    return EOK;
}

test$case(test_needs_build_many_files_not_exists)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR " my_tgt";
        arr$(char*) src = arr$new(src, _);
        arr$pushm(src, TBUILDDIR "my_src1.c", TBUILDDIR "my_src2");

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));

        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        tassert_eq(0, cexy.src_changed(tgt, src, arr$len(src)));
    }
    return EOK;
}

test$case(test_needs_build_many_files)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR " my_tgt";
        arr$(char*) src = arr$new(src, _);
        arr$pushm(src, TBUILDDIR "my_src1.c", TBUILDDIR "my_src2.c");
        e$ret(io.file.save(src[0], "// hello"));
        e$ret(io.file.save(src[1], "// world"));
        e$ret(io.file.save(tgt, ""));
        tassert_eq(0, cexy.src_changed(tgt, src, arr$len(src)));

        os.sleep(1.5);
        tassert_eq(0, cexy.src_changed(tgt, src, arr$len(src)));
        e$ret(io.file.save(src[1], "// world again"));
        tassert_eq(1, cexy.src_changed(tgt, src, arr$len(src)));
    }
    return EOK;
}

test$case(test_src_changed_include_direct_changes)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* tgt = TBUILDDIR " my_tgt";
        char* src = TBUILDDIR "my_src.c";
        tassert_eq(0, cexy.src_include_changed(NULL, src, NULL));
        tassert_eq(0, cexy.src_include_changed(tgt, NULL, NULL));
        tassert_eq(0, cexy.src_include_changed(TBUILDDIR, NULL, NULL)); // directory target
        tassert_eq(0, cexy.src_include_changed(TBUILDDIR, src, NULL));
        tassert_eq(0, cexy.src_include_changed(tgt, TBUILDDIR, NULL));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        e$ret(io.file.save(src, "// world"));
        tassert_eq(1, cexy.src_include_changed(tgt, src, NULL));

        e$ret(io.file.save(tgt, ""));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        os.sleep(1.5);
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
        e$ret(io.file.save(src, "// world again"));
        tassert_eq(1, cexy.src_include_changed(tgt, src, NULL));
    }
    return EOK;
}

test$case(test_src_changed_include)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* tgt = TBUILDDIR "my_tgt";
        char* src = TBUILDDIR "my_src.c";
        char* src2 = TBUILDDIR "my_src2.c";

        e$ret(io.file.save(tgt, ""));
        e$ret(io.file.save(src, "#include \"my_src2.c\""));
        e$ret(io.file.save(src2, "// I am include"));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        os.sleep(1.5);
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
        e$ret(io.file.save(src2, "// I am include again"));
        tassert_eq(1, cexy.src_include_changed(tgt, src, NULL));
    }
    return EOK;
}

test$case(test_src_changed_include_not_in_include_path)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* tgt = TBUILDDIR "t1/my_tgt";
        char* src = TBUILDDIR "t1/my_src.c";
        char* src2 = TBUILDDIR "t1/my_src2.c";
        e$ret(os.fs.mkdir(TBUILDDIR "t1/"));

        e$ret(io.file.save(tgt, ""));
        e$ret(io.file.save(src, "#include \"my_src2.c\""));
        e$ret(io.file.save(src2, "// I am include"));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        os.sleep(1.5);
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
        e$ret(io.file.save(src2, "// I am include again"));
        tassert_eq(1, cexy.src_include_changed(tgt, src, NULL));
    }
    return EOK;
}

test$case(test_src_changed_include_skips_system)
{
    char* tgt = TBUILDDIR "my_tgt";
    char* src = TBUILDDIR "my_src.c";
    char* src2 = TBUILDDIR "my_src2.c";

    e$ret(io.file.save(tgt, ""));
    e$ret(io.file.save(src, "#include <my_src2.c>"));
    e$ret(io.file.save(src2, "// I am include"));
    tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

    os.sleep(1.5);
    tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
    e$ret(io.file.save(src2, "// I am include again"));
    tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
    return EOK;
}

test$case(test_process_fn_match)
{
    tassert_eq(true, _cexy__fn_match(str$s("ns_foo"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("cex_ns_foo"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("cex_ns__foo__asd"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("cex_ns__foo__a_sd"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("ns__foo__asd"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("ns__foo__a_sd"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("ns__fo_o__a_sd"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("ns__foo___a_sd"), str$s("ns")));
    tassert_eq(true, _cexy__fn_match(str$s("cex_ns__foo"), str$s("ns")));

    tassert_eq(false, _cexy__fn_match(str$s("cex_ns__foo__"), str$s("ns")));
    tassert_eq(false, _cexy__fn_match(str$s("ns__foo__"), str$s("ns")));

    tassert_eq(false, _cexy__fn_match(str$s("ns_foo"), str$s("ns_")));
    tassert_eq(false, _cexy__fn_match(str$s("ns_foo_"), str$s("ns")));
    tassert_eq(false, _cexy__fn_match(str$s("_ns_foo"), str$s("ns")));
    tassert_eq(false, _cexy__fn_match(str$s("cex__ns_foo"), str$s("ns")));
    tassert_eq(false, _cexy__fn_match(str$s("_ns_foo"), str$s("ns")));
    tassert_eq(false, _cexy__fn_match(str$s("_cex_ns_foo"), str$s("ns")));

    return EOK;
}

test$case(test_process_fn_subnamespace)
{
    tassert_eq((str_s){0}, _cexy__fn_subnamespace(str$s("ns_foo"), str$s("ns")));
    tassert_eq(str$s("foo"), _cexy__fn_subnamespace(str$s("ns__foo__bar"), str$s("ns")));
    tassert_eq(str$s("fo_o"), _cexy__fn_subnamespace(str$s("ns__fo_o__bar"), str$s("ns")));
    tassert_eq(str$s("foo"), _cexy__fn_subnamespace(str$s("cex_ns__foo__bar"), str$s("ns")));
    tassert_eq(str$s("foo"), _cexy__fn_subnamespace(str$s("cex_ns__foo__bar_baz"), str$s("ns")));
    tassert_eq(str$s("ns_foo"), _cexy__fn_subnamespace(str$s("cex_ns__ns_foo__bar_baz"), str$s("ns")));

    tassert_eq((str_s){0}, _cexy__fn_subnamespace(str$s("ns__foo__bar"), str$s("bar")));
    tassert_eq((str_s){0}, _cexy__fn_subnamespace(str$s("ns__foo__bar"), str$s("foo")));

    return EOK;
}

test$case(test_lib_fetch_check_args)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* paths[] = { "cex.h", "cexstd/random", "cexstd/testing/fff.h" }; 
        tassert_er(
            Error.argument,
            cexy.utils.git_lib_fetch("", "HEAD", TBUILDDIR, false, true, paths, arr$len(paths))
        );
        tassert_er(
            Error.argument,
            cexy.utils.git_lib_fetch(NULL, "HEAD", TBUILDDIR, false, true, paths, arr$len(paths))
        );

        arr$(char*) paths2 = arr$new(paths2, _);
        arr$pushm(paths2, "a", "b", "c");
        tassert_er(
            Error.argument,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.gi",
                "HEAD",
                TBUILDDIR,
                false,
                true,
                paths2,
                arr$len(paths2)
            )
        );
    }
    return EOK;
}

test$case(test_git_lib_fetch)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* paths[] = { "cex.h", "cexstd/random", "cexstd/testing/fff.h" };
        tassert(!os.path.exists(TBUILDDIR "/out/"));
        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                "HEAD",
                TBUILDDIR "out/",
                false,
                true,
                paths,
                arr$len(paths)
            )
        );

        tassert(os.path.exists(TBUILDDIR "/out/"));
        tassert(os.path.exists(TBUILDDIR "/out/cex.h"));
        tassert(os.path.exists(TBUILDDIR "/out/cexstd/testing/fff.h"));
        tassert(os.path.exists(TBUILDDIR "/out/cexstd/random/Random.c"));
        tassert(os.path.exists(TBUILDDIR "/out/cexstd/random/Random.h"));

        // cwd changed back
        tassert(os.path.exists("cex.h"));
        tassert(os.path.exists("cex.c"));
        tassert(os.path.exists("tests/"));
    }
    return EOK;
}

test$case(test_git_lib_fetch_no_preserve_dirs)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* paths[] = { "cex.h", "cexstd/random", "cexstd/testing/fff.h" };
        tassert(!os.path.exists(TBUILDDIR "/out/"));

        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                NULL,
                TBUILDDIR "out/",
                false,
                false,
                paths,
                arr$len(paths)
            )
        );

        tassert(os.path.exists(TBUILDDIR "/out/"));
        tassert(os.path.exists(TBUILDDIR "/out/cex.h"));
        tassert(os.path.exists(TBUILDDIR "/out/fff.h"));
        tassert(os.path.exists(TBUILDDIR "/out/random/Random.c"));
        tassert(os.path.exists(TBUILDDIR "/out/random/Random.h"));
    }
    return EOK;
}

test$case(test_git_lib_fetch_no_rewrite)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* paths[] = { "cex.h", "cexstd/random", "cexstd/testing/fff.h" };
        tassert(!os.path.exists(TBUILDDIR "/out/"));

        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                NULL,
                TBUILDDIR "out/",
                false,
                false,
                paths,
                arr$len(paths)
            )
        );

        tassert(os.path.exists(TBUILDDIR "/out/"));
        tassert(os.path.exists(TBUILDDIR "/out/cex.h"));
        tassert(os.path.exists(TBUILDDIR "/out/fff.h"));
        tassert(os.path.exists(TBUILDDIR "/out/random/Random.c"));
        tassert(os.path.exists(TBUILDDIR "/out/random/Random.h"));

        u32 nfiles = 0;
        for$each (it, os.fs.find(TBUILDDIR "/out/*.*", true, _)) {
            nfiles++;
            auto stat = os.fs.stat(it);
            tassert(stat.is_valid);
            tassert(stat.size > 100);

            // "Edit" files
            tassert_er(EOK, io.file.save(it, "foo"));
        }
        tassert_ge(nfiles, 4);

        // This run should be dry (all exist all ignored)
        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                NULL,
                TBUILDDIR "out/",
                false,
                false,
                paths,
                arr$len(paths)
            )
        );

        nfiles = 0;
        for$each (it, os.fs.find(TBUILDDIR "/out/*.*", true, _)) {
            nfiles++;
            auto stat = os.fs.stat(it);
            tassert(stat.is_valid);
            tassert_eq(stat.size, 3);
        }
        tassert_ge(nfiles, 4);

        // Remove cex.h and make lib update again
        tassert_eq(EOK, os.fs.remove(TBUILDDIR "/out/cex.h"));
        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                NULL,
                TBUILDDIR "out/",
                false,
                false,
                paths,
                arr$len(paths)
            )
        );
        nfiles = 0;
        for$each (it, os.fs.find(TBUILDDIR "/out/*.*", true, _)) {
            nfiles++;
            auto stat = os.fs.stat(it);
            tassert(stat.is_valid);
            // cex.h got updated, because it didn't exist
            if (str.ends_with(it, "cex.h")) {
                tassert_gt(stat.size, 100);
            } else {
                tassert_eq(stat.size, 3);
            }
        }
        tassert_ge(nfiles, 4);
    }
    return EOK;
}

test$case(test_git_lib_fetch_update)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* paths[] = { "cex.h", "cexstd/random", "cexstd/testing/fff.h" };
        tassert(!os.path.exists(TBUILDDIR "/out/"));

        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                NULL,
                TBUILDDIR "out/",
                false,
                false,
                paths,
                arr$len(paths)
            )
        );

        tassert(os.path.exists(TBUILDDIR "/out/"));
        tassert(os.path.exists(TBUILDDIR "/out/cex.h"));
        tassert(os.path.exists(TBUILDDIR "/out/fff.h"));
        tassert(os.path.exists(TBUILDDIR "/out/random/Random.c"));
        tassert(os.path.exists(TBUILDDIR "/out/random/Random.h"));

        u32 nfiles = 0;
        for$each (it, os.fs.find(TBUILDDIR "/out/*.*", true, _)) {
            nfiles++;
            auto stat = os.fs.stat(it);
            tassert(stat.is_valid);
            tassert(stat.size > 100);

            // "Edit" files
            tassert_er(EOK, io.file.save(it, "foo"));
        }
        tassert_ge(nfiles, 4);
        tassert_eq(EOK, io.file.save(TBUILDDIR "/out/random/MyFile.h", "bar"));


        // This run should be dry (all exist all ignored)
        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                NULL,
                TBUILDDIR "out/",
                false,
                false,
                paths,
                arr$len(paths)
            )
        );

        nfiles = 0;
        for$each (it, os.fs.find(TBUILDDIR "/out/*.*", true, _)) {
            nfiles++;
            auto stat = os.fs.stat(it);
            tassert(stat.is_valid);
            tassert_eq(stat.size, 3);
        }
        tassert_ge(nfiles, 5);

        tassert_er(
            Error.ok,
            cexy.utils.git_lib_fetch(
                "https://github.com/alexveden/cex.git",
                NULL,
                TBUILDDIR "out/",
                true, // update
                false,
                paths,
                arr$len(paths)
            )
        );

        nfiles = 0;
        tassert(os.path.exists(TBUILDDIR "/out/random/MyFile.h"));
        for$each (it, os.fs.find(TBUILDDIR "/out/*.*", true, _)) {
            nfiles++;
            auto stat = os.fs.stat(it);
            tassert(stat.is_valid);
            if (str.ends_with(it, "MyFile.h")) {
                tassert_eq(stat.size, 3);
            } else {
                tassert_gt(stat.size, 100);
            }
        }
        tassert_ge(nfiles, 5);
    }
    return EOK;
}

#else
test$case(not_supported_by_platform)
{
    return EOK;
}
#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
