#!/usr/bin/env bash
#
# build_wasm.sh - compile the reference gifsicle engine to WebAssembly.
#
# Usage:
#   web/wasm/build_wasm.sh                # needs `emcc` (Emscripten) on PATH
#   web/wasm/build_wasm.sh --out DIR      # default DIR is web/wasm/dist/
#
# Output (all generated, git-ignored): gifsicle.js + gifsicle.wasm (MODULARIZE
# factory `createGifsicle`, main-thread use, no pthreads, MEMFS only) plus the
# staged COPYING.gifsicle engine licence text every build ships.
#
# Reference source lives in reference_code/gifsicle/ and is never modified:
# config.wasm.h is staged as config.h in a temporary include directory.
# Engine VERSION is always "1.96" (upstream identity).
#
set -euo pipefail

SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SELF/../.." && pwd)"
SRC="$REPO_ROOT/reference_code/gifsicle"
CONFIG_SOURCE="$SELF/config.wasm.h"
OUT="$SELF/dist"

want_help=0
for arg in "$@"; do
  case "$arg" in
    -h|--help) want_help=1 ;;
    --out=*) OUT="${arg#--out=}" ;;
    *) echo "ERROR: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done
if [[ "$want_help" == 1 ]]; then
  echo "usage: build_wasm.sh [--out=DIR]"
  exit 0
fi

if ! command -v emcc >/dev/null 2>&1; then
  echo "ERROR: emcc not found - install Emscripten (https://emscripten.org/docs/getting_started/downloads.html) and retry." >&2
  exit 1
fi
if [[ ! -d "$SRC" ]]; then
  echo "ERROR: gifsicle source not found at $SRC" >&2
  exit 1
fi
if [[ ! -f "$CONFIG_SOURCE" ]]; then
  echo "ERROR: missing wasm build config at $CONFIG_SOURCE" >&2
  exit 1
fi
if [[ ! -f "$REPO_ROOT/COPYING.gifsicle" ]]; then
  echo "ERROR: missing engine licence text at $REPO_ROOT/COPYING.gifsicle" >&2
  exit 1
fi

echo "==> Using compiler: $(command -v emcc)"
emcc --version | head -1

CONFIG_STAGE="$(mktemp -d "${TMPDIR:-/tmp}/gifscythe-wasm-config.XXXXXX")"
trap 'rm -rf "$CONFIG_STAGE"' EXIT
cp "$CONFIG_SOURCE" "$CONFIG_STAGE/config.h"

# Object list matches scripts/build_engine.sh (upstream gifsicle_SOURCES
# minus giftoc/gifview/gifdiff): the wasm engine is the same program.
OBJS="gifsicle gifread gifwrite giffunc gifunopt optimize merge quantize support xform kcolor clp fmalloc"

cd "$SRC"
SRCS=""
for o in $OBJS; do
  SRCS="$SRCS src/$o.c"
done

mkdir -p "$OUT"
# MODULARIZE factory for main-thread, synchronous callMain use. No pthreads
# (single-threaded config above), novic filesystem beyond MEMFS, and the
# runtime stays alive across runs (EXIT_RUNTIME=0) so one page session can
# optimize repeatedly without reloading the module.
# shellcheck disable=SC2086
emcc -O3 -DHAVE_CONFIG_H -DVERSION='"1.96"' \
  -I"$CONFIG_STAGE" -Iinclude -Isrc \
  $SRCS -o "$OUT/gifsicle.js" \
  -sWASM=1 \
  -sMODULARIZE=1 \
  -sEXPORT_NAME=createGifsicle \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXIT_RUNTIME=0 \
  -sEXPORTED_RUNTIME_METHODS=FS,callMain || {
    echo "ERROR: Emscripten build failed" >&2
    exit 1
  }

cp "$REPO_ROOT/COPYING.gifsicle" "$OUT/COPYING.gifsicle"

for f in "$OUT/gifsicle.js" "$OUT/gifsicle.wasm" "$OUT/COPYING.gifsicle"; do
  [[ -s "$f" ]] || { echo "ERROR: build incomplete: missing or empty $f" >&2; exit 1; }
done

echo "==> Done. Output:"
ls -l "$OUT/gifsicle.js" "$OUT/gifsicle.wasm" "$OUT/COPYING.gifsicle"
echo "==> Prove it on a real file: node $SELF/prove_wasm.mjs"
