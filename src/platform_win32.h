#pragma once
#if !defined(CEX_PLATFORM_WIN32_H) && defined(_WIN32)
#define CEX_PLATFORM_WIN32_H

// ============================================================
// Platform: Windows (Win32 API) type/constant/function declarations
//
// This header replaces #include <windows.h> for CEX's purposes.
// It uses explicit A-suffixed API names (no TCHAR macros).
// ============================================================

#include <stddef.h> // size_t

// ---------------------------------------------------------------------------
// Basic type aliases (matching Win32 ABI)
// ---------------------------------------------------------------------------
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef void*               HANDLE;
typedef long long           LONGLONG;

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------
typedef struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

typedef struct WIN32_FIND_DATAA {
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

typedef union LARGE_INTEGER {
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct SYSTEM_INFO {
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

typedef struct SECURITY_ATTRIBUTES {
    DWORD nLength;
    void* lpSecurityDescriptor;
    BOOL  bInheritHandle;
} SECURITY_ATTRIBUTES;
typedef SECURITY_ATTRIBUTES* LPSECURITY_ATTRIBUTES;

typedef struct STARTUPINFOA {
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

typedef struct PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
} PROCESS_INFORMATION;
typedef PROCESS_INFORMATION* LPPROCESS_INFORMATION;

typedef struct OVERLAPPED {
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
#define INVALID_HANDLE_VALUE ((HANDLE)(~(size_t)0))
#define MAX_PATH             260
#define FALSE                0
#define TRUE                 1

// Error codes
#define ERROR_NO_MORE_FILES  18L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_PATH_NOT_FOUND 3L
#define ERROR_ACCESS_DENIED  5L
#define ERROR_MR_MID_NOT_FOUND 317L

// File operations
#define MOVEFILE_REPLACE_EXISTING 1

// Process startup
#define STARTF_USESTDHANDLES 0x00000100

// Standard handles
#define STD_INPUT_HANDLE  ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE  ((DWORD)-12)

// FormatMessage flags
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200

// Language
#define LANG_USER_DEFAULT 0x0400

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

#endif // CEX_PLATFORM_WIN32_H
