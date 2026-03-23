#!/usr/bin/env bash
# Starts VorpalEngine and writes crash-safe logs.
#
# Stdout is forced to line-buffered mode via stdbuf so each printf is
# flushed to disk immediately — a compositor crash won't lose buffered output.
# Stderr (Vulkan validation layer) is merged into the same stream.
# Output goes to both the terminal and a timestamped file under logs/.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
LOG_DIR="$SCRIPT_DIR/logs"
BINARY="$BUILD_DIR/vorpal_engine"

if [[ ! -x "$BINARY" ]]; then
    echo "error: $BINARY not found or not executable" >&2
    exit 1
fi

mkdir -p "$LOG_DIR"

LOGFILE="$LOG_DIR/vorpal_$(date +%Y%m%d_%H%M%S).log"

echo "logging to $LOGFILE"

cd "$BUILD_DIR"

# -oL  line-buffer stdout  (each printf call flushes before a crash)
# -eL  line-buffer stderr  (captures Vulkan validation layer output)
# 2>&1 merges both streams before tee sees them
stdbuf -oL -eL "$BINARY" 2>&1 | tee "$LOGFILE"
