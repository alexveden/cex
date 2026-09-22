#!/usr/bin/env bash
# Traceback backend experiment matrix.
#
# Exploratory only: crash programs are expected to fail, so their exit codes are
# recorded but never propagated. The script always exits 0.
#
# Usage: ./run.sh            # full matrix for the current platform
#        CC=clang ./run.sh   # override compiler
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/build/traceback_out"
mkdir -p "$OUT"

OS="$(uname -s)"
case "$OS" in
    Linux*) PLATFORM=linux ;;
    Darwin*) PLATFORM=macos ;;
    MINGW* | MSYS* | CYGWIN*) PLATFORM=windows ;;
    *) PLATFORM=unknown ;;
esac

CC="${CC:-cc}"
if [ "$PLATFORM" = "windows" ]; then
    EXE=".exe"
else
    EXE=""
fi

RDYNAMIC=""
LDLIB=""
NOPIE=""
DBGHELP=""
if [ "$PLATFORM" = "linux" ]; then
    RDYNAMIC="-rdynamic"
    LDLIB="-ldl"
    NOPIE="-no-pie"
fi
if [ "$PLATFORM" = "windows" ]; then
    DBGHELP="-ldbghelp"
fi

LOG="$OUT/_summary.txt"
: > "$LOG"

log() {
    printf '%s\n' "$*" | tee -a "$LOG"
}

# build_run <name> <src> <tag> [flags...]
build_run() {
    local name="$1" src="$2" tag="$3"
    shift 3
    local bin="$OUT/${name}__${tag}${EXE}"
    local build_log="$OUT/${name}__${tag}.build.log"
    local run_log="$OUT/${name}__${tag}.log"

    if ! "$CC" "$@" -o "$bin" "$src" 2> "$build_log"; then
        log "BUILD-FAIL  $name [$tag] flags: $*"
        return
    fi
    (cd "$OUT" && "./$(basename "$bin")" > "$run_log" 2>&1; echo "exit=$?" >> "$run_log") 2>> "$run_log" || true
    log "RAN         $name [$tag] flags: $*"
}

# build_run_modes <name> <src> <tag> <modes> [flags...]
build_run_modes() {
    local name="$1" src="$2" tag="$3" modes="$4"
    shift 4
    local bin="$OUT/${name}__${tag}${EXE}"
    local build_log="$OUT/${name}__${tag}.build.log"

    if ! "$CC" "$@" -o "$bin" "$src" 2> "$build_log"; then
        log "BUILD-FAIL  $name [$tag] flags: $*"
        return
    fi
    for mode in $modes; do
        local run_log="$OUT/${name}__${tag}__mode${mode}.log"
        (cd "$OUT" && "./$(basename "$bin")" "$mode" > "$run_log" 2>&1; echo "exit=$?" >> "$run_log") 2>> "$run_log" || true
        log "RAN         $name [$tag] mode=$mode flags: $*"
    done
}

# resolve_raw <tag>: symbolize tb_unwind_raw output with addr2line (Linux only)
resolve_raw() {
    local tag="$1"
    local bin="$OUT/tb_unwind_raw__${tag}${EXE}"
    local raw_log="$OUT/tb_unwind_raw__${tag}.log"
    local out_log="$OUT/tb_unwind_raw__${tag}.addr2line.log"

    [ "$PLATFORM" = "linux" ] || return
    [ -f "$raw_log" ] || return
    command -v addr2line > /dev/null 2>&1 || return

    local addrs
    addrs="$(grep -oE '0x[0-9a-fA-F]+' "$raw_log" | sort -u)"
    [ -n "$addrs" ] || return
    printf '%s\n' "$addrs" | addr2line -f -C -e "$bin" > "$out_log" 2>&1 || true
    log "SYMBOLIZED  tb_unwind_raw [$tag] -> $(basename "$out_log")"
}

log "platform=$PLATFORM compiler=$CC os=$OS"

# --- execinfo (POSIX) ---
build_run tb_execinfo tb_execinfo.c "O0_g" -O0 -g
build_run tb_execinfo tb_execinfo.c "O2_g" -O2 -g
build_run tb_execinfo tb_execinfo.c "O0_g_rdyn" -O0 -g $RDYNAMIC
build_run tb_execinfo tb_execinfo.c "O2_g_rdyn" -O2 -g $RDYNAMIC

# --- _Unwind_Backtrace + dladdr ---
build_run tb_unwind tb_unwind.c "O0_g_rdyn" -O0 -g $RDYNAMIC $LDLIB
build_run tb_unwind tb_unwind.c "O2_g_rdyn" -O2 -g $RDYNAMIC $LDLIB

# --- _Unwind_Backtrace raw + offline addr2line ---
build_run tb_unwind_raw tb_unwind_raw.c "O0_g" -O0 -g $NOPIE
build_run tb_unwind_raw tb_unwind_raw.c "O2_g" -O2 -g $NOPIE
resolve_raw "O0_g"
resolve_raw "O2_g"

# --- ASAN ---
build_run tb_asan tb_asan.c "O1_g_asan" -O1 -g -fsanitize=address

# --- __builtin_return_address, frame pointers on/off ---
build_run tb_retaddr tb_retaddr.c "O0_g" -O0 -g
build_run tb_retaddr tb_retaddr.c "O2_g" -O2 -g
build_run tb_retaddr tb_retaddr.c "O2_g_fp" -O2 -g -fno-omit-frame-pointer
build_run tb_retaddr tb_retaddr.c "O2_g_nofp" -O2 -g -fomit-frame-pointer

# --- crash signal handlers ---
build_run_modes tb_signals tb_signals.c "O0_g" "0 1 2 3" -O0 -g
build_run_modes tb_signals tb_signals.c "O2_g" "0 1 2 3" -O2 -g

# --- Windows CaptureStackBackTrace ---
if [ "$PLATFORM" = "windows" ]; then
    build_run_modes tb_win32 tb_win32.c "O0_g" "p c" -O0 -g
    build_run_modes tb_win32 tb_win32.c "O0_g_dbghelp" "p c" -O0 -g -DCEX_TB_DBGHELP $DBGHELP
else
    build_run_modes tb_win32 tb_win32.c "O0_g" "p" -O0 -g
fi

log ""
log "done: logs in $OUT"

exit 0
