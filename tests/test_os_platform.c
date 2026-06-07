#include "src/all.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(os_platform_to_str)
{
    tassert_eq(0, OSPlatform__unknown);
    tassert_eq(1, OSPlatform__linux);

    tassert_eq(os.platform.to_str(0), NULL);
    tassert_eq(os.platform.to_str(OSPlatform__count), NULL);
    tassert_eq(os.platform.to_str(120398), NULL);
    tassert_eq(os.platform.to_str((OSPlatform_e)(-1)), NULL);
    tassert_eq("android", os.platform.to_str(OSPlatform__android));
    tassert_eq("win", os.platform.to_str(OSPlatform__win));
    tassert_eq("macos", os.platform.to_str(OSPlatform__macos));
    tassert_eq("linux", (char*)OSPlatform_str[OSPlatform__linux]);
    tassert_eq("linux", os.platform.to_str(OSPlatform__linux));
    tassert_eq("wasm", os.platform.to_str(OSPlatform__wasm));


    return EOK;
}


test$case(os_platform_from_str)
{

    tassert_eq(OSPlatform__unknown, os.platform.from_str(""));
    tassert_eq(OSPlatform__unknown, os.platform.from_str(NULL));
    tassert_eq(OSPlatform__win, os.platform.from_str("win"));
    tassert_eq(OSPlatform__unknown, os.platform.from_str("ualkdsjal"));
    tassert_eq(OSPlatform__android, os.platform.from_str("android"));

    return EOK;
}

test$case(os_arch_to_str)
{
    tassert_eq(0, OSArch__unknown);
    tassert_eq(1, OSArch__x86_32);

    tassert_eq(os.platform.arch_to_str(0), NULL);
    tassert_eq(os.platform.arch_to_str((OSArch_e)(-1)), NULL);
    tassert_eq(os.platform.arch_to_str(OSArch__count), NULL);
    tassert_eq(os.platform.arch_to_str(10928301), NULL);

    tassert_eq(os.platform.arch_to_str(OSArch__x86_32), "x86_32");
    tassert_eq(os.platform.arch_to_str(OSArch__x86_64), "x86_64");
    tassert_eq(os.platform.arch_to_str(OSArch__arm), "arm");
    tassert_eq(os.platform.arch_to_str(OSArch__wasm64), "wasm64");

    return EOK;
}

test$case(os_arch_from_str)
{
    tassert_eq(os.platform.arch_from_str(""), OSArch__unknown);
    tassert_eq(os.platform.arch_from_str(NULL), OSArch__unknown);
    tassert_eq(os.platform.arch_from_str("alsdjaldsj"), OSArch__unknown);
    tassert_eq(os.platform.arch_from_str("wasm64"), OSArch__wasm64);

    return EOK;
}

test$case(os_platform_current)
{
    tassert_ne(os.platform.current(), OSArch__unknown);
    tassert_ne(os.platform.current_str(), NULL);
    tassert_eq(os.platform.current_str(), os.platform.to_str(os.platform.current()));

#if defined(_WIN32)
    tassert_eq(os.platform.current(), OSPlatform__win);
#elif defined(__EMPSTRIPTEN__)
    tassert_eq(os.platform.current(), OSPlatform__wasm);
#endif

    return EOK;
}
test$main();
