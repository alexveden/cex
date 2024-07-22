#include "os.h"
#include <cex.h>
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

static Exception
os__listdir(str_c path, sbuf_c* buf)
{
    if (unlikely(path.len == 0)) {
        return Error.argument;
    }
    if (buf == NULL) {
        return Error.argument;
    }

    // use own as temp buffer for dir name (because path, may not be a valid null-term string)
    DIR* dp = NULL;
    Exc result = Error.ok;

    uassert(sbuf.isvalid(buf) && "buf is not valid sbuf_c (or missing initialization)");

    sbuf.clear(buf);

    e$goto(result = sbuf.append(buf, path), fail);

    dp = opendir(*buf);

    if (unlikely(dp == NULL)) {
        result = Error.not_found;
        goto fail;
    }
    sbuf.clear(buf);

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        var d = s$(ep->d_name);

        if (str.cmp(d, s$(".")) == 0) {
            continue;
        }
        if (str.cmp(d, s$("..")) == 0) {
            continue;
        }

        uassert(d.buf[d.len] == '\0' && "not null term");

        // we store list of files as '\n' separated records
        // we just expand +1 char, so therefore we need extra
        // sbuf.append() call for '\n' because d is already null
        // terminated string
        d.buf[d.len] = '\n';
        d.len++;
        e$goto(result = sbuf.append(buf, d), fail);
    }

end:
    if (dp != NULL) {
        (void)closedir(dp);
    }
    return result;

fail:
    sbuf.clear(buf);
    goto end;
}

static Exception
os__getcwd(sbuf_c* out)
{
    uassert(sbuf.isvalid(out) && "out is not valid sbuf_c (or missing initialization)");

    e$ret(sbuf.grow(out, PATH_MAX + 1));
    sbuf.clear(out);

    errno = 0;
    if (unlikely(getcwd(*out, sbuf.capacity(out)) == NULL)) {
        return strerror(errno);
    }

    sbuf.update_len(out);

    return EOK;
}
