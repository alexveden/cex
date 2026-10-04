#define CEX_LOG_LVL 5 /* 0 (mute all) - 5 (log$trace) */
#define TBUILDDIR "tests/build/cexytest/"
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#define cexy$build_dir TBUILDDIR
#define cexy$disable_cex_precompiling
#define cexy$cc_args_sanitizer "-fstack-protector-strong"
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

test$case(test_target_make_oom)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        char* src = TBUILDDIR "my_src.c";
        e$ret(io.file.save(src, "#include <my_src2.c>"));

        test$alloc_set_oom_on_call(1);
        char* tgt_file = cexy.target_make(src, TBUILDDIR "my_tgt_dir", "my_tgt", test$alloc);
        test$alloc_set_oom_on_call(0);
        tassert(tgt_file == NULL);
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

test$case(test_find_app_target_src_null_out)
{
    tassert_er(Error.assert, cexy.app.find_app_target_src(mem$, "foo", NULL));
    return EOK;
}

test$case(test_coverage_bench_rejected)
{
    char* argv[] = { "test", "--coverage", "bench", "tests/test_cexy.c" };
    tassert_er(Error.argument, cexy.cmd.simple_test(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_coverage_engine_invalid)
{
    char* argv[] = {
        "test", "--coverage", "--coverage-engine", "bogus", "build", "tests/test_cexy.c"
    };
    tassert_er(Error.argument, cexy.cmd.simple_test(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_coverage_engine_llvm_requires_clang)
{
    char* cc[] = { cexy$cc };
    if (str.find(cc[0], "clang")) { return EOK; }

    char* argv[] = {
        "test", "--coverage", "--coverage-engine", "llvm", "build", "tests/test_cexy.c"
    };
    tassert_er(Error.argument, cexy.cmd.simple_test(arr$len(argv), argv, NULL));
    return EOK;
}

#if !defined(__clang__) && !defined(_WIN32)

test$case(test_coverage_flag)
{
    mem$scope(tmem$, _)
    {
        char* cc[] = { cexy$cc };
        if (str.find(cc[0], "clang")) { return EOK; }

        char* src = TBUILDDIR "test_cov_tmp.c";
        e$ret(io.file.save(src, "int main(void) { return 0; }\n"));

        char* test_target = cexy.target_make(src, cexy$build_dir, ".test", _);
        char* gcno_glob = str.fmt(_, "%s-*.gcno", test_target);

        char* argv[] = { "test", "--coverage", "build", src };
        e$ret(cexy.cmd.simple_test(arr$len(argv), argv, NULL));
        tassert(arr$len(os.fs.find(gcno_glob, false, _)) > 0);

        e$ret(cexy.test.clean(src));
        tassert(arr$len(os.fs.find(gcno_glob, false, _)) == 0);
    }
    return EOK;
}

#endif  // #if !defined(__clang__) && !defined(_WIN32)

static bool
test_cexy_mock_isatty_true(FILE* file)
{
    (void)file;
    return true;
}

test$case(test_colorize_ansi_branches)
{
    tassert_eq((char*)_cexy__colorize_ansi((str_s){ 0 }, str$s("foo"), 0), "\033[0m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("foo"), str$s("foo"), 0), "\033[1;31m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("return"), str$s("foo"), 0), "\033[1;33m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("i32"), str$s("foo"), 0), "\033[1;32m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("bar"), str$s("foo"), '('), "\033[1;34m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("my_s"), str$s("foo"), 0), "\033[1;32m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("my_e"), str$s("foo"), 0), "\033[1;32m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("my_c"), str$s("foo"), 0), "\033[1;32m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("my_kw"), str$s("foo"), 0), "\033[1;32m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("#define"), str$s("foo"), 0), "\033[1;35m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("mem$"), str$s("foo"), 0), "\033[1;33m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("some$thing"), str$s("foo"), 0), "\033[33m");
    tassert_eq((char*)_cexy__colorize_ansi(str$s("blah"), str$s("foo"), 0), "\033[0m");
    return EOK;
}

test$case(test_colorize_print_tty)
{
    mem$scope(tmem$, _)
    {
        char* path = TBUILDDIR "colorize_out.txt";
        FILE* out = NULL;
        e$ret(io.fopen(&out, path, "w"));
        test$mock_scope(io) {
            io.isatty = test_cexy_mock_isatty_true;
            _cexy__colorize_print(str$s("i32 foo(int x);"), str$s("target"), out);
        }
        io.fclose(&out);

        char* got = io.file.load(path, _);
        tassert(got != NULL);
        tassert(str.find(got, "\033[1;32mi32\033[0m") != NULL);
        tassert(str.find(got, "\033[1;34mfoo\033[0m") != NULL);

        // non-tty path prints plain text
        FILE* plain = NULL;
        e$ret(io.fopen(&plain, path, "w"));
        _cexy__colorize_print(str$s("plain text"), str$s("plain"), plain);
        io.fclose(&plain);
        got = io.file.load(path, _);
        tassert_eq(got, "plain text");
    }
    return EOK;
}

test$case(test_pkgconf_parse_tokens)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);

        tassert_er(EOK, _cexy__utils__pkgconf_parse(_, &args, "-I/foo  -DBAR\tbaz\n"));
        tassert_eq((int)arr$len(args), 3);
        tassert_eq(args[0], "-I/foo");
        tassert_eq(args[1], "-DBAR");
        tassert_eq(args[2], "baz");

        arr$clear(args);
        tassert_er(EOK, _cexy__utils__pkgconf_parse(_, &args, "-I\"a b\" -DSINGLE='x y'"));
        tassert_eq((int)arr$len(args), 2);
        tassert_eq(args[0], "-I\"a b\"");
        tassert_eq(args[1], "-DSINGLE='x y'");

        arr$clear(args);
        tassert_er(EOK, _cexy__utils__pkgconf_parse(_, &args, "a\\ b c"));
        tassert_eq((int)arr$len(args), 2);
        tassert_eq(args[0], "a\\ b");
        tassert_eq(args[1], "c");

        arr$clear(args);
        tassert_er(EOK, _cexy__utils__pkgconf_parse(_, &args, "   "));
        tassert_eq((int)arr$len(args), 0);

        tassert_er(Error.assert, _cexy__utils__pkgconf_parse(_, &args, NULL));
    }
    return EOK;
}

test$case(test_make_compile_flags)
{
    mem$scope(tmem$, _)
    {
        char* flags_file = TBUILDDIR "compile_flags.txt";

        tassert_er(
            Error.assert,
            cexy.utils.make_compile_flags(TBUILDDIR "not_flags.txt", false, NULL)
        );
        tassert_er(Error.null_or_empty, cexy.utils.make_compile_flags(flags_file, false, NULL));

        arr$(char*) flags = arr$new(flags, _);
        arr$pushm(flags, "-Wall", "-fsanitize=address", "-I./foo");
        tassert_er(EOK, cexy.utils.make_compile_flags(flags_file, false, flags));

        char* content = io.file.load(flags_file, _);
        tassert(content != NULL);
        tassert(str.find(content, "-Wall") != NULL);
        tassert(str.find(content, "-I./foo") != NULL);
        tassert(str.find(content, "-fsanitize") == NULL);

        tassert_er(EOK, cexy.utils.make_compile_flags(flags_file, true, NULL));
        content = io.file.load(flags_file, _);
        tassert(content != NULL);
        tassert(str.len(content) > 0);
    }
    return EOK;
}

test$case(test_cmd_config)
{
    char* argv[] = { "config" };
    tassert_er(EOK, cexy.cmd.config(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_cmd_stats)
{
    mem$scope(tmem$, _)
    {
        (void)_;
        e$ret(io.file.save(TBUILDDIR "stat_src.c", "// comment\nint foo(void) { return 1; }\n"));
        e$ret(io.file.save(
            TBUILDDIR "stat_test.c", "int test_foo(void) { uassert(1); return 1; }\n"
        ));

        char* argv[] = { "stats", "-v", TBUILDDIR "stat_*.c" };
        tassert_er(EOK, cexy.cmd.stats(arr$len(argv), argv, NULL));
    }
    return EOK;
}

test$case(test_cmd_libfetch_error)
{
    char* argv[] = { "libfetch", "-u", "", "some.h" };
    tassert_er(Error.argument, cexy.cmd.libfetch(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_target_make_errors)
{
    mem$scope(tmem$, _)
    {
        tassert(cexy.target_make(NULL, TBUILDDIR, "x", _) == NULL);
        tassert(cexy.target_make("", TBUILDDIR, "x", _) == NULL);
        tassert(cexy.target_make(TBUILDDIR "nope.c", NULL, "x", _) == NULL);
        tassert(cexy.target_make(TBUILDDIR "nope.c", TBUILDDIR, NULL, _) == NULL);
        tassert(cexy.target_make(TBUILDDIR "nope.c", TBUILDDIR, "", _) == NULL);
        tassert(cexy.target_make(TBUILDDIR "nope.c", TBUILDDIR, "x", _) == NULL);
    }
    return EOK;
}

test$case(test_create_errors)
{
    char* target = TBUILDDIR "test_cexytest_created.c";
    e$ret(cexy.test.create(target, false));
    tassert(os.path.exists(target));
    tassert_er(Error.exists, cexy.test.create(target, false));
    tassert_er(Error.argument, cexy.test.create("all", false));
    tassert_er(Error.argument, cexy.test.create(TBUILDDIR "test_*.c", false));
    return EOK;
}

test$case(test_clean_errors)
{
    e$ret(os.fs.mkpath(TBUILDDIR "tests/"));
    tassert_er(EOK, cexy.test.clean("all"));
    tassert_er(Error.exists, cexy.test.clean(TBUILDDIR "test_nonexistent_xyz.c"));
    return EOK;
}

test$case(test_make_target_pattern_errors)
{
    tassert_er(Error.argsparse, cexy.test.make_target_pattern(NULL));

    char* bad = "src/foo.c";
    tassert_er(Error.argsparse, cexy.test.make_target_pattern(&bad));

    char* all = "all";
    tassert_er(EOK, cexy.test.make_target_pattern(&all));
    tassert_eq(all, "tests/test_*.c");
    return EOK;
}

test$case(test_test_run_unsupported_cmd)
{
    tassert_er(Error.argument, cexy.test.run("tests/test_cexy.c", "bogus", 0, NULL));
    return EOK;
}

test$case(test_process_errors)
{
    char* noarg[] = { "process", NULL };
    tassert_er(Error.argsparse, cexy.cmd.process(arr$len(noarg) - 1, noarg, NULL));

    char* missing[] = { "process", TBUILDDIR "nope.c" };
    tassert_er(Error.not_found, cexy.cmd.process(arr$len(missing), missing, NULL));
    return EOK;
}

test$case(test_pkgconf)
{
    mem$scope(tmem$, _)
    {
        char* pc_dir = TBUILDDIR "pc/";
        e$ret(os.fs.mkpath(pc_dir));
        e$ret(io.file.save(
            os$path_join(_, pc_dir, "cextest.pc"),
            "prefix=/usr\n"
            "Name: cextest\n"
            "Description: test\n"
            "Version: 1.0\n"
            "Cflags: -I/foo/include\n"
            "Libs: -lcextest\n"
        ));

        char* old_pc_path = os.env.get("PKG_CONFIG_PATH", NULL);
        if (old_pc_path) { old_pc_path = str.clone(old_pc_path, _); }
        e$ret(os.env.set("PKG_CONFIG_PATH", pc_dir));

        arr$(char*) out = arr$new(out, _);
        tassert_er(EOK, cexy$pkgconf(_, &out, "--cflags", "cextest"));
        tassert(arr$len(out) > 0);
        tassert(str.find(out[0], "-I/foo/include") != NULL);

        if (old_pc_path) {
            e$ret(os.env.set("PKG_CONFIG_PATH", old_pc_path));
        } else {
            e$ret(os.env.unset("PKG_CONFIG_PATH"));
        }
    }
    return EOK;
}

test$case(test_cmd_stats_exclude_and_tokens)
{
    e$ret(io.file.save(
        TBUILDDIR "stat_src2.c",
        "#define FOO \\\n    1\n"
        "// single line comment\n"
        "/* multi\nline\ncomment */\n"
        "int foo(void) { return 1; }\n"
    ));
    e$ret(io.file.save(TBUILDDIR "stat_excl.c", "int e(void) { return 0; }\n"));

    char* argv[] = { "stats", "-v", TBUILDDIR "stat_*.c", "!" TBUILDDIR "stat_excl.c" };
    tassert_er(EOK, cexy.cmd.stats(arr$len(argv), argv, NULL));

    char* only_excl[] = { "stats", "!" };
    tassert_er(EOK, cexy.cmd.stats(arr$len(only_excl), only_excl, NULL));
    return EOK;
}

test$case(test_cmd_stats_parse_error)
{
    e$ret(io.file.save(TBUILDDIR "stat_bad.c", "\"hello\nworld\"\n"));
    char* argv[] = { "stats", TBUILDDIR "stat_bad.c" };
    tassert_er(Error.integrity, cexy.cmd.stats(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_simple_test_invalid_command)
{
    char* argv[] = { "test", "bogus", "x" };
    tassert_er(Error.argsparse, cexy.cmd.simple_test(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_simple_test_create_clean_cli)
{
    char* target = TBUILDDIR "test_cli_created.c";
    char* create[] = { "test", "create", target };
    tassert_er(EOK, cexy.cmd.simple_test(arr$len(create), create, NULL));
    tassert(os.path.exists(target));

    e$ret(os.fs.mkpath(TBUILDDIR "tests/"));
    char* clean[] = { "test", "clean", "all" };
    tassert_er(EOK, cexy.cmd.simple_test(arr$len(clean), clean, NULL));
    return EOK;
}

test$case(test_simple_test_jobs_build)
{
    e$ret(cexy.test.create(TBUILDDIR "test_multi_a.c", false));
    e$ret(cexy.test.create(TBUILDDIR "test_multi_b.c", false));
    char* argv[] = { "test", "-j", "1", "build", TBUILDDIR "test_multi_*.c" };
    tassert_er(EOK, cexy.cmd.simple_test(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_simple_test_bench_build)
{
    char* target = TBUILDDIR "test_bench_cli.c";
    e$ret(cexy.test.create(target, false));
    char* argv[] = { "test", "bench", target };
    tassert_er(EOK, cexy.cmd.simple_test(arr$len(argv), argv, NULL));
    return EOK;
}

test$case(test_simple_test_coverage_run)
{
    char* src = TBUILDDIR "test_covrun.c";
    e$ret(io.file.save(
        src,
        "#define CEX_IMPLEMENTATION\n"
        "#define CEX_TEST\n"
        "#include \"cex.h\"\n"
        "test$case(ok) { tassert_eq(1, 1); return EOK; }\n"
        "test$main();\n"
    ));
    char* argv[] = { "test", "--coverage", "run", src };
    tassert_er(EOK, cexy.cmd.simple_test(arr$len(argv), argv, NULL));
    return EOK;
}

static bool
test_cexy_mock_cmd_exists_false(char* cmd_exe)
{
    (void)cmd_exe;
    return false;
}

static Exception
test_cexy_mock_cmd_create_ok(os_cmd_c* self, char** args, usize args_len, os_cmd_flags_s* flags)
{
    (void)self;
    (void)args;
    (void)args_len;
    (void)flags;
    return EOK;
}

static Exception
test_cexy_mock_cmd_create_fail(os_cmd_c* self, char** args, usize args_len, os_cmd_flags_s* flags)
{
    (void)self;
    (void)args;
    (void)args_len;
    (void)flags;
    return Error.runtime;
}

static Exception
test_cexy_mock_cmd_wait_ok(os_cmd_c* procs, usize cnt, f64 timeout_sec)
{
    (void)procs;
    (void)cnt;
    (void)timeout_sec;
    return EOK;
}

static Exception
test_cexy_mock_cmd_wait_fail(os_cmd_c* procs, usize cnt, f64 timeout_sec)
{
    (void)procs;
    (void)cnt;
    (void)timeout_sec;
    return Error.runtime;
}

static char*
test_cexy_mock_cmd_read_empty(os_cmd_c* self, IAllocator allc)
{
    (void)self;
    (void)allc;
    return "";
}

static char*
test_cexy_mock_cmd_read_long(os_cmd_c* self, IAllocator allc)
{
    (void)self;
    (void)allc;
    return "0123456789012345678901234567890123456789012345678901234567890";
}

static char*
test_cexy_mock_cmd_read_nonhex(os_cmd_c* self, IAllocator allc)
{
    (void)self;
    (void)allc;
    return "zzzz";
}

test$case(test_git_hash_error_paths)
{
    mem$scope(tmem$, _)
    {
        test$mock_scope(os) {
            os.cmd.exists = test_cexy_mock_cmd_exists_false;
            tassert_eq(cexy.utils.git_hash(_), NULL);
        }
        test$mock_scope(os) {
            os.cmd.create = test_cexy_mock_cmd_create_fail;
            tassert_eq(cexy.utils.git_hash(_), NULL);
        }
        test$mock_scope(os) {
            os.cmd.create = test_cexy_mock_cmd_create_ok;
            os.cmd.wait = test_cexy_mock_cmd_wait_fail;
            tassert_eq(cexy.utils.git_hash(_), NULL);
        }
        test$mock_scope(os) {
            os.cmd.create = test_cexy_mock_cmd_create_ok;
            os.cmd.wait = test_cexy_mock_cmd_wait_ok;
            os.cmd.read_all = test_cexy_mock_cmd_read_empty;
            tassert_eq(cexy.utils.git_hash(_), NULL);
        }
        test$mock_scope(os) {
            os.cmd.create = test_cexy_mock_cmd_create_ok;
            os.cmd.wait = test_cexy_mock_cmd_wait_ok;
            os.cmd.read_all = test_cexy_mock_cmd_read_long;
            tassert_eq(cexy.utils.git_hash(_), NULL);
        }
        test$mock_scope(os) {
            os.cmd.create = test_cexy_mock_cmd_create_ok;
            os.cmd.wait = test_cexy_mock_cmd_wait_ok;
            os.cmd.read_all = test_cexy_mock_cmd_read_nonhex;
            tassert_eq(cexy.utils.git_hash(_), NULL);
        }
    }
    return EOK;
}

test$case(test_src_changed_edges)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR "edge_tgt";
        char* src_file = TBUILDDIR "edge_src.c";
        e$ret(io.file.save(src_file, "// x\n"));
        char* srcs[] = { src_file };

        tassert_eq(0, cexy.src_changed(tgt, srcs, 0));   // empty array
        tassert_eq(0, cexy.src_changed(NULL, srcs, 1));  // NULL target
        tassert_eq(0, cexy.src_changed(TBUILDDIR, srcs, 1));  // dir target

        char* tgt2 = TBUILDDIR "edge_tgt2";
        e$ret(io.file.save(tgt2, ""));
        char* dirsrc[] = { TBUILDDIR };
        tassert_eq(0, cexy.src_changed(tgt2, dirsrc, 1));  // non-file src
    }
    return EOK;
}

test$case(test_src_include_changed_edges)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR "inc_tgt";
        char* src = TBUILDDIR "inc_src.c";
        e$ret(io.file.save(tgt, ""));
        e$ret(io.file.save(src, "#include \"missing.h\"\nint x(void) { return 0; }\n"));

        // directory target
        tassert_eq(0, cexy.src_include_changed(TBUILDDIR, src, NULL));

        // alt include path that does not exist
        arr$(char*) alt = arr$new(alt, _);
        arr$push(alt, TBUILDDIR "nope_dir/");
        tassert_eq(0, cexy.src_include_changed(tgt, src, alt));

        // bad include (too short to be a real path)
        e$ret(io.file.save(src, "#include <>\nint x(void) { return 0; }\n"));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        // non .c/.h source: only the mtime check runs
        char* src_txt = TBUILDDIR "inc_src.txt";
        e$ret(io.file.save(src_txt, "plain text\n"));
        os.sleep(1.2);
        char* tgt3 = TBUILDDIR "inc_tgt3";
        e$ret(io.file.save(tgt3, ""));
        tassert_eq(0, cexy.src_include_changed(tgt3, src_txt, NULL));
    }
    return EOK;
}

test$case(test_test_run_missing_target)
{
    tassert_er(Error.not_found, cexy.test.run(TBUILDDIR "test_nope.c", "run", 0, NULL));
    return EOK;
}

test$case(test_test_run_quiet)
{
    char* src = TBUILDDIR "test_runq.c";
    e$ret(io.file.save(
        src,
        "#define CEX_IMPLEMENTATION\n"
        "#define CEX_TEST\n"
        "#include \"cex.h\"\n"
        "test$case(ok) { tassert_eq(1, 1); return EOK; }\n"
        "test$main();\n"
    ));
    char* build[] = { "test", "build", src };
    tassert_er(EOK, cexy.cmd.simple_test(arr$len(build), build, NULL));

    char* noargs[] = { NULL };
    tassert_er(EOK, cexy.test.run(TBUILDDIR "test_*.c", "run", 0, noargs));
    return EOK;
}

test$case(test_test_run_failure)
{
    char* bad = TBUILDDIR "test_runbad.c";
    e$ret(io.file.save(
        bad,
        "#define CEX_IMPLEMENTATION\n"
        "#define CEX_TEST\n"
        "#include \"cex.h\"\n"
        "test$case(bad) { tassert_eq(1, 2); return EOK; }\n"
        "test$main();\n"
    ));
    char* build[] = { "test", "build", bad };
    tassert_er(EOK, cexy.cmd.simple_test(arr$len(build), build, NULL));

    char* noargs[] = { NULL };
    tassert_er(Error.runtime, cexy.test.run(bad, "run", 0, noargs));
    return EOK;
}

test$case(test_pkgconf_parse_escaped_quote)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        tassert_er(EOK, _cexy__utils__pkgconf_parse(_, &args, "-I\"a\\b\" -c"));
        tassert_eq((int)arr$len(args), 2);
        tassert_eq(args[0], "-I\"a\\b\"");
        tassert_eq(args[1], "-c");
    }
    return EOK;
}

test$case(test_colorize_print_token_edges)
{
    mem$scope(tmem$, _)
    {
        char* path = TBUILDDIR "colorize_edges.txt";
        FILE* out = NULL;
        e$ret(io.fopen(&out, path, "w"));
        test$mock_scope(io) {
            io.isatty = test_cexy_mock_isatty_true;
            _cexy__colorize_print(str$s("foo"), str$s("foo"), out);
            _cexy__colorize_print(str$s("foo (bar)"), str$s("foo"), out);
            _cexy__colorize_print(str$s("(*ptr)"), str$s("foo"), out);
        }
        io.fclose(&out);
        char* got = io.file.load(path, _);
        tassert(got != NULL);
        tassert(str.find(got, "foo") != NULL);
    }
    return EOK;
}

test$case(test_cmd_libfetch_ok_no_paths)
{
    char* argv[] = { "libfetch" };
    tassert_er(EOK, cexy.cmd.libfetch(arr$len(argv), argv, NULL));
    return EOK;
}

#endif  // #if !defined(__EMSCRIPTEN__)

test$main();
