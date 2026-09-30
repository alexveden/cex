#pragma once
#include "coverage.h"

#if defined(CEX_BUILD) || defined(CEX_NEW)

#define _COVERAGE_INFO_DEFAULT "coverage.info"
#define _COVERAGE_HTML_DEFAULT cexy$build_dir "/coverage"

static Exception _coverage__capture_cmd(arr$(char*) args, char** out, IAllocator allc);

static bool
_coverage__is_clang(void)
{
    char* cc[] = { cexy$cc };
    return arr$len(cc) > 0 && str.find(cc[0], "clang") != NULL;
}

static Exception
_coverage__resolve_tool(arr$(char*)* out, char* tool, IAllocator allc)
{
    uassert(out != NULL);
    uassert(tool != NULL);
    (void)allc;

    if (os.cmd.exists(tool)) {
        arr$push(*out, tool);
        return EOK;
    }
#    ifdef _WIN32
    if (os.cmd.exists("perl")) {
        str_s path_env = str.sstr(os.env.get("PATH", NULL));
        if (path_env.buf != NULL) {
            for$iter (str_s, it, str.slice.iter_split(path_env, ";", &it.iterator)) {
                char* script = str.fmt(allc, "%S/%s", it.val, tool);
                if (os.path.exists(script)) {
                    arr$pushm(*out, "perl", script);
                    return EOK;
                }
            }
        }
    }
#    endif
    log$error("Coverage requires '%s' in PATH\n", tool);
    return e$raise(Error.runtime, "coverage tool not found");
}

static Exception
_coverage__find_test_binaries(char* target, arr$(char*)* out, IAllocator allc)
{
    uassert(out != NULL);

    char* pattern = target;
    e$ret(cexy.test.make_target_pattern(&pattern));

    *out = arr$new(*out, allc);
    if (str.ends_with(pattern, "test_*.c")) {
        // *.test* also matches lcov artifacts (.test-*.gcno/.gcda) and the macOS .dSYM DWARF
        // (a bundle whose inner file is named like the executable); keep only real executables
        arr$(char*) found = os.fs.find(str.fmt(allc, "%s/tests/*.test*", cexy$build_dir), true, allc);
        for$each (bin, found) {
            if (str.find(bin, ".dSYM") != NULL) { continue; }
            if (str.ends_with(bin, ".test") || str.ends_with(bin, ".test.exe")) {
                arr$push(*out, bin);
            }
        }
        return EOK;
    }
    if (unlikely(!os.path.exists(pattern))) {
        return e$raise(Error.not_found, "test file not found");
    }
    char* bin = cexy.target_make(pattern, cexy$build_dir, ".test", allc);
    if (unlikely(bin == NULL)) { return e$raise(Error.runtime, "failed to make test target"); }
    arr$push(*out, bin);
    return EOK;
}

static Exception
_coverage__llvm_add_objects(char* target, arr$(char*)* args, IAllocator allc)
{
    uassert(args != NULL);

    arr$(char*) bins = NULL;
    e$ret(_coverage__find_test_binaries(target, &bins, allc));
    if (unlikely(arr$len(bins) == 0)) {
        return e$raise(Error.not_found, "no instrumented test binaries, run `cex coverage run`");
    }
    for$each (bin, bins) {
        arr$push(*args, str.fmt(allc, "--object=%s", bin));
    }
    return EOK;
}

static Exception
_coverage__llvm_merge_profdata(char** out_profdata, IAllocator allc)
{
    uassert(out_profdata != NULL);

    *out_profdata = NULL;
    arr$(char*) profraw = os.fs.find(str.fmt(allc, "%s/*.profraw", cexy$build_dir), true, allc);
    if (unlikely(arr$len(profraw) == 0)) {
        return e$raise(Error.not_found, "no .profraw data, run `cex coverage run` first");
    }

    char* profdata = str.fmt(allc, "%s/coverage.profdata", cexy$build_dir);
    arr$(char*) args = arr$new(args, allc);
    e$ret(_coverage__resolve_tool(&args, "llvm-profdata", allc));
    arr$pushm(args, "merge", "-sparse");
    arr$pusha(args, profraw);
    arr$pushm(args, "-o", profdata, NULL);
    char* merge_out = NULL;
    e$ret(_coverage__capture_cmd(args, &merge_out, allc));
    if (merge_out != NULL) { io.fprintf(stderr, "%s", merge_out); }

    *out_profdata = profdata;
    return EOK;
}

static Exception
_coverage__capture_cmd(arr$(char*) args, char** out, IAllocator allc)
{
    uassert(out != NULL);

    *out = NULL;
    os_cmd_c cmd = { 0 };
    e$ret(os.cmd.create(&cmd, args, arr$len(args), NULL));
    *out = os.cmd.read_all(&cmd, allc);
    e$ret(os.cmd.wait(&cmd, 1, 0));
    return EOK;
}

static Exception
_coverage__run_redirected(arr$(char*) args, char* out_file, char* err_file, IAllocator allc)
{
    uassert(args != NULL);
    uassert(out_file != NULL);
    uassert(err_file != NULL);

    // os.cmd pipes stderr into a stream nobody drains (hides tool errors, can deadlock), so run
    // the tool through a shell that redirects stdout/stderr straight to files.
    arr$(char*) sh = arr$new(sh, allc);
    arr$pushm(sh, "sh", "-c", "out=$1; err=$2; shift 2; \"$@\" > \"$out\" 2> \"$err\"", "_");
    arr$pushm(sh, out_file, err_file);
    arr$pusha(sh, args);

    char* sh_out = NULL;
    Exc err = _coverage__capture_cmd(sh, &sh_out, allc);
    if (err != EOK) {
        char* err_txt = io.file.load(err_file, allc);
        if (err_txt != NULL && err_txt[0] != '\0') {
            log$error("Coverage tool failed:\n%s\n", err_txt);
        }
        return err;
    }
    if (os.path.exists(err_file)) { if (os.fs.remove(err_file)) {} }
    return EOK;
}

static bool
_coverage__lcov_can_ignore_errors(IAllocator allc)
{
    arr$(char*) args = arr$new(args, allc);
    if (_coverage__resolve_tool(&args, "lcov", allc)) { return false; }
    arr$pushm(args, "--version", NULL);

    char* out = NULL;
    if (_coverage__capture_cmd(args, &out, allc)) { return false; }
    return out != NULL && str.find(out, "version 1.") == NULL;
}

static Exception
_coverage__add_lcov_gcov_tool(arr$(char*)* args, IAllocator allc)
{
    uassert(args != NULL);

    if (!_coverage__is_clang()) { return EOK; }
    char* wrapper = NULL;
#    ifdef _WIN32
    wrapper = str.fmt(allc, "%s/llvm-gcov.bat", cexy$build_dir);
    e$ret(io.file.save(wrapper, "@echo off\nllvm-cov gcov %*\n"));
#    else
    wrapper = str.fmt(allc, "%s/llvm-gcov.sh", cexy$build_dir);
    char* script = "#!/bin/sh\nexec llvm-cov gcov \"$@\"\n";
    FILE* file = NULL;
    e$ret(io.fopen(&file, wrapper, "wb"));
    e$except (err, io.fwrite(file, script, str.len(script))) {
        io.fclose(&file);
        return err;
    }
    io.fclose(&file);
    arr$(char*) chmod_args = arr$new(chmod_args, allc);
    arr$pushm(chmod_args, "chmod", "+x", wrapper, NULL);
    char* chmod_out = NULL;
    if (_coverage__capture_cmd(chmod_args, &chmod_out, allc)) {}
#    endif
    char* gcov_tool = os.path.absolute(wrapper, allc);
    gcov_tool = str.replace(gcov_tool, "\\", "/", allc);
    log$debug("Coverage gcov tool: %s (exists=%d)\n", gcov_tool, (int)os.path.exists(gcov_tool));
    arr$pushm(*args, "--gcov-tool", gcov_tool);
    return EOK;
}

static Exception
_coverage__run_lcov_capture(char* output, IAllocator allc)
{
    arr$(char*) gcda = os.fs.find(str.fmt(allc, "%s/*.gcda", cexy$build_dir), true, allc);
    if (unlikely(arr$len(gcda) == 0)) {
        return e$raise(Error.not_found, "no .gcda data, run `cex coverage run` first");
    }

    arr$(char*) args = arr$new(args, allc);
    e$ret(_coverage__resolve_tool(&args, "lcov", allc));
    arr$pushm(args, "--capture", "--quiet", "--directory", cexy$build_dir);
    if (_coverage__lcov_can_ignore_errors(allc)) {
        arr$pushm(
            args,
            "--ignore-errors",
            "inconsistent,inconsistent,mismatch,mismatch,empty,unused,source,format,unsupported,unsupported"
        );
    }
    e$ret(_coverage__add_lcov_gcov_tool(&args, allc));
    arr$pushm(args, "--output-file", output, NULL);
    char* stdout_file = str.fmt(allc, "%s/coverage-lcov.stdout", cexy$build_dir);
    char* err_file = str.fmt(allc, "%s/coverage-lcov.stderr", cexy$build_dir);
    e$ret(_coverage__run_redirected(args, stdout_file, err_file, allc));
    if (os.fs.remove(stdout_file)) {}
    return EOK;
}

static char*
_coverage__relativize_path(char* path, char* cwd, IAllocator allc)
{
    uassert(path != NULL);
    uassert(cwd != NULL);

    char sep[2] = { os$PATH_SEP, '\0' };
    char* p = str.replace(os.path.normalize(path, allc), sep, "/", allc);
    char* c = str.replace(os.path.normalize(cwd, allc), sep, "/", allc);
    if (p == NULL || c == NULL) { return p; }

    if (str.len(p) >= 3 && p[1] == ':' && p[2] == '/') {
        p = str.fmt(allc, "/%c/%s", (char)tolower((unsigned char)p[0]), p + 3);
    }
    if (str.len(c) >= 3 && c[1] == ':' && c[2] == '/') {
        c = str.fmt(allc, "/%c/%s", (char)tolower((unsigned char)c[0]), c + 3);
    }

    str_s ps = str.sstr(p);
    str_s cs = str.sstr(c);
    if (!str.slice.starts_with(ps, cs)) { return p; }
    if (ps.len > cs.len && ps.buf[cs.len] != '/') { return p; }

    ps = str.slice.remove_prefix(ps, cs);
    if (ps.len > 0 && ps.buf[0] == '/') { ps = str.slice.sub(ps, 1, (isize)ps.len); }
    return str.slice.clone(ps, allc);
}

static Exception
_coverage__relativize_info(char* info_path, IAllocator allc)
{
    uassert(info_path != NULL);

    char* cwd = os.fs.getcwd(allc);
    if (cwd == NULL) { return Error.os; }
    char* content = io.file.load(info_path, allc);
    if (content == NULL) { return Error.io; }

    // Stream line by line instead of split_lines + join: the latter clones every line, which
    // blows up on large tracefiles.
    FILE* file = NULL;
    e$ret(io.fopen(&file, info_path, "wb"));

    Exc err = EOK;
    char* cur = content;
    while (err == EOK && *cur != '\0') {
        char* nl = str.find(cur, "\n");
        usize len = (nl != NULL) ? (usize)(nl - cur) : str.len(cur);
        if (len > 0 && cur[len - 1] == '\r') { len--; }

        if (len >= 3 && cur[0] == 'S' && cur[1] == 'F' && cur[2] == ':') {
            str_s sf = { .buf = cur + 3, .len = len - 3 };
            char* path = str.slice.clone(sf, allc);
            char* rel = (path != NULL) ? _coverage__relativize_path(path, cwd, allc) : NULL;
            if (rel == NULL) {
                err = Error.memory;
                break;
            }
            err = io.fwrite(file, "SF:", 3);
            if (err == EOK) { err = io.fwrite(file, rel, str.len(rel)); }
        } else {
            err = io.fwrite(file, cur, len);
        }
        if (err == EOK) { err = io.fwrite(file, "\n", 1); }

        if (nl == NULL) { break; }
        cur = nl + 1;
    }

    io.fclose(&file);
    return err;
}

static Exception
_coverage__capture_info(char* engine, char* info_path, char* target, IAllocator allc)
{
    uassert(engine != NULL);
    uassert(info_path != NULL);
    uassert(target != NULL);

    if (str.eq(engine, "llvm")) {
        char* profdata = NULL;
        e$ret(_coverage__llvm_merge_profdata(&profdata, allc));

        arr$(char*) args = arr$new(args, allc);
        e$ret(_coverage__resolve_tool(&args, "llvm-cov", allc));
        arr$pushm(args, "export", "--format=lcov");
        e$ret(_coverage__llvm_add_objects(target, &args, allc));
        arr$push(args, str.fmt(allc, "-instr-profile=%s", profdata));
        arr$push(args, NULL);

        if (os.cmd.exists("sh")) {
            char* err_file = str.fmt(allc, "%s/coverage-llvm.stderr", cexy$build_dir);
            if (unlikely(err_file == NULL)) { return Error.memory; }
            e$ret(_coverage__run_redirected(args, info_path, err_file, allc));
        } else {
            char* content = NULL;
            e$ret(_coverage__capture_cmd(args, &content, allc));
            if (unlikely(content == NULL || content[0] == '\0')) {
                return e$raise(Error.runtime, "failed to export llvm coverage");
            }
            e$ret(io.file.save(info_path, content));
        }
    } else {
        e$ret(_coverage__run_lcov_capture(info_path, allc));
    }
    return _coverage__relativize_info(info_path, allc);
}

typedef struct
{
    char* path;
    i64 lf;
    i64 lh;
    i64 fnf;
    i64 fnh;
    i64 da_total;
    i64 da_hit;
    i64 fna_total;
    i64 fna_hit;
    arr$(u32) missed;
    arr$(char*) uncovered_funcs;
} _coverage__file_s;

static Exception
_coverage__parse_info(char* content, arr$(_coverage__file_s)* out, IAllocator allc)
{
    uassert(content != NULL);
    uassert(out != NULL);

    *out = arr$new(*out, allc);
    _coverage__file_s f = { 0 };
    for$each (line, str.split_lines(content, allc)) {
        if (str.starts_with(line, "SF:")) {
            if (f.path != NULL) { arr$push(*out, f); }
            f = (_coverage__file_s){ .path = line + 3, .lf = -1, .lh = -1, .fnf = -1, .fnh = -1 };
            f.missed = arr$new(f.missed, allc);
            f.uncovered_funcs = arr$new(f.uncovered_funcs, allc);
        } else if (f.path == NULL) {
            continue;
        } else if (str.starts_with(line, "DA:")) {
            char* end = NULL;
            long ln = strtol(line + 3, &end, 10);
            long count = (end != NULL && *end == ',') ? strtol(end + 1, NULL, 10) : 0;
            f.da_total++;
            if (count > 0) {
                f.da_hit++;
            } else {
                arr$push(f.missed, (u32)ln);
            }
        } else if (str.starts_with(line, "LF:")) {
            f.lf = atoi(line + 3);
        } else if (str.starts_with(line, "LH:")) {
            f.lh = atoi(line + 3);
        } else if (str.starts_with(line, "FNF:")) {
            f.fnf = atoi(line + 4);
        } else if (str.starts_with(line, "FNH:")) {
            f.fnh = atoi(line + 4);
        } else if (str.starts_with(line, "FNA:")) {
            char* end = NULL;
            strtol(line + 4, &end, 10);
            long count = (end != NULL && *end == ',') ? strtol(end + 1, &end, 10) : 0;
            char* name = (end != NULL && *end == ',') ? end + 1 : NULL;
            f.fna_total++;
            if (count > 0) {
                f.fna_hit++;
            } else if (name != NULL) {
                arr$push(f.uncovered_funcs, name);
            }
        } else if (str.starts_with(line, "FNDA:")) {
            char* end = NULL;
            long count = strtol(line + 5, &end, 10);
            char* name = (end != NULL && *end == ',') ? end + 1 : NULL;
            f.fna_total++;
            if (count > 0) {
                f.fna_hit++;
            } else if (name != NULL) {
                arr$push(f.uncovered_funcs, name);
            }
        }
    }
    if (f.path != NULL) { arr$push(*out, f); }
    return EOK;
}

static Exception
_coverage__emit_file(sbuf_c* out, _coverage__file_s* f, u64* t_lf, u64* t_lh, u64* t_ff, u64* t_fh)
{
    i64 lf = f->lf >= 0 ? f->lf : f->da_total;
    i64 lh = f->lh >= 0 ? f->lh : f->da_hit;
    i64 ff = f->fnf >= 0 ? f->fnf : f->fna_total;
    i64 fh = f->fnh >= 0 ? f->fnh : f->fna_hit;
    if (lf <= 0) { return EOK; }

    *t_lf += (u64)lf;
    *t_lh += (u64)lh;
    *t_ff += (u64)ff;
    *t_fh += (u64)fh;

    f64 line_pct = 100.0 * (f64)lh / (f64)lf;
    f64 func_pct = ff > 0 ? 100.0 * (f64)fh / (f64)ff : 0.0;
    e$ret(sbuf.appendf(
        out,
        "%s  %.1f%% lines (%ld/%ld)  %.1f%% funcs (%ld/%ld)",
        f->path,
        line_pct,
        lh,
        lf,
        func_pct,
        fh,
        ff
    ));
    e$ret(sbuf.append(out, "\n"));
    return EOK;
}

static char*
_coverage__format_text(char* content, char* file_filter, IAllocator allc)
{
    uassert(content != NULL);

    arr$(_coverage__file_s) files = NULL;
    e$goto(_coverage__parse_info(content, &files, allc), fail);

    sbuf_c out = sbuf.create(1024, allc);
    u64 t_lf = 0, t_lh = 0, t_ff = 0, t_fh = 0;
    for$eachp (f, files) {
        if (file_filter != NULL && !str.match(f->path, file_filter)) { continue; }
        e$goto(_coverage__emit_file(&out, f, &t_lf, &t_lh, &t_ff, &t_fh), fail);
    }

    f64 line_pct = t_lf > 0 ? 100.0 * (f64)t_lh / (f64)t_lf : 0.0;
    f64 func_pct = t_ff > 0 ? 100.0 * (f64)t_fh / (f64)t_ff : 0.0;
    e$goto(
        sbuf.appendf(
            &out,
            "Total: %.1f%% lines (%lu/%lu)  %.1f%% funcs (%lu/%lu)\n",
            line_pct,
            t_lh,
            t_lf,
            func_pct,
            t_fh,
            t_ff
        ),
        fail
    );

    return out;
fail:
    sbuf.destroy(&out);
    return NULL;
}

static char*
_coverage__format_json(char* content, char* file_filter, IAllocator allc)
{
    uassert(content != NULL);

    arr$(_coverage__file_s) files = NULL;
    e$goto(_coverage__parse_info(content, &files, allc), fail);

    sbuf_c out = sbuf.create(1024, allc);
    sbuf_c body = sbuf.create(1024, allc);
    u64 t_lf = 0, t_lh = 0, t_ff = 0, t_fh = 0;
    bool first = true;
    for$eachp (f, files) {
        if (file_filter != NULL && !str.match(f->path, file_filter)) { continue; }

        i64 lf = f->lf >= 0 ? f->lf : f->da_total;
        i64 lh = f->lh >= 0 ? f->lh : f->da_hit;
        i64 ff = f->fnf >= 0 ? f->fnf : f->fna_total;
        i64 fh = f->fnh >= 0 ? f->fnh : f->fna_hit;
        if (lf <= 0) { continue; }

        t_lf += (u64)lf;
        t_lh += (u64)lh;
        t_ff += (u64)ff;
        t_fh += (u64)fh;

        if (!first) { e$goto(sbuf.append(&body, ","), fail); }
        first = false;
        e$goto(
            sbuf.appendf(
                &body,
                "{\"path\":\"%s\",\"lines_hit\":%ld,\"lines_found\":%ld,"
                "\"funcs_hit\":%ld,\"funcs_found\":%ld",
                f->path,
                lh,
                lf,
                fh,
                ff
            ),
            fail
        );
        if (lh == 0) {
            e$goto(sbuf.append(&body, ",\"fully_uncovered\":true"), fail);
        } else {
            e$goto(sbuf.append(&body, ",\"missed_lines\":["), fail);
            for (usize i = 0; i < arr$len(f->missed); i++) {
                e$goto(sbuf.appendf(&body, "%s%u", i > 0 ? "," : "", f->missed[i]), fail);
            }
            e$goto(sbuf.append(&body, "]"), fail);
        }
        e$goto(sbuf.append(&body, ",\"uncovered_funcs\":["), fail);
        for (usize i = 0; i < arr$len(f->uncovered_funcs); i++) {
            e$goto(
                sbuf.appendf(&body, "%s\"%s\"", i > 0 ? "," : "", f->uncovered_funcs[i]),
                fail
            );
        }
        e$goto(sbuf.append(&body, "]}"), fail);
    }

    e$goto(
        sbuf.appendf(
            &out,
            "{\"total\":{\"lines_hit\":%lu,\"lines_found\":%lu,\"funcs_hit\":%lu,"
            "\"funcs_found\":%lu},\"files\":[",
            t_lh,
            t_lf,
            t_fh,
            t_ff
        ),
        fail
    );
    e$goto(sbuf.append(&out, body), fail);
    e$goto(sbuf.append(&out, "]}\n"), fail);

    sbuf.destroy(&body);
    return out;
fail:
    sbuf.destroy(&body);
    sbuf.destroy(&out);
    return NULL;
}

/// Builds and runs tests with coverage instrumentation
Exception
coverage_run(char* engine, char* target)
{
    uassert(engine != NULL);
    uassert(target != NULL);

    char* argv[] = { "test", "--coverage", "--coverage-engine", engine, "run", target };
    return cexy.cmd.simple_test(arr$len(argv), argv, NULL);
}

/// Aggregates coverage and prints a per-source report (text, json or html)
Exception
coverage_report(char* engine, char* format, char* output, char* file_filter, char* target)
{
    uassert(engine != NULL);
    uassert(target != NULL);

    if (format == NULL) { format = "text"; }
    if (unlikely(!str.match(format, "(text|json|html)"))) {
        return e$raise(Error.argument, "invalid report format, expected text|json|html");
    }
    bool html = str.eq(format, "html");
    bool json = str.eq(format, "json");
    if (unlikely(html && file_filter != NULL)) {
        return e$raise(Error.argument, "file filter is not supported for html report");
    }
    if (output == NULL) { output = _COVERAGE_HTML_DEFAULT; }

    mem$scope(tmem$, _)
    {
        if (!html) {
            char* info = str.fmt(_, "%s/coverage.info", cexy$build_dir);
            e$ret(_coverage__capture_info(engine, info, target, _));

            char* content = io.file.load(info, _);
            if (content == NULL) { return Error.io; }
            char* report = json ? _coverage__format_json(content, file_filter, _)
                                : _coverage__format_text(content, file_filter, _);
            if (report == NULL) { return Error.memory; }
            io.printf("%s", report);
            return EOK;
        }

        if (str.eq(engine, "llvm")) {
            char* profdata = NULL;
            e$ret(_coverage__llvm_merge_profdata(&profdata, _));

            arr$(char*) args = arr$new(args, _);
            e$ret(_coverage__resolve_tool(&args, "llvm-cov", _));
            arr$pushm(args, "show");
            e$ret(_coverage__llvm_add_objects(target, &args, _));
            arr$push(args, str.fmt(_, "-instr-profile=%s", profdata));
            arr$pushm(args, "--format=html", str.fmt(_, "--output-dir=%s", output), NULL);
            e$ret(os$cmda(args, arr$len(args)));
        } else {
            char* info = str.fmt(_, "%s/coverage.info", cexy$build_dir);
            e$ret(_coverage__capture_info(engine, info, target, _));

            arr$(char*) args = arr$new(args, _);
            e$ret(_coverage__resolve_tool(&args, "genhtml", _));
            arr$pushm(args, "--quiet", info, "-o", output, NULL);
            e$ret(os$cmda(args, arr$len(args)));
        }
        log$info("Coverage report: %s\n", output);
    }
    return EOK;
}

/// Exports aggregated coverage as an lcov .info tracefile
Exception
coverage_export(char* engine, char* output, char* target)
{
    uassert(engine != NULL);
    uassert(target != NULL);

    if (output == NULL) { output = _COVERAGE_INFO_DEFAULT; }
    mem$scope(tmem$, _)
    {
        e$ret(_coverage__capture_info(engine, output, target, _));
        log$info("Coverage exported: %s\n", output);
    }
    return EOK;
}

/// Removes coverage artifacts (.profraw/.profdata/.gcno/.gcda)
Exception
coverage_clean(char* target)
{
    uassert(target != NULL);

    mem$scope(tmem$, _)
    {
        char* pattern = target;
        e$ret(cexy.test.make_target_pattern(&pattern));

        arr$(char*) globs = arr$new(globs, _);
        if (str.ends_with(pattern, "test_*.c")) {
            arr$pushm(globs, "%s/*.gcno", "%s/*.gcda");
        } else {
            if (unlikely(!os.path.exists(pattern))) {
                return e$raise(Error.not_found, "test file not found");
            }
            char* test_target = cexy.target_make(pattern, cexy$build_dir, ".test", _);
            if (unlikely(test_target == NULL)) {
                return e$raise(Error.runtime, "failed to make test target");
            }
            arr$pushm(
                globs,
                str.fmt(_, "%s-*.gcno", test_target),
                str.fmt(_, "%s-*.gcda", test_target)
            );
        }
        arr$pushm(globs, "%s/*.profraw", "%s/*.profdata");
        for$each (glob, globs) {
            for$each (file, os.fs.find(str.fmt(_, glob, cexy$build_dir), true, _)) {
                if (os.fs.remove(file)) {}
            }
        }
        log$info("Coverage artifacts cleaned: %s\n", target);
    }
    return EOK;
}

/// CLI: build/run tests with coverage, then report or export it
Exception
coverage_cmd(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    char* engine = NULL;
    char* format = NULL;
    char* output = NULL;
    char* file = NULL;

    // clang-format off
    char* process_help =
        "Manage project test coverage with two engines:\n"
        "- `llvm`: clang only, source-based coverage via llvm-profdata + llvm-cov\n"
        "- `lcov`: compiler --coverage + lcov/genhtml (gcc or clang)\n"
        "\n`run` builds and runs tests with instrumentation, leaving raw data in\n"
        "cexy$build_dir (.profraw for llvm, .gcno/.gcda for lcov). `report` aggregates\n"
        "and prints a per-file report (text, json) with uncovered lines and functions.\n"
        "`export` writes an lcov .info tracefile for external tools (merge several with\n"
        "`lcov -a a.info -o merged.info`). `clean`\n"
        "removes the raw coverage artifacts only, leaving test binaries intact.\n";
    char* epilog_help =
        "\nCommand examples: \n"
        "cex coverage run all                          - build+run all tests with coverage\n"
        "cex coverage report all                       - aggregate and print text report\n"
        "cex coverage report --format=json all         - machine-readable json report\n"
        "cex coverage report --file 'src/str.c' all    - only sources matching the glob\n"
        "cex coverage report --format=html all         - write html report\n"
        "cex coverage export -o coverage.info all      - write lcov .info tracefile\n"
        "cex coverage report --engine=lcov all         - force the lcov engine\n"
        "cex coverage clean all                        - remove coverage artifacts\n"
        "json fields: total{lines_hit,lines_found,funcs_hit,funcs_found} "
        "files[]{path,lines_hit,lines_found,funcs_hit,funcs_found,missed_lines,uncovered_funcs,fully_uncovered}\n";
    // clang-format on

    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "coverage {run,report,export,clean} [options] all|tests/test_file.c",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(
            argparse$opt_help(),
            argparse$opt(&engine, '\0', "engine", .help = "Coverage engine: auto|llvm|lcov"),
            argparse$opt(&format, '\0', "format", .help = "Report format: text|json|html"),
            argparse$opt(&file, '\0', "file", .help = "Only report sources matching this glob"),
            argparse$opt(&output, 'o', "output", .help = "Output file (.info) or dir (html)"),
        ),
    };

    if (unlikely(argc < 2)) {
        io.printf("Usage: ./cex coverage {run,report,export,clean} all|tests/test_file.c\n");
        return e$raise(Error.argsparse, "coverage requires run|report|export|clean");
    }

    mem$scope(tmem$, _)
    {
        // argparse expects options before positionals; hoist them so both orders work
        arr$(char*) opts = arr$new(opts, _);
        arr$(char*) pos = arr$new(pos, _);
        for (i32 i = 1; i < argc; i++) {
            char* a = argv[i];
            if (a[0] != '-') {
                arr$push(pos, a);
                continue;
            }
            arr$push(opts, a);
            bool takes_value =
                !str.eq(a, "-h") && !str.eq(a, "--help") && str.find(a, "=") == NULL;
            if (takes_value && i + 1 < argc) { arr$push(opts, argv[++i]); }
        }
        arr$(char*) ordered = arr$new(ordered, _);
        arr$push(ordered, argv[0]);
        arr$pusha(ordered, opts);
        arr$pusha(ordered, pos);

        e$ret(argparse.parse(&cmd_args, arr$len(ordered), ordered));

        char* subcmd = argparse.next(&cmd_args);
        char* target = argparse.next(&cmd_args);

        if (unlikely(subcmd == NULL || !str.match(subcmd, "(run|report|export|clean)"))) {
            argparse.usage(&cmd_args);
            return e$raise(
                Error.argsparse, "invalid coverage command, expected run|report|export|clean"
            );
        }
        if (target == NULL) { target = "all"; }

        bool is_clang = _coverage__is_clang();
        if (engine == NULL || str.eq(engine, "auto")) { engine = is_clang ? "llvm" : "lcov"; }
        if (unlikely(!str.match(engine, "(llvm|lcov)"))) {
            argparse.usage(&cmd_args);
            return e$raise(Error.argsparse, "invalid engine, expected auto|llvm|lcov");
        }
        if (unlikely(str.eq(engine, "llvm") && !is_clang)) {
            return e$raise(Error.argument, "coverage engine 'llvm' requires clang compiler");
        }

        if (str.eq(subcmd, "run")) { return coverage_run(engine, target); }
        if (str.eq(subcmd, "report")) {
            return coverage_report(engine, format, output, file, target);
        }
        if (str.eq(subcmd, "export")) { return coverage_export(engine, output, target); }
        return coverage_clean(target);
    }
    return EOK;
}

CEX_NAMESPACE_DEF struct __cex_namespace__coverage coverage = {
    // Autogenerated by CEX
    // clang-format off

    .clean = coverage_clean,
    .cmd = coverage_cmd,
    .export = coverage_export,
    .report = coverage_report,
    .run = coverage_run,

    // clang-format on
};
#undef _COVERAGE_INFO_DEFAULT
#undef _COVERAGE_HTML_DEFAULT

#endif // #if defined(CEX_BUILD) || defined(CEX_NEW)
