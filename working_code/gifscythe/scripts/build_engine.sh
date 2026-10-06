#!/usr/bin/env bash
#
# build_engine.sh - Build the gifsicle engine that Gifscythe bundles.
#
# Usage:
#   ./scripts/build_engine.sh            # build the native engine (x86_64 glibc Linux; see NOTE)
#   ./scripts/build_engine.sh --windows  # cross-compile gifsicle.exe (needs mingw-w64)
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

TARGET=native
EXE="gifsicle"
want_help=0
for arg in "$@"; do
  case "$arg" in
    --windows) TARGET=windows; EXE="gifsicle.exe" ;;
    -h|--help) want_help=1 ;;
    *) echo "ERROR: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done
if [[ "$want_help" == 1 ]]; then
  echo "usage: build_engine.sh [--windows]"
  exit 0
fi

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SRC="$REPO_ROOT/reference_code/gifsicle"
OUT="$REPO_ROOT/working_code/gifscythe/release"
CONFIG_SOURCE="$REPO_ROOT/working_code/gifscythe/build_support/gifsicle/config.native.h"

VERSION="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' \
  "$REPO_ROOT/working_code/gifscythe/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
VERSION="${VERSION:-0.1.0}"
ENGINE_VERSION="1.96"

# GS-209 (S37, owner decision): the ADVERTISED native target is exactly one —
# x86_64 glibc Linux. config.native.h is a single fixed config (glibc headers,
# glibc random(), LP64 type sizes, gettimeofday, no per-target feature checks),
# so macOS, 32-bit Linux and musl targets were never actually supported — the
# header merely happened to compile on the x86_64 glibc host (A-09's "attempt the
# advertised native build on macOS/32-bit/non-glibc" observation). Generating the
# config from feature checks per target is the alternative the audit offered and
# the owner rejected in favour of narrowing the promise (OD-17 = a already says
# the shipped product is Windows-only and Linux is the test battery).
#
# The check below makes the narrowing visible instead of leaving it in prose: it
# WARNS (never fails) on any other host, because the build may still work there by
# luck and a hard failure would break sandboxes and other unix CI images for a
# promise this repo no longer makes.
gs_host_arch="$(uname -m 2>/dev/null || echo unknown)"
if [[ "$TARGET" != "windows" ]] && ! (ldd --version 2>/dev/null | head -1 | grep -qi "glibc\|GNU libc"); then
  echo "WARNING: no glibc detected on this host. The advertised native target is x86_64 glibc Linux;" >&2
  echo "         config.native.h is a fixed glibc/x86_64 configuration and this build is UNSUPPORTED here" >&2
  echo "         (it may still work by luck — that is exactly the assumption GS-209 narrowed away)." >&2
elif [[ "$TARGET" != "windows" && "$gs_host_arch" != "x86_64" ]]; then
  echo "WARNING: host arch is '$gs_host_arch', not x86_64. The advertised native target is x86_64 glibc Linux;" >&2
  echo "         config.native.h fixes LP64 sizes and glibc behaviour, so this build is UNSUPPORTED here (GS-209)." >&2
fi

echo "==> Building gifsicle engine (target: $TARGET, product: $VERSION, engine: $ENGINE_VERSION)"

if [[ ! -d "$SRC" ]]; then
  echo "ERROR: gifsicle source not found at $SRC" >&2
  exit 1
fi
if [[ ! -f "$CONFIG_SOURCE" ]]; then
  echo "ERROR: missing product build config at $CONFIG_SOURCE" >&2
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

# Keep the upstream snapshot immutable: sources include <config.h>, but that
# product-owned header is staged into a temporary include directory rather
# than written into reference_code/gifsicle. Windows still pre-includes the
# upstream win32cfg.h, whose GIFSICLE_CONFIG_H guard wins over this fallback.
CONFIG_STAGE="$(mktemp -d "${TMPDIR:-/tmp}/gifscythe-engine-config.XXXXXX")"
trap 'rm -rf "$CONFIG_STAGE"' EXIT
cp "$CONFIG_SOURCE" "$CONFIG_STAGE/config.h"

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
  #     <config.h>, so the staged product config must be on the search path.
  #     It shares the GIFSICLE_CONFIG_H guard, so its native values are skipped
  #     after win32cfg.h wins. Do not put the staged config ahead of
  #     win32cfg.h in the pre-include order.
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
    -I"$CONFIG_STAGE" -Iinclude -Isrc \
    $SRCS -o "$OUT/$VERSION/$EXE" || {
      echo "ERROR: Windows engine compile failed" >&2
      exit 1
    }
else
  # shellcheck disable=SC2086
  "$CC" -O2 -DHAVE_CONFIG_H -DVERSION=\"$ENGINE_VERSION\" \
    -I"$CONFIG_STAGE" -Iinclude -Isrc \
    $SRCS -lm -lpthread -o "$OUT/$VERSION/$EXE"
fi

echo "==> Done. Output: $OUT/$VERSION/$EXE"
if [[ "$TARGET" == "native" ]]; then
  "$OUT/$VERSION/$EXE" --version 2>&1 | head -1 || true
fi
