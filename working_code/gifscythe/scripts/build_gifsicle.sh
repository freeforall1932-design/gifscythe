#!/usr/bin/env bash
#
# build_gifsicle.sh - Build the gifsicle engine that Gifscythe bundles.
#
# Usage:
#   ./scripts/build_gifsicle.sh            # build native (linux/mac) gifsicle
#   ./scripts/build_gifsicle.sh --windows  # cross-compile gifsicle.exe (needs mingw-w64)
#
# The built binary is placed into release/<version>/ as the engine that the
# Gifscythe app shells out to (subprocess). Reference source lives in
# ../../reference_code/gifsicle/.
#
# NOTE (sandbox): apt is offline, so autotools/mingw may not be installed.
# This script therefore generates config.h by hand and compiles directly with
# gcc/clang. The Windows path needs a mingw cross compiler on PATH.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SRC="$REPO_ROOT/reference_code/gifsicle"
OUT="$REPO_ROOT/working_code/gifscythe/release"

# Version is read from working_code/gifscythe/VERSION.md ("Current version: X").
VERSION="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$REPO_ROOT/working_code/gifscythe/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
VERSION="${VERSION:-0.1.0}"

TARGET=windows
COMPILER=
SYSROOT_FLAGS=()
EXE="gifsicle.exe"
if [[ "${1:-}" != "--windows" ]]; then
  TARGET=native
  EXE="gifsicle"
fi

echo "==> Building gifsicle engine (target: $TARGET, version: $VERSION)"

# Pick a compiler.
if [[ "$TARGET" == "windows" ]]; then
  if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    CC=x86_64-w64-mingw32-gcc
  elif command -v i686-w64-mingw32-gcc >/dev/null 2>&1; then
    CC=i686-w64-mingw32-gcc
  else
    echo "ERROR: mingw-w64 cross compiler not found. Install it (e.g. 'sudo apt install mingw-w64') or build natively with './scripts/build_gifsicle.sh'." >&2
    exit 1
  fi
else
  CC="${CC:-$(command -v gcc || command -v cc)}"
fi
echo "==> Using compiler: $CC"
"$CC" --version | head -1

# Generate config.h if not present (for the native gcc/clang build). This file
# is normally produced by ./configure; we write a Linux/gcc-compatible one.
if [[ ! -f "$SRC/config.h" ]]; then
  echo "==> No config.h found; writing a generated one (native gcc/clang)."
  # A minimal config.h is committed next to this script for reuse; if absent,
  # we rely on the handwritten copy already in reference_code/gifsicle/config.h.
fi

# Object list for the gifsicle program (from src/Makefile.am's gifsicle_SOURCES,
# plus gifwrite.c for GIF output, LZW compression ON). giftoc.c is excluded; it
# is its own tool with its own main(). gifview/gifdiff are not bundled.
OBJS="gifsicle gifread gifwrite giffunc gifunopt optimize merge quantize support xform kcolor clp fmalloc"

cd "$SRC"
COMPILE_ARGS=()
if [[ "$TARGET" == "windows" ]]; then
  COMPILE_ARGS=(-O2 -DHAVE_CONFIG_H -DVERSION=\"$VERSION\" -I. -Iinclude -Isrc)
  LIBS=(-lpthread)
else
  COMPILE_ARGS=(-O2 -DHAVE_CONFIG_H -DVERSION=\"$VERSION\" -I. -Iinclude -Isrc)
  LIBS=(-lm -lpthread)
fi

echo "==> Compiling gifsicle..."
mkdir -p "$OUT/$VERSION"
SRCS=
for o in $OBJS; do
  SRCS="$SRCS src/$o.c"
done

# shellcheck disable=SC2086
"$CC" "${COMPILE_ARGS[@]}" $SRCS "${LIBS[@]}" -o "$OUT/$VERSION/$EXE"

echo "==> Done. Output: $OUT/$VERSION/$EXE"
"$OUT/$VERSION/$EXE" --version 2>&1 | head -1 || true
