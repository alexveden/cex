// Freestanding os, expect POSIX-kind of API. Anyway this is a list of used API functions

#include <dirent.h>
#include <spawn.h>
#include <stddef.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// NOTE: used in _subprocess.h, I don't know WHY!
char **environ;

char*
realpath(const char* restrict path, char* restrict resolved_path)
{
    (void)resolved_path;
    return (char*)path;
}

int
lstat(const char* restrict pathname, struct stat* restrict statbuf)
{
    (void)pathname;
    (void)statbuf;
    return -1;
}

int
stat(const char* restrict pathname, struct stat* restrict statbuf)
{
    (void)pathname;
    (void)statbuf;
    return -1;
}

int
fstat(int fd, struct stat* statbuf)
{
    (void)fd;
    (void)statbuf;
    return -1;
}


int
open(const char* pathname, int flags, ...)
{
    (void)pathname;
    (void)flags;
    return -1;
}

int
close(int fd)
{
    (void)fd;
    return -1;
}

ssize_t
read(int fd, void* buf, size_t count)
{
    return -1;
}

ssize_t
write(int fd, const void* buf, size_t count)
{
    return -1;
}


int
rename(const char* oldpath, const char* newpath)
{
    (void)oldpath;
    (void)newpath;
    return -1;
}


int
remove(const char* pathname)
{
    return -1;
}


int
mkdir(const char* pathname, mode_t mode)
{
    return -1;
}

char*
getcwd(char* buf, size_t size)
{
    return NULL;
}


int
chdir(const char* path)
{
    return -1;
}

int
setenv(const char* name, const char* value, int overwrite)
{
    return -1;
}

char*
getenv(const char* name)
{
    return NULL;
}

[[noreturn]] void
exit(int status)
{
    __builtin_trap();
}

int
execvp(const char* file, char* const argv[])
{
    return -1;
}

pid_t
fork(void)
{
    return 0;
}


int
access(const char* pathname, int mode)
{
    return -1;
}


int
clock_gettime(clockid_t clockid, struct timespec* tp)
{
    tp->tv_sec = 77;
    tp->tv_nsec = 999;
    return 0;
}

int
usleep(useconds_t usec)
{
    return 0;
}

struct dirent*
readdir(DIR* dirp)
{
    return NULL;
}

DIR*
opendir(const char* name)
{
    return NULL;
}

int
closedir(DIR* dirp)
{
    return -1;
}


pid_t
waitpid(pid_t pid, int* wstatus, int options)
{
    return 0;
}

int
fileno(FILE* stream)
{
    return -1;
}


int
kill(pid_t pid, int sig)
{
    return -1;
}

FILE*
fdopen(int fd, const char* mode)
{
    return NULL;
}

long sysconf(int name) {
    return -1;
}


int
posix_spawn(
    pid_t* restrict pid,
    const char* restrict path,
    const posix_spawn_file_actions_t* restrict file_actions,
    const posix_spawnattr_t* restrict attrp,
    char* const argv[restrict],
    char* const envp[restrict]
)
{
    return -1;
}
int
posix_spawnp(
    pid_t* restrict pid,
    const char* restrict file,
    const posix_spawn_file_actions_t* restrict file_actions,
    const posix_spawnattr_t* restrict attrp,
    char* const argv[restrict],
    char* const envp[restrict]
)
{
    return -1;
}

int
pipe(int pipefd[2])
{
    return -1;
}

int
posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* __file_actions)
{
    return -1;
}

int
posix_spawn_file_actions_addopen(
    posix_spawn_file_actions_t* __restrict __file_actions,
    int __fd,
    const char* __restrict __path,
    int __oflag,
    mode_t __mode
)
{
    return -1;
}

int
posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* __file_actions, int __fd)
{
    return -1;
}

int
posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* __file_actions, int __fd, int __newfd)
{
    return -1;
}
int posix_spawn_file_actions_init (posix_spawn_file_actions_t *
					  __file_actions) {
	return -1;
}
