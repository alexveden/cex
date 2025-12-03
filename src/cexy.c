#if !defined(cex$enable_minimal)

#include "all.h"
#include "src/cex_base.h"
#include <stdbool.h>
#include <stdio.h>
#if defined(CEX_BUILD) || defined(CEX_NEW)

#    include <ctype.h>
#    include <time.h>

static void
cexy_build_self(int argc, char** argv, char* cex_source)
{
    mem$scope(tmem$, _)
    {
        uassert(str.ends_with(argv[0], "cex") || str.ends_with(argv[0], "cex.exe"));
        char* bin_path = argv[0];
        bool has_darg_before_cmd = (argc > 1 && str.starts_with(argv[1], "-D"));
        (void)has_darg_before_cmd;

#    if cexy$create_compile_flags
        e$except (err, cexy.utils.make_compile_flags("compile_flags.txt", true, NULL)) {}
#    endif

#    ifdef _CEX_SELF_BUILD
        if (!has_darg_before_cmd) {
            log$trace("Checking self build for executable: %s\n", bin_path);
            if (!cexy.src_include_changed(bin_path, cex_source, NULL)) {
                log$trace("%s unchanged, skipping self build\n", cex_source);
                // cex.c/cex.h are up to date no rebuild needed
                return;
            }
        } else {
            log$trace("Passed extra -Dflags to cex command, build now\n");
        }
#    endif

        log$info("Rebuilding self: %s -> %s\n", cex_source, bin_path);
        char* old_name = str.fmt(_, "%s.old", bin_path);
        if (os.path.exists(bin_path)) {
            if (os.fs.remove(old_name)) {}
            e$except (err, os.fs.rename(bin_path, old_name)) { goto fail_recovery; }
        }
        arr$(char*) args = arr$new(args, _, .capacity = 64);
        sbuf_c dargs_sbuf = sbuf.create(256, _);
        arr$pushm(args, cexy$cex_self_cc, "-D_CEX_SELF_BUILD", "-g");
        e$goto(sbuf.append(&dargs_sbuf, "-D_CEX_SELF_DARGS=\""), err);

        i32 dflag_idx = 1;
        for$each (darg, argv + 1, argc - 1) {
            if (str.starts_with(darg, "-D")) {
                if (!str.eq(darg, "-D")) {
                    arr$push(args, darg);
                    e$goto(sbuf.appendf(&dargs_sbuf, "%s ", darg), fail_recovery);
                }
                dflag_idx++;
            } else {
                break; // stop at first non -D<flag>
            }
        }
        if (dflag_idx > 1 && (dflag_idx >= argc || !str.eq(argv[dflag_idx], "config"))) {
            log$error("Expected config command after passing -D<ARG> to ./cex\n");
            goto fail_recovery;
        }
#    ifdef _CEX_SELF_DARGS
        if (dflag_idx == 1) {
            // new compilation no flags (maybe due to source changes, we need to maintain old flags)
            log$trace("Preserving CEX_SELF_DARGS: %s\n", _CEX_SELF_DARGS);
            for$iter (
                str_s,
                it,
                str.slice.iter_split(str.sstr(_CEX_SELF_DARGS), " ", &it.iterator)
            ) {
                str_s clean_it = str.slice.strip(it.val);
                if (clean_it.len == 0) { continue; }
                if (!str.slice.starts_with(clean_it, str$s("-D"))) { continue; }
                if (str.slice.eq(clean_it, str$s("-D"))) { continue; }
                auto _darg = str.fmt(_, "%S", clean_it);
                arr$push(args, _darg);
                e$goto(sbuf.appendf(&dargs_sbuf, "%s ", _darg), err);
            }
        }
#    endif
        e$goto(sbuf.append(&dargs_sbuf, "\""), err);

        arr$push(args, dargs_sbuf);

        char* custom_args[] = { cexy$cex_self_args };
        arr$pusha(args, custom_args);

        arr$pushm(args, "-o", bin_path, cex_source, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        os_cmd_c _cmd = { 0 };
        e$except (err, os.cmd.run(args, arr$len(args), &_cmd)) { goto fail_recovery; }
        e$except (err, os.cmd.join(&_cmd, 0, NULL)) { goto fail_recovery; }

        // All good new build successful, remove old binary
        if (os.fs.remove(old_name)) {}

        // run rebuilt cex executable again
        arr$clear(args);
        arr$pushm(args, bin_path);
        for$each (darg, argv + dflag_idx, argc - dflag_idx) { arr$push(args, darg); }
        arr$pushm(args, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        e$except (err, os.cmd.run(args, arr$len(args), &_cmd)) { goto err; }
        if (os.cmd.join(&_cmd, 0, NULL)) { goto err; }
        exit(0); // unconditionally exit after build was successful
    fail_recovery:
        if (os.path.exists(old_name)) {
            e$except (err, os.fs.rename(old_name, bin_path)) { goto err; }
        }
        goto err;
    err:
        exit(1);
    }
}

static bool
cexy_src_include_changed(char* target_path, char* src_path, arr$(char*) alt_include_path)
{
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    if (unlikely(src_path == NULL)) {
        log$error("src_path is NULL\n");
        return false;
    }

    auto src_meta = os.fs.stat(src_path);
    if (!src_meta.is_valid) {
        (void)e$raise(src_meta.error, "Error src: %s", src_path);
        return false;
    } else if (!src_meta.is_file || src_meta.is_symlink) {
        (void)e$raise("Bad type", "src is not a file: %s", src_path);
        return false;
    }

    auto target_meta = os.fs.stat(target_path);
    if (!target_meta.is_valid) {
        if (target_meta.error == Error.not_found) {
            return true;
        } else {
            (void)e$raise(target_meta.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_meta.is_file || target_meta.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    if (src_meta.mtime > target_meta.mtime) {
        // File itself changed
        log$debug("Src changed: %s\n", src_path);
        return true;
    }

    if (!str.ends_with(src_path, ".c") && !str.ends_with(src_path, ".h")) {
        // We only parse includes for appropriate .c/.h files
        return false;
    }

    mem$scope(tmem$, _)
    {
        arr$(char*) incl_path = arr$new(incl_path, _);
        if (arr$len(alt_include_path) > 0) {
            for$each (p, alt_include_path) {
                arr$push(incl_path, p);
                if (!os.path.exists(p)) { log$warn("alt_include_path not exists: %s\n", p); }
            }
        } else {
            char* def_incl_path[] = { cexy$cc_include };
            for$each (p, def_incl_path) {
                char* clean_path = p;
                if (str.starts_with(p, "-I")) {
                    clean_path = p + 2;
                } else if (str.starts_with(p, "-iquote=")) {
                    clean_path = p + strlen("-iquote=");
                }
                if (!os.path.exists(clean_path)) {
                    log$trace("cexy$cc_include not exists: %s\n", clean_path);
                    continue;
                }
                arr$push(incl_path, clean_path);
            }
            // Adding relative to src_path directory as a search target
            arr$push(incl_path, os.path.dirname(src_path, _));
        }

        char* code = io.file.load(src_path, _);
        if (code == NULL) {
            (void)e$raise("IOError", "src is not a file: '%s'", src_path);
            return false;
        }

        (void)CexTkn_str;
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        while ((t = CexParser.next_token(&lx)).type) {
            if (t.type != CexTkn__preproc) { continue; }
            if (str.slice.starts_with(t.value, str$s("include"))) {
                str_s incf = str.slice.sub(t.value, strlen("include"), 0);
                incf = str.slice.strip(incf);
                if (incf.len <= 4) { // <.h>
                    log$warn(
                        "Bad include in: %s (item: %S at line: %d)\n",
                        src_path,
                        t.value,
                        lx.line
                    );
                    continue;
                }
                log$trace("Processing include: '%S'\n", incf);
                if (!str.slice.match(incf, "\"*.[hc]\"*")) {
                    // system includes skipped
                    log$trace("Skipping include: '%S'\n", incf);
                    continue;
                }
                incf = str.slice.sub(incf, 1, 0);
                incf = str.slice.sub(incf, 0, str.slice.index_of(incf, str$s("\"")) - 1);

                mem$scope(tmem$, _)
                {
                    char extensions[] = { 'c', 'h' };
                    for$each (ext, extensions) {
                        char* inc_fn = str.fmt(_, "%S%c", incf, ext);
                        uassert(inc_fn != NULL);
                        for$each (inc_dir, incl_path) {
                            char* try_path = os$path_join(_, inc_dir, inc_fn);
                            uassert(try_path != NULL);
                            auto src_meta = os.fs.stat(try_path);
                            log$trace("Probing include: %s\n", try_path);
                            if (src_meta.is_valid && src_meta.mtime > target_meta.mtime) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

static bool
cexy_src_changed(char* target_path, char** src_array, usize src_array_len)
{
    if (unlikely(src_array == NULL || src_array_len == 0)) {
        if (src_array == NULL) {
            log$error("src_array is NULL, which may indicate error\n");
        } else {
            log$error("src_array is empty\n");
        }
        return false;
    }
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    auto target_ftype = os.fs.stat(target_path);
    if (!target_ftype.is_valid) {
        if (target_ftype.error == Error.not_found) {
            log$trace("Target '%s' not exists, needs build.\n", target_path);
            return true;
        } else {
            (void)e$raise(target_ftype.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_ftype.is_file || target_ftype.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    bool has_error = false;
    for$each (p, src_array, src_array_len) {
        auto ftype = os.fs.stat(p);
        if (!ftype.is_valid) {
            (void)e$raise(ftype.error, "Error src: %s", p);
            has_error = true;
        } else if (!ftype.is_file || ftype.is_symlink) {
            (void)e$raise("Bad type", "src is not a file: %s", p);
            has_error = true;
        } else {
            if (has_error) { continue; }
            if (ftype.mtime > target_ftype.mtime) {
                log$trace("Source changed '%s'\n", p);
                return true;
            }
        }
    }

    return false;
}

static char*
cexy_target_make(char* src_path, char* build_dir, char* name_or_extension, IAllocator allocator)
{
    uassert(allocator != NULL);

    if (src_path == NULL || src_path[0] == '\0') {
        log$error("src_path is NULL or empty\n");
        return NULL;
    }
    if (build_dir == NULL) {
        log$error("build_dir is NULL\n");
        return NULL;
    }
    if (name_or_extension == NULL || name_or_extension[0] == '\0') {
        log$error("name_or_extension is NULL or empty\n");
        return NULL;
    }
    if (!os.path.exists(src_path)) {
        log$error("src_path does not exist: '%s'\n", src_path);
        return NULL;
    }

    char* result = NULL;
    if (name_or_extension[0] == '.') {
        // starts_with .ext, make full path as following: build_dir/src_path/src_file.ext[.exe]
        result = str.fmt(
            allocator,
            "%s%c%s%s%s",
            build_dir,
            os$PATH_SEP,
            src_path,
            name_or_extension,
            cexy$build_ext_exe
        );
        uassert(result != NULL && "memory error");
    } else {
        // probably a program name, make full path: build_dir/name_or_extension[.exe]
        result = str.fmt(
            allocator,
            "%s%c%s%s",
            build_dir,
            os$PATH_SEP,
            name_or_extension,
            cexy$build_ext_exe
        );
        uassert(result != NULL && "memory error");
    }
    e$except (err, os.fs.mkpath(result)) { mem$free(allocator, result); }

    return result;
}

Exception
cexy__fuzz__create(char* target)
{
    if (!str.slice.starts_with(os.path.split(target, false), str$s("fuzz_"))) {
        return e$raise(Error.argument, "Fuzz file must start with `fuzz_` prefix, got: %s", target);
    }
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Fuzz file already exists: %s", target);
    }
    e$ret(os.fs.mkpath(target));

    mem$scope(tmem$, _)
    {
        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        cg$pn("#define CEX_IMPLEMENTATION");
        cg$pn("#include \"cex.h\"");

        cg$pn("");
        cg$pn("/*");
        cg$func("fuzz$setup(void)", "")
        {
            cg$pn("// This function allows programmatically seed new corpus for fuzzer");
            cg$pn("io.printf(\"CORPUS: %s\\n\", fuzz$corpus_dir);");
            cg$scope("memcg$scope(tmem$, _)", "")
            {
                cg$pn("char* fn = str.fmt(_, \"%s/my_case\", fuzz$corpus_dir);");
                cg$pn("(void)fn;");
                cg$pn("// io.file.save(fn, \"my seed data\");");
            }
        }
        cg$pn("*/");

        cg$pn("");
        cg$func("int\nfuzz$case(const u8* data, usize size)", "")
        {
            cg$pn("// TODO: do your stuff based on input data and size");
            cg$if("size > 2 && data[0] == 'C' && data[1] == 'E' && data[2] == 'X'", "")
            {
                cg$pn("__builtin_trap();");
            }
            cg$pn("return 0;");
        }

        cg$pn("");
        cg$pn("fuzz$main();");

        e$ret(io.file.save(target, buf));
    }
    return EOK;
}

Exception
cexy__test__create(char* target, bool include_sample)
{
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Test file already exists: %s", target);
    }
    if (str.eq(target, "all") || str.find(target, "*")) {
        return e$raise(
            Error.argument,
            "You must pass exact file path, not pattern, got: %s",
            target
        );
    }
    e$ret(os.fs.mkpath(target));

    mem$scope(tmem$, _)
    {
        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        cg$pn("#define CEX_IMPLEMENTATION");
        cg$pn("#define CEX_TEST");
        cg$pn("#include \"cex.h\"");
        if (include_sample) { cg$pn("#include \"lib/mylib.c\""); }
        cg$pn("");
        cg$pn("//test$setup_case() {return EOK;}");
        cg$pn("//test$teardown_case() {return EOK;}");
        cg$pn("//test$setup_suite() {return EOK;}");
        cg$pn("//test$teardown_suite() {return EOK;}");
        cg$pn("");
        cg$scope("test$case(%s)", (include_sample) ? "mylib_test_case" : "my_test_case")
        {
            if (include_sample) {
                cg$pn("tassert_eq(mylib_add(1, 2), 3);");
                cg$pn("// Next will be available after calling `cex process lib/mylib.c`");
                cg$pn("// tassert_eq(mylib.add(1, 2), 3);");
            } else {
                cg$pn("tassert_eq(1, 0);");
            }
            cg$pn("return EOK;");
        }
        cg$pn("");
        cg$pn("test$main();");

        e$ret(io.file.save(target, buf));
    }
    return EOK;
}

Exception
cexy__test__clean(char* target)
{

    if (str.eq(target, "all")) {
        log$info("Cleaning all tests\n");
        e$ret(os.fs.remove_tree(cexy$build_dir "/tests/"));
    } else {
        log$info("Cleaning target: %s\n", target);
        if (!os.path.exists(target)) {
            return e$raise(Error.exists, "Test target not exists: %s", target);
        }

        mem$scope(tmem$, _)
        {
            char* test_target = cexy.target_make(target, cexy$build_dir, ".test", _);
            e$ret(os.fs.remove(test_target));
        }
    }
    return EOK;
}

Exception
cexy__test__make_target_pattern(char** target)
{
    if (target == NULL) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '(null)', expected all or tests/test_some_file.c"
        );
    }
    if (str.eq(*target, "all")) { *target = "tests/test_*.c"; }

    if (!str.match(*target, "*test*.c")) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            *target
        );
    }
    return EOK;
}

Exception
cexy__test__run(char* target, bool is_debug, int argc, char** argv)
{
    Exc result = EOK;
    u32 n_tests = 0;
    u32 n_failed = 0;
    mem$scope(tmem$, _)
    {
        if (str.ends_with(target, "test_*.c")) {
            // quiet mode
            io.printf("-------------------------------------\n");
            io.printf("Running Tests: %s\n", target);
            io.printf("-------------------------------------\n\n");
        } else {
            if (!os.path.exists(target)) {
                return e$raise(Error.not_found, "Test file not found: %s", target);
            }
        }

        for$each (test_src, os.fs.find(target, true, _)) {
            n_tests++;
            char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
            arr$(char*) args = arr$new(args, _);
            if (is_debug) { arr$pushm(args, cexy$debug_cmd); }
            arr$pushm(args, test_target, );
            if (str.ends_with(target, "test_*.c")) { arr$push(args, "--quiet"); }
            arr$pusha(args, argv, argc);
            arr$push(args, NULL);
            if (os$cmda(args)) {
                log$error("<<<<<<<<<<<<<<<<<< Test failed: %s\n", test_target);
                n_failed++;
                result = Error.runtime;
            }
        }
    }
    if (str.ends_with(target, "test_*.c")) {
        io.printf("\n-------------------------------------\n");
        io.printf("Total: %d Passed: %d Failed: %d\n", n_tests, n_tests - n_failed, n_failed);
        io.printf("-------------------------------------\n\n");
    }
    return result;
}

static int
_cexy__decl_comparator(const void* a, const void* b)
{
    cex_decl_s** _a = (cex_decl_s**)a;
    cex_decl_s** _b = (cex_decl_s**)b;

    // Makes str__sub__ to be placed after
    isize isub_a = str.slice.index_of(_a[0]->name, str$s("__"));
    isize isub_b = str.slice.index_of(_b[0]->name, str$s("__"));
    if (unlikely(isub_a != isub_b)) {
        if (isub_a != -1) {
            return 1;
        } else {
            return -1;
        }
    }

    return str.slice.qscmp(&_a[0]->name, &_b[0]->name);
}

static str_s
_cexy__process_make_brief_docs(cex_decl_s* decl)
{
    str_s brief_str = { 0 };
    if (!decl->docs.buf) { return brief_str; }

    if (str.slice.starts_with(decl->docs, str$s("///"))) {
        // doxygen style ///
        brief_str = str.slice.strip(str.slice.sub(decl->docs, 3, 0));
    } else if (str.slice.match(decl->docs, "/\\*[\\*!]*")) {
        // doxygen style /** or /*!
        for$iter (
            str_s,
            it,
            str.slice.iter_split(str.slice.sub(decl->docs, 3, -2), "\n", &it.iterator)
        ) {
            str_s line = str.slice.strip(it.val);
            if (line.len == 0) { continue; }
            brief_str = line;
            break;
        }
    }
    isize bridx = (str.slice.index_of(brief_str, str$s("@brief")));
    if (bridx == -1) { bridx = str.slice.index_of(brief_str, str$s("\\brief")); }
    if (bridx != -1) {
        brief_str = str.slice.strip(str.slice.sub(brief_str, bridx + strlen("@brief"), 0));
    }

    return brief_str;
}

static Exception
_cexy__process_gen_struct(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    cg$scope("struct __cex_namespace__%S ", ns_prefix)
    {
        cg$pn("// Autogenerated by CEX");
        cg$pn("// clang-format off");
        cg$pn("");
        for$each (it, decls) {
            str_s clean_name = it->name;
            if (str.slice.starts_with(clean_name, str$s("cex_"))) {
                clean_name = str.slice.sub(clean_name, 4, 0);
            }
            clean_name = str.slice.sub(clean_name, ns_prefix.len + 1, 0);

            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                uassert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        cg$pf("} %S;", subn);
                    }

                    // new subnamespace begin
                    cg$pn("");
                    cg$pf("struct {", subn);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            str_s brief_str = _cexy__process_make_brief_docs(it);
            if (brief_str.len) { 
                // Handling comments + special multi-line treatment
                for$iter(str_s, it, str.slice.iter_split(brief_str, "\n", &it.iterator)) {
                    str_s _line = str.slice.strip(it.val);
                    if (str.slice.starts_with(_line, str$s("///"))) {
                        _line = str.slice.sub(_line, 3, 0);
                    }
                    if (str.slice.starts_with(_line, str$s("* "))) {
                        _line = str.slice.sub(_line, 2, 0);
                    }
                    _line = str.slice.strip(_line);
                    if (_line.len) {
                        cg$pf("/// %S", _line); 
                    }
                }
            }
            cg$pf("%-15s (*%S)(%s);", it->ret_type, clean_name, it->args);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            cg$pf("} %S;", subn);
        }
        cg$pn("");
        cg$pn("// clang-format on");
    }
    cg$pa("%s", ";");

    if (!cg$is_valid()) { return e$raise(Error.runtime, "Code generation error occured\n"); }
    return EOK;
}

static Exception
_cexy__process_gen_var_def(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    cg$scope("const struct __cex_namespace__%S %S = ", ns_prefix, ns_prefix)
    {
        cg$pn("// Autogenerated by CEX");
        cg$pn("// clang-format off");
        cg$pn("");
        for$each (it, decls) {
            str_s clean_name = it->name;
            if (str.slice.starts_with(clean_name, str$s("cex_"))) {
                clean_name = str.slice.sub(clean_name, 4, 0);
            }
            clean_name = str.slice.sub(clean_name, ns_prefix.len + 1, 0);

            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                e$assert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        cg$pn("},");
                    }

                    // new subnamespace begin
                    cg$pn("");
                    cg$pf(".%S = {", new_ns);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            cg$pf(".%S = %S,", clean_name, it->name);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            cg$pf("},", subn);
        }
        cg$pn("");
        cg$pn("// clang-format on");
    }
    cg$pa("%s", ";");

    if (!cg$is_valid()) { return e$raise(Error.runtime, "Code generation error occured\n"); }
    return EOK;
}

static Exception
_cexy__process_update_code(
    char* code_file,
    bool only_update,
    sbuf_c cex_h_struct,
    sbuf_c cex_h_var_decl,
    sbuf_c cex_c_var_def
)
{
    mem$scope(tmem$, _)
    {
        char* code = io.file.load(code_file, _);
        e$assert(code && "failed loading code");

        bool is_header = str.ends_with(code_file, ".h");

        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        sbuf_c new_code = sbuf.create(10 * 1024 + (lx.content_end - lx.content), _);
        CexParser.reset(&lx);

        str_s code_buf = { .buf = lx.content, .len = 0 };
        bool has_module_def = false;
        bool has_module_decl = false;
        bool has_module_struct = false;
        arr$(cex_token_s) items = arr$new(items, _);

#    define $dump_prev()                                                                           \
        code_buf.len = t.value.buf - code_buf.buf - ((t.value.buf > lx.content) ? 1 : 0);          \
        if (code_buf.buf != NULL)                                                                  \
            e$ret(sbuf.appendf(&new_code, "%.*S\n", code_buf.len, code_buf));                      \
        code_buf = (str_s){ 0 }
#    define $dump_prev_comment()                                                                   \
        for$each (it, items) {                                                                     \
            if (it.type == CexTkn__comment_single || it.type == CexTkn__comment_multi) {           \
                e$ret(sbuf.appendf(&new_code, "%.*S\n", it.value.len, it.value));                  \
            } else {                                                                               \
                break;                                                                             \
            }                                                                                      \
        }

        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__cex_module_def) {
                e$assert(!is_header && "expected in .c file buf got header");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_def) { e$ret(sbuf.appendf(&new_code, "%s\n", cex_c_var_def)); }
                has_module_def = true;
            } else if (t.type == CexTkn__cex_module_decl) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_decl) { e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl)); }
                has_module_decl = true;
            } else if (t.type == CexTkn__cex_module_struct) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_struct) { e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_struct)); }
                has_module_struct = true;
            } else {
                if (code_buf.buf == NULL) { code_buf.buf = t.value.buf; }
                code_buf.len = t.value.buf - code_buf.buf + t.value.len;
            }
        }
        if (code_buf.len) { e$ret(sbuf.appendf(&new_code, "%.*S\n", code_buf.len, code_buf)); }
        if (!is_header) {
            if (!has_module_def) {
                if (only_update) { return EOK; }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_c_var_def));
            }
            e$ret(io.file.save(code_file, new_code));
        } else {
            if (!has_module_struct) {
                if (only_update) { return EOK; }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_h_struct));
            }
            if (!has_module_decl) {
                if (only_update) { return EOK; }
                e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl));
            }
            e$ret(io.file.save(code_file, new_code));
        }
    }

#    undef $dump_prev
#    undef $dump_prev_comment
    return EOK;
}

static bool
_cexy__fn_match(str_s fn_name, str_s ns_prefix)
{

    mem$scope(tmem$, _)
    {
        char* fn_sub_pattern = str.fmt(_, "%S__[a-zA-Z0-9+]__*", ns_prefix);
        char* fn_sub_pattern_cex = str.fmt(_, "cex_%S__[a-zA-Z0-9+]__*", ns_prefix);
        char* fn_pattern = str.fmt(_, "%S_*", ns_prefix);
        char* fn_pattern_cex = str.fmt(_, "cex_%S_*", ns_prefix);
        char* fn_private = str.fmt(_, "%S__*", ns_prefix);
        char* fn_private_cex = str.fmt(_, "cex_%S__*", ns_prefix);

        if (str.slice.starts_with(fn_name, str$s("_"))) { return false; }

        if (str.slice.match(fn_name, fn_sub_pattern) ||
            str.slice.match(fn_name, fn_sub_pattern_cex)) {
            return true;
        } else if ((str.slice.match(fn_name, fn_private) ||
                    str.slice.match(fn_name, fn_private_cex)) ||
                   (!str.slice.match(fn_name, fn_pattern_cex) &&
                    !str.slice.match(fn_name, fn_pattern))) {
            return false;
        }
    }

    return true;
}

static str_s
_cexy__fn_dotted(str_s fn_name, char* expected_ns, IAllocator alloc)
{
    str_s clean_name = fn_name;
    if (str.slice.starts_with(clean_name, str$s("cex_"))) {
        clean_name = str.slice.sub(clean_name, 4, 0);
    }
    if (!str.eq(expected_ns, "cex") && !str.slice.starts_with(clean_name, str.sstr(expected_ns))) {
        return (str_s){ 0 };
    }

    isize ns_idx = str.slice.index_of(clean_name, str$s("_"));
    if (ns_idx < 0) { return (str_s){ 0 }; }
    str_s ns = str.slice.sub(clean_name, 0, ns_idx);
    clean_name = str.slice.sub(clean_name, ns.len + 1, 0);

    str_s subn = { 0 };
    if (str.slice.starts_with(clean_name, str$s("_"))) {
        // sub-namespace
        isize ns_end = str.slice.index_of(clean_name, str$s("__"));
        if (ns_end < 0) {
            // looks like a private func, str__something
            return (str_s){ 0 };
        }
        subn = str.slice.sub(clean_name, 1, ns_end);
        clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
    }
    if (subn.len) {
        return str.sstr(str.fmt(alloc, "%S.%S.%S", ns, subn, clean_name));
    } else {
        return str.sstr(str.fmt(alloc, "%S.%S", ns, clean_name));
    }
}

static Exception
cexy__cmd__process(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    char* process_help = ""
    "process command intended for building CEXy interfaces from your source code\n\n"
    "For example: you can create foo_fun1(), foo_fun2(), foo__bar__fun3(), foo__bar__fun4()\n"
    "   these functions will be processed and wrapped to a `foo` namespace, so you can     \n"
    "   access them via foo.fun1(), foo.fun2(), foo.bar.fun3(), foo.bar.fun4()\n\n" 

    "Requirements / caveats: \n"
    "1. You must have foo.c and foo.h in the same folder\n"
    "2. Filename must start with `foo` - namespace prefix\n"
    "3. Each function in foo.c that you'd like to add to namespace must start with `foo_`\n"
    "4. For adding sub-namespace use `foo__subname__` prefix\n"
    "5. Only one level of sub-namespace is allowed\n"
    "6. You may not declare function signature in header, and ony use .c static functions\n"
    "7. Functions with `static inline` are not included into namespace\n" 
    "8. Functions with prefix `foo__some` are considered internal and not included\n"
    "9. New namespace is created when you use exact foo.c argument, `all` just for updates\n"

    ;
    // clang-format on

    char* ignore_kw = cexy$process_ignore_kw;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "process [options] all|path/some_file.c",
        .description = process_help,
        .epilog = "\nUse `all` for updates, and exact path/some_file.c for creating new\n",
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(
                &ignore_kw,
                'i',
                "ignore",
                .help = "ignores `keyword` or `keyword()` from processed function signatures\n  uses cexy$process_ignore_kw"
            ),
        ),
    };
    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* target = argparse.next(&cmd_args);

    if (target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or path/some_file.c",
            target
        );
    }


    bool only_update = true;
    if (str.eq(target, "all")) {
        target = "*.c";
    } else {
        if (!os.path.exists(target)) {
            return e$raise(Error.not_found, "Target file not exists: '%s'", target);
        }
        only_update = false;
    }


    mem$scope(tmem$, _)
    {
        char* build_path = os.path.abs(cexy$build_dir, _);
        char* test_path = os.path.abs("./tests/", _);

        for$each (src_fn, os.fs.find(target, true, _)) {
            if (only_update) {
                char* abspath = os.path.abs(src_fn, _);
                if (str.starts_with(abspath, build_path) || str.starts_with(abspath, test_path)) {
                    continue;
                }
            }
            char* basename = os.path.basename(src_fn, _);
            if (str.starts_with(basename, "test") || str.eq(basename, "cex.c")) { continue; }
            mem$scope(tmem$, _)
            {
                e$assert(str.ends_with(src_fn, ".c") && "file must end with .c");

                char* hdr_fn = str.clone(src_fn, _);
                hdr_fn[str.len(hdr_fn) - 1] = 'h'; // .c -> .h

                str_s ns_prefix = str.sub(os.path.basename(src_fn, _), 0, -2); // src.c -> src
                log$debug(
                    "Cex Processing src: '%s' hdr: '%s' prefix: '%S'\n",
                    src_fn,
                    hdr_fn,
                    ns_prefix
                );
                if (!os.path.exists(hdr_fn)) {
                    if (only_update) {
                        log$debug("CEX skipped (no .h file for: %s)\n", src_fn);
                        continue;
                    } else {
                        return e$raise(Error.not_found, "Header file not exists: '%s'", hdr_fn);
                    }
                }
                char* code = io.file.load(src_fn, _);
                e$assert(code && "failed loading code");
                arr$(cex_token_s) items = arr$new(items, _);
                arr$(cex_decl_s*) decls = arr$new(decls, _, .capacity = 128);

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) {
                        return e$raise(
                            Error.integrity,
                            "Error parsing file %s, at line: %d, cursor: %d",
                            src_fn,
                            lx.line,
                            (i32)(lx.cur - lx.content)
                        );
                    }
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, cexy$process_ignore_kw, _);
                    if (d == NULL) { continue; }
                    if (d->type != CexTkn__func_def) { continue; }
                    if (d->is_inline && d->is_static) { continue; }
                    if (!_cexy__fn_match(d->name, ns_prefix)) { continue; }
                    log$trace("FN: %S ret_type: '%s' args: '%s'\n", d->name, d->ret_type, d->args);
                    arr$push(decls, d);
                }
                if (arr$len(decls) == 0) {
                    log$info("CEX skipped (no cex decls found in : %s)\n", src_fn);
                    continue;
                }

                arr$sort(decls, _cexy__decl_comparator);

                sbuf_c cex_h_struct = sbuf.create(10 * 1024, _);
                sbuf_c cex_h_var_decl = sbuf.create(1024, _);
                sbuf_c cex_c_var_def = sbuf.create(10 * 1024, _);

                e$ret(sbuf.appendf(
                    &cex_h_var_decl,
                    "CEX_NAMESPACE struct __cex_namespace__%S %S;\n",
                    ns_prefix,
                    ns_prefix
                ));
                e$ret(_cexy__process_gen_struct(ns_prefix, decls, &cex_h_struct));
                e$ret(_cexy__process_gen_var_def(ns_prefix, decls, &cex_c_var_def));
                e$ret(_cexy__process_update_code(
                    src_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));
                e$ret(_cexy__process_update_code(
                    hdr_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));

                // log$info("cex_h_struct: \n%s\n", cex_h_struct);
                log$info("CEX processed: %s\n", src_fn);
            }
        }
    }
    return EOK;
}

static Exception
cexy__cmd__stats(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    f64 tstart = os.timer();

    // clang-format off
    char* stats_help = ""
    "Parses full project code and calculates lines of code and code quality metrics"
    ;
    // clang-format on

    bool verbose = false;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "stats [options] [src/*.c] [path/some_file.c] [!exclude/*.c]",
        .description = stats_help,
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(&verbose, 'v', "verbose", .help = "prints individual file lines as well"),
        ),
    };
    e$ret(argparse.parse(&cmd_args, argc, argv));


    struct code_stats
    {
        u32 n_files;
        u32 n_asserts;
        u32 n_lines_code;
        u32 n_lines_comments;
        u32 n_lines_total;
    } code_stats = { 0 }, test_stats = { 0 };

    mem$scope(tmem$, _)
    {
        hm$(char*, bool) src_files = hm$new(src_files, _, .capacity = 1024);
        hm$(char*, bool) excl_files = hm$new(excl_files, _, .capacity = 128);

        char* target = argparse.next(&cmd_args);
        if (target == NULL) { target = "*.[ch]"; }

        do {
            bool is_exclude = false;
            if (target[0] == '!') {
                if (target[1] == '\0') { continue; }
                is_exclude = true;
                target++;
            }
            for$each (src_fn, os.fs.find(target, true, _)) {
                char* p = os.path.abs(src_fn, _);
                if (is_exclude) {
                    log$trace("Ignoring: %s\n", p);
                    hm$set(excl_files, p, true);
                } else {
                    hm$set(src_files, p, true);
                    // log$trace("Including: %s\n", p);
                }
            }
        } while ((target = argparse.next(&cmd_args)));

        // WARNING: sorting of hashmap items is a dead end, hash indexes get invalidated
        qsort(src_files, hm$len(src_files), sizeof(*src_files), str.qscmp);

        if (verbose) {
            io.printf("Files found: %d excluded: %d\n", hm$len(src_files), hm$len(excl_files));
        }
        for$each (src_fn, src_files) {
            if (hm$getp(excl_files, src_fn.key)) { continue; }

            char* basename = os.path.basename(src_fn.key, _);
            if (str.eq(basename, "cex.h")) { continue; }
            struct code_stats* stats = (str.find(basename, "test") != NULL) ? &test_stats
                                                                            : &code_stats;

            mem$scope(tmem$, _)
            {
                char* code = io.file.load(src_fn.key, _);
                if (!code) { return e$raise(Error.os, "Error opening file: '%s'", src_fn); }
                stats->n_files++;

                if (code[0] != '\0') { stats->n_lines_total++; }
                CexParser_c lx = CexParser.create(code, 0, false);
                cex_token_s t;

                u32 last_line = 0;
                u32 file_loc = 0;
                while ((t = CexParser.next_token(&lx)).type) {
                    if (t.type == CexTkn__error) {
                        return e$raise(
                            Error.integrity,
                            "Error parsing file %s, at line: %d, cursor: %d",
                            src_fn,
                            lx.line,
                            (i32)(lx.cur - lx.content)
                        );
                    }
                    switch (t.type) {
                        case CexTkn__ident:
                            if (t.value.len >= 6) {
                                for (usize i = 0; i < t.value.len; i++) {
                                    t.value.buf[i] = tolower(t.value.buf[i]);
                                }
                                if (str.slice.starts_with(t.value, str$s("uassert")) ||
                                    str.slice.starts_with(t.value, str$s("assert")) ||
                                    str.slice.starts_with(t.value, str$s("ensure")) ||
                                    str.slice.starts_with(t.value, str$s("enforce")) ||
                                    str.slice.starts_with(t.value, str$s("expect")) ||
                                    str.slice.starts_with(t.value, str$s("e$assert")) ||
                                    str.slice.starts_with(t.value, str$s("tassert"))) {
                                    stats->n_asserts++;
                                }
                            }
                            goto def;
                        case CexTkn__comment_multi: {
                            for$each (c, t.value.buf, t.value.len) {
                                if (c == '\n') { stats->n_lines_comments++; }
                            }
                        }
                            fallthrough();
                        case CexTkn__comment_single:
                            stats->n_lines_comments++;
                            break;
                        case CexTkn__preproc: {
                            for$each (c, t.value.buf, t.value.len) {
                                if (c == '\n') {
                                    stats->n_lines_code++;
                                    file_loc++;
                                }
                            }
                        }
                            fallthrough();
                        default:
                        def:
                            if (last_line < lx.line) {
                                stats->n_lines_code++;
                                file_loc++;
                                stats->n_lines_total += (lx.line - last_line);
                                last_line = lx.line;
                            }
                            break;
                    }
                }
                if (verbose) {
                    char* pcur = str.replace(src_fn.key, os.fs.getcwd(_), ".", _);
                    io.printf("%5d loc | %s\n", file_loc, pcur);
                }
            }
        }
    }

    // clang-format off
    io.printf("Project stats (parsed in %0.3fsec)\n", os.timer()-tstart);
    io.printf("--------------------------------------------------------\n");
    io.printf("%-25s|  %10s  |  %10s  |\n", "Metric", "Code   ", "Tests   ");
    io.printf("--------------------------------------------------------\n");
    io.printf("%-25s|  %10d  |  %10d  |\n", "Files", code_stats.n_files, test_stats.n_files);
    io.printf("%-25s|  %10d  |  %10d  |\n", "Asserts", code_stats.n_asserts, test_stats.n_asserts);
    io.printf("%-25s|  %10d  |  %10d  |\n", "Lines of code", code_stats.n_lines_code, test_stats.n_lines_code);
    io.printf("%-25s|  %10d  |  %10d  |\n", "Lines of comments", code_stats.n_lines_comments, test_stats.n_lines_comments);
    io.printf("%-25s|  %10.2f%% |  %10.2f%% |\n", "Asserts per LOC",
        (code_stats.n_lines_code) ? ((double)code_stats.n_asserts/(double)code_stats.n_lines_code*100.0) : 0.0, 
        (test_stats.n_lines_code) ? ((double)test_stats.n_asserts/(double)test_stats.n_lines_code*100.0) : 0.0
        ); 
    io.printf("%-25s|  %10.2f%% |         <<<  |\n", "Total asserts per LOC",
        (code_stats.n_lines_code) ? ((double)(code_stats.n_asserts+test_stats.n_asserts)/(double)code_stats.n_lines_code*100.0) : 0.0 
        );
    io.printf("--------------------------------------------------------\n");
    // clang-format on
    return EOK;
}

static bool
_cexy__is_str_pattern(char* s)
{
    if (s == NULL) { return false; }
    char pat[] = { '*', '?', '(', '[' };

    while (*s) {
        for (u32 i = 0; i < arr$len(pat); i++) {
            if (unlikely(pat[i] == *s)) { return true; }
        }
        s++;
    }
    return false;
}

static int
_cexy__help_qscmp_decls_type(const void* a, const void* b)
{
    // this struct fields must match to cexy.cmd.help() hm$
    const struct
    {
        str_s key;
        cex_decl_s* value;
    }* _a = a;
    typeof(_a) _b = b;
    if (_a->value->type != _b->value->type) {
        return _b->value->type - _a->value->type;
    } else {
        return str.slice.qscmp(&_a->key, &_b->key);
    }
}

static const char*
_cexy__colorize_ansi(str_s token, str_s exact_match, char current_char)
{
    if (token.len == 0) { return "\033[0m"; }

    static const char* types[] = {
        "\033[1;31m", // exact match red
        "\033[1;33m", // keywords
        "\033[1;32m", // types
        "\033[1;34m", // func/macro call
        "\033[1;35m", // #macro
        "\033[33m",   // macro const
    };
    static struct
    {
        str_s item;
        u8 style;
    } keywords[] = {
        { str$s("return"), 1 },
        { str$s("if"), 1 },
        { str$s("else"), 1 },
        { str$s("while"), 1 },
        { str$s("do"), 1 },
        { str$s("goto"), 1 },
        { str$s("mem$"), 1 },
        { str$s("tmem$"), 1 },
        { str$s("mem$scope"), 1 },
        { str$s("IAllocator"), 1 },
        { str$s("for$each"), 1 },
        { str$s("for$iter"), 1 },
        { str$s("e$except"), 1 },
        { str$s("e$except_silent"), 1 },
        { str$s("e$except_silent"), 1 },
        { str$s("char"), 2 },
        { str$s("var"), 2 },
        { str$s("arr$"), 2 },
        { str$s("Exception"), 2 },
        { str$s("Exc"), 2 },
        { str$s("const"), 2 },
        { str$s("FILE"), 2 },
        { str$s("int"), 2 },
        { str$s("bool"), 2 },
        { str$s("void"), 2 },
        { str$s("usize"), 2 },
        { str$s("isize"), 2 },
        { str$s("struct"), 2 },
        { str$s("enum"), 2 },
        { str$s("union"), 2 },
        { str$s("u8"), 2 },
        { str$s("i8"), 2 },
        { str$s("u16"), 2 },
        { str$s("i16"), 2 },
        { str$s("u32"), 2 },
        { str$s("i32"), 2 },
        { str$s("u64"), 2 },
        { str$s("i64"), 2 },
        { str$s("f64"), 2 },
        { str$s("f32"), 2 },
    };
    if (token.buf != NULL) {
        if (str.slice.eq(token, exact_match)) { return types[0]; }
        for$each (it, keywords) {
            if (it.item.len == token.len) {
                if (memcmp(it.item.buf, token.buf, token.len) == 0) {
                    uassert(it.style < arr$len(types));
                    return types[it.style];
                }
            }
        }
        // looks like function/macro call
        if (current_char == '(') { return types[3]; }

        // CEX style type suffix
        if (str.slice.ends_with(token, str$s("_s")) || str.slice.ends_with(token, str$s("_e")) ||
            str.slice.ends_with(token, str$s("_c")) || str.slice.ends_with(token, str$s("_kw"))) {
            return types[2];
        }

        // #preproc directive
        if (token.buf[0] == '#') { return types[4]; }

        // some$macro constant
        if (str.slice.index_of(token, str$s("$")) >= 0) { return types[5]; }
    }

    return "\033[0m"; // no color, not matched
}
static void
_cexy__colorize_print(str_s code, str_s name, FILE* output)
{
    if (code.buf == NULL || !io.isatty(output)) {
        io.fprintf(output, "%S", code);
        return;
    }

    str_s token = { .buf = code.buf, .len = 0 };
    bool in_token = false;
    (void)name;
    for (usize i = 0; i < code.len; i++) {
        char c = code.buf[i];

        if (!in_token) {
            if (isalpha(c) || c == '_' || c == '$' || c == '#') {
                in_token = true;
                token.buf = &code.buf[i];
                token.len = 1;
            } else {
                putc(c, output);
            }
        } else {
            switch (c) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                case '\v':
                case '\f':
                case '(':
                case ')':
                case '{':
                case '}':
                case '[':
                case ']':
                case ';':
                case '*':
                case '/':
                case '+':
                case '-':
                case ',': {
                    char _c = c;
                    if (c == ' ' && i < code.len - 1 && code.buf[i + 1] == '(') {
                        _c = '(';
                    } else if (c == ')' && i > 2 && token.buf[-1] == '*' && token.buf[-2] == '(') {
                        _c = '(';
                    }
                    io.fprintf(output, "%s%S\033[0m", _cexy__colorize_ansi(token, name, _c), token);
                    putc(c, output);
                    in_token = false;
                    break;
                }
                default:
                    token.len++;
            }
        }
    }

    if (in_token) {
        io.fprintf(output, "%s%S\033[0m", _cexy__colorize_ansi(token, name, 0), token);
    }
    return;
}

static Exception
_cexy__display_full_info(
    cex_decl_s* d,
    char* base_ns,
    bool show_source,
    bool show_example,
    arr$(cex_decl_s*) cex_ns_decls,
    FILE* output
)
{
    uassert(output);

    str_s name = d->name;
    str_s base_name = str.sstr(base_ns);

    mem$scope(tmem$, _)
    {
        if (!cex_ns_decls) {
            io.fprintf(output, "Symbol found at %s:%d\n\n", d->file, d->line + 1);
        }

        if (d->type == CexTkn__func_def) {
            name = _cexy__fn_dotted(d->name, base_ns, _);
            if (!name.buf) {
                // error converting to dotted name (fallback)
                name = d->name;
            }
        }
        if (d->docs.buf) {
            // strip doxygen tags
            if (str.slice.starts_with(d->docs, str$s("/**"))) {
                d->docs = str.slice.sub(d->docs, 3, 0);
            }
            if (str.slice.ends_with(d->docs, str$s("*/"))) {
                d->docs = str.slice.sub(d->docs, 0, -2);
            }
            _cexy__colorize_print(d->docs, name, output);
            io.fprintf(output, "\n");
        }

        if (output != stdout) {
            // For export using c code block (markdown compatible)
            io.fprintf(output, "\n```c\n");
        }

        cex_decl_s* ns_struct = NULL;

        if (cex_ns_decls) {
            // This only passed if we have cex namespace struct here
            mem$scope(tmem$, _)
            {

                hm$(str_s, cex_decl_s*) ns_symbols = hm$new(ns_symbols, _, .capacity = 128);
                for$each (it, cex_ns_decls) {
                    // Check if __namespace$ exists
                    if (it->type == CexTkn__macro_const &&
                        str.slice.starts_with(it->name, str$s("__"))) {
                        if (str.slice.eq(str.slice.sub(it->name, 2, -1), base_name)) {
                            ns_struct = it;
                        }
                    }
                    if (!str.slice.starts_with(it->name, base_name)) { continue; }

                    if ((it->type == CexTkn__macro_func || it->type == CexTkn__macro_const)) {
                        if (it->name.buf[base_name.len] != '$') { continue; }
                    } else if (it->type == CexTkn__typedef) {
                        if (it->name.buf[base_name.len] != '_') { continue; }
                        if (!(str.slice.ends_with(it->name, str$s("_c")) ||
                              str.slice.ends_with(it->name, str$s("_s")) || str.slice.ends_with(it->name, str$s("_kw")))) {
                            continue;
                        }
                    } else if (it->type == CexTkn__cex_module_struct) {
                        ns_struct = it;
                        continue; // does not add, use special treatment
                    } else {
                        continue;
                    }

                    if (hm$getp(ns_symbols, it->name) != NULL) {
                        // Maybe new with docs?
                        if (it->docs.buf) { hm$set(ns_symbols, it->name, it); }
                        continue; // duplicate
                    } else {
                        hm$set(ns_symbols, it->name, it);
                    }
                }
                // WARNING: sorting of hashmap items is a dead end, hash indexes get invalidated
                qsort(ns_symbols, hm$len(ns_symbols), sizeof(*ns_symbols), str.slice.qscmp);

                for$each (it, ns_symbols) {
                    if (it.value->docs.buf) {
                        str_s brief_str = _cexy__process_make_brief_docs(it.value);
                        if (brief_str.len) { io.fprintf(output, "/// %S\n", brief_str); }
                    }
                    char* l = NULL;
                    if (it.value->type == CexTkn__macro_func) {
                        l = str.fmt(_, "#define %S(%s)\n", it.value->name, it.value->args);
                    } else if (it.value->type == CexTkn__macro_const) {
                        l = str.fmt(_, "#define %S\n", it.value->name);
                    } else if (it.value->type == CexTkn__typedef) {
                        l = str.fmt(_, "%s %S\n", it.value->ret_type, it.value->name);
                    }
                    if (l) {
                        _cexy__colorize_print(str.sstr(l), name, output);
                        io.fprintf(output, "\n");
                    }
                }
            }
            io.fprintf(output, "\n\n");
        }

        if (d->type == CexTkn__macro_const || d->type == CexTkn__macro_func) {
            if (str.slice.starts_with(d->name, str$s("__"))) {
                if (output != stdout) { io.fprintf(output, "\n```\n"); }
                goto end; // NOTE: it's likely placeholder i.e. __foo$ - only for docs, just skip
            }
            io.fprintf(output, "#define ");
        }

        if (sbuf.len(&d->ret_type)) {
            _cexy__colorize_print(str.sstr(d->ret_type), name, output);
            io.fprintf(output, " ");
        }

        _cexy__colorize_print(name, name, output);
        io.fprintf(output, " ");

        if (sbuf.len(&d->args)) {
            io.fprintf(output, "(");
            _cexy__colorize_print(str.sstr(d->args), name, output);
            io.fprintf(output, ")");
        }
        if (!show_source && d->type == CexTkn__func_def) {
            io.fprintf(output, ";");
        } else if (d->body.buf) {
            _cexy__colorize_print(d->body, name, output);
            io.fprintf(output, ";");
        } else if (ns_struct) {
            _cexy__colorize_print(ns_struct->body, name, output);
        }
        io.fprintf(output, "\n");

        if (output != stdout) {
            // For export using c code block (markdown compatible)
            io.fprintf(output, "\n```\n");
        }

        // No examples for whole namespaces (early exit)
        if (!show_example || ns_struct) { goto end; }

        // Looking for a random example
        srand(time(NULL));
        io.fprintf(output, "\nSearching for examples of '%S'\n", name);
        arr$(char*) sources = os.fs.find("./*.[hc]", true, _);

        u32 n_used = 0;
        for$each (src_fn, sources) {
            mem$scope(tmem$, _)
            {
                char* code = io.file.load(src_fn, _);
                if (code == NULL) {
                    return e$raise(Error.not_found, "Error loading: %s\n", src_fn);
                }
                arr$(cex_token_s) items = arr$new(items, _);

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) { break; }
                    if (t.type != CexTkn__func_def) { continue; }
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
                    if (d == NULL) { continue; }
                    if (d->body.buf == NULL) { continue; }
                    if (str.slice.index_of(d->body, name) != -1) {
                        n_used++;
                        double dice = (double)rand() / (RAND_MAX + 1.0);
                        if (dice < 0.25) {
                            io.fprintf(output, "\n\nFound at %s:%d\n", src_fn, d->line);

                            if (output != stdout) { io.fprintf(output, "\n```c\n"); }

                            io.fprintf(output, "%s %S(%s)\n", d->ret_type, d->name, d->args);
                            _cexy__colorize_print(d->body, name, output);
                            io.fprintf(output, "\n");

                            if (output != stdout) { io.fprintf(output, "\n```\n"); }

                            goto end;
                        }
                    }
                }
            }
        }
        if (n_used == 0) {
            io.fprintf(output, "No usages of %S in the codebase\n", name);
        } else {
            io.fprintf(
                output,
                "%d usages of %S in the codebase, but no random pick, try again!\n",
                n_used,
                name
            );
        }
    }
end:
    if (output != stdout) {
        if (io.fflush(output)) {};
        io.fclose(&output);
    }
    return EOK;
}

static Exception
cexy__cmd__help(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    char* process_help = "Symbol / documentation search tool for C projects";
    char* epilog_help = 
        "\nQuery examples: \n"
        "cex help                     - list all namespaces in project directory\n"
        "cex help foo                 - find any symbol containing 'foo' (case sensitive)\n"
        "cex help foo.                - find namespace prefix: foo$, Foo_func(), FOO_CONST, etc\n"
        "cex help os$                 - find CEX namespace help (docs, macros, functions, types)\n"
        "cex help 'foo_*_bar'         - find using pattern search for symbols (see 'cex help str.match')\n"
        "cex help '*_(bar|foo)'       - find any symbol ending with '_bar' or '_foo'\n"
        "cex help str.find            - display function documentation if exactly matched\n"
        "cex help 'os$PATH_SEP'       - display macro constant value if exactly matched\n"
        "cex help str_s               - display type info and documentation if exactly matched\n"
        "cex help --source str.find   - display function source if exactly matched\n"
        "cex help --example str.find  - display random function use in codebase if exactly matched\n"
    ;
    char* filter = "./*.[hc]";
    char* out_file = NULL;
    bool show_source = false;
    bool show_example = false;

    // clang-format on
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "help [options] [query]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(&filter, 'f', "filter", .help = "file pattern for searching"),
            argparse$opt(&show_source, 's', "source", .help = "show full source on match"),
            argparse$opt(
                &show_example,
                'e',
                "example",
                .help = "finds random example in source base"
            ),
            argparse$opt(&out_file, 'o', "out", .help = "write output of command to file"),
        ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) { return Error.argsparse; }
    char* query = argparse.next(&cmd_args);
    str_s query_s = str.sstr(query);

    FILE* output = NULL;
    if (out_file) {
        e$ret(io.fopen(&output, out_file, "w"));
    } else {
        output = stdout;
    }

    mem$arena(1024 * 100, arena)
    {

        arr$(char*) sources = os.fs.find(filter, true, arena);
        if (os.fs.stat("./cex.h").is_symlink) { arr$push(sources, "./cex.h"); }
        arr$sort(sources, str.qscmp);

        char* query_pattern = NULL;
        bool is_namespace_filter = false;
        if (str.match(query, "[a-zA-Z0-9+].") || str.match(query, "[a-zA-Z0-9+]$")) {
            query_pattern = str.fmt(arena, "%S[._$]*", str.sub(query, 0, -1));
            is_namespace_filter = true;
        } else if (_cexy__is_str_pattern(query)) {
            query_pattern = query;
        } else {
            query_pattern = str.fmt(arena, "*%s*", query);
        }
        char* build_path = os.path.abs(cexy$build_dir, arena);
        char* test_path = os.path.abs("./tests/", arena);

        hm$(str_s, cex_decl_s*) names = hm$new(names, arena, .capacity = 1024);
        hm$(char*, char*) cex_ns_map = hm$new(cex_ns_map, arena, .capacity = 256);
        hm$set(cex_ns_map, "./cex.h", "cex");

        for$each (src_fn, sources) {
            mem$scope(tmem$, _)
            {
                char* abspath = os.path.abs(src_fn, _);
                if (str.starts_with(abspath, build_path) || str.starts_with(abspath, test_path)) {
                    continue;
                }

                auto basename = os.path.basename(src_fn, _);
                if (str.starts_with(basename, "_") || str.starts_with(basename, "test_")) {
                    continue;
                }
                char* base_ns = str.fmt(_, "%S", str.sub(basename, 0, -2));
                log$trace("Loading: %s (namespace: %s)\n", src_fn, base_ns);

                char* code = io.file.load(src_fn, arena);
                if (code == NULL) {
                    return e$raise(Error.not_found, "Error loading: %s\n", src_fn);
                }
                arr$(cex_token_s) items = arr$new(items, _);
                arr$(cex_decl_s*) all_decls = arr$new(all_decls, _);
                cex_decl_s* ns_decl = NULL;

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) {
                        log$error("Error parsing: %s at line: %d\n", src_fn, lx.line);
                        break;
                    }
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, arena);
                    if (d == NULL) { continue; }
                    arr$push(all_decls, d);

                    d->file = src_fn;
                    if (d->type == CexTkn__cex_module_struct || d->type == CexTkn__cex_module_def) {
                        log$trace("Found cex namespace: %s (namespace: %s)\n", src_fn, base_ns);
                        hm$set(cex_ns_map, src_fn, str.clone(base_ns, arena));
                    }

                    if (query == NULL) {
                        if (d->type == CexTkn__macro_const || d->type == CexTkn__macro_func) {
                            isize dollar = str.slice.index_of(d->name, str$s("$"));
                            str_s macro_ns = str.slice.sub(d->name, 0, dollar + 1);
                            if (dollar > 0 && !hm$getp(names, macro_ns)) {
                                hm$set(names, macro_ns, d);
                            }
                        } else if (d->type == CexTkn__typedef ||
                                   d->type == CexTkn__cex_module_struct) {
                            if (!hm$getp(names, d->name)) { hm$set(names, d->name, d); }
                        }
                    } else {
                        if (d->type == CexTkn__func_def) {
                            if (str.eq(query, "cex.") &&
                                str.slice.starts_with(d->name, str$s("cex_"))) {
                                continue;
                            }
                        }
                        str_s fndotted = (d->type == CexTkn__func_def)
                                           ? _cexy__fn_dotted(d->name, base_ns, _)
                                           : d->name;

                        if (str.slice.eq(d->name, query_s) || str.slice.eq(fndotted, query_s)) {
                            if (d->type == CexTkn__cex_module_def) { continue; }
                            if (d->type == CexTkn__typedef && d->ret_type[0] == '\0') { continue; }
                            if (is_namespace_filter) { continue; }
                            // We have full match display full help
                            return _cexy__display_full_info(
                                d,
                                base_ns,
                                show_source,
                                show_example,
                                NULL,
                                output
                            );
                        }

                        bool has_match = false;
                        if (str.slice.match(d->name, query_pattern)) { has_match = true; }
                        if (str.slice.match(fndotted, query_pattern)) { has_match = true; }
                        if (is_namespace_filter) {
                            str_s prefix = str.sub(query, 0, -1);
                            str_s sub_name = str.slice.sub(d->name, 0, prefix.len);
                            if (prefix.buf[prefix.len] == '.') {
                                // query case: ./cex help foo.
                                if (str.slice.eqi(sub_name, prefix) &&
                                    sub_name.buf[prefix.len] == '_') {
                                    if (d->type == CexTkn__func_def && str.eqi(query, "cex.")) {
                                        // skipping other namespaces of cex, e.g. cex_str_len()
                                        continue;
                                    }
                                    has_match = true;
                                }
                            } else {
                                // query case: ./cex help foo$
                                if (d->type == CexTkn__macro_const &&
                                    str.slice.starts_with(d->name, str$s("__"))) {
                                    // include __foo$ (doc name)
                                    sub_name = str.slice.sub(d->name, 2, -1);
                                    if (str.slice.eq(sub_name, prefix)) {
                                        ns_decl = d;
                                        has_match = true;
                                    }
                                }
                                if (str.slice.eq(sub_name, prefix)) {
                                    if (d->type == CexTkn__cex_module_struct &&
                                        str.slice.eq(d->name, prefix)) {
                                        // full match of CEX namespace, query: os$, d->name = 'os'
                                        ns_decl = d;
                                        has_match = true;
                                    } else {
                                        switch (sub_name.buf[prefix.len]) {
                                            case '_':
                                                if (d->type != CexTkn__typedef) { continue; }
                                                if (!(str.slice.ends_with(d->name, str$s("_c")) ||
                                                      str.slice.ends_with(d->name, str$s("_s")))) {
                                                    continue;
                                                }
                                                fallthrough();
                                            case '$':
                                                if (!ns_decl) { ns_decl = d; }
                                                has_match = true;
                                                break;
                                            default:
                                                continue;
                                        }
                                    }
                                }
                            }
                        }
                        if (has_match) { hm$set(names, d->name, d); }
                    }
                }

                if (ns_decl) {
                    char* ns_prefix = str.slice.clone(str.sub(query, 0, -1), _);
                    return _cexy__display_full_info(
                        ns_decl,
                        ns_prefix,
                        false,
                        false,
                        all_decls,
                        output
                    );
                }
                if (arr$len(names) == 0) { continue; }
            }
        }

        // WARNING: sorting of hashmap items is a dead end, hash indexes get invalidated
        qsort(names, hm$len(names), sizeof(*names), _cexy__help_qscmp_decls_type);

        for$each (it, names) {
            if (query == NULL) {
                switch (it.value->type) {
                    case CexTkn__cex_module_struct:
                        io.fprintf(output, "%-20s", "cexy namespace");
                        break;
                    case CexTkn__macro_func:
                    case CexTkn__macro_const:
                        io.fprintf(output, "%-20s", "macro namespace");
                        break;
                    default:
                        io.fprintf(output, "%-20s", CexTkn_str[it.value->type]);
                }
                io.fprintf(output, " %-30S %s:%d\n", it.key, it.value->file, it.value->line + 1);
            } else {
                str_s name = it.key;
                char* cex_ns = hm$get(cex_ns_map, (char*)it.value->file);
                if (cex_ns && it.value->type == CexTkn__func_def) {
                    name = _cexy__fn_dotted(name, cex_ns, arena);
                    if (!name.buf) {
                        // something weird happened, fallback
                        log$trace(
                            "Failed to make dotted name from %S, cex_ns: %s\n",
                            it.key,
                            cex_ns
                        );
                        name = it.key;
                    }
                }

                io.fprintf(
                    output,
                    "%-20s %-30S %s:%d\n",
                    CexTkn_str[it.value->type],
                    name,
                    it.value->file,
                    it.value->line + 1
                );
            }
        }
    }

    if (output && output != stdout) { io.fclose(&output); }

    return EOK;
}

static Exception
cexy__cmd__config(int argc, char** argv, void* user_ctx)
{
    Exc result = EOK;
    (void)argc;
    (void)argv;
    (void)user_ctx;

    // clang-format off
    char* process_help = "Check project and system environment";
    char* epilog_help = 
        "\nProject setup examples: \n"
    ;

    // clang-format on
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "config [options]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(argparse$opt_help(), ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) { return Error.argsparse; }
    // clang-format off
#define $env                                                                                \
    "\ncexy$* variables used in build system, see `cex help 'cexy$cc'` for more info\n"            \
    "* CEX_LOG_LVL               " cex$stringize(CEX_LOG_LVL) "\n"                                             \
    "* cexy$build_dir            " cexy$build_dir "\n"                                             \
    "* cexy$src_dir              " cexy$src_dir "\n"                                             \
    "* cexy$cc                   " cexy$cc "\n"                                                    \
    "* cexy$cc_include           " cex$stringize(cexy$cc_include) "\n"                             \
    "* cexy$cc_args_sanitizer    " cex$stringize(cexy$cc_args_sanitizer) "\n"                                \
    "* cexy$cc_args              " cex$stringize(cexy$cc_args) "\n"                                \
    "* cexy$cc_args_test         " cex$stringize(cexy$cc_args_test) "\n"                           \
    "* cexy$ld_args              " cex$stringize(cexy$ld_args) "\n"                                \
    "* cexy$fuzzer               " cex$stringize(cexy$fuzzer) "\n"                                \
    "* cexy$debug_cmd            " cex$stringize(cexy$debug_cmd) "\n"                              \
    "* cexy$pkgconf_cmd          " cex$stringize(cexy$pkgconf_cmd) "\n"                              \
    "* cexy$pkgconf_libs         " cex$stringize(cexy$pkgconf_libs) "\n"                              \
    "* cexy$process_ignore_kw    " cex$stringize(cexy$process_ignore_kw) "\n"\
    "* cexy$cex_self_args        " cex$stringize(cexy$cex_self_args) "\n"\
    "* cexy$cex_self_cc          " cexy$cex_self_cc "\n" // clang-format on

    io.printf("%s", $env);

    mem$scope(tmem$, _)
    {
        bool has_git = os.cmd.exists("git");

        io.printf("\nTools installed (optional):\n");
        io.printf("* git                       %s\n", has_git ? "OK" : "Not found");
        char* pkgconf_cmd[] = { cexy$pkgconf_cmd };
        bool has_pkg_config = false;
        if (arr$len(pkgconf_cmd) && pkgconf_cmd[0] && pkgconf_cmd[0][0] != '\0') {
            has_pkg_config = os.cmd.exists(pkgconf_cmd[0]);
        }
        if (has_pkg_config) {
            io.printf(
                "* cexy$pkgconf_cmd          %s (%s)\n",
                "OK",
                cex$stringize(cexy$pkgconf_cmd)
            );
        } else {
            switch (os.platform.current()) {
                case OSPlatform__win:
                    io.printf(
                        "* cexy$pkgconf_cmd          %s\n",
                        "Not found (try `pacman -S pkg-config` via MSYS2 or try this https://github.com/skeeto/u-config)"
                    );
                    break;
                case OSPlatform__macos:
                    io.printf(
                        "* cexy$pkgconf_cmd          %s\n",
                        "Not found (try `brew install pkg-config`)"
                    );
                    break;
                default:
                    io.printf(
                        "* cexy$pkgconf_cmd          %s\n",
                        "Not found (try to install `pkg-config` using package manager)"
                    );
                    break;
            }
        }

        char* triplet[] = { cexy$vcpkg_triplet };
        char* vcpkg_root = cexy$vcpkg_root;

        // NOLINTNEXTLINE
        if (arr$len(triplet) && triplet[0] != NULL && triplet[0][0] != '\0') {
            io.printf("* cexy$vcpkg_root           %s\n", (vcpkg_root) ? vcpkg_root : "Not set)");
            io.printf("* cexy$vcpkg_triplet        %s\n", triplet[0]);
            if (!vcpkg_root) {
                log$error("Build system expects vcpkg to be installed and configured\n");
                result = "vcpkg not installed/misconfigured";
            }
        } else {
            io.printf("* cexy$vcpkg_root           %s\n", (vcpkg_root) ? vcpkg_root : "Not set");
            io.printf("* cexy$vcpkg_triplet        %s\n", "Not set");
        }

        char* pkgconf_libargs[] = { cexy$pkgconf_libs };
        if (arr$len(pkgconf_libargs)) {
            arr$(char*) args = arr$new(args, _);
            Exc err = cexy$pkgconf(_, &args, "--libs", cexy$pkgconf_libs);
            if (err == EOK) {
                io.printf(
                    "* cexy$pkgconf_libs         %s (all found)\n",
                    str.join(args, arr$len(args), " ", _)
                );
            } else {
                io.printf("* cexy$pkgconf_libs         %s [%s]\n", "ERROR", err);
                // NOLINTNEXTLINE
                if (err == Error.runtime && arr$len(triplet) && triplet[0] != NULL) {
                    // pkg-conf failed to find lib it could be a missing .pc
                    io.printf(
                        "WARNING: Looks like vcpkg library (.a file) exists, but missing/misspelled .pc file for pkg-conf. \n"
                    );
                    io.printf("\tTry to resolve it manually via cexy$ld_args.\n");
                    io.printf(
                        "\tPKG_CONFIG_LIBDIR: %s/installed/%s/lib/pkgconfig\n",
                        vcpkg_root,
                        triplet[0]
                    );
                }
                io.printf("\tCompile with `#define CEX_LOG_LVL 5` for more info\n");
                result = "Missing Libs";
            }
        }

        io.printf("\nGlobal environment:\n");
        io.printf(
            "* Cex Version               %d.%d.%d (%s)\n",
            cex$version_major,
            cex$version_minor,
            cex$version_patch,
            cex$version_date
        );
        io.printf("* Git Hash                  %s\n", has_git ? cexy.utils.git_hash(_) : "(no git)");
        io.printf("* os.platform.current()     %s\n", os.platform.to_str(os.platform.current()));
        io.printf("* ./cex -D<ARGS> config     %s\n", cex$stringize(_CEX_SELF_DARGS));
    }


#    undef $env

    return result;
}

static Exception
cexy__cmd__libfetch(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    char* process_help = "Fetching 3rd party libraries via git (by default it uses cex git repo as source)";
    char* epilog_help = 
        "\nCommand examples: \n"
        "cex libfetch lib/test/fff.h                            - fetch signle header lib from CEX repo\n"
        "cex libfetch -U cex.h                                  - update cex.h to most recent version\n"
        "cex libfetch lib/random/                               - fetch whole directory recursively from CEX lib\n"
        "cex libfetch --git-label=v2.0 file.h                   - fetch using specific label or commit\n"
        "cex libfetch -u https://github.com/m/lib.git file.h    - fetch from arbitrary repo\n"
        "cex help --example cexy.utils.git_lib_fetch            - you can call it from your cex.c (see example)\n"
    ;
    // clang-format on

    char* git_url = "https://github.com/alexveden/cex.git";
    char* git_label = "HEAD";
    char* out_dir = "./";
    bool update_existing = false;
    bool preserve_dirs = true;

    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "libfetch [options]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(
            argparse$opt_help(),
            argparse$opt(&git_url, 'u', "git-url", .help = "Git URL of the repository"),
            argparse$opt(&git_label, 'l', "git-label", .help = "Git label"),
            argparse$opt(
                &out_dir,
                'o',
                "out-dir",
                .help = "Output directory relative to project root"
            ),
            argparse$opt(
                &update_existing,
                'U',
                "update",
                .help = "Force replacing existing code with repository files"
            ),
            argparse$opt(
                &preserve_dirs,
                'p',
                "preserve-dirs",
                .help = "Preserve directory structure as in repo"
            ),
        ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) { return Error.argsparse; }

    e$ret(cexy.utils.git_lib_fetch(
        git_url,
        git_label,
        out_dir,
        update_existing,
        preserve_dirs,
        (char**)cmd_args.argv,
        cmd_args.argc
    ));

    return EOK;
}

static Exception
cexy__cmd__simple_test(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "test [options] {run,build,create,clean,debug} all|tests/test_file.c [--test-options]",
        .description = _cexy$cmd_test_help,
        .epilog = _cexy$cmd_test_epilog,
        argparse$opt_list(argparse$opt_help(), ),
    };

    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* cmd = argparse.next(&cmd_args);
    char* target = argparse.next(&cmd_args);

    if (!str.match(cmd, "(run|build|create|clean|debug)") || target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Invalid command: '%s' or target: '%s'", cmd, target);
    }

    if (str.eq(cmd, "create")) {
        e$ret(cexy.test.create(target, false));
        return EOK;
    } else if (str.eq(cmd, "clean")) {
        e$ret(cexy.test.clean(target));
        return EOK;
    }
    bool single_test = !str.eq(target, "all");
    e$ret(cexy.test.make_target_pattern(&target)); // validation + convert 'all' -> "tests/test_*.c"

    log$info("Tests building: %s\n", target);
    // Build stage
    u32 n_tests = 0;
    u32 n_built = 0;
    (void)n_tests;
    (void)n_built;
    mem$scope(tmem$, _)
    {
        for$each (test_src, os.fs.find(target, true, _)) {
            char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
            log$trace("Test src: %s -> %s\n", test_src, test_target);
            fflush(stdout); // typically for CI
            n_tests++;
            if (!single_test && !cexy.src_include_changed(test_target, test_src, NULL)) {
                continue;
            }
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, cexy$cc, );
            // NOTE: reconstructing char*[] because some cexy$ variables might be empty
            char* cc_args_test[] = { cexy$cc_args_test };
            char* cc_include[] = { cexy$cc_include };
            char* cc_ld_args[] = { cexy$ld_args };
            arr$pusha(args, cc_args_test);
            arr$pusha(args, cc_include);
            arr$push(args, test_src);
            arr$pusha(args, cc_ld_args);
            char* pkgconf_libargs[] = { cexy$pkgconf_libs };
            if (arr$len(pkgconf_libargs)) {
                e$ret(cexy$pkgconf(_, &args, "--cflags", "--libs", cexy$pkgconf_libs));
            }
            arr$pushm(args, "-o", test_target);


            arr$push(args, NULL);
            e$ret(os$cmda(args));
            n_built++;
        }
    }

    log$info("Tests building: %d tests processed, %d tests built\n", n_tests, n_built);
    fflush(stdout);

    if (str.match(cmd, "(run|debug)")) {
        e$ret(cexy.test.run(target, str.eq(cmd, "debug"), cmd_args.argc, cmd_args.argv));
    }
    return EOK;
}

static Exception
cexy__utils__make_new_project(char* proj_dir)
{
    log$info("Creating new boilerplate CEX project in '%s'\n", proj_dir);
    mem$scope(tmem$, _)
    {
        if (!str.eq(proj_dir, ".")) {
            if (os.path.exists(proj_dir)) {
                return e$raise(Error.exists, "New project dir already exists: %s", proj_dir);
            }
            e$ret(os.fs.mkdir(proj_dir));
            log$info("Copying this 'cex.h' into '%s/cex.h'\n", proj_dir);
            e$ret(os.fs.copy("./cex.h", os$path_join(_, proj_dir, "cex.h")));
        } else {
            // Creating in the current dir
            if (os.path.exists("./cex.c")) {
                return e$raise(
                    Error.exists,
                    "New project seems to de already initialized, cex.c exists"
                );
            }
        }

        log$info("Making '%s/cex.c'\n", proj_dir);
        auto cex_c = os$path_join(_, proj_dir, "cex.c");
        auto lib_h = os$path_join(_, proj_dir, "lib", "mylib.h");
        auto lib_c = os$path_join(_, proj_dir, "lib", "mylib.c");
        auto app_c = os$path_join(_, proj_dir, "src", "myapp.c");
        e$assert(!os.path.exists(cex_c) && "cex.c already exists");
        e$assert(!os.path.exists(lib_h) && "mylib.h already exists");
        e$assert(!os.path.exists(lib_h) && "mylib.c already exists");
        e$assert(!os.path.exists(app_c) && "myapp.c already exists");

#    ifdef _cex_main_boilerplate
        e$ret(io.file.save(os$path_join(_, proj_dir, "cex.c"), _cex_main_boilerplate));
#    else
        e$ret(os.fs.copy("src/cex_boilerplate.c", cex_c));
#    endif

        // Simple touching file first ./cex call will update its flags
        e$ret(io.file.save(os$path_join(_, proj_dir, "compile_flags.txt"), ""));

        // Basic cex-related stuff
        char* git_ignore = "cex\n"
                           "build/\n"
                           "compile_flags.txt\n";
        e$ret(io.file.save(os$path_join(_, proj_dir, ".gitignore"), git_ignore));

        sbuf_c buf = sbuf.create(1024 * 10, _);
        {
            log$info("Making '%s'\n", lib_h);
            e$ret(os.fs.mkpath(lib_h));
            sbuf.clear(&buf);
            cg$init(&buf);
            cg$pn("#pragma once");
            cg$pn("#include \"cex.h\"");
            cg$pn("");
            cg$pn("i32 mylib_add(i32 a, i32 b);");
            e$ret(io.file.save(lib_h, buf));
        }

        {
            log$info("Making '%s'\n", lib_c);
            e$ret(os.fs.mkpath(lib_c));
            sbuf.clear(&buf);
            cg$init(&buf);
            cg$pn("#include \"mylib.h\"");
            cg$pn("#include \"cex.h\"");
            cg$pn("");
            cg$func("i32 mylib_add(i32 a, i32 b)", "")
            {
                cg$pn("return a + b;");
            }
            e$ret(io.file.save(lib_c, buf));
        }

        {
            log$info("Making '%s'\n", app_c);
            e$ret(os.fs.mkpath(app_c));
            sbuf.clear(&buf);
            cg$init(&buf);
            cg$pn("#define CEX_IMPLEMENTATION");
            cg$pn("#include \"cex.h\"");
            cg$pn("#include \"lib/mylib.c\"  /* NOTE: include .c to make unity build! */");
            cg$pn("");
            cg$func("int main(int argc, char** argv)", "")
            {
                cg$pn("(void)argc;");
                cg$pn("(void)argv;");
                cg$pn("io.printf(\"MOCCA - Make Old C Cexy Again!\\n\");");
                cg$pn("io.printf(\"1 + 2 = %d\\n\", mylib_add(1, 2));");
                cg$pn("return 0;");
            }
            e$ret(io.file.save(app_c, buf));
        }

        auto test_c = os$path_join(_, proj_dir, "tests", "test_mylib.c");
        log$info("Making '%s'\n", test_c);
        e$ret(cexy.test.create(test_c, true));
        log$info("Compiling new cex app for a new project...\n");
        auto old_dir = os.fs.getcwd(_);

        e$ret(os.fs.chdir(proj_dir));
        char* bin_path = "cex" cexy$build_ext_exe;
        char* old_name = str.fmt(_, "%s.old", bin_path);
        if (os.path.exists(bin_path)) {
            if (os.path.exists(old_name)) { e$ret(os.fs.remove(old_name)); }
            e$ret(os.fs.rename(bin_path, old_name));
        }
        e$ret(os$cmd(cexy$cex_self_cc, "-o", bin_path, "cex.c"));
        if (os.path.exists(old_name)) {
            if (os.fs.remove(old_name)) {
                // WTF: win32 might lock old_name, try to remove it, but maybe no luck
            }
        }
        e$ret(os.fs.chdir(old_dir));
        log$info("New project has been created in %s\n", proj_dir);
    }

    return EOK;
}

static Exception
cexy__cmd__new(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "new [new_proj_dir]",
        .description = "Creates new boilerplate CEX project",
        argparse$opt_list(argparse$opt_help(), ),
    };

    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* proj_dir = argparse.next(&cmd_args);

    if (proj_dir == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Expected new project directory path.");
    }

    e$ret(cexy.utils.make_new_project(proj_dir));

    return EOK;
}

Exception
cexy__app__create(char* target)
{
    mem$scope(tmem$, _)
    {
        char* app_src = os$path_join(_, cexy$src_dir, target, str.fmt(_, "%s.c", target));
        if (os.path.exists(app_src)) {
            return e$raise(Error.exists, "App file already exists: %s", app_src, target);
        }
        e$ret(os.fs.mkpath(app_src));

        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        // clang-format off
        cg$pn("#include \"cex.h\"");
        cg$pn("");
        cg$func("Exception\n%s(int argc, char** argv)\n", target)
        {
            cg$pn("bool my_flag = false;");
            cg$scope("argparse_c args = ", target)
            {
                cg$pn(".description = \"New CEX App\",");
                cg$pn("argparse$opt_list(");
                cg$pn("    argparse$opt_help(),");
                cg$pn("    argparse$opt(&my_flag, 'c', \"ctf\", .help = \"Capture the flag\"),");
                cg$pn("),");
            }
            cg$pa(";\n", "");
            cg$pn("if (argparse.parse(&args, argc, argv)) { return Error.argsparse; }");
            cg$pn("io.printf(\"MOCCA - Make Old C Cexy Again!\\n\");");
            cg$pn("io.printf(\"%s\\n\", (my_flag) ? \"Flag is captured\" : \"Pass --ctf to capture the flag\");");

            cg$pn("return EOK;");
        };
        // clang-format on
        e$ret(io.file.save(app_src, buf));
    }

    mem$scope(tmem$, _)
    {
        char* app_src = os$path_join(_, cexy$src_dir, target, "main.c");
        if (os.path.exists(app_src)) {
            return e$raise(Error.exists, "App file already exists: %s", app_src, target);
        }
        e$ret(os.fs.mkpath(app_src));

        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        // clang-format off
        cg$pn("// NOTE: main.c serves as unity build container + detaching allows unit testing of app's code");
        cg$pn("#define CEX_IMPLEMENTATION");
        cg$pn("#include \"cex.h\"");
        cg$pf("#include \"%s.c\"", target);
        cg$pn("// TODO: add your app sources here (include .c)");
        cg$pn("");
        cg$func("int\nmain(int argc, char** argv)\n", "")
        {
            cg$pf("if (%s(argc, argv) != EOK) { return 1; }", target);
            cg$pn("return 0;");
        }
        // clang-format on
        e$ret(io.file.save(app_src, buf));
    }
    return EOK;
}

Exception
cexy__app__run(char* target, bool is_debug, int argc, char** argv)
{
    mem$scope(tmem$, _)
    {
        char* app_src;
        e$ret(cexy.app.find_app_target_src(_, target, &app_src));
        char* app_exe = cexy.target_make(app_src, cexy$build_dir, target, _);
        arr$(char*) args = arr$new(args, _);
        if (is_debug) { arr$pushm(args, cexy$debug_cmd); }
        arr$pushm(args, app_exe, );
        arr$pusha(args, argv, argc);
        arr$push(args, NULL);
        e$ret(os$cmda(args));
    }
    return EOK;
}

Exception
cexy__app__clean(char* target)
{
    mem$scope(tmem$, _)
    {
        char* app_src;
        e$ret(cexy.app.find_app_target_src(_, target, &app_src));
        char* app_exe = cexy.target_make(app_src, cexy$build_dir, target, _);
        if (os.path.exists(app_exe)) {
            log$info("Removing: %s\n", app_exe);
            e$ret(os.fs.remove(app_exe));
        }
    }
    return EOK;
}

Exception
cexy__app__find_app_target_src(IAllocator allc, char* target, char** out_result)
{
    uassert(out_result != NULL);
    *out_result = NULL;

    if (target == NULL) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            target
        );
    }
    if (str.eq(target, "all")) {
        return e$raise(Error.argsparse, "all target is not supported for this command");
    }

    if (_cexy__is_str_pattern(target)) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected alphanumerical name, patterns are not allowed",
            target
        );
    }
    if (!str.match(target, "[a-zA-Z0-9_+]")) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected alphanumerical name",
            target
        );
    }
    char* app_src = str.fmt(allc, "%s%c%s.c", cexy$src_dir, os$PATH_SEP, target);
    log$trace("Probing %s\n", app_src);
    if (!os.path.exists(app_src)) {
        mem$free(allc, app_src);

        app_src = os$path_join(allc, cexy$src_dir, target, "main.c");
        log$trace("Probing %s\n", app_src);
        if (!os.path.exists(app_src)) {
            mem$free(allc, app_src);
            return e$raise(Error.not_found, "App target source not found: %s", target);
        }
    }
    *out_result = app_src;
    return EOK;
}

static Exception
cexy__cmd__simple_app(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "app [options] {run,build,create,clean,debug} APP_NAME [--app-options app args]",
        argparse$opt_list(argparse$opt_help(), ),
    };

    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* cmd = argparse.next(&cmd_args);
    char* target = argparse.next(&cmd_args);

    if (!str.match(cmd, "(run|build|create|clean|debug)") || target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Invalid command: '%s' or target: '%s'", cmd, target);
    }

    if (str.eq(cmd, "create")) {
        e$ret(cexy.app.create(target));
        return EOK;
    } else if (str.eq(cmd, "clean")) {
        e$ret(cexy.app.clean(target));
        return EOK;
    }
    e$assert(os.path.exists(cexy$src_dir) && cexy$src_dir " not exists");

    mem$scope(tmem$, _)
    {
        char* app_src;
        e$ret(cexy.app.find_app_target_src(_, target, &app_src));
        char* app_exec = cexy.target_make(app_src, cexy$build_dir, target, _);
        log$trace("App src: %s -> %s\n", target, app_exec);
        if (!cexy.src_include_changed(app_exec, app_src, NULL)) { goto run; }
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, cexy$cc, );
        // NOTE: reconstructing char*[] because some cexy$ variables might be empty
        char* cc_args[] = { cexy$cc_args };
        char* cc_include[] = { cexy$cc_include };
        char* ld_args[] = { cexy$ld_args };
        char* pkgconf_libargs[] = { cexy$pkgconf_libs };
        arr$pusha(args, cc_args);
        arr$pusha(args, cc_include);
        if (arr$len(pkgconf_libargs)) {
            e$ret(cexy$pkgconf(_, &args, "--cflags", cexy$pkgconf_libs));
        }
        arr$push(args, (char*)app_src);
        arr$pusha(args, ld_args);
        if (arr$len(pkgconf_libargs)) {
            e$ret(cexy$pkgconf(_, &args, "--libs", cexy$pkgconf_libs));
        }
        arr$pushm(args, "-o", app_exec);


        arr$push(args, NULL);
        e$ret(os$cmda(args));

    run:
        if (str.match(cmd, "(run|debug)")) {
            e$ret(cexy.app.run(target, str.eq(cmd, "debug"), cmd_args.argc, cmd_args.argv));
        }
    }


    return EOK;
}

Exception
cexy__cmd__simple_fuzz(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    mem$scope(tmem$, _)
    {
        u32 max_time = 0;
        bool debug = false;
        u32 max_timeout_sec = 10;

        argparse_c cmd_args = {
            .program_name = "./cex",
            .usage = "fuzz [options] {run, create, debug} all|fuzz/some/fuzz_file.c [-fuz-options]",
            .description = "Compiles and runs fuzz test on target",
            argparse$opt_list(
                argparse$opt_help(),
                argparse$opt(
                    &max_time,
                    '\0',
                    "max-time",
                    .help = "Maximum time per fuzz in seconds (60 seconds if 'all' target)"
                ),
                argparse$opt(
                    &max_timeout_sec,
                    '\0',
                    "timeout",
                    .help = "Timeout in seconds for hanging task"
                ),
            ),
        };

        e$ret(argparse.parse(&cmd_args, argc, argv));
        char* cmd = argparse.next(&cmd_args);
        char* src = argparse.next(&cmd_args);
        if (src == NULL) {
            argparse.usage(&cmd_args);
            io.printf("Bad fuzz file argument\n");
            return Error.argsparse;
        }
        if (!str.match(cmd, "(run|create|debug)")) {
            argparse.usage(&cmd_args);
            return e$raise(Error.argsparse, "Invalid fuzz command: '%s'", cmd);
        }
        if (str.eq(cmd, "create")) {
            e$ret(cexy.fuzz.create(src));
            return EOK;
        } else if (str.eq(cmd, "debug")) {
            debug = true;
        }


        bool run_all = false;
        if (str.eq(src, "all")) {
            src = "fuzz/fuzz_*.c";
            run_all = true;
            if (max_time == 0) { max_time = 60; }
        } else {
            if (!os.path.exists(src)) {
                return e$raise(Error.not_found, "target not found: %s", src);
            }
        }

        char* proj_dir = os.path.abs(".", _);
        bool is_afl_fuzzer = false;

        for$each (src_file, os.fs.find(src, true, _)) {
            fflush(stdout); // typically for CI
            e$ret(os.fs.chdir(proj_dir));

            char* dir = os.path.dirname(src_file, _);
            char* file = os.path.basename(src_file, _);
            if (str.ends_with(dir, ".out") || str.ends_with(dir, ".afl") ||
                str.ends_with(dir, "_corpus")) {
                continue;
            }
            e$assert(str.ends_with(file, ".c"));
            str_s prefix = str.sub(file, 0, -2);

            char* target_exe = str.fmt(_, "%s/%S.fuzz", dir, prefix);
            arr$(char*) args = arr$new(args, _);
            arr$clear(args);
            if (!run_all || cexy.src_include_changed(target_exe, src_file, NULL)) {
                arr$pushm(args, cexy$fuzzer);
                e$assert(arr$len(args) > 0 && "empty cexy$fuzzer");
                e$assertf(os.cmd.exists(args[0]), "fuzzer command not found: %s", args[0]);
                if (str.find(args[0], "afl")) { is_afl_fuzzer = true; }
                if (is_afl_fuzzer) { arr$push(args, "-DCEX_FUZZ_AFL"); }

                char* cc_include[] = { cexy$cc_include };
                char* cc_ld_args[] = { cexy$ld_args };
                arr$pusha(args, cc_include);

                char* pkgconf_libargs[] = { cexy$pkgconf_libs };
                if (arr$len(pkgconf_libargs)) {
                    e$ret(cexy$pkgconf(_, &args, "--cflags", cexy$pkgconf_libs));
                }
                arr$push(args, src_file);
                arr$pusha(args, cc_ld_args);
                if (arr$len(pkgconf_libargs)) {
                    e$ret(cexy$pkgconf(_, &args, "--libs", cexy$pkgconf_libs));
                }
                arr$pushm(args, "-o", target_exe);
                arr$push(args, NULL);

                e$ret(os$cmda(args));
            }

            e$ret(os.fs.chdir(dir));

            arr$clear(args);
            if (is_afl_fuzzer) {
                // AFL++ or something
                e$ret(os.env.set("ASAN_OPTIONS", ""));

                if (debug) { e$assert(false && "AFL fuzzer debugging is not supported"); }
                arr$pushm(args, "afl-fuzz");

                if (cmd_args.argc > 0) {
                    // Fully user driven arguments
                    arr$pusha(args, cmd_args.argv, cmd_args.argc);
                } else {
                    char* corpus_dir = str.fmt(_, "%S_corpus", prefix);
                    char* corpus_dir_out = str.fmt(_, "%S_corpus.afl", prefix);
                    arr$pushm(args, "-i", corpus_dir, "-o", corpus_dir_out);

                    arr$pushm(args, "-t", str.fmt(_, "%d", max_timeout_sec * 1000));

                    if (max_time > 0) { arr$pushm(args, "-V", str.fmt(_, "%d", max_time)); }

                    char* dict_file = str.fmt(_, "%S.dict", prefix);
                    if (os.path.exists(dict_file)) { arr$pushm(args, "-x", dict_file); }

                    // adding exe file
                    arr$pushm(args, "--", str.fmt(_, "./%S.fuzz", prefix));
                }
            } else {
                // clang - libFuzzer
                if (debug) { arr$pushm(args, cexy$debug_cmd); }
                arr$pushm(
                    args,
                    str.fmt(_, "./%S.fuzz", prefix),
                    str.fmt(_, "-artifact_prefix=%S.", prefix)
                );
                if (cmd_args.argc > 0) {
                    arr$pusha(args, cmd_args.argv, cmd_args.argc);
                } else {
                    if (!debug) { arr$push(args, str.fmt(_, "-timeout=%d", max_timeout_sec)); }
                    if (max_time > 0) {
                        arr$pushm(args, str.fmt(_, "-max_total_time=%d", max_time));
                    }

                    char* dict_file = str.fmt(_, "%S.dict", prefix);
                    if (os.path.exists(dict_file)) {
                        arr$push(args, str.fmt(_, "-dict=%s", dict_file));
                    }

                    char* corpus_dir = str.fmt(_, "%S_corpus", prefix);
                    if (os.path.exists(corpus_dir)) {
                        char* corpus_dir_tmp = str.fmt(_, "%S_corpus.out", prefix);
                        e$ret(os.fs.mkdir(corpus_dir_tmp));

                        arr$push(args, corpus_dir_tmp);
                        arr$push(args, corpus_dir);
                    }
                }
            }

            arr$push(args, NULL);
            e$ret(os$cmda(args));
        }
    }
    return EOK;
}

static char*
cexy__utils__git_hash(IAllocator allc)
{
    mem$arena(1024, _)
    {
        if (!os.cmd.exists("git")) {
            log$error("git command not found, not installed or PATH issue\n");
            return NULL;
        }

        char* args[] = { "git", "rev-parse", "HEAD", NULL };
        os_cmd_c c = { 0 };
        e$except (
            err,
            os.cmd.create(&c, args, arr$len(args), &(os_cmd_flags_s){ .no_window = true })
        ) {
            return NULL;
        }
        char* output = os.cmd.read_all(&c, _);
        int err_code = 0;
        e$except_silent (err, os.cmd.join(&c, 0, &err_code)) {
            log$error("`git rev-parse HEAD` error: %s err_code: %d\n", err, err_code);
            return NULL;
        }
        if (output == NULL || output[0] == '\0') {
            log$error("`git rev-parse HEAD` returned empty result\n");
            return NULL;
        }

        str_s clean_hash = str.sstr(output);
        clean_hash = str.slice.strip(clean_hash);
        if (clean_hash.len > 50) {
            log$error("`git rev-parse HEAD` unexpectedly long hash, got: '%S'\n", clean_hash);
            return NULL;
        }

        if (!str.slice.match(clean_hash, "[0-9a-fA-F+]")) {
            log$error("`git rev-parse HEAD` expected hexadecimal hash, got: '%S'\n", clean_hash);
            return NULL;
        }

        return str.fmt(allc, "%S", clean_hash);
    }
    return NULL;
}

static Exception
_cexy__utils__pkgconf_parse(IAllocator allc, arr$(char*) * out_cc_args, char* pkgconf_output)
{
    e$assert(pkgconf_output != NULL);
    e$assert(out_cc_args != NULL);
    e$assert(allc != NULL);

    char* cur = pkgconf_output;
    char* arg = NULL;
    while (*cur) {
        switch (*cur) {
            case ' ':
            case '\t':
            case '\v':
            case '\f':
            case '\n':
            case '\r': {
                if (arg) {
                    str_s a = { .buf = arg, .len = cur - arg };
                    if (a.len > 0) { arr$push(*out_cc_args, str.slice.clone(a, allc)); }
                }
                arg = NULL;
                break;
            }
            case '"':
            case '\'': {
                if (!arg) { arg = cur; }
                char quote = *cur;
                cur++;
                while (*cur) {
                    if (*cur == '\\') {
                        cur += 2;
                        continue;
                    }
                    if (*cur == quote) { break; }
                    cur++;
                }
                break;
            }
            default: {
                if (!arg) { arg = cur; }
                if (*cur == '\\') { cur++; }
                break;
            }
        }

        if (*cur) { cur++; }
    }

    if (arg) {
        str_s a = { .buf = arg, .len = cur - arg };
        if (a.len > 0) { arr$push(*out_cc_args, str.slice.clone(a, allc)); }
    }

    return EOK;
}

static Exception
cexy__utils__pkgconf(
    IAllocator allc,
    arr$(char*) * out_cc_args,
    char** pkgconf_args,
    usize pkgconf_args_len
)
{
    e$assert(allc->meta.is_arena && "expected ArenaAllocator or mem$scope(tmem$, _)");
    e$assert(out_cc_args != NULL);
    e$assert(pkgconf_args != NULL);
    e$assert(pkgconf_args_len > 0);

    os_cmd_c c = { 0 };

    mem$arena(2048, _)
    {
        arr$(char*) args = arr$new(args, _);
        char* vcpkg_root = cexy$vcpkg_root;
        char* triplet[] = { cexy$vcpkg_triplet };

        if (vcpkg_root) {
            log$trace("Looking vcpkg libs at '%s' triplet='%s'\n", vcpkg_root, triplet[0]);
            if (!os.path.exists(vcpkg_root)) {
                return e$raise(Error.not_found, "cexy$vcpkg_root not exists: %s", vcpkg_root);
            }
            char* triplet_path = str.fmt(_, "%s/installed/%s", vcpkg_root, triplet[0]);
            if (!os.path.exists(triplet_path)) {
                return e$raise(Error.not_found, "vcpkg triplet path not exists: %s", triplet_path);
            }
            char* lib_path = str.fmt(_, "%s/lib/", triplet_path);
            if (!os.path.exists(lib_path)) {
                return e$raise(Error.not_found, "vcpkg lib path not exists: %s", lib_path);
            }
            char* inc_path = str.fmt(_, "%s/include/", triplet_path);
            if (!os.path.exists(inc_path)) {
                return e$raise(Error.not_found, "vcpkg include path not exists: %s", inc_path);
            }
            char* pkgconf_path = str.fmt(_, "%s/lib/pkgconfig", triplet_path);
            if (!os.path.exists(pkgconf_path)) {
                return e$raise(
                    Error.not_found,
                    "vcpkg lib/pkgconfig path not exists: %s",
                    pkgconf_path
                );
            }

            log$trace("Setting: PKG_CONFIG_LIBDIR='%s'\n", pkgconf_path);
            e$ret(os.env.set("PKG_CONFIG_LIBDIR", pkgconf_path));

            log$trace("Using PKG_CONFIG_LIBDIR from vcpkg, not all packages provide .pc files!\n");
            for$each (it, pkgconf_args, pkgconf_args_len) {
                if (str.starts_with(it, "--")) { continue; }

                // checking libfoo.a, and foo.a
                char* lib_search[] = {
                    str.fmt(
                        _,
                        "%s/%s%s" cexy$build_ext_lib_stat, // append platform specific .a or .lib
                        lib_path,
                        (str.starts_with(it, "lib")) ? "" : "lib",
                        it
                    ),
                    str.fmt(
                        _,
                        "%s/%S" cexy$build_ext_lib_stat, // append platform specific .a or .lib
                        lib_path,
                        (str.starts_with(it, "lib")) ? str.sub(it, 3, 0) : str.sstr(it)
                    )
                };
                bool is_found = false;
                for$each (lib_file_name, lib_search) {
                    log$trace("Probing lib: %s\n", lib_file_name);
                    if (os.path.exists(lib_file_name)) {
                        log$trace("Found lib: %s\n", lib_file_name);
                        is_found = true;
                    }
                }

                if (!is_found) {
                    return e$raise(
                        Error.not_found,
                        "vcpkg: lib '%s' not found (try `vcpkg install %s`) dir: %s",
                        it,
                        it,
                        lib_path
                    );
                }
            }
        }
        arr$pushm(args, cexy$pkgconf_cmd);
        arr$pusha(args, pkgconf_args, pkgconf_args_len);
        arr$push(args, NULL);
        log$trace("Looking system libs with: %s\n", args[0]);

#    if CEX_LOG_LVL > 4
        os$cmd(args[0], "--print-errors", "--variable", "pc_path", "pkg-config");
        io.printf("\n");
        log$trace("CMD: ");
        for (u32 i = 0; i < arr$len(args) - 1; i++) {
            char* a = args[i];
            io.printf(" ");
            if (str.find(a, " ")) {
                io.printf("\'%s\'", a);
            } else if (a == NULL || *a == '\0') {
                io.printf("\'(empty arg)\'");
            } else {
                io.printf("%s", a);
            }
        }
        io.printf("\n");

#    endif
        e$ret(
            os.cmd.create(&c, args, arr$len(args), &(os_cmd_flags_s){ .combine_stdouterr = true })
        );

        char* output = os.cmd.read_all(&c, _);
        e$except_silent (err, os.cmd.join(&c, 0, NULL)) {
            log$error("%s program error:\n%s\n", cexy$pkgconf_cmd, output);
            return err;
        }
        e$ret(_cexy__utils__pkgconf_parse(allc, out_cc_args, output));
    }

    return EOK;
}

static Exception
cexy__utils__make_compile_flags(
    char* flags_file,
    bool include_cexy_flags,
    arr$(char*) cc_flags_or_null
)
{
    mem$scope(tmem$, _)
    {
        e$assert(str.ends_with(flags_file, "compile_flags.txt") && "unexpected file name");

        arr$(char*) args = arr$new(args, _);
        if (include_cexy_flags) {
            char* cc_args[] = { cexy$cc_args };
            char* cc_include[] = { cexy$cc_include };
            arr$pusha(args, cc_args);
            arr$pusha(args, cc_include);
            char* pkgconf_libargs[] = { cexy$pkgconf_libs };
            if (arr$len(pkgconf_libargs)) {
                e$except (err, cexy$pkgconf(_, &args, "--cflags", cexy$pkgconf_libs)) {}
            }
        }
        if (cc_flags_or_null != NULL) { arr$pusha(args, cc_flags_or_null); }
        if (arr$len(args) == 0) { return e$raise(Error.empty, "Compiler flags are empty"); }

        FILE* fh;
        e$ret(io.fopen(&fh, flags_file, "w"));
        for$each (it, args) {
            if (str.starts_with(it, "-fsanitize")) { continue; }
            e$goto(io.file.writeln(fh, it), end); // write args
        }
    end:
        io.fclose(&fh);
    }

    return EOK;
}

static Exception
cexy__utils__git_lib_fetch(
    char* git_url,
    char* git_label,
    char* out_dir,
    bool update_existing,
    bool preserve_dirs,
    char** repo_paths,
    usize repo_paths_len
)
{
    if (git_url == NULL || git_url[0] == '\0') {
        return e$raise(Error.argument, "Empty or null git_url");
    }
    if (!str.ends_with(git_url, ".git")) {
        return e$raise(Error.argument, "git_url must end with .git, got: %s", git_url);
    }
    if (git_label == NULL || git_label[0] == '\0') { git_label = "HEAD"; }
    log$info("Checking libs from: %s @ %s\n", git_url, git_label);
    char* out_build_dir = cexy$build_dir "/cexy_git/";
    e$ret(os.fs.mkpath(out_build_dir));

    mem$scope(tmem$, _)
    {
        bool needs_update = false;
        for$each (it, repo_paths, repo_paths_len) {
            char* out_file = (preserve_dirs)
                               ? str.fmt(_, "%s/%s", out_dir, it)
                               : str.fmt(_, "%s/%s", out_dir, os.path.basename(it, _));

            log$info("Lib file: %s -> %s\n", it, out_file);
            if (!os.path.exists(out_file)) { needs_update = true; }
        }
        if (!needs_update && !update_existing) {
            log$info("Nothing to update, lib files exist, run with update flag if needed\n");
            return EOK;
        }

        str_s base_name = os.path.split(git_url, false);
        e$assert(base_name.len > 4);
        e$assert(str.slice.ends_with(base_name, str$s(".git")));
        base_name = str.slice.sub(base_name, 0, -4);

        char* repo_dir = str.fmt(_, "%s/%S/", out_build_dir, base_name);
        if (os.path.exists(repo_dir)) { e$ret(os.fs.remove_tree(repo_dir)); }

        e$ret(os$cmd(
            "git",
            "clone",
            "--depth=1",
            "--quiet",
            "--filter=blob:none",
            "--no-checkout",
            "--",
            git_url,
            repo_dir
        ));


        arr$(char*)
            git_checkout_args = arr$new(git_checkout_args, _, .capacity = repo_paths_len + 10);
        arr$pushm(git_checkout_args, "git", "-C", repo_dir, "checkout", git_label, "--");
        arr$pusha(git_checkout_args, repo_paths, repo_paths_len);
        arr$push(git_checkout_args, NULL);
        e$ret(os$cmda(git_checkout_args));

        for$each (it, repo_paths, repo_paths_len) {
            char* in_path = str.fmt(_, "%s/%s", repo_dir, it);

            char* out_path = (preserve_dirs)
                               ? str.fmt(_, "%s/%s", out_dir, it)
                               : str.fmt(_, "%s/%s", out_dir, os.path.basename(it, _));
            auto in_stat = os.fs.stat(in_path);
            if (!in_stat.is_valid) {
                return e$raise(in_stat.error, "Invalid stat for path: %s", in_path);
            }

            auto out_stat = os.fs.stat(out_path);
            if (!out_stat.is_valid || update_existing) {
                log$info("Updating file: %s -> %s\n", it, out_path);
                if (in_stat.is_directory) {
                    if (out_stat.is_valid && update_existing) {
                        e$assert(out_stat.is_directory && "out_path expected to be a directory");
                        str_s src_dir = str.sstr(in_path);
                        str_s dst_dir = str.sstr(out_path);
                        for$each (fname, os.fs.find(str.fmt(_, "%S/*", src_dir), true, _)) {
                            e$assert(str.starts_with(fname, in_path));
                            char* out_file = str.fmt(
                                _,
                                "%S/%S",
                                dst_dir,
                                str.sub(fname, src_dir.len, 0)
                            );
                            log$debug("Replacing: %s\n", out_file);
                            e$ret(os.fs.mkpath(out_file));
                            if (os.path.exists(out_file)) { e$ret(os.fs.remove(out_file)); }
                            e$ret(os.fs.copy(fname, out_file));
                        }

                    } else {
                        e$ret(os.fs.copy_tree(in_path, out_path));
                    }
                } else {
                    // Updating single file
                    if (out_stat.is_valid && update_existing) { e$ret(os.fs.remove(out_path)); }
                    e$ret(os.fs.mkpath(out_path));
                    e$ret(os.fs.copy(in_path, out_path));
                }
            }
        }

#    ifdef _WIN32
        // WTF, windows git makes some files read-only which fails remove_tree later!
        e$ret(os$cmd("attrib", "-r", str.fmt(_, "%s/*", out_build_dir), "/s", "/d"));
#    endif
    }
    // We are done, cleanup!
    e$ret(os.fs.remove_tree(out_build_dir));

    return EOK;
}

const struct __cex_namespace__cexy cexy = {
    // Autogenerated by CEX
    // clang-format off

    .build_self = cexy_build_self,
    .src_changed = cexy_src_changed,
    .src_include_changed = cexy_src_include_changed,
    .target_make = cexy_target_make,

    .app = {
        .clean = cexy__app__clean,
        .create = cexy__app__create,
        .find_app_target_src = cexy__app__find_app_target_src,
        .run = cexy__app__run,
    },

    .cmd = {
        .config = cexy__cmd__config,
        .help = cexy__cmd__help,
        .libfetch = cexy__cmd__libfetch,
        .new = cexy__cmd__new,
        .process = cexy__cmd__process,
        .simple_app = cexy__cmd__simple_app,
        .simple_fuzz = cexy__cmd__simple_fuzz,
        .simple_test = cexy__cmd__simple_test,
        .stats = cexy__cmd__stats,
    },

    .fuzz = {
        .create = cexy__fuzz__create,
    },

    .test = {
        .clean = cexy__test__clean,
        .create = cexy__test__create,
        .make_target_pattern = cexy__test__make_target_pattern,
        .run = cexy__test__run,
    },

    .utils = {
        .git_hash = cexy__utils__git_hash,
        .git_lib_fetch = cexy__utils__git_lib_fetch,
        .make_compile_flags = cexy__utils__make_compile_flags,
        .make_new_project = cexy__utils__make_new_project,
        .pkgconf = cexy__utils__pkgconf,
    },

    // clang-format on
};
#endif // defined(CEX_BUILD)

#endif
