#include "src/all.c"
#include "src/all.h"

#define TBUILDDIR "tests/build/os_cmd_test/"
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

#if !defined(__EMSCRIPTEN__)
char*
test_app(char* app_name, IAllocator allc)
{
    uassert(app_name != NULL);
    uassert(allc != NULL);
    char* result = os$path_join(
        allc,
        "build",
        "tests",
        "os_test",
        str.fmt(
            allc,
            "%s.c.%s%s",
            app_name,
            os.platform.to_str(os.platform.current()),
            (os.platform.current() == OSPlatform__win) ? ".exe" : ""
        )
    );
    // log$debug("Making test app: %s\n", result);
    return result;
}

test$case(os_cmd_exists)
{
    log$debug("Current platform: %s\n", os.platform.current_str());
    log$debug("PATH env: %s\n", os.env.get("PATH", NULL));

    tassert_eq(false, os.cmd.exists(""));
    tassert_eq(false, os.cmd.exists(NULL));
    tassert_eq(false, os.cmd.exists("alskdislkdfjslkfjk"));
    tassert_eq(true, os.cmd.exists("./cex"));

    tassert_eq(true, os.path.exists(TBUILDDIR));
    tassert_eq(false, os.cmd.exists(TBUILDDIR));

#ifdef _WIN32
    e$ret(io.file.save(TBUILDDIR "mycmd1.exe", "test"));
    e$ret(io.file.save(TBUILDDIR "mycmd2.bat", "test"));
    e$ret(io.file.save(TBUILDDIR "mycmd3.cmd", "test"));

    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd1.exe"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd2.bat"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd3.cmd"));

    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd1"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd2"));
    tassert_eq(true, os.cmd.exists(TBUILDDIR "mycmd3"));

    tassert_eq(true, os.cmd.exists("cmd.exe"));
    tassert_eq(true, os.cmd.exists("cmd"));
    tassert_eq(true, os.cmd.exists("./cex.exe"));
    tassert_eq(true, os.cmd.exists(".\\cex.exe"));
    tassert_eq(true, os.cmd.exists(".\\cex"));
#else
    tassert_eq(true, os.cmd.exists("ls"));
#endif

    return EOK;
}

test$case(os_cmd_create)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        char* args[] = { test_app("write_lines", _), NULL };
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 1);
    }
    return EOK;
}

test$case(os_cmd_file_handles)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert(os.cmd.fstderr(&c) == c._subpr.stderr_file);
        tassert(os.cmd.fstdout(&c) == c._subpr.stdout_file);
        tassert(os.cmd.fstdin(&c) == c._subpr.stdin_file);

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        tassert_er(Error.runtime, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 1);
    }
    return EOK;
}

test$case(os_cmd_read_all_small)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "10", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }

        // tassert_eq(strlen(output) / 10, arr$len(lines));
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(output[strlen(output) - 2], '\r');
            tassert_eq(output[strlen(output) - 1], '\n');
            tassert_eq(strlen(output) / 11, arr$len(lines));
        } else {
            tassert_eq(strlen(output) / 10, arr$len(lines));
        }
    }
    return EOK;
}

test$case(os_cmd_read_all_huge)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 100000);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }
    }
    return EOK;
}

test$case(os_cmd_read_line_huge)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* line;
        u32 lcnt = 0;
        while ((line = os.cmd.read_line(&c, _))) {
            tassert_eq(str.fmt(_, "%09d", lcnt), line);
            lcnt++;
        }
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        tassert_eq(lcnt, 100000);
    }
    return EOK;
}

test$case(os_cmd_read_all_only_stdout)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stderr", "10", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_eq(output, "");

        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);
    }
    return EOK;
}


test$case(os_cmd_read_all_combined_stderr)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stderr", "10", NULL);
        tassert_er(
            EOK,
            os.cmd.create(&c, args, arr$len(args), &(os_cmd_flags_s){ .combine_stdouterr = 1 })
        );

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }

        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(output[strlen(output) - 2], '\r');
            tassert_eq(output[strlen(output) - 1], '\n');
            tassert_eq(strlen(output) / 11, arr$len(lines));
        } else {
            tassert_eq(strlen(output) / 10, arr$len(lines));
        }
    }
    return EOK;
}

test$case(os_cmd_join_timeout)
{
    os_cmd_c c = { 0 };
    // WTF: if we call subprocess_terminate() on empty struct it will kill self!
    tassert_eq(0, os.cmd.is_alive(&c));
    e$ret(os.cmd.kill(&c));

    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("sleep", _), "2", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert_eq(1, os.cmd.is_alive(&c));

        tassert_er(Error.timeout, os.cmd.wait(&c, 1, 1));
        tassert_eq(os.cmd.ret_code(&c), -1);

        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert_er(Error.ok, os.cmd.wait(&c, 1, 3));
        tassert_eq(os.cmd.ret_code(&c), 0);
    }
    return EOK;
}

test$case(os_cmd_huge_join)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert_er(Error.timeout, os.cmd.wait(&c, 1, 1));
        tassert_eq(os.cmd.ret_code(&c), -1);
    }
    return EOK;
}

test$case(os_cmd_huge_join_stderr)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stderr", "100000", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        tassert_er(Error.timeout, os.cmd.wait(&c, 1, 1));
        tassert_eq(os.cmd.ret_code(&c), -1);
    }
    return EOK;
}

test$case(os_cmd_stdin_communucation)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("echo_server", _), NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));
        tassert(os.cmd.is_alive(&c));

        tassert_eq("welcome to echo server", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "hello"));
        tassert_eq("out: hello", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "world"));
        tassert_eq("out: world", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "end"));

        tassert_er(Error.ok, os.cmd.wait(&c, 1, 1));
    }
    return EOK;
}

test$case(os_cmd_read_all_small_wdelay)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines_delay", _), "stdout", "10", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        log$debug("write_lines_delay output:\n`%s`", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert_eq(os.cmd.ret_code(&c), 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) { tassert_eq(str.fmt(_, "%09d", i), lines[i]); }

        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(output[strlen(output) - 2], '\r');
            tassert_eq(output[strlen(output) - 1], '\n');
            tassert_eq(strlen(output) / 11, arr$len(lines));
        } else {
            tassert_eq(strlen(output) / 10, arr$len(lines));
        }
    }
    return EOK;
}

test$case(os_cmd_run)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_lines", _), "stdout", "10", NULL);
        tassert_er(EOK, os.cmd.run(args, arr$len(args), &c));
#if defined(_WIN32)
        tassert(c._subpr.hProcess != NULL);
#endif
        tassert_er(EOK, os.cmd.wait(&c, 1, 0));
    }
    return EOK;
}

test$case(os_cmd_run_macro_space_in_args)
{
    mem$scope(tmem$, _)
    {
        tassert_er(Error.runtime, os$cmd(test_app("write_arg", _)));
        tassert_er(EOK, os$cmd(test_app("write_arg", _), "hello world"));
        tassert_er(EOK, os$cmd(test_app("write_arg", _), "hello=\"world\""));
        tassert_er(EOK, os$cmd(test_app("write_arg", _), "hello=\"world big world\""));
    }
    return EOK;
}

test$case(os_cmd_run_read_all)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, test_app("write_arg", _), "hello world", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        log$debug("write_arg output:\n`%s`", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));

        tassert(str.starts_with(output, "1st argument: 'hello world'"));
    }
    return EOK;
}

test$case(os_cmd_env_inheritance)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        e$ret(os.env.set("TEST_CEX_ENV", "cool!"));
        arr$pushm(args, test_app("write_env", _), "TEST_CEX_ENV", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        printf("%s\n", output);
        tassert_er(Error.ok, os.cmd.wait(&c, 1, 0));
        tassert(str.starts_with(output, "TEST_CEX_ENV=cool!"));
        tassert_eq(os.cmd.ret_code(&c), 0);
    }
    return EOK;
}

test$case(os_cmd_env_inheritance_cmd_run)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        e$ret(os.env.set("TEST_CEX_ENV1", "cool!"));
        tassert_er(EOK, os$cmd(test_app("write_env", _), "TEST_CEX_ENV1"));
    }
    return EOK;
}

test$case(os_cmd_env_inheritance_cmd_run_not_found)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        tassert_er(Error.runtime, os$cmd(test_app("write_env", _), "TEST_CEX_ENV_ASLDKJLDJS"));
    }
    return EOK;
}

test$case(os_cmd_wait_all_infinite)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"100", "300", "400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }
        tassert_er(EOK, os.cmd.wait(c, arr$len(c), 0));
    }
    io.printf("Done waiting\n");
    return EOK;
}


test$case(os_cmd_wait_all_timeout)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"1100", "1300", "1400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            io.printf("run sleep_ms %s\n", timeouts[i]);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
            fflush(stdout);
        }

        tassert_er(Error.timeout, os.cmd.wait(c, arr$len(c), 1));

        for$eachp(it, c) {
            tassert_eq(1, os.cmd.is_alive(it));
            // living process always return -1 return code
            tassert_eq(-1, os.cmd.ret_code(it));
        }

        tassert_er(EOK, os.cmd.wait(c, arr$len(c), 1));
        for$eachp(it, c) {
            tassert_eq(0, os.cmd.is_alive(it));
            tassert_eq(0, os.cmd.ret_code(it));
        }
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_all_timeout_partial_done)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"100", "1300", "1400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }

        tassert_er(Error.timeout, os.cmd.wait(c, arr$len(c), 1));

        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(0, os.cmd.ret_code(&c[0]));
        tassert_eq(1, os.cmd.is_alive(&c[1]));
        tassert_eq(-1, os.cmd.ret_code(&c[1]));
        tassert_eq(1, os.cmd.is_alive(&c[2]));
        tassert_eq(-1, os.cmd.ret_code(&c[2]));

        tassert_er(EOK, os.cmd.wait(c, arr$len(c), 1));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(0, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_all_timeout_partial_done_errs)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"117", "1300", "1400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }

        tassert_er(Error.timeout, os.cmd.wait(c, arr$len(c), 1));

        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(1, os.cmd.is_alive(&c[1]));
        tassert_eq(-1, os.cmd.ret_code(&c[1]));
        tassert_eq(1, os.cmd.is_alive(&c[2]));
        tassert_eq(-1, os.cmd.ret_code(&c[2]));

        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 1));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));

        // redundant call, still works
        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 1));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_all_infinite_partial_done_errs)
{
    os_cmd_c c[3] = { 0 };
    char* timeouts[3] = {"117", "300", "400"};

    mem$scope(tmem$, _)
    {
        for(u32 i = 0; i < arr$len(c); i++ ){
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, test_app("sleep_ms", _), timeouts[i], NULL);
            tassert_er(EOK, os.cmd.run(args, arr$len(args), &c[i]));
        }

        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 0));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));

        // redundant call, still works
        tassert_er(Error.runtime, os.cmd.wait(c, arr$len(c), 0));
        tassert_eq(0, os.cmd.is_alive(&c[0]));
        tassert_eq(17, os.cmd.ret_code(&c[0]));
        tassert_eq(0, os.cmd.is_alive(&c[1]));
        tassert_eq(0, os.cmd.ret_code(&c[1]));
        tassert_eq(0, os.cmd.is_alive(&c[2]));
        tassert_eq(0, os.cmd.ret_code(&c[2]));
    }
    io.printf("Done waiting\n");
    return EOK;
}

test$case(os_cmd_wait_on_zii_data)
{
    os_cmd_c c[3] = { 0 };

    tassert_er(Error.ok, os.cmd.wait(c, arr$len(c), 0));
    tassert_eq(0, os.cmd.is_alive(&c[0]));
    tassert_eq(0, os.cmd.ret_code(&c[0]));
    tassert_eq(0, os.cmd.is_alive(&c[1]));
    tassert_eq(0, os.cmd.ret_code(&c[1]));
    tassert_eq(0, os.cmd.is_alive(&c[2]));
    tassert_eq(0, os.cmd.ret_code(&c[2]));
    return EOK;
}
#else
test$case(os_cmd_not_supported_by_platform)
{
    return EOK;
}
#endif  // #if !defined(__EMSCRIPTEN__)


test$main();
