#include "src/all.c"

#define TBUILDDIR "tests/build/os_fs_test/"
test$setup_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    e$ret(os.fs.mkpath(TBUILDDIR));
    return EOK;
}
test$teardown_case()
{
    if (os.fs.remove_tree(TBUILDDIR)) {};
    return EOK;
}

Exception
test_dir_walk(char* path, os_fs_stat_s ftype, void* user_ctx)
{
    u32* cnt = (u32*)user_ctx;
    uassert(cnt != NULL);
    printf(
        "path walker: %s, is_dir: %d, is_link: %d, user_ctx: %p\n",
        path,
        ftype.is_directory,
        ftype.is_symlink,
        user_ctx
    );
    *cnt += 1;
    return EOK;
}

static char*
p(char* path, IAllocator allc)
{
    uassert(path);
    uassert(allc);

    sbuf_c buf = sbuf.create(4096, allc);

    for$iter (str_s, it, str.slice.iter_split(str.sstr(path), "/\\", &it.iterator)) {
        if (it.idx.i == 0) {
            if (sbuf.appendf(&buf, "%S", it.val)) { uassert(false && "appendf failed"); }
        } else {
            if (sbuf.appendf(&buf, "%c%S", os$PATH_SEP, it.val)) {
                uassert(false && "appendf failed");
            }
        }
    }
    return buf;
}

test$case(test_os_dir_walk_print)
{
    u32 ncalls = 0;

    // recursive walk (each file + directory + non recursive symlinks)
    mem$scope(tmem$, _)
    {
        char* path = p("tests/data/dir1", _);
        log$debug("Walking: %s\n", path);
        tassert_er(EOK, os.fs.dir_walk(path, true, test_dir_walk, &ncalls));

        if (os.platform.current() == OSPlatform__wasm) {
            tassert_ge(ncalls, 7); // emscripten doesn't walk symlink dirs
        } else if (os.platform.current() == OSPlatform__win) {
            tassert_ge(ncalls, 7);
        } else {
            tassert_eq(8, ncalls);
        }

        // non-recursive
        ncalls = 0;
        tassert_er(EOK, os.fs.dir_walk("tests/data/dir1", false, test_dir_walk, &ncalls));
        if (os.platform.current() == OSPlatform__wasm) {
            tassert_ge(ncalls, 4);
        } else if (os.platform.current() == OSPlatform__win) {
            tassert_ge(ncalls, 4);
        } else {
            tassert_eq(5, ncalls);
        }
    }

    return EOK;
}

test$case(test_os_find)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find(p("tests/data/dir1/", _), false, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);
        ;
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file1.csv", _), true);
        hm$set(exp_files, p("tests/data/dir1/file3.txt", _), true);
        if (os.platform.current() == OSPlatform__win || os.platform.current() == OSPlatform__wasm) {
            // Windows/WASM doesn't support symlinks - treat them as files
            hm$set(exp_files, p("tests/data/dir1/file1_symlink.csv", _), true);
        }

        u32 nit = 0;
        for$each (it, files) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_find_inside_foreach)
{

    mem$scope(tmem$, _)
    {
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file1.csv", _), true);
        hm$set(exp_files, p("tests/data/dir1/file3.txt", _), true);
        if (os.platform.current() == OSPlatform__win || os.platform.current() == OSPlatform__wasm) {
            // Windows doesn't support symlinks
            hm$set(exp_files, p("tests/data/dir1/file1_symlink.csv", _), true);
        }

        u32 nit = 0;
        for$each (it, os.fs.find(p("tests/data/dir1/", _), false, _)) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_find_exact)
{

    mem$scope(tmem$, _)
    {
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file1.csv", _), true);

        u32 nit = 0;
        for$each (it, os.fs.find(p("tests/data/dir1/file1.csv", _), false, _)) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_find_exact_recursive)
{

    mem$scope(tmem$, _)
    {
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file1.csv", _), true);

        u32 nit = 0;
        for$each (it, os.fs.find(p("tests/data/dir1/file1.csv", _), true, _)) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_find_bad_pattern)
{

    mem$scope(tmem$, _)
    {
        u32 nit = 0;
        for$each (it, os.fs.find("lakjdalksjdlkjzxcoiasdznxcas", true, _)) {
            log$debug("WTF Found: %s\n", it);
            nit++;
        }
        tassert_eq(nit, 0);
    }

    return EOK;
}

test$case(test_os_find_no_trailing_slash)
{

    mem$scope(tmem$, _)
    {
        // NOTE: when dir1 is a directory, and input path is without a pattern
        //       dir1 considered as a pattern
        arr$(char*) files = os.fs.find(p("tests/data/dir1", _), false, _);
        tassert(files != NULL);
        tassert_eq(arr$len(files), 0);
    }

    return EOK;
}

test$case(test_os_find_pattern)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find(p("tests/data/dir1/*.txt", _), false, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);

        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file3.txt", _), true);

        u32 nit = 0;
        for$each (it, files) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_find_pattern_glob)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find(p("tests/data/dir1/file?.(txt|csv)", _), false, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);
        ;
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file1.csv", _), true);
        hm$set(exp_files, p("tests/data/dir1/file3.txt", _), true);

        u32 nit = 0;
        for$each (it, files) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_find_pattern_with_relative_non_norm_path)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find(p("tests/data/../../tests/data/dir1/*.txt", _), false, _);
        tassert(files != NULL);
        tassert_eq(arr$len(files), 1);
    }

    return EOK;
}

test$case(test_os_find_recursive)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find(p("tests/data/dir1/*.csv", _), true, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);
        ;
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file1.csv", _), true);
        hm$set(exp_files, p("tests/data/dir1/dir2/dir3/file4.csv", _), true);
        hm$set(exp_files, p("tests/data/dir1/dir2/file2.csv", _), true);
        if (os.platform.current() == OSPlatform__win) {
            // Windows doesn't support symlinks
            hm$set(exp_files, p("tests/data/dir1/file1_symlink.csv", _), true);
            hm$set(exp_files, p("tests/data/dir1/dir2_symlink/dir3/file4.csv", _), true);
            // hm$set(exp_files, p("tests/data/dir1/dir2_symlink/dir3", _), true);
            hm$set(exp_files, p("tests/data/dir1/dir2_symlink/file2.csv", _), true);
            hm$set(exp_files, p("tests/data/dir1/file1_symlink.csv", _), true);
        } else if (os.platform.current() == OSPlatform__wasm) {
            hm$set(exp_files, p("tests/data/dir1/file1_symlink.csv", _), true);
        }

        u32 nit = 0;
        for$each (it, files) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_find_direct_match)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find(p("tests/data/dir1/file1.csv", _), true, _);
        tassert(files != NULL);
        tassert_eq(arr$len(files), 1);
        ;
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, p("tests/data/dir1/file1.csv", _), true);

        u32 nit = 0;
        for$each (it, files) {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

    return EOK;
}

test$case(test_os_getcwd)
{
    mem$scope(tmem$, _)
    {
        auto p = os.fs.getcwd(_);
        tassert(p != NULL);
        if (os.platform.current() == OSPlatform__wasm) {
            tassert_eq(true, str.ends_with(p, "/"));
        } else {
            tassert_eq(true, str.ends_with(p, "cex"));
        }

        tassert_er(EOK, os.fs.chdir("tests"));
        p = os.fs.getcwd(_);
        tassert_eq(true, str.ends_with(p, "tests"));
        tassert_er(EOK, os.fs.chdir(".."));

        p = os.fs.getcwd(_);
        if (os.platform.current() == OSPlatform__wasm) {
            tassert_eq(true, str.ends_with(p, "/"));
        } else {
            tassert_eq(true, str.ends_with(p, "cex"));
        }
    }

    return EOK;
}

test$case(test_os_path_exists)
{
    tassert_eq(0, os.path.exists(NULL));
    tassert_eq(0, os.path.exists(""));
    tassert_eq(1, os.path.exists("."));
    tassert_eq(1, os.path.exists(".."));
    tassert_eq(1, os.path.exists("./tests"));
    tassert_eq(1, os.path.exists(__FILE__));
    tassert_eq(0, os.path.exists("./tests/test_os_posix.cpp"));

    auto st = os.fs.stat("tests");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(1, os.path.exists("tests"));

#ifdef _WIN32
    st = os.fs.stat(".\\tests\\");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(1, os.path.exists("tests\\"));
#endif

    char buf[PATH_MAX + 10];
    memset(buf, 'a', arr$len(buf));
    buf[PATH_MAX + 8] = '\0';
    uassert_disable();

    tassert_eq(0, os.path.exists(buf));

    return EOK;
}

test$case(test_dir_stat)
{
    tassert_eq(0, os.path.exists(NULL));
    tassert_eq(0, os.path.exists(""));
    tassert_eq(1, os.path.exists("."));
    tassert_eq(1, os.path.exists(".."));
    tassert_eq(1, os.path.exists("./tests"));
    tassert_eq(1, os.path.exists(__FILE__));
    tassert_eq(0, os.path.exists("./tests/test_os_posix.cpp"));

    auto st = os.fs.stat("tests");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(st.is_directory, true);
    tassert_ne(st.mtime, 0);

    st = os.fs.stat("./tests");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(st.is_directory, true);
    tassert_ne(st.mtime, 0);

    st = os.fs.stat("./tests//");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(st.is_directory, true);

    st = os.fs.stat("tests/");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(st.is_directory, true);

#ifdef _WIN32
    st = os.fs.stat(".\\tests\\");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(st.is_directory, true);
    tassert_eq(1, os.path.exists("tests\\"));

    st = os.fs.stat("tests\\");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(st.is_directory, true);
    tassert_eq(1, os.path.exists("tests\\"));

    st = os.fs.stat("tests\\\\");
    tassertf(st.is_valid, "os.fs.stat('tests/') error : %s", st.error);
    tassert_eq(st.is_directory, true);
    tassert_eq(1, os.path.exists("tests\\"));
#endif

    return EOK;
}

test$case(test_os_mkdir)
{
    tassert_er(Error.argument, os.fs.mkdir(NULL));
    tassert_er(Error.argument, os.fs.mkdir(""));

    e$except_silent (err, os.fs.remove(TBUILDDIR "mytestdir")) {
        if (err != Error.not_found) { tassert_er(Error.not_found, err); }
    }

    auto ftype = os.fs.stat(TBUILDDIR "mytestdir");
    tassert_eq(ftype.is_valid, 0);
    tassert_eq(ftype.is_directory, 0);
    tassert_eq(ftype.is_file, 0);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);
    tassert_er(ftype.error, Error.not_found);

    tassert_eq(0, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_er(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestdir"));
    ftype = os.fs.stat(TBUILDDIR "mytestdir");
    tassert_eq(ftype.is_valid, 1);
    tassert_ne(ftype.mtime, 0);
    tassert_eq(ftype.is_directory, 1);
    tassert_eq(ftype.is_file, 0);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    // Already exists no error
    tassert_er(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));

    tassert_er(Error.ok, os.fs.remove(TBUILDDIR "mytestdir"));
    tassert_eq(0, os.path.exists(TBUILDDIR "mytestdir"));

    ftype = os.fs.stat("tests/data/dir1/dir2_symlink");
    if (os.platform.current() == OSPlatform__wasm) {
        tassert_eq(ftype.is_valid, 0);
        tassert_eq(0, os.path.exists("tests/data/dir1/dir2_symlink"));
    } else {
        tassert_eq(ftype.is_valid, 1);
        tassert_eq(ftype.is_directory, 1);
        tassert_eq(ftype.is_file, 0);
        tassert_eq(ftype.is_symlink, (os.platform.current() == OSPlatform__win) ? 0 : 1);
        tassert_eq(ftype.is_other, 0);
        tassert_eq(ftype.is_other, 0);
        tassert_ne(ftype.mtime, 0);
    }

    return EOK;
}

test$case(test_os_fs_stat)
{

    tassert_eq(1, os.path.exists(__FILE__));
    auto ftype = os.fs.stat(__FILE__);
    tassert_eq(ftype.is_valid, 1);
    tassert_ne(ftype.mtime, 0);
    tassert_eq(ftype.is_directory, 0);
    tassert_eq(ftype.is_file, 1);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);
    tassert_gt(ftype.size, 0);

    return EOK;
}

test$case(test_os_rename_dir)
{
    if (os.fs.remove(TBUILDDIR "mytestdir")) {}

    tassert_eq(0, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_er(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestdir"));

    tassert_eq(1, os.path.exists(TBUILDDIR "mytestdir"));

    tassert_er(Error.argument, os.fs.rename("", "foo"));
    tassert_er(Error.argument, os.fs.rename(NULL, "foo"));
    tassert_er(Error.argument, os.fs.rename("foo", ""));
    tassert_er(Error.argument, os.fs.rename("foo", NULL));

    tassert_er(Error.ok, os.fs.rename(TBUILDDIR "mytestdir", TBUILDDIR "mytestdir2"));
    tassert_eq(0, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestdir2"));
    auto ftype = os.fs.stat(TBUILDDIR "mytestdir2");
    tassert_eq(ftype.is_valid, 1);
    tassert_ne(ftype.mtime, 0);
    tassert_eq(ftype.is_directory, 1);
    tassert_eq(ftype.is_file, 0);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    tassert_er(EOK, os.fs.remove(TBUILDDIR "mytestdir2"));


    return EOK;
}

test$case(test_os_rename_file)
{
    if (os.fs.remove(TBUILDDIR "mytestfile.txt")) {}
    if (os.fs.remove(TBUILDDIR "mytestfile.txt2")) {}
    if (os.fs.remove(TBUILDDIR "mytestfile.txt3")) {}

    tassert_eq(0, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "mytestfile.txt", "foo"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "mytestfile.txt3", "bar"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt3"));

    tassert_er(Error.ok, os.fs.rename(TBUILDDIR "mytestfile.txt", TBUILDDIR "mytestfile.txt2"));
    tassert_eq(0, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt2"));

    auto ftype = os.fs.stat(TBUILDDIR "mytestfile.txt2");
    tassert_eq(ftype.is_valid, 1);
    tassert_eq(ftype.is_directory, 0);
    tassert_eq(ftype.is_file, 1);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    tassert_er(Error.exists, os.fs.rename(TBUILDDIR "mytestfile.txt3", TBUILDDIR "mytestfile.txt2"));
    tassert_eq(0, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt2"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt3"));

    tassert_er(Error.not_found, os.fs.remove(TBUILDDIR "mytestfile.txt"));
    tassert_er(EOK, os.fs.remove(TBUILDDIR "mytestfile.txt2"));
    tassert_er(EOK, os.fs.remove(TBUILDDIR "mytestfile.txt3"));


    return EOK;
}

test$case(test_os_remove_file)
{
    if (os.fs.remove(TBUILDDIR "mytestfile.txt")) {}

    tassert_eq(0, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "mytestfile.txt", "foo"));

    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_er(EOK, os.fs.remove(TBUILDDIR "mytestfile.txt"));
    tassert_eq(0, os.path.exists(TBUILDDIR "mytestfile.txt"));

    return EOK;
}

test$case(test_os_path_join)
{
    mem$scope(tmem$, ta)
    {
        arr$(char*) parts = arr$new(parts, ta);
        arr$pushm(parts, "cexstr", str.fmt(ta, "%s_%d.txt", "foo", 10));
        char* p = os.path.join(parts, arr$len(parts), ta);
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq("cexstr\\foo_10.txt", p);
        } else {
            tassert_eq("cexstr/foo_10.txt", p);
        }
    }
    return EOK;
}

test$case(test_os_setenv)
{
    // get non existing
    tassert_eq(os.env.get("test_os_posix", NULL), NULL);
    // get non existing, with default
    tassert_eq(os.env.get("test_os_posix", "envdef"), "envdef");

    // set env
    tassert_er(EOK, os.env.set("test_os_posix", "foo"));
    tassert_eq(os.env.get("test_os_posix", NULL), "foo");

    // set with replacing
    tassert_er(EOK, os.env.set("test_os_posix", "bar"));
    tassert_eq(os.env.get("test_os_posix", NULL), "bar");

    return EOK;
}
test$case(test_os_path_split)
{

    // dir part
    tassert_eq(str.slice.eq(os.path.split("foo.bar", true), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split("foo", true), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split(".", true), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split("..", true), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split("./", true), str$s(".")), 1);
    tassert_eq(str.slice.eq(os.path.split("../", true), str$s("..")), 1);
    tassert_eq(str.slice.eq(os.path.split("/my", true), str$s("/")), 1);
    tassert_eq(str.slice.eq(os.path.split("/", true), str$s("/")), 1);
    tassert_eq(str.slice.eq(os.path.split("asd/a", true), str$s("asd")), 1);
    tassert_eq(str.slice.eq(os.path.split("\\foo\\bar\\baz", true), str$s("\\foo\\bar")), 1);

    // file part
    tassert_eq(str.slice.eq(os.path.split("foo.bar", false), str$s("foo.bar")), 1);
    tassert_eq(str.slice.eq(os.path.split("foo", false), str$s("foo")), 1);
    tassert_eq(str.slice.eq(os.path.split(".", false), str$s(".")), 1);
    tassert_eq(str.slice.eq(os.path.split("..", false), str$s("..")), 1);
    tassert_eq(str.slice.eq(os.path.split("./", false), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split("../", false), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split("/my", false), str$s("my")), 1);
    tassert_eq(str.slice.eq(os.path.split("/", false), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split("asd/", false), str$s("")), 1);
    tassert_eq(str.slice.eq(os.path.split("asd/a", false), str$s("a")), 1);
    tassert_eq(str.slice.eq(os.path.split("\\foo\\bar\\baz", false), str$s("baz")), 1);

    return EOK;
}

test$case(test_os_path_basename_dirname)
{
    tassert_eq(os.path.basename(NULL, mem$), NULL);
    tassert_eq(os.path.basename("", mem$), NULL);
    tassert_eq(os.path.dirname(NULL, mem$), NULL);
    tassert_eq(os.path.dirname("", mem$), NULL);

    mem$scope(tmem$, _)
    {
        tassert_eq("baz.txt", os.path.basename("foo/bar/baz.txt", _));
        tassert_eq(".txt", os.path.basename("foo/bar/.txt", _));
        tassert_eq("txt", os.path.basename("foo/bar/txt", _));
        tassert_eq("", os.path.basename("foo/bar/", _));

        tassert_eq("foo/bar", os.path.dirname("foo/bar/baz.txt", _));
        tassert_eq("/", os.path.dirname("/foo", _));
    }

    return EOK;
}

test$case(test_os_mkpath)
{
    log$debug("PATH_MAX: %d\n", PATH_MAX);
    e$ret(os.fs.remove_tree(TBUILDDIR));

    tassert(!os.path.exists(TBUILDDIR));
    tassert(!os.path.exists(TBUILDDIR "mydir1/"));
    tassert(!os.path.exists(TBUILDDIR "mydir1/foo.txt"));

    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "mydir1/foo.txt"));
    tassert(os.path.exists(TBUILDDIR "mydir1/"));
    tassert(!os.path.exists(TBUILDDIR "mydir1/foo.txt"));

    tassert(!os.path.exists(TBUILDDIR "mydir2/"));
    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "mydir2/"));
    tassert(os.path.exists(TBUILDDIR "mydir2/"));
    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "mydir2/foo.txt"));
    tassert(!os.path.exists(TBUILDDIR "mydir2/foo.txt"));


    // removing existing
    e$ret(os.fs.remove_tree(TBUILDDIR));
    tassert(!os.path.exists(TBUILDDIR));

    // removing non-existing raised Error.not_found
    tassert_er(Error.not_found, os.fs.remove_tree(TBUILDDIR));

    return EOK;
}

test$case(test_os_copy_file)
{
    if (os.fs.remove(TBUILDDIR "mytestfile.txt")) {}
    if (os.fs.remove(TBUILDDIR "mytestfile.txt2")) {}

    tassert_eq(0, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "mytestfile.txt", "foo"));

    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_eq(0, os.path.exists(TBUILDDIR "mytestfile.txt2"));

    tassert_er(Error.ok, os.fs.copy(TBUILDDIR "mytestfile.txt", TBUILDDIR "mytestfile.txt2"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_eq(1, os.path.exists(TBUILDDIR "mytestfile.txt2"));

    auto ftype = os.fs.stat(TBUILDDIR "mytestfile.txt2");
    tassert_eq(ftype.is_valid, 1);
    tassert_eq(ftype.is_directory, 0);
    tassert_eq(ftype.is_file, 1);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    mem$scope(tmem$, _)
    {
        auto content = io.file.load(TBUILDDIR "mytestfile.txt2", _);
        tassert(content);
        tassert_eq(content, "foo");
    }

    tassert_er(Error.exists, os.fs.copy(TBUILDDIR "mytestfile.txt", TBUILDDIR "mytestfile.txt2"));
    tassert_er(Error.not_found, os.fs.copy(TBUILDDIR "alksdjaldj.txt", TBUILDDIR "mytestfile.txt4"));

    return EOK;
}


test$case(test_os_copy_tree)
{

    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "in/foo/"));
    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "in/foo/aaa/"));
    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "in/bar/"));

    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/foo/1.txt", "1"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/foo/2.txt", "2"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/3.txt", "3"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/4.txt", "4"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/foo/aaa/5.txt", "5"));

    tassert(os.path.exists(TBUILDDIR "in/foo/1.txt"));
    tassert(os.path.exists(TBUILDDIR "in/foo/2.txt"));
    tassert(os.path.exists(TBUILDDIR "in/bar/"));
    tassert(os.path.exists(TBUILDDIR "in/3.txt"));
    tassert(os.path.exists(TBUILDDIR "in/4.txt"));
    tassert(os.path.exists(TBUILDDIR "in/foo/aaa/5.txt"));

    tassert(!os.path.exists(TBUILDDIR "out/"));
    tassert_er(Error.ok, os.fs.copy_tree(TBUILDDIR "in/", TBUILDDIR "out/"));

    tassert(os.path.exists(TBUILDDIR "out/"));
    tassert(os.path.exists(TBUILDDIR "out/foo/1.txt"));
    tassert(os.path.exists(TBUILDDIR "out/foo/2.txt"));
    tassert(os.path.exists(TBUILDDIR "out/bar/"));
    tassert(os.path.exists(TBUILDDIR "out/3.txt"));
    tassert(os.path.exists(TBUILDDIR "out/4.txt"));
    tassert(os.path.exists(TBUILDDIR "out/foo/aaa/5.txt"));
    return EOK;
}

test$case(test_os_copy_tree_sanity_checks)
{

    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "in/foo/"));
    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "in/foo/aaa/"));
    tassert_er(EOK, os.fs.mkpath(TBUILDDIR "in/bar/"));

    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/foo/1.txt", "1"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/foo/2.txt", "2"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/3.txt", "3"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/4.txt", "4"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "in/foo/aaa/5.txt", "5"));

    tassert_er(Error.argument, os.fs.copy_tree(NULL, TBUILDDIR "out/"));
    tassert_er(Error.argument, os.fs.copy_tree("", TBUILDDIR "out/"));
    tassert_er(Error.argument, os.fs.copy_tree(TBUILDDIR "in/3.txt", TBUILDDIR "out/"));
    tassert_er(Error.argument, os.fs.copy_tree(TBUILDDIR "in", NULL));
    tassert_er(Error.argument, os.fs.copy_tree(TBUILDDIR "in", ""));
    tassert_er(Error.not_found, os.fs.copy_tree(TBUILDDIR "inalskdjalksjd", ""));
    tassert_er(Error.exists, os.fs.copy_tree(TBUILDDIR "in", TBUILDDIR "in/foo"));

    tassert_er(Error.ok, os.fs.copy_tree(TBUILDDIR "in", TBUILDDIR "out"));
    tassert(os.path.exists(TBUILDDIR "out/"));
    tassert(os.path.exists(TBUILDDIR "out/foo/1.txt"));
    tassert(os.path.exists(TBUILDDIR "out/foo/2.txt"));
    tassert(os.path.exists(TBUILDDIR "out/bar/"));
    tassert(os.path.exists(TBUILDDIR "out/3.txt"));
    tassert(os.path.exists(TBUILDDIR "out/4.txt"));
    tassert(os.path.exists(TBUILDDIR "out/foo/aaa/5.txt"));

    return EOK;
}


test$case(test_os_path_abs)
{
    tassert_eq(os.path.abs(NULL, mem$), NULL);
    tassert_eq(os.path.abs("", mem$), NULL);

    mem$scope(tmem$, _)
    {
        auto p = os.fs.getcwd(_);
        tassert(p != NULL);
        if (os.platform.current() == OSPlatform__wasm) {
            tassert_eq(p, "/");
        } else {
            tassert_eq(true, str.ends_with(p, "cex"));
        }

        auto abs_cwd = os.path.abs(".", _);
        tassert(abs_cwd != NULL);
        tassert(str.starts_with(abs_cwd, p));

        abs_cwd = os.path.abs("tests/..", _);
        tassert(abs_cwd != NULL);
        tassert(str.starts_with(abs_cwd, p));
        tassert(os.path.exists(abs_cwd));

        if (os.platform.current() == OSPlatform__win) {
            tassert(str.find(abs_cwd, "\\"));
            tassert(str.find(abs_cwd, ":\\")); // drive letter
            tassert(!str.find(abs_cwd, "/"));  // path converted to backslashes
        } else {
            tassert(str.find(abs_cwd, "/"));
            tassert(!str.find(abs_cwd, "\\"));
        }

        // Already-absolute paths (POSIX)
        if (os.platform.current() != OSPlatform__win) {
            tassert_eq(os.path.abs("/", _), "/");
            tassert_eq(os.path.abs("/a/b/c", _), "/a/b/c");
            tassert_eq(os.path.abs("/a/../b", _), "/b");
        }

        // Already-absolute paths (Windows)
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(os.path.abs("C:\\", _), "C:\\");
            tassert_eq(os.path.abs("C:\\a\\b\\c", _), "C:\\a\\b\\c");
            tassert_eq(os.path.abs("C:/a/b/c", _), "C:\\a\\b\\c");
            tassert_eq(os.path.abs("C:\\a\\..\\b", _), "C:\\b");
            tassert_eq(os.path.abs("\\\\server\\share", _), "\\\\server\\share");
            tassert_eq(os.path.abs("\\\\server\\share\\a\\..\\b", _), "\\\\server\\share\\b");
        }

        // .. resolves to parent directory
        {
            auto parent = os.path.dirname(p, _);
            tassert(parent != NULL);
            auto r = os.path.abs("..", _);
            tassert(r != NULL);
            tassert_eq(r, parent);
        }

        // Non-existent path (works without filesystem access now)
        {
            auto r = os.path.abs("nonexistent_dir", _);
            tassert(r != NULL);
            char* expected = os.path.normalize(str.fmt(_, "%s%cnonexistent_dir", p, os$PATH_SEP), _);
            tassert_eq(r, expected);
        }

        // Messy relative with . and ..
        {
            auto r = os.path.abs("./././foo/../bar", _);
            tassert(r != NULL);
            char* expected = os.path.normalize(str.fmt(_, "%s%cbar", p, os$PATH_SEP), _);
            tassert_eq(r, expected);
        }
    }

    return EOK;
}


test$case(test_os_path_normpath_basic)
{
    mem$scope(tmem$, _)
    {
        tassert_eq(os.path.normalize(NULL, _), ".");
        tassert_eq(os.path.normalize("", _), ".");
        tassert_eq(os.path.normalize(".", _), ".");
        tassert_eq(os.path.normalize("..", _), "..");
        tassert_eq(os.path.normalize("/", _), "/");

        if (os$PATH_SEP == '/') {
            tassert_eq(os.path.normalize("a/b/c", _), "a/b/c");
            tassert_eq(os.path.normalize("a/./b", _), "a/b");
            tassert_eq(os.path.normalize("a/b/../c", _), "a/c");
            tassert_eq(os.path.normalize("../../x", _), "../../x");
            tassert_eq(os.path.normalize("a//b///c", _), "a/b/c");
            tassert_eq(os.path.normalize("a/../../b", _), "../b");
        } else {
            tassert_eq(os.path.normalize("a/b/c", _), "a\\b\\c");
            tassert_eq(os.path.normalize("a/./b", _), "a\\b");
            tassert_eq(os.path.normalize("a/b/../c", _), "a\\c");
            tassert_eq(os.path.normalize("../../x", _), "..\\..\\x");
            tassert_eq(os.path.normalize("a//b///c", _), "a\\b\\c");
            tassert_eq(os.path.normalize("a/../../b", _), "..\\b");
        }

        tassert_eq(os.path.normalize("/a/../b", _), "/b");
        tassert_eq(os.path.normalize("/../x", _), "/x");
        tassert_eq(os.path.normalize("foo/bar/..", _), "foo");
        tassert_eq(os.path.normalize("a/b/..", _), "a");
        tassert_eq(os.path.normalize("..", _), "..");
        tassert_eq(os.path.normalize(".", _), ".");
    }
    return EOK;
}


test$case(test_os_path_normpath_backslash)
{
    mem$scope(tmem$, _)
    {
        tassert_eq(os.path.normalize("a\\b\\c", _), os$PATH_SEP == '/' ? "a/b/c" : "a\\b\\c");
        tassert_eq(os.path.normalize("a\\.\\b", _), os$PATH_SEP == '/' ? "a/b" : "a\\b");
        tassert_eq(os.path.normalize("a\\b\\..\\c", _), os$PATH_SEP == '/' ? "a/c" : "a\\c");
    }
    return EOK;
}


test$case(test_os_path_normpath_edge)
{
    mem$scope(tmem$, _)
    {
        // Just a leading slash on parts
        tassert_eq(os.path.normalize("/a/b/../c", _), "/a/c");
        tassert_eq(os.path.normalize("a/..", _), ".");
        tassert_eq(os.path.normalize("a/../..", _), "..");
        tassert_eq(os.path.normalize("/a/../../b", _), "/b");
    }
    return EOK;
}


test$case(test_os_path_normpath_leak)
{
    // Use mem$ (heap) allocator and explicitly free results to verify no memory leaks.
    // Running under ASAN will report any unfreed allocations.
    char* r = NULL;

    r = os.path.normalize(NULL, mem$);
    tassert_eq(r, ".");
    mem$free(mem$, r);

    r = os.path.normalize("", mem$);
    tassert_eq(r, ".");
    mem$free(mem$, r);

    r = os.path.normalize("/", mem$);
    tassert_eq(r, "/");
    mem$free(mem$, r);

    if (os$PATH_SEP == '/') {
        r = os.path.normalize("a/b/c", mem$);
        tassert_eq(r, "a/b/c");
        mem$free(mem$, r);

        r = os.path.normalize("a/./b", mem$);
        tassert_eq(r, "a/b");
        mem$free(mem$, r);

        r = os.path.normalize("a/b/../c", mem$);
        tassert_eq(r, "a/c");
        mem$free(mem$, r);

        r = os.path.normalize("../../x", mem$);
        tassert_eq(r, "../../x");
        mem$free(mem$, r);

        r = os.path.normalize("a//b///c", mem$);
        tassert_eq(r, "a/b/c");
        mem$free(mem$, r);

        r = os.path.normalize("a/../../b", mem$);
        tassert_eq(r, "../b");
        mem$free(mem$, r);
    } else {
        r = os.path.normalize("a/b/c", mem$);
        tassert_eq(r, "a\\b\\c");
        mem$free(mem$, r);

        r = os.path.normalize("a/./b", mem$);
        tassert_eq(r, "a\\b");
        mem$free(mem$, r);

        r = os.path.normalize("a/b/../c", mem$);
        tassert_eq(r, "a\\c");
        mem$free(mem$, r);

        r = os.path.normalize("../../x", mem$);
        tassert_eq(r, "..\\..\\x");
        mem$free(mem$, r);

        r = os.path.normalize("a//b///c", mem$);
        tassert_eq(r, "a\\b\\c");
        mem$free(mem$, r);

        r = os.path.normalize("a/../../b", mem$);
        tassert_eq(r, "..\\b");
        mem$free(mem$, r);
    }

    r = os.path.normalize("/a/../b", mem$);
    tassert_eq(r, "/b");
    mem$free(mem$, r);

    r = os.path.normalize("/../x", mem$);
    tassert_eq(r, "/x");
    mem$free(mem$, r);

    r = os.path.normalize("a\\b\\c", mem$);
    tassert_eq(r, os$PATH_SEP == '/' ? "a/b/c" : "a\\b\\c");
    mem$free(mem$, r);

    r = os.path.normalize("foo/bar/..", mem$);
    tassert_eq(r, "foo");
    mem$free(mem$, r);

    r = os.path.normalize(".", mem$);
    tassert_eq(r, ".");
    mem$free(mem$, r);

    r = os.path.normalize("..", mem$);
    tassert_eq(r, "..");
    mem$free(mem$, r);

    r = os.path.normalize("a/..", mem$);
    tassert_eq(r, ".");
    mem$free(mem$, r);

    r = os.path.normalize("/a/../../b", mem$);
    tassert_eq(r, "/b");
    mem$free(mem$, r);

    return EOK;
}


test$case(test_os_path_normpath_unicode)
{
    mem$scope(tmem$, _)
    {
        // Single-component and absolute assertions (platform independent)
        tassert_eq(os.path.normalize("ファイル/../ドキュメント", _), "ドキュメント");
        tassert_eq(os.path.normalize("test／file", _), "test／file");
        tassert_eq(os.path.normalize("/日本語/a/../b", _), "/日本語/b");
        tassert_eq(os.path.normalize("/a/b/c/\xC3\xA9", _), "/a/b/c/\xC3\xA9");

        // Multi-component assertions (join separator depends on platform)
        if (os$PATH_SEP == '/') {
            tassert_eq(os.path.normalize("привет/мир", _), "привет/мир");
            tassert_eq(os.path.normalize("ファイル/ドキュメント", _), "ファイル/ドキュメント");
            tassert_eq(os.path.normalize("ファイル/./ドキュメント", _), "ファイル/ドキュメント");
            tassert_eq(os.path.normalize("a/‥/b", _), "a/‥/b");
            tassert_eq(os.path.normalize("テスト//ドキュメント", _), "テスト/ドキュメント");
            tassert_eq(os.path.normalize("a/e\xCC\x81/file", _), "a/e\xCC\x81/file");
            tassert_eq(os.path.normalize("\xC3\xA9/file", _), "\xC3\xA9/file");
            tassert_eq(os.path.normalize("a/.\xCC\x80./b", _), "a/.\xCC\x80./b");
            tassert_eq(os.path.normalize("a/b/\xE2\x80\x8B/c", _), "a/b/\xE2\x80\x8B/c");
            tassert_eq(os.path.normalize("a//b/\xC3\xA9/\xC3\xA9/../c", _), "a/b/\xC3\xA9/c");
            tassert_eq(os.path.normalize("привет/./мир/../test", _), "привет/test");
        } else {
            tassert_eq(os.path.normalize("привет/мир", _), "привет\\мир");
            tassert_eq(os.path.normalize("ファイル/ドキュメント", _), "ファイル\\ドキュメント");
            tassert_eq(os.path.normalize("ファイル/./ドキュメント", _), "ファイル\\ドキュメント");
            tassert_eq(os.path.normalize("a/‥/b", _), "a\\‥\\b");
            tassert_eq(os.path.normalize("テスト//ドキュメント", _), "テスト\\ドキュメント");
            tassert_eq(os.path.normalize("a/e\xCC\x81/file", _), "a\\e\xCC\x81\\file");
            tassert_eq(os.path.normalize("\xC3\xA9/file", _), "\xC3\xA9\\file");
            tassert_eq(os.path.normalize("a/.\xCC\x80./b", _), "a\\.\xCC\x80.\\b");
            tassert_eq(os.path.normalize("a/b/\xE2\x80\x8B/c", _), "a\\b\\\xE2\x80\x8B\\c");
            tassert_eq(os.path.normalize("a//b/\xC3\xA9/\xC3\xA9/../c", _), "a\\b\\\xC3\xA9\\c");
            tassert_eq(os.path.normalize("привет/./мир/../test", _), "привет\\test");
        }
    }
    return EOK;
}


test$main();
