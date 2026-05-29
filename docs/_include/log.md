

Simple console logging with file:line location prefix.

`log$error` / `log$warn` / `log$info` / `log$debug` / `log$trace`

- Output format: `[INFO]    ( file.c:14 func() ) message`
- Supports CEX formatting engine
- Compile-time level control via `#define CEX_LOG_LVL <level>`

Log levels:

- 0 — mute all (including asserts, tracebacks, errors)
- 1 — `log$error` + asserts + tracebacks
- 2 — `log$warn`
- 3 — `log$info`
- 4 — `log$debug` (default)
- 5 — `log$trace`

Example:
```c
#define CEX_LOG_LVL 3
int main(void)
{
    log$info("Hello from CEX\n");
    return 0;
}
```



```c
/// Log debug (when CEX_LOG_LVL > 3)
#define log$debug(format, ...)

/// Log error (when CEX_LOG_LVL > 0)
#define log$error(format, ...)

/// Log info  (when CEX_LOG_LVL > 2)
#define log$info(format, ...)

/// Log trace (when CEX_LOG_LVL > 4)
#define log$trace(format, ...)

/// Log warning  (when CEX_LOG_LVL > 1)
#define log$warn(format, ...)




```
