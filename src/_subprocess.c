#if !defined(cex$enable_minimal) || defined(cex$enable_os)

#        include "platform_win32.c"
#include "_subprocess.h"

#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#endif

#if defined(_WIN32)
_cex_subprocess_weak int _cex_subprocess_create_named_pipe_helper(void **rd, void **wr);
int _cex_subprocess_create_named_pipe_helper(void **rd, void **wr) {
  const unsigned long pipeAccessInbound = 0x00000001;
  const unsigned long fileFlagOverlapped = 0x40000000;
  const unsigned long pipeTypeByte = 0x00000000;
  const unsigned long pipeWait = 0x00000000;
  const unsigned long genericWrite = 0x40000000;
  const unsigned long openExisting = 3;
  const unsigned long fileAttributeNormal = 0x00000080;
  const void *const invalidHandleValue =
      _CEX_SUBPROCESS_PTR_CAST(void *, ~(_CEX_SUBPROCESS_CAST(_cex_subprocess_intptr_t, 0)));
  struct _cex_subprocess_security_attributes_s saAttr = {sizeof(saAttr),
                                                    _CEX_SUBPROCESS_NULL, 1};
  char name[256] = {0};
  static _cex_subprocess_tls long index = 0;
  const long unique = index++;

#if defined(_MSC_VER) && _MSC_VER < 1900
#pragma warning(push, 1)
#pragma warning(disable : 4996)
  _snprintf(name, sizeof(name) - 1,
            "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld",
            GetCurrentProcessId(), GetCurrentThreadId(), unique);
#pragma warning(pop)
#else
  snprintf(name, sizeof(name) - 1,
           "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld",
           GetCurrentProcessId(), GetCurrentThreadId(), unique);
#endif

  *rd =
      CreateNamedPipeA(name, pipeAccessInbound | fileFlagOverlapped,
                       pipeTypeByte | pipeWait, 1, 4096, 4096, _CEX_SUBPROCESS_NULL,
                       _CEX_SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr));

  if (invalidHandleValue == *rd) {
    return -1;
  }

  *wr = CreateFileA(name, genericWrite, _CEX_SUBPROCESS_NULL,
                    _CEX_SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr),
                    openExisting, fileAttributeNormal, _CEX_SUBPROCESS_NULL);

  if (invalidHandleValue == *wr) {
    return -1;
  }

  return 0;
}
#endif

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
static int _cex_subprocess_pipe(int fds[2]) {
  int flags;

  if (0 != pipe(fds)) {
    return -1;
  }

  flags = fcntl(fds[0], F_GETFD);
  if (0 > flags || 0 != fcntl(fds[0], F_SETFD, flags | FD_CLOEXEC)) {
    goto fail;
  }

  flags = fcntl(fds[1], F_GETFD);
  if (0 > flags || 0 != fcntl(fds[1], F_SETFD, flags | FD_CLOEXEC)) {
    goto fail;
  }

  return 0;

fail:
  {
    int saved_errno = errno;
    close(fds[0]);
    close(fds[1]);
    fds[0] = -1;
    fds[1] = -1;
    errno = saved_errno;
  }
  return -1;
}
#endif

int _cex_subprocess_create(const char *const commandLine[], int options,
                      struct _cex_subprocess_s *const out_process) {
  return _cex_subprocess_create_ex(commandLine, options, _CEX_SUBPROCESS_NULL,
                              out_process);
}

int _cex_subprocess_create_ex(const char *const commandLine[], int options,
                         const char *const environment[],
                         struct _cex_subprocess_s *const out_process) {
#if defined(_WIN32)
  int fd;
  void *rd, *wr;
  char *commandLineCombined;
  _cex_subprocess_size_t len;
  int i, j;
  int need_quoting;
  unsigned long flags = 0;
  const unsigned long startFUseStdHandles = 0x00000100;
  const unsigned long handleFlagInherit = 0x00000001;
  const unsigned long createNoWindow = 0x08000000;
  struct _cex_subprocess_subprocess_information_s processInfo;
  struct _cex_subprocess_security_attributes_s saAttr = {sizeof(saAttr),
                                                    _CEX_SUBPROCESS_NULL, 1};
  char *used_environment = _CEX_SUBPROCESS_NULL;
  struct _cex_subprocess_startup_info_s startInfo = {0,
                                                _CEX_SUBPROCESS_NULL,
                                                _CEX_SUBPROCESS_NULL,
                                                _CEX_SUBPROCESS_NULL,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                _CEX_SUBPROCESS_NULL,
                                                _CEX_SUBPROCESS_NULL,
                                                _CEX_SUBPROCESS_NULL,
                                                _CEX_SUBPROCESS_NULL};

  startInfo.cb = sizeof(startInfo);
  startInfo.dwFlags = startFUseStdHandles;

  if (_cex_subprocess_option_no_window == (options & _cex_subprocess_option_no_window)) {
    flags |= createNoWindow;
  }

  if (_cex_subprocess_option_inherit_environment !=
      (options & _cex_subprocess_option_inherit_environment)) {
    if (_CEX_SUBPROCESS_NULL == environment) {
      used_environment = _CEX_SUBPROCESS_CONST_CAST(char *, "\0\0");
    } else {
      // We always end with two null terminators.
      len = 2;

      for (i = 0; environment[i]; i++) {
        for (j = 0; '\0' != environment[i][j]; j++) {
          len++;
        }

        // For the null terminator too.
        len++;
      }

      used_environment = _CEX_SUBPROCESS_CAST(char *, _alloca(len));

      // Re-use len for the insertion position
      len = 0;

      for (i = 0; environment[i]; i++) {
        for (j = 0; '\0' != environment[i][j]; j++) {
          used_environment[len++] = environment[i][j];
        }

        used_environment[len++] = '\0';
      }

      // End with the two null terminators.
      used_environment[len++] = '\0';
      used_environment[len++] = '\0';
    }
  } else {
    if (_CEX_SUBPROCESS_NULL != environment) {
      return -1;
    }
  }

  if (!CreatePipe(&rd, &wr, _CEX_SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr),
                  0)) {
    return -1;
  }

  if (!SetHandleInformation(wr, handleFlagInherit, 0)) {
    return -1;
  }

  fd = _open_osfhandle(_CEX_SUBPROCESS_PTR_CAST(_cex_subprocess_intptr_t, wr), 0);

  if (-1 != fd) {
    out_process->stdin_file = _fdopen(fd, "wb");

    if (_CEX_SUBPROCESS_NULL == out_process->stdin_file) {
      return -1;
    }
  }

  startInfo.hStdInput = rd;

  if (options & _cex_subprocess_option_enable_async) {
    if (_cex_subprocess_create_named_pipe_helper(&rd, &wr)) {
      return -1;
    }
  } else {
    if (!CreatePipe(&rd, &wr,
                    _CEX_SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 0)) {
      return -1;
    }
  }

  if (!SetHandleInformation(rd, handleFlagInherit, 0)) {
    return -1;
  }

  fd = _open_osfhandle(_CEX_SUBPROCESS_PTR_CAST(_cex_subprocess_intptr_t, rd), 0);

  if (-1 != fd) {
    out_process->stdout_file = _fdopen(fd, "rb");

    if (_CEX_SUBPROCESS_NULL == out_process->stdout_file) {
      return -1;
    }
  }

  startInfo.hStdOutput = wr;

  if (_cex_subprocess_option_combined_stdout_stderr ==
      (options & _cex_subprocess_option_combined_stdout_stderr)) {
    out_process->stderr_file = out_process->stdout_file;
    startInfo.hStdError = startInfo.hStdOutput;
  } else {
    if (options & _cex_subprocess_option_enable_async) {
      if (_cex_subprocess_create_named_pipe_helper(&rd, &wr)) {
        return -1;
      }
    } else {
      if (!CreatePipe(&rd, &wr,
                      _CEX_SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 0)) {
        return -1;
      }
    }

    if (!SetHandleInformation(rd, handleFlagInherit, 0)) {
      return -1;
    }

    fd = _open_osfhandle(_CEX_SUBPROCESS_PTR_CAST(_cex_subprocess_intptr_t, rd), 0);

    if (-1 != fd) {
      out_process->stderr_file = _fdopen(fd, "rb");

      if (_CEX_SUBPROCESS_NULL == out_process->stderr_file) {
        return -1;
      }
    }

    startInfo.hStdError = wr;
  }

  if (options & _cex_subprocess_option_enable_async) {
    out_process->hEventOutput =
        CreateEventA(_CEX_SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 1, 1,
                     _CEX_SUBPROCESS_NULL);
    out_process->hEventError =
        CreateEventA(_CEX_SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 1, 1,
                     _CEX_SUBPROCESS_NULL);
  } else {
    out_process->hEventOutput = _CEX_SUBPROCESS_NULL;
    out_process->hEventError = _CEX_SUBPROCESS_NULL;
  }

  // Combine commandLine together into a single string
  len = 0;
  for (i = 0; commandLine[i]; i++) {
    // for the trailing \0
    len++;

    // Quote the argument if it has a space in it
    if (strpbrk(commandLine[i], "\t\v ") != _CEX_SUBPROCESS_NULL ||
        commandLine[i][0] == _CEX_SUBPROCESS_NULL)
      len += 2;

    for (j = 0; '\0' != commandLine[i][j]; j++) {
      switch (commandLine[i][j]) {
      default:
        break;
      case '\\':
        if (commandLine[i][j + 1] == '"') {
          len++;
        }

        break;
      case '"':
        len++;
        break;
      }
      len++;
    }
  }

  commandLineCombined = _CEX_SUBPROCESS_CAST(char *, _alloca(len));

  if (!commandLineCombined) {
    return -1;
  }

  // Gonna re-use len to store the write index into commandLineCombined
  len = 0;

  for (i = 0; commandLine[i]; i++) {
    if (0 != i) {
      commandLineCombined[len++] = ' ';
    }

    need_quoting = strpbrk(commandLine[i], "\t\v ") != _CEX_SUBPROCESS_NULL ||
                   commandLine[i][0] == _CEX_SUBPROCESS_NULL;
    if (need_quoting) {
      commandLineCombined[len++] = '"';
    }

    for (j = 0; '\0' != commandLine[i][j]; j++) {
      switch (commandLine[i][j]) {
      default:
        break;
      case '\\':
        if (commandLine[i][j + 1] == '"') {
          commandLineCombined[len++] = '\\';
        }

        break;
      case '"':
        commandLineCombined[len++] = '\\';
        break;
      }

      commandLineCombined[len++] = commandLine[i][j];
    }
    if (need_quoting) {
      commandLineCombined[len++] = '"';
    }
  }

  commandLineCombined[len] = '\0';

  if (!CreateProcessA(
          _CEX_SUBPROCESS_NULL,
          commandLineCombined, // command line
          _CEX_SUBPROCESS_NULL,     // process security attributes
          _CEX_SUBPROCESS_NULL,     // primary thread security attributes
          1,                   // handles are inherited
          flags,               // creation flags
          used_environment,    // used environment
          _CEX_SUBPROCESS_NULL,     // use parent's current directory
          _CEX_SUBPROCESS_PTR_CAST(LPSTARTUPINFOA,
                              &startInfo), // STARTUPINFO pointer
          _CEX_SUBPROCESS_PTR_CAST(LPPROCESS_INFORMATION, &processInfo))) {
    return -1;
  }

  out_process->hProcess = processInfo.hProcess;

  out_process->hStdInput = startInfo.hStdInput;

  // We don't need the handle of the primary thread in the called process.
  CloseHandle(processInfo.hThread);

  if (_CEX_SUBPROCESS_NULL != startInfo.hStdOutput) {
    CloseHandle(startInfo.hStdOutput);

    if (startInfo.hStdError != startInfo.hStdOutput) {
      CloseHandle(startInfo.hStdError);
    }
  }

  out_process->alive = 1;

  return 0;
#    elif defined(__EMSCRIPTEN__)
  (void)commandLine;
  (void)environment;
  (void)out_process;
  (void)options;
  uassert(false && "This functionality is not supported by WASM" );
  return -1;
#else
  int stdinfd[2] = {-1, -1};
  int stdoutfd[2] = {-1, -1};
  int stderrfd[2] = {-1, -1};
  pid_t child = 0;
  extern char **environ;
  char *const empty_environment[1] = {_CEX_SUBPROCESS_NULL};
  posix_spawn_file_actions_t actions = {0};
  char *const *used_environment;
  int spawn_rc;
  int actions_inited = 0;

  memset(out_process, 0, sizeof(*out_process));

  if (_cex_subprocess_option_inherit_environment ==
      (options & _cex_subprocess_option_inherit_environment)) {
    if (_CEX_SUBPROCESS_NULL != environment) {
      errno = EINVAL;
      return -1;
    }
  }

  if (0 != _cex_subprocess_pipe(stdinfd)) {
    goto fail;
  }

  if (0 != _cex_subprocess_pipe(stdoutfd)) {
    goto fail;
  }

  if (_cex_subprocess_option_combined_stdout_stderr !=
      (options & _cex_subprocess_option_combined_stdout_stderr)) {
    if (0 != _cex_subprocess_pipe(stderrfd)) {
      goto fail;
    }
  }

  if (environment) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
    used_environment = _CEX_SUBPROCESS_CONST_CAST(char *const *, environment);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  } else if (_cex_subprocess_option_inherit_environment ==
             (options & _cex_subprocess_option_inherit_environment)) {
    used_environment = environ;
  } else {
    used_environment = empty_environment;
  }

  spawn_rc = posix_spawn_file_actions_init(&actions);
  if (0 != spawn_rc) {
    errno = spawn_rc;
    goto fail;
  }
  actions_inited = 1;

  // Close the stdin write end
  spawn_rc = posix_spawn_file_actions_addclose(&actions, stdinfd[1]);
  if (0 != spawn_rc) {
    errno = spawn_rc;
    goto fail;
  }

  // Map the read end to stdin
  spawn_rc =
      posix_spawn_file_actions_adddup2(&actions, stdinfd[0], STDIN_FILENO);
  if (0 != spawn_rc) {
    errno = spawn_rc;
    goto fail;
  }

  // Close the stdout read end
  spawn_rc = posix_spawn_file_actions_addclose(&actions, stdoutfd[0]);
  if (0 != spawn_rc) {
    errno = spawn_rc;
    goto fail;
  }

  // Map the write end to stdout
  spawn_rc =
      posix_spawn_file_actions_adddup2(&actions, stdoutfd[1], STDOUT_FILENO);
  if (0 != spawn_rc) {
    errno = spawn_rc;
    goto fail;
  }

  if (_cex_subprocess_option_combined_stdout_stderr ==
      (options & _cex_subprocess_option_combined_stdout_stderr)) {
    spawn_rc = posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO,
                                                STDERR_FILENO);
    if (0 != spawn_rc) {
      errno = spawn_rc;
      goto fail;
    }
  } else {
    // Close the stderr read end
    spawn_rc = posix_spawn_file_actions_addclose(&actions, stderrfd[0]);
    if (0 != spawn_rc) {
      errno = spawn_rc;
      goto fail;
    }
    // Map the write end to stdout
    spawn_rc = posix_spawn_file_actions_adddup2(&actions, stderrfd[1],
                                                STDERR_FILENO);
    if (0 != spawn_rc) {
      errno = spawn_rc;
      goto fail;
    }
  }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
  if (_cex_subprocess_option_search_user_path ==
      (options & _cex_subprocess_option_search_user_path)) {
    spawn_rc =
        posix_spawnp(&child, commandLine[0], &actions, _CEX_SUBPROCESS_NULL,
                     _CEX_SUBPROCESS_CONST_CAST(char *const *, commandLine),
                     used_environment);
  } else {
    spawn_rc =
        posix_spawn(&child, commandLine[0], &actions, _CEX_SUBPROCESS_NULL,
                    _CEX_SUBPROCESS_CONST_CAST(char *const *, commandLine),
                    used_environment);
  }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

  if (0 != spawn_rc) {
    errno = spawn_rc;
    goto fail;
  }

  posix_spawn_file_actions_destroy(&actions);
  actions_inited = 0;

  out_process->child = child;
  out_process->alive = 1;

  // Close the stdin read end
  close(stdinfd[0]);
  stdinfd[0] = -1;
  // Store the stdin write end
  out_process->stdin_file = fdopen(stdinfd[1], "wb");
  if (_CEX_SUBPROCESS_NULL == out_process->stdin_file) {
    close(stdinfd[1]);
    stdinfd[1] = -1;
    goto fail_child;
  }
  stdinfd[1] = -1;

  // Close the stdout write end
  close(stdoutfd[1]);
  stdoutfd[1] = -1;
  // Store the stdout read end
  out_process->stdout_file = fdopen(stdoutfd[0], "rb");
  if (_CEX_SUBPROCESS_NULL == out_process->stdout_file) {
    close(stdoutfd[0]);
    stdoutfd[0] = -1;
    goto fail_child;
  }
  stdoutfd[0] = -1;

  if (_cex_subprocess_option_combined_stdout_stderr ==
      (options & _cex_subprocess_option_combined_stdout_stderr)) {
    out_process->stderr_file = out_process->stdout_file;
  } else {
    // Close the stderr write end
    close(stderrfd[1]);
    stderrfd[1] = -1;
    // Store the stderr read end
    out_process->stderr_file = fdopen(stderrfd[0], "rb");
    if (_CEX_SUBPROCESS_NULL == out_process->stderr_file) {
      close(stderrfd[0]);
      stderrfd[0] = -1;
      goto fail_child;
    }
    stderrfd[0] = -1;
  }

  return 0;

fail_child:
  {
    int saved_errno = errno;
    _cex_subprocess_terminate(out_process);
    _cex_subprocess_join(out_process, _CEX_SUBPROCESS_NULL);
    _cex_subprocess_destroy(out_process);
    errno = saved_errno;
  }

fail:
  {
    int saved_errno = errno;

    if (actions_inited) {
      posix_spawn_file_actions_destroy(&actions);
    }

    if (0 <= stdinfd[0]) {
      close(stdinfd[0]);
    }
    if (0 <= stdinfd[1]) {
      close(stdinfd[1]);
    }
    if (0 <= stdoutfd[0]) {
      close(stdoutfd[0]);
    }
    if (0 <= stdoutfd[1]) {
      close(stdoutfd[1]);
    }
    if (0 <= stderrfd[0]) {
      close(stderrfd[0]);
    }
    if (0 <= stderrfd[1]) {
      close(stderrfd[1]);
    }

    errno = saved_errno;
  }
  return -1;
#endif
}

int _cex_subprocess_join(struct _cex_subprocess_s *const process,
                    int *const out_return_code) {
#if defined(_WIN32)
  const unsigned long infinite = 0xFFFFFFFF;
  int rc = 0;

  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = _CEX_SUBPROCESS_NULL;
  }

  if (process->hStdInput) {
    CloseHandle(process->hStdInput);
    process->hStdInput = _CEX_SUBPROCESS_NULL;
  }

  if (process->hProcess) {
    if (0 != WaitForSingleObject(process->hProcess, infinite)) {
      rc = -1;
    } else if (out_return_code) {
      if (!GetExitCodeProcess(
              process->hProcess,
              _CEX_SUBPROCESS_PTR_CAST(unsigned long *, out_return_code))) {
        rc = -1;
      }
    }
  }

  process->alive = 0;

  return rc;
#else
  int rc = 0;

  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = _CEX_SUBPROCESS_NULL;
  }

  if (0 < process->child) {
    int status = 0;
    pid_t waited;

    do {
      waited = waitpid(process->child, &status, 0);
    } while (-1 == waited && EINTR == errno);

    if (-1 == waited) {
      process->return_status = EXIT_FAILURE;
      rc = -1;
    } else if (WIFEXITED(status)) {
      process->return_status = WEXITSTATUS(status);
    } else {
      process->return_status = EXIT_FAILURE;
    }
  }

  process->child = 0;
  process->alive = 0;

  if (out_return_code && 0 == rc) {
    *out_return_code = process->return_status;
  }

  return rc;
#endif
}

int _cex_subprocess_destroy(struct _cex_subprocess_s *const process) {
  FILE *stdout_file = process->stdout_file;
  FILE *stderr_file = process->stderr_file;

  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = _CEX_SUBPROCESS_NULL;
  }

  process->stdout_file = _CEX_SUBPROCESS_NULL;
  process->stderr_file = _CEX_SUBPROCESS_NULL;

  if (stdout_file) {
    fclose(stdout_file);
  }

  if (stderr_file && stderr_file != stdout_file) {
    fclose(stderr_file);
  }

#if defined(_WIN32)
  if (process->hProcess) {
    CloseHandle(process->hProcess);
    process->hProcess = _CEX_SUBPROCESS_NULL;

    if (process->hStdInput) {
      CloseHandle(process->hStdInput);
    }

    if (process->hEventOutput) {
      CloseHandle(process->hEventOutput);
    }

    if (process->hEventError) {
      CloseHandle(process->hEventError);
    }
  }
#endif

  return 0;
}

int _cex_subprocess_terminate(struct _cex_subprocess_s *const process) {
#if defined(_WIN32)
  if (!process->hProcess) {
    return -1;
  }
  return TerminateProcess(process->hProcess, 99) ? 0 : -1;
#else
  int result;

  if (0 >= process->child) {
    errno = EINVAL;
    return -1;
  }

  result = kill(process->child, 9);
  if (0 != result && ESRCH == errno) {
    return 0;
  }
  return result;
#endif
}

int _cex_subprocess_alive(struct _cex_subprocess_s *const process) {
  int is_alive = _CEX_SUBPROCESS_CAST(int, process->alive);

  if (!is_alive) {
    return 0;
  }
#if defined(_WIN32)
  {
    const unsigned long zero = 0x0;
    const unsigned long wait_timeout = 0x00000102L;

    if (!process->hProcess) {
      process->alive = 0;
      return 0;
    }

    is_alive = wait_timeout == WaitForSingleObject(process->hProcess, zero);
    if (!is_alive) {
      process->alive = 0;
      _cex_subprocess_join(process, _CEX_SUBPROCESS_NULL);
    }
  }
#else
  {
    int status = 0;
    pid_t waited;

    if (0 >= process->child) {
      process->child = 0;
      process->alive = 0;
      return 0;
    }

    do {
      waited = waitpid(process->child, &status, WNOHANG);
    } while (-1 == waited && EINTR == errno);

    if (0 == waited) {
      return 1;
    }

    if (-1 == waited) {
      process->return_status = EXIT_FAILURE;
    } else if (WIFEXITED(status)) {
      process->return_status = WEXITSTATUS(status);
    } else {
      process->return_status = EXIT_FAILURE;
    }

    process->child = 0;
    process->alive = 0;
    _cex_subprocess_join(process, _CEX_SUBPROCESS_NULL);
    is_alive = 0;
  }
#endif

  return is_alive;
}

#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic pop
#endif
#endif

#undef _CEX_SUBPROCESS_ATTRIBUTE
#undef _CEX_SUBPROCESS_CAST
#undef _CEX_SUBPROCESS_CONST_CAST
#undef _CEX_SUBPROCESS_NULL
#undef _CEX_SUBPROCESS_PTR_CAST
#undef _cex_subprocess_weak
#undef _cex_subprocess_tls

#endif
