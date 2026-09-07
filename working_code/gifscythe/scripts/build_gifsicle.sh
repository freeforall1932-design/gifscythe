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
# Engine VERSION is always "1.96" (upstream identity). The product version is
# used only for the release directory name.
#
# Windows builds MUST use win32cfg.h values (SIZEOF_UNSIGNED_LONG=4), never the
# Linux config.h — gifsicle main() static_asserts sizeof(unsigned long).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SRC="$REPO_ROOT/reference_code/gifsicle"
OUT="$REPO_ROOT/working_code/gifscythe/release"

VERSION="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' \
  "$REPO_ROOT/working_code/gifscythe/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
VERSION="${VERSION:-0.1.0}"
ENGINE_VERSION="1.96"

TARGET=native
EXE="gifsicle"
if [[ "${1:-}" == "--windows" ]]; then
  TARGET=windows
  EXE="gifsicle.exe"
fi

echo "==> Building gifsicle engine (target: $TARGET, product: $VERSION, engine: $ENGINE_VERSION)"

if [[ ! -d "$SRC" ]]; then
  echo "ERROR: gifsicle source not found at $SRC" >&2
  exit 1
fi
if [[ ! -f "$SRC/config.h" ]]; then
  echo "ERROR: missing $SRC/config.h (hand-written Linux config; required for native builds)." >&2
  exit 1
fi

if [[ "$TARGET" == "windows" ]]; then
  if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    CC=x86_64-w64-mingw32-gcc
  elif command -v i686-w64-mingw32-gcc >/dev/null 2>&1; then
    CC=i686-w64-mingw32-gcc
  else
    echo "ERROR: mingw-w64 cross compiler not found. Install mingw-w64 or build natively." >&2
    exit 1
  fi
else
  CC="${CC:-$(command -v gcc || command -v cc)}"
fi
echo "==> Using compiler: $CC"
"$CC" --version | head -1

# Object list matches upstream gifsicle_SOURCES (+ gifwrite; no giftoc/gifview/gifdiff).
OBJS="gifsicle gifread gifwrite giffunc gifunopt optimize merge quantize support xform kcolor clp fmalloc"

cd "$SRC"
mkdir -p "$OUT/$VERSION"

SRCS=""
for o in $OBJS; do
  SRCS="$SRCS src/$o.c"
done

if [[ "$TARGET" == "windows" ]]; then
  # Force Windows config via -include win32cfg.h and DO NOT define HAVE_CONFIG_H
  # (which would pull in the Linux config.h through gifsicle's includes).
  # win32cfg.h provides SIZEOF_UNSIGNED_LONG=4, PATHNAME_SEPARATOR='\\', RANDOM=rand.
  if [[ ! -f "$SRC/src/win32cfg.h" ]]; then
    echo "ERROR: missing $SRC/src/win32cfg.h" >&2
    exit 1
  fi
  # shellcheck disable=SC2086
  "$CC" -O2 -DVERSION=\"$ENGINE_VERSION\" \
    -include src/win32cfg.h \
    -Iinclude -Isrc \
    $SRCS -o "$OUT/$VERSION/$EXE" || {
      echo "ERROR: Windows engine compile failed" >&2
      exit 1
    }
else
  # shellcheck disable=SC2086
  "$CC" -O2 -DHAVE_CONFIG_H -DVERSION=\"$ENGINE_VERSION\" \
    -I. -Iinclude -Isrc \
    $SRCS -lm -lpthread -o "$OUT/$VERSION/$EXE"
fi

echo "==> Done. Output: $OUT/$VERSION/$EXE"
if [[ "$TARGET" == "native" ]]; then
  "$OUT/$VERSION/$EXE" --version 2>&1 | head -1 || true
fi
