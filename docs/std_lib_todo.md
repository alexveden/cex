# CEX `os` namespace — cross-stdlib gap analysis

Comparison target: Python (`os`/`os.path`/`shutil`/`subprocess`/`signal`/`tempfile`),
Go (`os`/`os/exec`/`path/filepath`/`syscall`), Rust (`std::fs`/`std::process`/`std::env`/`std::path`).

## Current scope (44 public functions)

| Sub-ns | Count | Functions |
|---|---|---|
| *(top)* | 5 | `cpu_count`, `get_last_error`, `hash`, `sleep`, `timer` |
| `cmd` | 13 | `create`, `exists`, `fstderr`, `fstdin`, `fstdout`, `is_alive`, `kill`, `read_all`, `read_line`, `ret_code`, `run`, `wait`, `write_line` |
| `env` | 2 | `get`, `set` |
| `fs` | 12 | `chdir`, `copy`, `copy_tree`, `dir_walk`, `find`, `getcwd`, `mkdir`, `mkpath`, `remove`, `remove_tree`, `rename`, `stat` |
| `path` | 6 | `abs`, `basename`, `dirname`, `exists`, `join`, `split` |
| `platform` | 6 | `arch_from_str`, `arch_to_str`, `current`, `current_str`, `from_str`, `to_str` |

---

## Tier 1 — Essential (ubiquitous in all three, high-utility)

| Function | Python | Go | Rust | Proposed home |
|---|---|---|---|---|
| `getpid()` | `os.getpid()` | `os.Getpid()` | `std::process::id()` | top-level `os.pid()` |
| `getppid()` | `os.getppid()` | `os.Getppid()` | *(Unix-only)* | top-level `os.parent_pid()` |
| `hostname()` | `socket.gethostname()` | `os.Hostname()` | *(extern)* | top-level `os.hostname()` |
| `current_exe()` | `sys.executable` | `os.Executable()` | `std::env::current_exe()` | top-level `os.current_exe()` |
| `exit(code)` | `sys.exit(n)` | `os.Exit(n)` | `std::process::exit()` | top-level `os.exit(i32)` |
| `env.unset()` | `del os.environ[k]` | `os.Unsetenv()` | `std::env::remove_var()` | `os.env.unset()` |
| `env.list()` | `os.environ` dict | `os.Environ()` | `std::env::vars()` | `os.env.list()` |
| `path.isabs()` | `os.path.isabs()` | `filepath.IsAbs()` | `Path::is_absolute()` | `os.path.is_abs()` |
| `path.splitext()` | `os.path.splitext()` | `filepath.Ext()` | `Path::extension()` | `os.path.splitext()` |
| `path.normpath()` | `os.path.normpath()` | `filepath.Clean()` | *(via canonicalize)* | `os.path.normpath()` |

---

## Tier 2 — Important (present in majority, frequent use)

| Function | Python | Go | Rust | Proposed home |
|---|---|---|---|---|
| `fs.chmod()` | `os.chmod()` | `os.Chmod()` | `std::fs::set_permissions()` | `os.fs.chmod()` |
| `fs.symlink()` | `os.symlink()` | `os.Symlink()` | `std::os::unix::fs::symlink()` | `os.fs.symlink()` |
| `fs.readlink()` | `os.readlink()` | `os.Readlink()` | `std::fs::read_link()` | `os.fs.readlink()` |
| `fs.link()` | `os.link()` | `os.Link()` | *(Unix-only)* | `os.fs.link()` |
| `fs.truncate()` | `os.truncate()` | `os.Truncate()` | `File::set_len()` | `os.fs.truncate()` |
| `fs.temp_dir()` | `tempfile.gettempdir()` | `os.TempDir()` | `std::env::temp_dir()` | `os.fs.temp_dir()` |
| `fs.disk_usage()` | `shutil.disk_usage()` | `syscall.Statfs()` | *(crate)* | `os.fs.disk_usage()` |
| `fs.access()` | `os.access()` | *(use stat)* | `std::fs::metadata()` | `os.fs.access()` |
| `path.expanduser()` | `os.path.expanduser()` | `os.UserHomeDir()` | *(dirs crate)* | `os.path.expanduser()` |
| `path.isdir()` / `isfile()` | `os.path.isdir()` | *(use stat)* | `Path::is_dir()` | `os.path.is_dir()` / `.is_file()` |
| `path.relpath()` | `os.path.relpath()` | `filepath.Rel()` | — | `os.path.relpath()` |
| `path.commonpath()` | `os.path.commonpath()` | — | — | `os.path.commonpath()` |
| `getuid()/getgid()` | `os.getuid()` | `os.Getuid()` | `std::os::unix::Uid` | `os.user.uid()` / `.gid()` |
| `getlogin()` | `os.getlogin()` | `os.UserHomeDir()` | — | `os.user.login()` |
| `get_home_dir()` | `pathlib.Path.home()` | `os.UserHomeDir()` | *(dirs crate)* | `os.user.home_dir()` |
| `umask()` | `os.umask()` | `os.Umask()` | — | `os.fs.umask()` |
| `pipe()` | `os.pipe()` | `os.Pipe()` | *(Unix-only)* | `os.fs.pipe()` |
| `signal.kill(pid,sig)` | `os.kill()` | `Process.Signal()` | *(Unix kill)* | `os.signal.kill()` |
| `terminal_size()` | `shutil.get_terminal_size()` | *(term pkg)* | — | `os.terminal_size()` |

---

## Tier 3 — Nice-to-have (less common, still standard in ≥2)

| Function | Python | Go | Rust | Proposed home |
|---|---|---|---|---|
| `fs.utime()` | `os.utime()` | `os.Chtimes()` | `File::set_modified()` | `os.fs.set_times()` |
| `fs.chown()` | `os.chown()` | `os.Chown()` | `std::os::unix::fs::chown()` | `os.fs.chown()` |
| `fs.mkstemp()` | `tempfile.mkstemp()` | `os.CreateTemp()` | `tempfile` crate | `os.fs.temp_file()` |
| `fs.mkdtemp()` | `tempfile.mkdtemp()` | `os.MkdirTemp()` | `tempfile` crate | `os.fs.temp_dir_create()` |
| `fs.link_count()` | *(via stat)* | `syscall.Stat_t.Nlink` | `Metadata.nlink()` | *(os.fs.stat has size)* |
| `getgroups()` | `os.getgroups()` | `os.Getgroups()` | — | `os.user.groups()` |
| `getloadavg()` | `os.getloadavg()` | — | — | `os.loadavg()` |
| `uname()` | `os.uname()` | — | — | `os.platform.uname()` |
| `uptime()` | — | — | — | `os.uptime()` |
| `env.expand()` | `os.path.expandvars()` | `os.ExpandEnv()` | — | `os.env.expand()` |

---

## Already covered elsewhere in CEX

| Capability | Location |
|---|---|
| `isatty()` | `io.isatty()` |
| CSPRNG / random bytes | *(none — `os.hash()` is SipHash, not suitable)* |
| File read/write/seek | `io.file.*` / `io.*` |
| Monotonic time / sleep | `os.timer()` / `os.sleep()` |
| Process creation + I/O | `os.cmd.*` (13 functions) |

---

## Suggested implementation priority by namespace

1. **`os.path`** (6 → 12) — `normpath`, `is_abs`, `splitext`, `is_dir`, `is_file`, `expanduser`, `relpath` — minimal surface, pure string logic, well-understood semantics, easy to test.
2. **`os.env`** (2 → 4) — `unset`, `list` — small, high symmetry value with existing `get`/`set`.
3. **Top-level `os`** (5 → 10) — `pid`, `hostname`, `current_exe`, `exit` — basic system identity.
4. **`os.fs`** (12 → 20+) — `symlink`, `readlink`, `chmod`, `truncate`, `temp_dir`, `access` — core filesystem ops.
5. **`os.signal`** (new) — `kill` — cross-platform process signalling.
6. **`os.user`** (new) — `uid`, `gid`, `login`, `home_dir` — user identity.
