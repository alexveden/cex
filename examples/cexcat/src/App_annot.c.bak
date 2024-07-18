#include "App.h"
#include <cex.h>

Exception
App_create(App_c* app, i32 argc, char* argv[], const Allocator_i* allocator)
{
    // NOTE: uassert - u(niversal)assert, which is the same as regular C assert, but shows
    // full call stack when condition inside it is false (with program abort() as well).
    // You may attach message to it by simply adding && "some text".
    // This assert can be disabled in test$case, by calling uassert_disable()
    // All such asserts totally turned off when you compile with -DNDEBUG
    uassert(allocator != NULL && "expected allocator");

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_str('c', "csv", &app->is_csv, .help = "parse input files as csv->json"),
    };

    const char* usage = "[-c/--csv] [[--] file1 file2 fileN]";

    argparse_c args = { .options = options,
                        .options_len = arr$len(options),
                        .usage = usage,
                        .description = "Basic cat utility with convert csv->json function",
                        .epilog = "\nIt has intentional bugs, try to fix them :)" };

    // NOTE: there is no need to print traceback here, because argparse.parse() already printed
    except(err, argparse.parse(&args, argc, argv))
    {
        return err;
    }

    // NOTE: after argparse.parse() remaining positional arguments are available via
    // argparse.argc() / argparse.argv()
    if (argparse.argc(&args) == 0) {
        argparse.usage(&args);
        return Error.argsparse;
    }

    app->files = argparse.argv(&args);
    app->files_count = argparse.argc(&args);
    app->allocator = allocator;

    return Error.ok;
}

// NOTE: private functions must have different name as file namespace, or use double underscore
static Exception
App__process_plain(App_c* app, io_c* file)
{
    uassert(app != NULL);
    uassert(file != NULL);

    if (io.size(file) == 0) {
        // NOTE: errors in CEX are string constants, you may use Error.<something> or just
        // return literal string with error description if it's not intended to be handled by
        // the caller side
        return "zero file size";
    }

    // str_c is a CEX string view, it's not compatible with char* with caveats
    str_c line; // it carries pointer + length

    Exc r = EOK;
    while ((r = io.readline(file, &line)) == EOK) {
        // // NOTE: using io.printf() is a custom implementation of printf() formatting,
        // // it allows us to pass str_c as %S (str_c and char* are not compatible)
        io.printf("%S\n", line);

        // NOTE: s$("string") is easiest way to convert string literals into CEX str_c
        if (str.contains(line, s$("cat"))) {
            io.printf("edited by cex cat with love\n");
        }

        // if (io.printf("%S\n", line)) {
        //     // this block is another way of handling errors in CEX. EOK is always false.
        //     // there's always a silent way of handling CEX errors, and printing custom messages
        //
        //     // This is how we can handle and report CEX messages when we faced some error. We
        //     // can return raise_exc(error, "format", ...), this function prints error message
        //     // it it's a top of CEX traceback prints
        //     return raise_exc(
        //         Error.integrity,
        //         "something went wrong with file reading, line len: %ld\n",
        //         line.len
        //     );
        // }
    }

    return Error.ok;
}

// NOTE: private functions must have different name as file namespace, or use double underscore
/**
 * @brief  This function reads csv file and converts its content into JSON-style output,
 * where each record has name of a column and value
 *
 * @param app
 * @param file
 * @return
 */
static Exception
App__process_csv(App_c* app, io_c* file)
{
    uassert(app != NULL);
    uassert(app->allocator != NULL);
    uassert(file != NULL);

    Exc result = EOK;

    sbuf_c rbuf; // result buffer for the JSON text
    e$goto(fail, sbuf.create(&rbuf, 1024, app->allocator));

    str_c contents;
    // try read all contents of the file, and reflect it via string viewer
    e$goto(fail, io.readall(file, &contents));

    // OK, file contents were read, we need something to map column names with indexes on next rows
    //
    //
    // in modern C it's possible to declare temporary structs just in the function
    struct csvcols
    {
        u64 col_idx;    // dict key
        char name[128]; // dict value
    };

    // CEX csvmap is designed to work with structs items. First element of the struct item expected
    // to be a dict key
    dict_c csvmap;
    e$goto(fail, dict$new(&csvmap, struct csvcols, col_idx, app->allocator));


    // NOTE:for$iter - is an universal iterator macro used across many CEX namespaces, api is quite
    // the same. All iterable functions must start with iter...(args...)
    for$iter(str_c, it, str.iter_split(contents, "\n", &it.iterator))
    {
        var line = str.strip(*it.val);

        // Parsing header
        if (unlikely(it.idx.i == 0)) {

            // print starting { bracket into result buffer
            e$goto(fail, sbuf.sprintf(&rbuf, "{\n"));

            // we are at the first row, let's store columns to
            //  remove leading trailing spaces
            if (str.len(line) == 0) {
                // empty header, nothing to do
                result = raise_exc(Error.empty, "empty header\n");
                goto fail;
            }


            for$iter(str_c, tok, str.iter_split(line, ",", &tok.iterator))
            {

                // preparing for storage of dict key
                struct csvcols col_rec = { .col_idx = tok.idx.i };

                // automatic variable type
                var col = str.strip(*tok.val);

                io.printf("%ld: '%S'\n", tok.idx.i, col);

                // str.copy() is a safe version for strncpy, which raises Exception on overflow
                // of bad inputs (like NULLs)
                e$goto(fail, str.copy(col, col_rec.name, arr$len(col_rec.name)));

                // store column information in the dict
                e$goto(fail, dict.set(&csvmap, &col_rec));
            }

            continue;
        }

        if (str.len(line) == 0) {
            continue;
        }

        // print starting { bracket into result buffer
        e$goto(fail, sbuf.sprintf(&rbuf, "  {"));

        for$iter(str_c, tok, str.iter_split(line, ",", &tok.iterator))
        {
            // getting column name
            struct csvcols* rec = dict.geti(&csvmap, tok.idx.i);
            if (rec == NULL) {
                result = raise_exc(
                    "column width mismatch",
                    "header len=%ld col=%ld\n",
                    dict.len(&csvmap),
                    tok.idx.key
                );
                goto fail;
            }

            var col = str.strip(*tok.val);

            // printing json body
            e$goto(fail, sbuf.sprintf(&rbuf, "\"%s\": \"%S\", ", rec->name, col));
        }
        e$goto(fail, sbuf.sprintf(&rbuf, "},\n"));
    }

    // end printout with }
    e$goto(fail, sbuf.sprintf(&rbuf, "}\n"));


    // output of the buffer
    // NOTE: sbuf_c is fully compatibl with char*, and it's guaranteed to be null-terminated
    io.printf("%s\n", rbuf);

fail:
    // cleanup
    dict.destroy(&csvmap);
    sbuf.destroy(&rbuf);
    return result;
}

Exception
App_main(App_c* app, const Allocator_i* allocator)
{
    (void)allocator;
    Exc result = EOK;

    // NOTE: this is io_c (_c stands for class/namespace), when you see types like this in CEX,
    // it means there're a bunch of functions attached to the namespace io.<func>, it's a CEX
    // convention.
    io_c f = { 0 };


    // NOTE: for$array is an iterator for plain arrays, `it` is an iterator variable which has
    // automatic type based on array element. it.val is a pointer to data, it.idx.i - index.
    for$array(it, app->files, app->files_count)
    {
        uassert(*it.val != NULL && "unexpected null file name");

        // NOTE: e$ret macro is one liner equivalent of except_traceback(err, <call>) {return err;}
        e$ret(io.fopen(&f, *it.val, "r", allocator));

        if (app->is_csv) {
            uassert(false && "TODO: implement");
            // CEX supports "goto fail" strategy of error handling, it's maybe one of legit ways of
            // using goto in C.

            // NOTE: e$goto is equivalent of except_traceback(err, <call>) {goto label;}
            e$goto(fail, App__process_csv(app, &f));
        } else {
            except_traceback(err, App__process_plain(app, &f))
            {
                // Another strategy of error handling is early returns, also works if there are
                // not too many resource to free
                io.close(&f);
                return err;
            }
        }

        // Regular close
        io.close(&f);
    }

fail:
    // In CEX all destructors are anti-fragile, they can tolerate NULL, already free data, etc.
    io.close(&f);

    return result;
}

void
App_destroy(App_c* app, const Allocator_i* allocator)
{
    (void)app;
    (void)allocator;
    utracef("App is shutting down\n");
}

const struct __class__App App = {
    // Autogenerated by CEX
    // clang-format off
    .create = App_create,
    .main = App_main,
    .destroy = App_destroy,
    // clang-format on
};
