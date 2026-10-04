#if !defined(cex$enable_minimal) || defined(cex$enable_os)

/*
   The latest version of this library is available on GitHub;
   https://github.com/sheredom/subprocess.h
*/

/*
   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/>
*/
#pragma once

#ifndef CEX_SUBPROCESS_H_INCLUDED
#define CEX_SUBPROCESS_H_INCLUDED
#include "cex_header.h"

#if defined(_MSC_VER)
#pragma warning(push, 1)

/* disable warning: '__cplusplus' is not defined as a preprocessor macro,
 * replacing with '0' for '#if/#elif' */
#pragma warning(disable : 4668)
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(__TINYC__)
#define CEX_SUBPROCESS_ATTRIBUTE(a) __attribute((a))
#else
#define CEX_SUBPROCESS_ATTRIBUTE(a) __attribute__((a))
#endif

#if defined(_MSC_VER)
#define _cex_subprocess_pure
#define _cex_subprocess_weak __inline
#define _cex_subprocess_tls __declspec(thread)
#elif defined(__MINGW32__)
#define _cex_subprocess_pure CEX_SUBPROCESS_ATTRIBUTE(pure)
#define _cex_subprocess_weak static CEX_SUBPROCESS_ATTRIBUTE(used)
#define _cex_subprocess_tls __thread
#elif defined(__clang__) || defined(__GNUC__) || defined(__TINYC__)
#define _cex_subprocess_pure CEX_SUBPROCESS_ATTRIBUTE(pure)
// #define _cex_subprocess_weak CEX_SUBPROCESS_ATTRIBUTE(weak)
#define _cex_subprocess_weak
#define _cex_subprocess_tls __thread
#else
#error Non clang, non gcc, non MSVC compiler found!
#endif


enum _cex_subprocess_option_e {
  // stdout and stderr are the same FILE.
  _cex_subprocess_option_combined_stdout_stderr = 0x1,

  // The child process should inherit the environment variables of the parent.
  _cex_subprocess_option_inherit_environment = 0x2,

  // Enable asynchronous reading of stdout/stderr before it has completed.
  _cex_subprocess_option_enable_async = 0x4,

  // Enable the child process to be spawned with no window visible if supported
  // by the platform.
  _cex_subprocess_option_no_window = 0x8,

  // Search for program names in the PATH variable. Always enabled on Windows.
  // Note: this will **not** search for paths in any provided custom environment
  // and instead uses the PATH of the spawning process.
  _cex_subprocess_option_search_user_path = 0x10
};

struct _cex_subprocess_s;

/// @brief Create a process.
/// @param command_line An array of strings for the command line to execute for
/// this process. The last element must be NULL to signify the end of the array.
/// The memory backing this parameter only needs to persist until this function
/// returns.
/// @param options A bit field of _cex_subprocess_option_e's to pass.
/// @param out_process The newly created process.
/// @return On success zero is returned.
_cex_subprocess_weak int _cex_subprocess_create(const char *const command_line[],
                                      int options,
                                      struct _cex_subprocess_s *const out_process);

/// @brief Create a process (extended create).
/// @param command_line An array of strings for the command line to execute for
/// this process. The last element must be NULL to signify the end of the array.
/// The memory backing this parameter only needs to persist until this function
/// returns.
/// @param options A bit field of _cex_subprocess_option_e's to pass.
/// @param environment An optional array of strings for the environment to use
/// for a child process (each element of the form FOO=BAR). The last element
/// must be NULL to signify the end of the array.
/// @param out_process The newly created process.
/// @return On success zero is returned.
///
/// If `options` contains `_cex_subprocess_option_inherit_environment`, then
/// `environment` must be NULL.
_cex_subprocess_weak int
_cex_subprocess_create_ex(const char *const command_line[], int options,
                     const char *const environment[],
                     struct _cex_subprocess_s *const out_process);

/// @brief Wait for a process to finish execution.
/// @param process The process to wait for.
/// @param out_return_code The return code of the returned process (can be
/// NULL).
/// @return On success zero is returned.
///
/// Joining a process will close the stdin pipe to the process.
_cex_subprocess_weak int _cex_subprocess_join(struct _cex_subprocess_s *const process,
                                    int *const out_return_code);

/// @brief Destroy a previously created process.
/// @param process The process to destroy.
/// @return On success zero is returned.
///
/// If the process to be destroyed had not finished execution, it may out live
/// the parent process.
_cex_subprocess_weak int _cex_subprocess_destroy(struct _cex_subprocess_s *const process);

/// @brief Terminate a previously created process.
/// @param process The process to terminate.
/// @return On success zero is returned.
///
/// If the process to be destroyed had not finished execution, it will be
/// terminated (i.e killed).
_cex_subprocess_weak int _cex_subprocess_terminate(struct _cex_subprocess_s *const process);

/// @brief Returns if the subprocess is currently still alive and executing.
/// @param process The process to check.
/// @return If the process is still alive non-zero is returned.
_cex_subprocess_weak int _cex_subprocess_alive(struct _cex_subprocess_s *const process);

#if defined(__cplusplus)
#define CEX_SUBPROCESS_CAST(type, x) static_cast<type>(x)
#define CEX_SUBPROCESS_PTR_CAST(type, x) reinterpret_cast<type>(x)
#define CEX_SUBPROCESS_CONST_CAST(type, x) const_cast<type>(x)
#define CEX_SUBPROCESS_NULL NULL
#else
#define CEX_SUBPROCESS_CAST(type, x) ((type)(x))
#define CEX_SUBPROCESS_PTR_CAST(type, x) ((type)(x))
#define CEX_SUBPROCESS_CONST_CAST(type, x) ((type)(x))
#define CEX_SUBPROCESS_NULL 0
#endif

#if !defined(_WIN32)
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(_WIN32)

#include "platform_win32.c"

#if (_MSC_VER < 1920)
#ifdef _WIN64
typedef __int64 _cex_subprocess_intptr_t;
typedef unsigned __int64 _cex_subprocess_size_t;
#else
typedef int _cex_subprocess_intptr_t;
typedef unsigned int _cex_subprocess_size_t;
#endif
#else
#include <inttypes.h>

typedef intptr_t _cex_subprocess_intptr_t;
typedef size_t _cex_subprocess_size_t;
#endif

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#ifdef __MINGW32__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

struct _cex_subprocess_subprocess_information_s {
  void *hProcess;
  void *hThread;
  unsigned long dwProcessId;
  unsigned long dwThreadId;
};

struct _cex_subprocess_security_attributes_s {
  unsigned long nLength;
  void *lpSecurityDescriptor;
  int bInheritHandle;
};

struct _cex_subprocess_startup_info_s {
  unsigned long cb;
  char *lpReserved;
  char *lpDesktop;
  char *lpTitle;
  unsigned long dwX;
  unsigned long dwY;
  unsigned long dwXSize;
  unsigned long dwYSize;
  unsigned long dwXCountChars;
  unsigned long dwYCountChars;
  unsigned long dwFillAttribute;
  unsigned long dwFlags;
  unsigned short wShowWindow;
  unsigned short cbReserved2;
  unsigned char *lpReserved2;
  void *hStdInput;
  void *hStdOutput;
  void *hStdError;
};

struct _cex_subprocess_overlapped_s {
  uintptr_t Internal;
  uintptr_t InternalHigh;
  union {
    struct {
      unsigned long Offset;
      unsigned long OffsetHigh;
    } DUMMYSTRUCTNAME;
    void *Pointer;
  } DUMMYUNIONNAME;

  void *hEvent;
};

#ifdef __MINGW32__
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if defined(_DLL)
#define CEX_SUBPROCESS_DLLIMPORT __declspec(dllimport)
#else
#define CEX_SUBPROCESS_DLLIMPORT
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif

CEX_SUBPROCESS_DLLIMPORT int __cdecl _fileno(FILE *);
CEX_SUBPROCESS_DLLIMPORT int __cdecl _open_osfhandle(_cex_subprocess_intptr_t, int);
CEX_SUBPROCESS_DLLIMPORT _cex_subprocess_intptr_t __cdecl _get_osfhandle(int);

#ifndef __MINGW32__
void *__cdecl _alloca(_cex_subprocess_size_t);
#else
#include <malloc.h>
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#else
typedef size_t _cex_subprocess_size_t;
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
struct _cex_subprocess_s {
  FILE *stdin_file;
  FILE *stdout_file;
  FILE *stderr_file;

#if defined(_WIN32)
  void *hProcess;
  void *hStdInput;
  void *hEventOutput;
  void *hEventError;
#else
  pid_t child;
  int return_status;
#endif

  _cex_subprocess_size_t alive;
};
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#undef CEX_SUBPROCESS_DLLIMPORT
#undef _cex_subprocess_pure

#endif /* CEX_SUBPROCESS_H_INCLUDED */

#endif
