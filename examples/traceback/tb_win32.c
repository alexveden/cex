// Traceback experiment: Windows CaptureStackBackTrace (+ optional dbghelp)
//
// Build without CEX_TB_DBGHELP to print raw addresses, or with
// -DCEX_TB_DBGHELP -ldbghelp to resolve function names from the PDB.
//
// argv[1] == "c" triggers an access violation and prints a traceback from a
// SetUnhandledExceptionFilter() handler; otherwise a plain traceback is printed.
#ifdef _WIN32

#    include <stdio.h>
#    include <windows.h>

#    ifdef CEX_TB_DBGHELP
#        include <dbghelp.h>
#    endif

static void
tb_print(void)
{
    void* frames[64];
    USHORT n = CaptureStackBackTrace(0, 64, frames, NULL);
    fprintf(stderr, "--- CaptureStackBackTrace (n=%u) ---\n", (unsigned)n);
#    ifdef CEX_TB_DBGHELP
    HANDLE proc = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (SymInitialize(proc, NULL, TRUE)) {
        char buf[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO* sym = (SYMBOL_INFO*)buf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 256;
        for (USHORT i = 0; i < n; i++) {
            DWORD64 addr = (DWORD64)frames[i];
            DWORD64 disp = 0;
            if (SymFromAddr(proc, addr, &disp, sym)) {
                fprintf(
                    stderr,
                    "%2u: %s +0x%llx\n",
                    (unsigned)i,
                    sym->Name,
                    (unsigned long long)disp
                );
            } else {
                fprintf(stderr, "%2u: 0x%llx\n", (unsigned)i, (unsigned long long)addr);
            }
        }
        SymCleanup(proc);
    }
#    else
    for (USHORT i = 0; i < n; i++) { fprintf(stderr, "%2u: %p\n", (unsigned)i, frames[i]); }
#    endif
}

static LONG WINAPI
tb_filter(EXCEPTION_POINTERS* ep)
{
    fprintf(
        stderr,
        "--- unhandled exception 0x%08lx ---\n",
        (unsigned long)ep->ExceptionRecord->ExceptionCode
    );
    tb_print();
    return EXCEPTION_EXECUTE_HANDLER;
}

static int
tb_level3(void)
{
    tb_print();
    return 0;
}

static int
tb_level2(void)
{
    return tb_level3();
}

static int
tb_level1(void)
{
    return tb_level2();
}

int
main(int argc, char** argv)
{
    if (argc > 1 && argv[1][0] == 'c') {
        SetUnhandledExceptionFilter(tb_filter);
        *(volatile int*)0 = 1; // access violation
    }
    return tb_level1();
}

#else

#    include <stdio.h>

int
main(void)
{
    printf("SKIP: Windows-only experiment\n");
    return 0;
}

#endif
