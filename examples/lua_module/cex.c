#if __has_include("cex_config.h")
// Custom config file
#    include "cex_config.h"
#else
// Overriding config values
#    define cexy$build_dir "./build"
#    define LUA_TAG "v5.3.5"
#    define LUA_GIT "https://github.com/lua/lua"
#    define LUA_DIR cexy$build_dir "/lua"
#    define cexy$cc_include "-I.", "-I" LUA_DIR "/include"
#endif

#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

#define LUA_LIB LUA_DIR "/mylualib" cexy$build_ext_lib_dyn // output of lua lib

Exception cmd_build_lua(int argc, char** argv, void* user_ctx);
Exception cmd_build_lua_lib(int argc, char** argv, void* user_ctx);
Exception cmd_test_lua_lib(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    cexy$initialize(); // cex self rebuild and init

    argparse_c args = {
        .description = cexy$description,
        .epilog = cexy$epilog,
        .usage = cexy$usage,
        argparse$cmd_list(
            cexy$cmd_all,
            cexy$cmd_test, /* feel free to make your own if needed */
            { .name = "build-lua", .func = cmd_build_lua, .help = "Download and build LUA" },
            { .name = "build-lib", .func = cmd_build_lua_lib, .help = "Build lua library" },
            { .name = "test-lib", .func = cmd_test_lua_lib, .help = "Test lua library" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    void* my_user_ctx = NULL; // passed as `user_ctx` to command
    if (argparse.run_command(&args, my_user_ctx)) { return 1; }
    return 0;
}

/// Custom build command for building lua runtime from source
Exception
cmd_build_lua(int argc, char** argv, void* user_ctx)
{
    e$assert(os.cmd.exists("git") && "git command not found");

    (void)argc;
    (void)argv;
    (void)user_ctx;
    if (!os.path.exists(LUA_DIR "/src/makefile")) {
        e$ret(os$cmd("git", "clone", "--branch", LUA_TAG, "--depth=1", LUA_GIT, LUA_DIR "/src/"));
    }

    e$assert(os.path.exists(LUA_DIR "/src/makefile") && "no makefile in lua repo");

    if (os.path.exists(LUA_DIR "/lua" cexy$build_ext_exe)) {
        log$info("LUA already exists in: %s\n", LUA_DIR);
        e$ret(os$cmd(LUA_DIR "/lua", "-v"));
        return EOK;
    }


    log$info("Building lua from source\n");
    mem$scope(tmem$, _)
    {
        log$info("Parsing LUA Makefile\n");

        char* old_cwd = os.fs.getcwd(_); // store current dir for restoring back at the end?
        e$ret(os.fs.chdir(LUA_DIR "/src/"));

        // Loading all vars and targets from LUA Makefile
        hm$(str_s, arr$(char*)) make_vars = hm$new(make_vars, _);

        FILE* makefile;
        e$ret(io.fopen(&makefile, "makefile", "r"));

        char* line = NULL;
        arr$(char*)* items = NULL;
        bool continue_backslash = false;
        while ((line = io.file.readln(makefile, _))) {
            str_s sl = str.slice.strip(str.sstr(line));
            isize val_start_idx = 0;

            if (items == NULL) {
                // New Makefile variable, i.e. CFLAGS=
                if (str.slice.match(sl, "[A-Z_+]*=*")) {
                    val_start_idx = str.slice.index_of(sl, str$s("="));
                    uassert(val_start_idx >= 0);

                    str_s make_var = str.slice.strip(str.slice.sub(sl, 0, val_start_idx));
                    log$trace("Variable: '%S'\n", make_var);

                    hm$set(make_vars, make_var, NULL);
                    items = hm$getp(make_vars, make_var);
                    uassert(*items == NULL);
                    arr$new(*items, _);
                    val_start_idx++;
                }
            }

            if (items) {
                // Parsing variable contents (including multiline)
                str_s value = str.slice.sub(sl, val_start_idx, 0);
                for$iter (str_s, it, str.slice.iter_split(value, " \t", &it.iterator)) {
                    it.val = str.slice.strip(it.val);
                    if (it.val.len == 0) { continue; }
                    if (it.val.buf[0] == '\\') { break; }
                    if (it.val.buf[0] == '#') { break; }
                    if (it.val.buf[0] == '$') { continue; }
                    arr$push(*items, str.slice.clone(it.val, _));
                    // io.printf("* '%S'\n", it.val);
                }

                continue_backslash = str.slice.ends_with(sl, str$s("\\"));
                if (!continue_backslash) {
                    items = NULL; // no parsing continuation
                }
            }
        }

        arr$(char*) args = arr$new(args, _);

        // Compilation stage
        log$info("Compiling sources (if needed)\n");
        for$each (src, os.fs.find("*.c", false, _)) {
            char* tgt = str.replace(src, ".c", ".o", _);
            if (!cexy.src_include_changed(tgt, src, NULL)) { continue; }

            io.printf("src: %s\n", src);
            arr$clear(args);
            arr$pushm(args, cexy$cc, "-c");
            arr$pusha(args, hm$get(make_vars, str$s("CFLAGS")));

            // NOTE: platform specific compiler flags
            if (os.platform.current() == OSPlatform__win) {
                arr$pushm(args, "-DLUA_BUILD_AS_DLL");
            } else if (os.platform.current() == OSPlatform__linux) {
                arr$pushm(args, "-DLUA_USE_LINUX");
            } else if (os.platform.current() == OSPlatform__macos) {
                arr$pushm(args, "-DLUA_USE_MACOSX");
            } else {
                unreachable();
            }

            arr$pushm(args, src, "-o", tgt);
            arr$push(args, NULL);
            e$ret(os$cmda(args));
        }

        // Linking lua executable
        log$info("Linking LUA\n");
        arr$clear(args);
        arr$pushm(args, cexy$cc, "-o", "../lua" cexy$build_ext_exe);
        arr$pushm(args, "lua.o");
        arr$pusha(args, hm$get(make_vars, str$s("CORE_O")));
        arr$pusha(args, hm$get(make_vars, str$s("AUX_O")));
        arr$pusha(args, hm$get(make_vars, str$s("LIB_O")));

        // NOTE: platform specific linker flags
        if (os.platform.current() == OSPlatform__win) {
            // NOTE: making .exe build to export symbols to static .a lib, because
            //       on windows it's mandatory to link dll against the .exe
            arr$pushm(args, "-Wl,--out-implib,../liblua.a", );
        } else if (os.platform.current() == OSPlatform__linux) {
            arr$pushm(args, "-Wl,-E", "-ldl", "-lreadline");
        } else if (os.platform.current() == OSPlatform__macos) {
            arr$pushm(args, "-ldl", "-lreadline");
        } else {
            unreachable();
        }

        arr$pusha(args, hm$get(make_vars, str$s("LIBS")));
        arr$pushm(args, NULL);
        e$ret(os$cmda(args));

        // Copying include headers
        log$info("Copying headers\n");
        e$ret(os.fs.mkdir("../include/"));
        for$each (hfile, os.fs.find("./*.h", false, _)) {
            str_s basename = os.path.split(hfile, false);
            char* dest = str.fmt(_, "../include/%S", basename);
            if (os.path.exists(dest)) { e$ret(os.fs.remove(dest)); }
            log$trace("%s -> %s\n", hfile, dest);
            e$ret(os.fs.copy(hfile, dest));
        }

        // Finishing
        e$ret(os.fs.chdir(old_cwd));
        log$info("LUA Build finished\n");

        e$ret(os$cmd(LUA_DIR "/lua", "-v"));
    }
    return EOK;
}

Exception
cmd_build_lua_lib(int argc, char** argv, void* user_ctx)
{
    log$info("Launching LUA Lib Build\n");

    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(
            args,
            cexy$cc,
            "-shared",
            "-fPIC",
            "-Wall",
            "-Wextra",
            "-Werror",
            cexy$cc_include,
        );
        if (os.platform.current() == OSPlatform__win) {
        } else if (os.platform.current() == OSPlatform__macos) {
            // Mitigating this error
            // Undefined symbols for architecture arm64:
            //    "_luaL_checknumber", referenced from:
            arr$pushm(args, "-undefined", "dynamic_lookup");
        }
        arr$pushm(args, "-o", LUA_LIB, "lib/mylib.c", "lib/mylib_lua.c");
        // Linker options
        if (os.platform.current() == OSPlatform__win) {
            // NOTE: on windows it's mandatory to have static symbols to have .dll linkage
            arr$pushm(args, "-L" LUA_DIR, "-llua");
        } else if (os.platform.current() == OSPlatform__macos) {
        }

        arr$pushm(args, NULL);
        e$ret(os$cmda(args));

        // Test steps
        // copy .so/.dll into $PATH or near lua folder
        // > lua
        /*
        local mylualib = require "mylualib"
        print(mylualib.myfunction(3, 4))  -- Prints 7
        print(mylualib.version)           -- Prints "1.0"
        */
    }

    return EOK;
}

Exception
cmd_test_lua_lib(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    if (!os.path.exists(LUA_DIR "/lua" cexy$build_ext_exe)) {
        return e$raise(Error.not_found, "LUA interpreter not exist, run `./cex build-lua` first");
    }

    if (!os.path.exists(LUA_LIB)) {
        return e$raise(
            Error.not_found,
            "LUA lib not found (%s), run `./cex build-lib` first",
            LUA_LIB
        );
    }

    // Making simple test script
    char* lua_test_script = "local mylualib = require \"mylualib\" \n"
                            "print(mylualib.myfunction(3, 4))\n"
                            "print(mylualib.version)\n"
                            "os.exit(0)\n";
    e$ret(io.file.save(LUA_DIR "/mylualib_test.lua", lua_test_script));

    // Launching script + parsing lua output in stdout
    e$ret(os.fs.chdir(LUA_DIR));

    // Printing out the command output
    char* lua_cmd[] = { "./lua", "mylualib_test.lua", NULL };
    e$ret(os$cmda(lua_cmd));

    // Another try but with auto checking output
    os_cmd_c cmd = { 0 };
    e$ret(os.cmd.create(&cmd, lua_cmd, arr$len(lua_cmd), NULL));
    mem$scope(tmem$, _)
    {
        char* cmd_out = os.cmd.read_all(&cmd, _);
        e$ret(os.cmd.wait(&cmd, 1, 1));
        // Checking the printout of the command
        e$assert(cmd_out != NULL);
        e$assert(str.len(cmd_out) > 5);
        e$assert(str.find(cmd_out, "7.0"));
        e$assert(str.find(cmd_out, "1.0-cex"));
    }

    return EOK;
}
