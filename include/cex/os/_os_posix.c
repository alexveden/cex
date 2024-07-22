#include "os.h"
#include <cex.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>

static Exception
os__listdir(str_c path, sbuf_c* own, const Allocator_i* allc)
{
    if (unlikely(path.len == 0)) {
        return Error.argument;
    }
    if (own == NULL) {
        return Error.argument;
    }
    if (allc == NULL) {
        return Error.argument;
    }

    Exc result = Error.ok;
    bool sbuf_created = false;
    if (sbuf.isvalid(own)) {
        utracef("own is valid\n");
        sbuf.clear(own);
    } else {
        e$ret(sbuf.create(own, 1024, allc));
        sbuf_created = true;
    }

    // use own as temp buffer for dir name (because path, may not be a valid null-term string)
    DIR* dp = NULL;
    e$goto(result = sbuf.append(own, path), fail);
    dp = opendir(*own);


    if (unlikely(dp == NULL)) {
        result = Error.not_found;
        goto fail;
    }
    sbuf.clear(own);

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        var d = s$(ep->d_name);

        if(str.cmp(d, s$(".")) == 0) {
            continue;
        }
        if(str.cmp(d, s$("..")) == 0) {
            continue;
        }

        uassert(d.buf[d.len] == '\0' && "not null term");

        // we store list of files as '\n' separated records
        // we just expand +1 char, so therefore we need extra
        // sbuf.append() call for '\n' because d is already null
        // terminated string
        d.buf[d.len] = '\n';
        d.len++;
        e$goto(result = sbuf.append(own, d), fail);
    }

end:
    if (dp != NULL) {
        (void)closedir(dp);
    }
    return result;

fail:
    if (sbuf_created) {
        sbuf.destroy(own);
    }
    goto end;
}
