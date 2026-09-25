#pragma once
/* Shared helpers and cases for test_cex_errors_lvl{0,1,2,3}.c and test_cex_errors_cap4.c.
 *
 * Include AFTER src/cex_errors.h (uses the e$* macros and the level knobs).
 */

#if CEX_TRACEBACK_LVL >= 1 && CEX_TRACEBACK_LVL <= 2
#    define TB_RECORDS 1
/// Expect _n recorded frames when buffering, otherwise the ring must stay empty
#    define tassert_frames(_n) tassert_eq(e$traceback_len, (u32)(_n))
#else
#    define TB_RECORDS 0
#    define tassert_frames(_n) tassert_eq(e$traceback_len, 0)
#endif

#ifndef _WIN32
/// Fork a child that runs the panic; true when it terminated abnormally (signal or nonzero exit)
test$noopt bool
is_panic_fatal_in_child(bool use_unreachable)
{
    pid_t pid = fork();
    if (pid < 0) { return false; }
    if (pid == 0) {
        (void)freopen("/dev/null", "w", stdout);
        (void)freopen("/dev/null", "w", stderr);
        if (use_unreachable) { unreachable(); }
        else { uassert(false); }
        _exit(0);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) != pid) { return false; }
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
#endif

/// Error.io when `i` is non-zero, else EOK; records nothing
test$noopt Exception
raw_err(int i)
{
    return i ? Error.io : EOK;
}

/// e$raise origin
test$noopt Exception
err_raise(int i)
{
    if (i) { return e$raise(Error.io, "raise io"); }
    return EOK;
}

/// e$assert failure returns Error.assert
test$noopt Exception
assert_fail(int i)
{
    e$assert(i);
    return EOK;
}

/// e$ret propagation
test$noopt Exception
err_ret(int i)
{
    e$ret(err_raise(i));
    return EOK;
}

/// e$goto propagation; the sentinel proves the label was reached
test$noopt Exception
err_goto(int i)
{
    e$goto(raw_err(i), out);
    return EOK;
out:
    return Error.runtime;
}

/// recursive e$raise + e$ret chain, yields depth + 1 frames
test$noopt Exception
chain(int depth)
{
    if (depth <= 0) { return e$raise(Error.io, "chain bottom"); }
    e$ret(chain(depth - 1));
    return EOK;
}

/// explicit 3-frame chain with a distinct func/msg per frame
test$noopt Exception
ladder_raise(void)
{
    return e$raise(Error.io, "ladder bottom");
}

test$noopt Exception
ladder_ret1(void)
{
    e$ret(ladder_raise());
    return EOK;
}

test$noopt Exception
ladder_ret2(void)
{
    e$ret(ladder_ret1());
    return EOK;
}

/// e$except, returns how many times the handler ran (0 or 1)
test$noopt int
err_except(int i)
{
    int n = 0;
    e$except(err, raw_err(i)) { (void)err; n++; }
    return n;
}

/// syscall-like helper: -1 + ENOENT when `i`, else 0
test$noopt int
sys_rc(int i)
{
    if (i) { errno = ENOENT; return -1; }
    return 0;
}

test$noopt int
err_except_errno(int i)
{
    int n = 0;
    e$except_errno(sys_rc(i)) { n++; }
    return n;
}

/// syscall-like helper: writes errno = EAGAIN and returns -1 when `i`
test$noopt int
sys_rc_keep_errno(int i)
{
    if (i) { errno = EAGAIN; return -1; }
    return 0;
}

test$noopt int
err_except_errno_keep(int i)
{
    int n = 0;
    e$except_errno(sys_rc_keep_errno(i)) { n++; }
    return n;
}

test$noopt void*
maybe_null(int i)
{
    return i ? NULL : (void*)1;
}

test$noopt int
err_except_null(int i)
{
    int n = 0;
    e$except_null(maybe_null(i)) { n++; }
    return n;
}

/// e$except_null with an int 0 (a null pointer constant)
test$noopt int
err_except_null_zero(void)
{
    int n = 0;
    e$except_null(0) { n++; }
    return n;
}

/// e$except_null with bool false (expands to 0)
test$noopt int
err_except_null_false(void)
{
    int n = 0;
    e$except_null(false) { n++; }
    return n;
}

test$noopt int
maybe_true(int i)
{
    return i;
}

test$noopt int
err_except_true(int i)
{
    int n = 0;
    e$except_true(maybe_true(i)) { n++; }
    return n;
}

test$setup_case()
{
    e$traceback_reset();
    return EOK;
}

test$case(raise_error)
{
    Exc e = err_raise(1);
    tassert_eq(e, Error.io);
    tassert_frames(1);
#if TB_RECORDS
    tassert_eq(e$traceback_arr[0].err, Error.io);
    tassert(e$traceback_arr[0].file != NULL);
#endif
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "raise io");
    tassert(e$traceback_arr[0].func != NULL);
#endif
    return EOK;
}

test$case(raise_ok)
{
    Exc e = err_raise(0);
    tassert_eq(e, EOK);
    tassert_frames(0);
    return EOK;
}

test$case(assert_error)
{
    Exc e = assert_fail(0);
    tassert_eq(e, Error.assert);
    tassert_frames(1);
#if TB_RECORDS
    tassert_eq(e$traceback_arr[0].err, Error.assert);
#endif
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "i");
#endif
    return EOK;
}

test$case(assert_ok)
{
    Exc e = assert_fail(1);
    tassert_eq(e, EOK);
    tassert_frames(0);
    return EOK;
}

#if CEX_TRACEBACK_LVL >= 1
test$case(uassert_disabled_returns)
{
    uassert_disable();
    uassert(false);
    uassert_enable();
    return EOK;
}
#endif

#ifndef _WIN32
test$case(uassert_fatal)
{
    tassert(is_panic_fatal_in_child(false));
    return EOK;
}

test$case(unreachable_fatal)
{
    tassert(is_panic_fatal_in_child(true));
    return EOK;
}

#if CEX_TRACEBACK_LVL >= 1
test$case(unreachable_fatal_when_disabled)
{
    uassert_disable();
    bool fatal = is_panic_fatal_in_child(true);
    uassert_enable();
    tassert(fatal);
    return EOK;
}
#endif
#endif

test$case(ret_error)
{
    Exc e = err_ret(1);
    tassert_eq(e, Error.io);
    tassert_frames(2);
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "raise io");
    tassert(str.find((char*)e$traceback_arr[1].msg, "err_raise") != NULL);
#endif
    return EOK;
}

test$case(ret_ok)
{
    Exc e = err_ret(0);
    tassert_eq(e, EOK);
    tassert_frames(0);
    return EOK;
}

test$case(goto_error)
{
    Exc e = err_goto(1);
    tassert_eq(e, Error.runtime);
    tassert_frames(1);
#if CEX_TRACEBACK_LVL == 2
    tassert(str.find((char*)e$traceback_arr[0].msg, "raw_err") != NULL);
#endif
    return EOK;
}

test$case(goto_ok)
{
    Exc e = err_goto(0);
    tassert_eq(e, EOK);
    tassert_frames(0);
    return EOK;
}

test$case(except_error)
{
    int n = err_except(1);
    tassert_eq(n, 1);
    tassert_frames(1);
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "raw_err(i)");
#endif
    return EOK;
}

test$case(except_ok)
{
    int n = err_except(0);
    tassert_eq(n, 0);
    tassert_frames(0);
    return EOK;
}

test$case(except_errno_error)
{
    errno = 0;
    int n = err_except_errno(1);
    int saved_errno = errno;
    tassert_eq(n, 1);
    tassert_eq(saved_errno, ENOENT);
    tassert_frames(1);
#if TB_RECORDS
    tassert_eq(e$traceback_arr[0].err, strerror(ENOENT));
#endif
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "sys_rc(i)");
#endif
    return EOK;
}

test$case(except_errno_ok)
{
    int n = err_except_errno(0);
    tassert_eq(n, 0);
    tassert_frames(0);
    return EOK;
}

test$case(except_errno_ok_preserves_errno)
{
    errno = EADDRINUSE;
    int n = err_except_errno(0);
    int saved_errno = errno;
    tassert_eq(n, 0);
    tassert_eq(saved_errno, EADDRINUSE);
    tassert_frames(0);
    return EOK;
}

test$case(except_errno_keeps_raised_errno)
{
    errno = EADDRINUSE;
    int n = err_except_errno_keep(1);
    tassert_eq(n, 1);
    tassert_eq(errno, EAGAIN);
    tassert_frames(1);
#if TB_RECORDS
    tassert_eq(e$traceback_arr[0].err, strerror(EAGAIN));
#endif
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "sys_rc_keep_errno(i)");
#endif
    return EOK;
}

test$case(except_null_error)
{
    int n = err_except_null(1);
    tassert_eq(n, 1);
    tassert_frames(1);
#if TB_RECORDS
    tassert_eq(e$traceback_arr[0].err, Error.null_or_empty);
#endif
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "maybe_null(i)");
#endif
    return EOK;
}

test$case(except_null_ok)
{
    int n = err_except_null(0);
    tassert_eq(n, 0);
    tassert_frames(0);
    return EOK;
}

test$case(except_null_zero_and_false)
{
    int nz = err_except_null_zero();
    tassert_eq(nz, 1);
    tassert_frames(1);
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "0");
#endif

    int nf = err_except_null_false();
    tassert_eq(nf, 1);
    tassert_frames(1);
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "false");
#endif
    return EOK;
}

test$case(except_true_error)
{
    int n = err_except_true(1);
    tassert_eq(n, 1);
    tassert_frames(1);
#if TB_RECORDS
    tassert_eq(e$traceback_arr[0].err, Error.runtime);
#endif
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "maybe_true(i)");
#endif
    return EOK;
}

test$case(except_true_ok)
{
    int n = err_except_true(0);
    tassert_eq(n, 0);
    tassert_frames(0);
    return EOK;
}

#if TB_RECORDS

test$case(frames_order)
{
    Exc e = ladder_ret2();
    tassert_eq(e, Error.io);
    tassert_eq(e$traceback_len, 3);
    tassert_eq(e$traceback_arr[0].err, Error.io);
    tassert_eq(e$traceback_arr[1].err, Error.io);
    tassert_eq(e$traceback_arr[2].err, Error.io);
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "ladder bottom");
    tassert_eq((char*)e$traceback_arr[1].msg, "ladder_raise()");
    tassert_eq((char*)e$traceback_arr[2].msg, "ladder_ret1()");
    tassert(str.find((char*)e$traceback_arr[0].func, "ladder_raise") != NULL);
    tassert(str.find((char*)e$traceback_arr[1].func, "ladder_ret1") != NULL);
    tassert(str.find((char*)e$traceback_arr[2].func, "ladder_ret2") != NULL);
#endif
    return EOK;
}

#if CEX_TRACEBACK_CAP >= 16
test$case(frames_many)
{
    Exc e = chain(8);
    tassert_eq(e, Error.io);
    tassert_eq(e$traceback_len, 9);
    tassert_eq(e$traceback_arr[0].err, Error.io);
    tassert_eq(e$traceback_arr[8].err, Error.io);
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "chain bottom");
    tassert_eq((char*)e$traceback_arr[8].msg, "chain(depth - 1)");
#endif
    return EOK;
}
#endif

test$case(frames_overflow)
{
    Exc e = chain(CEX_TRACEBACK_CAP + 8);
    tassert_eq(e, Error.io);
    tassert_eq(e$traceback_len, CEX_TRACEBACK_CAP);
    tassert_eq(e$traceback_arr[0].err, Error.io);
    tassert_eq(e$traceback_arr[CEX_TRACEBACK_CAP - 1].err, Error.io);
#if CEX_TRACEBACK_LVL == 2
    tassert_eq((char*)e$traceback_arr[0].msg, "chain bottom");
    tassert_eq((char*)e$traceback_arr[CEX_TRACEBACK_CAP - 1].msg, "chain(depth - 1)");
#endif
    return EOK;
}

test$case(traceback_fmt_nonempty)
{
    Exc e = ladder_ret2();
    tassert_eq(e, Error.io);
    tassert_eq(e$traceback_len, 3);

    char* s = e$traceback_fmt(mem$);
    tassert(str.find(s, "#0") != NULL);
    tassert(str.find(s, "#1") != NULL);
    tassert(str.find(s, "#2") != NULL);
    tassert(str.find(s, __FILE_NAME__) != NULL);
#if CEX_TRACEBACK_LVL == 2
    tassert(str.find(s, "ladder bottom") != NULL);
    tassert(str.find(s, "ladder_raise()") != NULL);
    tassert(str.find(s, "ladder_ret1()") != NULL);
    tassert(str.find(s, "ladder_ret2()") != NULL);
#endif
    sbuf.destroy(&s);
    return EOK;
}

test$case(traceback_print_nonempty)
{
    Exc e = ladder_ret2();
    tassert_eq(e, Error.io);
    tassert_eq(e$traceback_len, 3);

    FILE* f = tmpfile();
    tassert(f != NULL);

    e$traceback_print(f);

    Exc fe = io.fflush(f);
    tassert_eq(fe, EOK);
    io.rewind(f);

    char buf[512] = {0};
    isize rd = io.fread(f, buf, sizeof(buf) - 1);
    tassert(rd > 0);
    tassert(str.find(buf, "#0") != NULL);
    tassert(str.find(buf, "#2") != NULL);

    char* s = e$traceback_fmt(mem$);
    tassert_eq(buf, s);
    sbuf.destroy(&s);

    io.fclose(&f);
    return EOK;
}

#endif // TB_RECORDS

test$case(traceback_reset_drops)
{
    Exc e = err_raise(1);
    tassert_eq(e, Error.io);
    tassert_frames(1);

    e$traceback_reset();
    tassert_frames(0);
    return EOK;
}

test$case(traceback_fmt_empty)
{
    char* s = e$traceback_fmt(mem$);
    tassert_eq(s, "");
    sbuf.destroy(&s);
    return EOK;
}

test$case(traceback_print_empty)
{
    FILE* f = tmpfile();
    tassert(f != NULL);

    e$traceback_print(f);

    usize pos = 0;
    tassert_eq(io.ftell(f, &pos), EOK);
    tassert(pos == 0);

    io.fclose(&f);
    return EOK;
}
