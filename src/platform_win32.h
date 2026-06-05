#pragma once
#if !defined(CEX_PLATFORM_WIN32_H) && defined(_WIN32)
#define CEX_PLATFORM_WIN32_H

// ============================================================
// Platform: Windows (Win32 API) type/constant/function declarations
//
// This header replaces #include <windows.h> for CEX's purposes.
// It uses explicit A-suffixed API names (no TCHAR macros).
//
// NOTE: All definitions are guarded to avoid collisions when
// user code also includes <windows.h>.  Define CEX_NO_WIN32_TYPES
// to skip this header entirely and provide your own Win32 types.
// ============================================================

#include <stddef.h> // size_t

#if !defined(CEX_NO_WIN32_TYPES) && !defined(_WINDEF_)

// ---------------------------------------------------------------------------
// Basic type aliases (matching Win32 ABI)
// ---------------------------------------------------------------------------
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef void*               HANDLE;
typedef long long           LONGLONG;

// ---------------------------------------------------------------------------
// Structs (tag names match Windows SDK convention for collision safety)
// ---------------------------------------------------------------------------
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

typedef struct _WIN32_FIND_DATAA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
    DWORD    dwReserved0;
    DWORD    dwReserved1;
    char     cFileName[260];
    char     cAlternateFileName[14];
} WIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        long  HighPart;
    };
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct _SYSTEM_INFO {
    DWORD  dwOemId;
    DWORD  dwPageSize;
    void*  lpMinimumApplicationAddress;
    void*  lpMaximumApplicationAddress;
    DWORD* dwActiveProcessorMask;
    DWORD  dwNumberOfProcessors;
    DWORD  dwProcessorType;
    DWORD  dwAllocationGranularity;
    unsigned short wProcessorLevel;
    unsigned short wProcessorRevision;
} SYSTEM_INFO;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    void* lpSecurityDescriptor;
    BOOL  bInheritHandle;
} SECURITY_ATTRIBUTES;
typedef SECURITY_ATTRIBUTES* LPSECURITY_ATTRIBUTES;

typedef struct _STARTUPINFOA {
    DWORD  cb;
    char*  lpReserved;
    char*  lpDesktop;
    char*  lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    unsigned short wShowWindow;
    unsigned short cbReserved2;
    unsigned char* lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA;
typedef STARTUPINFOA* LPSTARTUPINFOA;
typedef STARTUPINFOA STARTUPINFO;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
} PROCESS_INFORMATION;
typedef PROCESS_INFORMATION* LPPROCESS_INFORMATION;

typedef struct _OVERLAPPED {
    uintptr_t Internal;
    uintptr_t InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        };
        void* Pointer;
    };
    HANDLE hEvent;
} OVERLAPPED;
typedef OVERLAPPED* LPOVERLAPPED;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(~(size_t)0))
#endif
#ifndef MAX_PATH
#define MAX_PATH             260
#endif
#ifndef FALSE
#define FALSE                0
#endif
#ifndef TRUE
#define TRUE                 1
#endif

// Error codes
#ifndef ERROR_NO_MORE_FILES
#define ERROR_NO_MORE_FILES  18L
#endif
#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND 2L
#endif
#ifndef ERROR_PATH_NOT_FOUND
#define ERROR_PATH_NOT_FOUND 3L
#endif
#ifndef ERROR_ACCESS_DENIED
#define ERROR_ACCESS_DENIED  5L
#endif
#ifndef ERROR_MR_MID_NOT_FOUND
#define ERROR_MR_MID_NOT_FOUND 317L
#endif

// File operations
#ifndef MOVEFILE_REPLACE_EXISTING
#define MOVEFILE_REPLACE_EXISTING 1
#endif

// Process startup
#ifndef STARTF_USESTDHANDLES
#define STARTF_USESTDHANDLES 0x00000100
#endif

// Standard handles
#ifndef STD_INPUT_HANDLE
#define STD_INPUT_HANDLE  ((DWORD)-10)
#endif
#ifndef STD_OUTPUT_HANDLE
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#endif
#ifndef STD_ERROR_HANDLE
#define STD_ERROR_HANDLE  ((DWORD)-12)
#endif

// FormatMessage flags
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#endif
#ifndef FORMAT_MESSAGE_IGNORE_INSERTS
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#endif

// Language
#ifndef LANG_USER_DEFAULT
#define LANG_USER_DEFAULT 0x0400
#endif

// PATH_MAX fallback (defined by <limits.h> on MinGW, may be missing on MSVC)
#ifndef PATH_MAX
#define PATH_MAX 260
#endif

// ---------------------------------------------------------------------------
// API function declarations
// ---------------------------------------------------------------------------

// --- kernel32.dll ---
__declspec(dllimport) DWORD    __stdcall GetLastError(void);
__declspec(dllimport) void     __stdcall Sleep(DWORD milliseconds);
__declspec(dllimport) DWORD    __stdcall GetCurrentProcessId(void);
__declspec(dllimport) DWORD    __stdcall GetCurrentThreadId(void);
__declspec(dllimport) HANDLE   __stdcall GetStdHandle(DWORD nStdHandle);
__declspec(dllimport) BOOL     __stdcall CloseHandle(HANDLE hObject);
__declspec(dllimport) BOOL     __stdcall ReadFile(HANDLE, void*, DWORD, DWORD*, LPOVERLAPPED);
__declspec(dllimport) BOOL     __stdcall CreatePipe(HANDLE*, HANDLE*, LPSECURITY_ATTRIBUTES, DWORD);
__declspec(dllimport) HANDLE   __stdcall CreateNamedPipeA(const char*, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPSECURITY_ATTRIBUTES);
__declspec(dllimport) HANDLE   __stdcall CreateFileA(const char*, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
__declspec(dllimport) HANDLE   __stdcall CreateEventA(LPSECURITY_ATTRIBUTES, BOOL, BOOL, const char*);
__declspec(dllimport) BOOL     __stdcall CreateProcessA(const char*, char*, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, void*, const char*, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
__declspec(dllimport) BOOL     __stdcall SetHandleInformation(HANDLE, DWORD, DWORD);
__declspec(dllimport) DWORD    __stdcall WaitForSingleObject(HANDLE, DWORD);
__declspec(dllimport) BOOL     __stdcall GetExitCodeProcess(HANDLE, DWORD*);
__declspec(dllimport) BOOL     __stdcall TerminateProcess(HANDLE, unsigned int);
__declspec(dllimport) DWORD    __stdcall WaitForMultipleObjects(DWORD, HANDLE const*, BOOL, DWORD);
__declspec(dllimport) BOOL     __stdcall GetOverlappedResult(HANDLE, LPOVERLAPPED, DWORD*, BOOL);

// --- kernel32.dll (file system) ---
__declspec(dllimport) HANDLE   __stdcall FindFirstFileA(const char*, WIN32_FIND_DATAA*);
__declspec(dllimport) BOOL     __stdcall FindNextFileA(HANDLE, WIN32_FIND_DATAA*);
__declspec(dllimport) BOOL     __stdcall FindClose(HANDLE);
__declspec(dllimport) BOOL     __stdcall MoveFileExA(const char*, const char*, DWORD);
__declspec(dllimport) BOOL     __stdcall DeleteFileA(const char*);
__declspec(dllimport) BOOL     __stdcall RemoveDirectoryA(const char*);
__declspec(dllimport) BOOL     __stdcall CopyFileA(const char*, const char*, BOOL);
__declspec(dllimport) DWORD    __stdcall GetFullPathNameA(const char*, DWORD, char*, char**);
__declspec(dllimport) BOOL     __stdcall QueryPerformanceFrequency(LARGE_INTEGER*);
__declspec(dllimport) BOOL     __stdcall QueryPerformanceCounter(LARGE_INTEGER*);
__declspec(dllimport) void     __stdcall GetSystemInfo(SYSTEM_INFO*);
__declspec(dllimport) DWORD    __stdcall FormatMessageA(DWORD, void*, DWORD, DWORD, char*, DWORD, void*);

// --- kernel32.dll (debug, test-only) ---
__declspec(dllimport) BOOL     __stdcall IsBadReadPtr(const void*, size_t);

#endif // !CEX_NO_WIN32_TYPES && !_WINDEF_

#endif // CEX_PLATFORM_WIN32_H
