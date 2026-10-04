#define CEX_PANIC_VERBOSITY 1
#include "src/all.c"
#include "cex_errors_fork.h"

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(NDEBUG)

static void
_run_mem_prefix_panic(void)
{
    uassert_disable();
    cex$platform_panic(_cex_errors_mem_prefix, __FILE_NAME__, __LINE__, __func__, "boom");
}

// The uassert-suppression shortcut in _cex_errors_panic_handler only applies to the
// [ASSERT] prefix; any other prefix must stay fatal even while uassert is disabled.
test$case(non_assert_prefix_fatal_when_uassert_disabled)
{
    tassert(is_fatal_in_child(_run_mem_prefix_panic));
    return EOK;
}

#endif

test$main();
