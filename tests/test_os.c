#include "src/all.c"
#include <math.h>

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(test_timer)
{
    f64 t = os.timer();
    tassert(t > 0);
    tassert(!isnan(t));
    tassert(t < INFINITY);
    tassert(t > -INFINITY);
    // timer starts from first call of os.timer(), so numbers should be small
    tassert_le(t, 5);

    f64 t2 = os.timer();
    tassert(t2 >= t);
    tassert_le(t2, 5);

    os.sleep(0.1);
    t2 = os.timer();
    f64 tdiff = t2 - t;
    // NOTE: CI timings may be very slow, we estimate order of magnitude
    tassertf(tdiff > 0.1 && tdiff < 0.35, "%g", tdiff);

    t = t2;
    os.sleep(1.1);
    t2 = os.timer();
    tdiff = t2 - t;
    tassertf(tdiff > 1.1 && tdiff < 1.35, "%g", tdiff);

    return EOK;
}

test$case(test_cpu_count)
{

    tassert_ge(os.cpu_count(), 1);
    tassert_le(os.cpu_count(), 128);

    return EOK;
}

test$case(test_time_scope)
{
    os$time_scope()
    {
        os.sleep(0.1);
    }
    os$time_scope()
    {
        os.sleep(1.1);
    }
    os$time_scope() {}
    os$time_scope()
    {
        break; // early exit warning
    }

    return EOK;
}

test$case(test_hash)
{
    // --- NULL / empty ---
    tassert_eq(os.hash(NULL, 0, 10), 0);
    tassert_eq(os.hash("", 0, 10), 0);
    tassert_eq(os.hash("hello", 0, 0), 0);
    tassert_eq(os.hash("hello", 0, 42), 0);

    // --- basic smoke: delegates to _cexds__hash_bytes ---
    char cstr[] = { "hello" };
    u64 h0 = os.hash(cstr, sizeof(cstr), 0);
    tassert(h0 != 0);
    tassert_eq(_cexds__hash_bytes(cstr, sizeof(cstr), 0), h0);
    tassert_eq(h0, 6329348214770146015UL);

    return EOK;
}

test$case(test_getpid)
{
    i32 pid = os.env.getpid();
    tassert_gt(pid, 0);
    return EOK;
}

test$case(test_env_set_unset)
{
    tassert_eq(os.env.get("cex_test_env", NULL), NULL);
    tassert_er(EOK, os.env.set("cex_test_env", "val"));
    tassert_eq(os.env.get("cex_test_env", NULL), "val");
    tassert_er(EOK, os.env.unset("cex_test_env"));
    tassert_eq(os.env.get("cex_test_env", NULL), NULL);
    return EOK;
}

test$case(test_env_executable_path)
{
    mem$scope(tmem$, _)
    {
        char* exe = os.env.executable_path(_);
        tassert_ne(exe, NULL);
        tassert_gt(str.len(exe), 0);
#    ifdef _WIN32
        tassert(str.ends_with(exe, ".exe"));
#    endif
        tassert(str.find(exe, "test_os"));
    }
    return EOK;
}

test$case(test_env_home_dir)
{
    mem$scope(tmem$, _)
    {
        char* home = os.env.home_dir(_);
        tassert_ne(home, NULL);
        tassert_gt(str.len(home), 0);
        if (os$PATH_SEP == '/') {
            tassert_eq(home[0], '/');
        } else {
            tassert(str.find(home, ":\\"));
        }

        // Set HOME to a known value and verify home_dir returns it
#    ifdef _WIN32
        char* saved_up = os.env.get("USERPROFILE", NULL);
        char* saved_up_clone = saved_up ? str.clone(saved_up, _) : NULL;
        tassert_er(EOK, os.env.set("USERPROFILE", "C:\\fake_home"));
        tassert_eq(os.env.home_dir(_), "C:\\fake_home");
        if (saved_up_clone) { tassert_er(EOK, os.env.set("USERPROFILE", saved_up_clone)); }
#    else
        char* saved = os.env.get("HOME", NULL);
        char* saved_clone = saved ? str.clone(saved, _) : NULL;
        tassert_er(EOK, os.env.set("HOME", "/tmp/fake_home_dir"));
        tassert_eq(os.env.home_dir(_), "/tmp/fake_home_dir");
        if (saved_clone) { tassert_er(EOK, os.env.set("HOME", saved_clone)); }
#    endif
        // Confirm it's back to original
        tassert_ne(os.env.home_dir(_), NULL);
    }
    return EOK;
}

test$case(test_env_home_dir_unset)
{
    mem$scope(tmem$, _)
    {
#    ifdef _WIN32
        char* saved_up = os.env.get("USERPROFILE", NULL);
        char* saved_up_clone = saved_up ? str.clone(saved_up, _) : NULL;
        char* saved_hd = os.env.get("HOMEDRIVE", NULL);
        char* saved_hd_clone = saved_hd ? str.clone(saved_hd, _) : NULL;
        char* saved_hp = os.env.get("HOMEPATH", NULL);
        char* saved_hp_clone = saved_hp ? str.clone(saved_hp, _) : NULL;

        // When all three env vars are unset, home_dir returns NULL
        tassert_er(EOK, os.env.unset("USERPROFILE"));
        tassert_er(EOK, os.env.unset("HOMEDRIVE"));
        tassert_er(EOK, os.env.unset("HOMEPATH"));
        tassert_eq(os.env.home_dir(_), NULL);

        // Restore USERPROFILE and verify it works
        if (saved_up_clone) { tassert_er(EOK, os.env.set("USERPROFILE", saved_up_clone)); }
        tassert_ne(os.env.home_dir(_), NULL);

        // Unset USERPROFILE but keep HOMEDRIVE+HOMEPATH — should fallback
        tassert_er(EOK, os.env.unset("USERPROFILE"));
        if (saved_hd_clone && saved_hp_clone) {
            tassert_er(EOK, os.env.set("HOMEDRIVE", saved_hd_clone));
            tassert_er(EOK, os.env.set("HOMEPATH", saved_hp_clone));
            tassert_ne(os.env.home_dir(_), NULL);
        }

        // Full restore
        if (saved_up_clone) { tassert_er(EOK, os.env.set("USERPROFILE", saved_up_clone)); }
        if (saved_hd_clone) { tassert_er(EOK, os.env.set("HOMEDRIVE", saved_hd_clone)); }
        if (saved_hp_clone) { tassert_er(EOK, os.env.set("HOMEPATH", saved_hp_clone)); }
#    else
        char* saved = os.env.get("HOME", NULL);
        char* saved_clone = saved ? str.clone(saved, _) : NULL;

        tassert_er(EOK, os.env.unset("HOME"));
        tassert_eq(os.env.home_dir(_), NULL);

        if (saved_clone) { tassert_er(EOK, os.env.set("HOME", saved_clone)); }
        tassert_ne(os.env.home_dir(_), NULL);
#    endif
    }
    return EOK;
}

test$main();
