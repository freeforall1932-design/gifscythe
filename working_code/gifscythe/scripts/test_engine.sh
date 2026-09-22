#!/usr/bin/env bash
#
# test_engine.sh - End-to-end test of the Gifscythe engine pipeline.
#
# Usage: ./scripts/test_engine.sh
# Requires the engine built at release/<version>/gifsicle (native).

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
VERSION="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' \
  "$REPO_ROOT/working_code/gifscythe/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
VERSION="${VERSION:-0.1.0}"

ENGINE="$REPO_ROOT/working_code/gifscythe/release/$VERSION/gifsicle"
if [[ ! -x "$ENGINE" && -x "$ENGINE.exe" ]]; then
  ENGINE="$ENGINE.exe"
fi
SRC_GIF="$REPO_ROOT/reference_code/gifsicle/logo.gif"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if [[ ! -x "$ENGINE" ]]; then
  echo "FAIL: engine not built at $ENGINE. Run ./scripts/build_engine.sh first." >&2
  exit 1
fi
if [[ ! -f "$SRC_GIF" ]]; then
  echo "FAIL: sample GIF not found at $SRC_GIF" >&2
  exit 1
fi

PASS=0
FAIL=0
ok()  { echo "  PASS: $1"; PASS=$((PASS + 1)); }
bad() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

echo "==> Testing engine pipeline (using $ENGINE)"

INFO_OUT="$("$ENGINE" --info "$SRC_GIF" 2>&1)"
if echo "$INFO_OUT" | grep -q "12 images"; then ok "info reads multi-frame gif"; else bad "info reads multi-frame gif"; fi

"$ENGINE" "$SRC_GIF" --loopcount=0 -O3 -o "$WORK/opt.gif" 2>/dev/null
if [[ -s "$WORK/opt.gif" ]]; then ok "optimize -O3 writes output"; else bad "optimize -O3 writes output"; fi

"$ENGINE" "$SRC_GIF" -O3 --lossy=80 -o "$WORK/lossy.gif" 2>/dev/null
if [[ -s "$WORK/lossy.gif" ]]; then ok "lossy writes output"; else bad "lossy writes output"; fi

"$ENGINE" "$SRC_GIF" --resize-fit 30x66 -O2 -o "$WORK/r.gif" 2>/dev/null
RSZ="$("$ENGINE" --info "$WORK/r.gif" 2>/dev/null | grep -oE "logical screen [0-9]+x[0-9]+" | head -1)"
if [[ "$RSZ" == *"30x66"* ]]; then ok "resize-fit -> $RSZ"; else bad "resize-fit -> got '$RSZ'"; fi

mkdir -p "$WORK/exp"
"$ENGINE" -e "$SRC_GIF" --output="$WORK/exp/f" 2>/dev/null
# Count frames without ls|wc fragility
N=0
for f in "$WORK/exp"/f.*; do
  [[ -e "$f" ]] || continue
  N=$((N + 1))
done
if [[ "$N" == "12" ]]; then ok "explode extracts $N frames"; else bad "explode extracts frames (got $N)"; fi

echo "==> Done. $PASS passed, $FAIL failed."
[[ "$FAIL" -eq 0 ]]
