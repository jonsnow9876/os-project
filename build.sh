#!/bin/bash
# ============================================================
#  build.sh — Compile the Green Thread C++ simulator
#  Usage: ./build.sh
# ============================================================

set -e   # exit immediately on error

CPP_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$CPP_DIR/green_thread.cpp"
OUT="$CPP_DIR/green_thread"

echo "🔨  Compiling green_thread.cpp ..."

g++ -std=c++17 \
    -O2 \
    -Wall \
    -o "$OUT" \
    "$SRC"

echo "✅  Build successful → $OUT"
echo "👉  Test it: $OUT 3 4"
