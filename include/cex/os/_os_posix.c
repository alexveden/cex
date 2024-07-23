#include "os.h"
#include <cex.h>
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

static Exception
os__listdir_(str_c path, sbuf_c* buf)
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
os__getcwd_(sbuf_c* out)
{
    uassert(sbuf.isvalid(out) && "out is not valid sbuf_c (or missing initialization)");

    except_silent(err, sbuf.grow(out, PATH_MAX + 1))
    {
        return err;
    }
    sbuf.clear(out);

    errno = 0;
    if (unlikely(getcwd(*out, sbuf.capacity(out)) == NULL)) {
        return strerror(errno);
    }

    sbuf.update_len(out);

    return EOK;
}

static Exception
os__path__exists_(str_c path)
{
    if (!str.is_valid(path) || path.len == 0) {
        return Error.argument;
    }

    int ret_code = 0;
    if (path.buf[path.len] != '\0') {
        // it's non null term path!
        char path_buf[PATH_MAX + 1];
        except_silent(err, str.copy(path, path_buf, arr$len(path_buf)))
        {
            return err;
        }
        ret_code = access(path_buf, F_OK);
    } else {
        ret_code = access(path.buf, F_OK);
    }

    if (ret_code < 0){
        if(errno == ENOENT){
            return Error.not_found;
        } else {
            return strerror(errno);
        }
    }

    return Error.ok;
}
