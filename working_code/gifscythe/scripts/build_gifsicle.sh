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
  # Windows config recipe (mirrors upstream src/Makefile.mingw):
  #   * -include src/win32cfg.h defines GIFSICLE_CONFIG_H FIRST, providing
  #     SIZEOF_UNSIGNED_LONG=4 (Win64 is LLP64!), PATHNAME_SEPARATOR='\\', etc.
  #   * Several 1.96 sources (support.c, kcolor.c, gifsicle.c, merge.c,
  #     optimize.c, quantize.c, xform.c) do an UNCONDITIONAL #include
  #     <config.h>, so -I. MUST be on the search path or the compile dies
  #     with "config.h: No such file or directory" (this was the CI Windows
  #     failure, run #18 step 4). The root config.h shares the
  #     GIFSICLE_CONFIG_H guard, so its Linux values are skipped and
  #     win32cfg.h wins. Do not remove -I. and do not put config.h ahead
  #     of win32cfg.h in the include order.
  #   * -DHAVE_UINTPTR_T -DHAVE_INTTYPES_H -DHAVE_CONFIG_H=1 follow upstream
  #     Makefile.mingw (MinGW lacks the _MSC_VER guards win32cfg.h uses).
  #   * No -DVERSION here: win32cfg.h already defines VERSION as
  #     "1.96 (Windows)" (upstream Windows identity); passing -DVERSION
  #     only caused a redefinition warning and never took effect.
  if [[ ! -f "$SRC/src/win32cfg.h" ]]; then
    echo "ERROR: missing $SRC/src/win32cfg.h" >&2
    exit 1
  fi
  # shellcheck disable=SC2086
  "$CC" -O2 -DHAVE_CONFIG_H=1 -DHAVE_UINTPTR_T -DHAVE_INTTYPES_H \
    -include src/win32cfg.h \
    -I. -Iinclude -Isrc \
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
