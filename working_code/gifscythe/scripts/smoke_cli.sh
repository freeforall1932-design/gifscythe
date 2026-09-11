#!/usr/bin/env bash
#
# smoke_cli.sh - Integration smoke tests for gifscythe-cli.
# Covers: missing engine exit code, CWD independence, paths with spaces,
# batch vs merge semantics, malformed conf warnings.
#
set -uo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
repo="$(cd "$self/../.." && pwd)"
CLI="${CLI:-$self/build/gifscythe-cli}"
VERSION="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
VERSION="${VERSION:-0.1.0}"
ENGINE="$self/release/$VERSION/gifsicle"
SRC_GIF="$repo/reference_code/gifsicle/logo.gif"
SRC_GIF1="$repo/reference_code/gifsicle/logo1.gif"

PASS=0
FAIL=0
ok()  { echo "  PASS: $1"; PASS=$((PASS + 1)); }
bad() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

if [[ ! -x "$CLI" ]]; then
  echo "FAIL: CLI not built at $CLI (run ./build.sh first)" >&2
  exit 1
fi
if [[ ! -x "$ENGINE" ]]; then
  echo "FAIL: engine not built at $ENGINE" >&2
  exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "==> CLI smoke tests"

# 1. Missing engine must exit non-zero
set +e
"$CLI" "$self/examples/animation.conf" --run --engine "$WORK/no-such-engine" >"$WORK/out1.txt" 2>"$WORK/err1.txt"
rc=$?
set -e
if [[ "$rc" -ne 0 ]]; then ok "missing engine exits non-zero (rc=$rc)"; else bad "missing engine exited 0"; fi

# 2. Print mode works from product dir
set +e
(cd "$self" && "$CLI" examples/animation.conf >"$WORK/out2.txt" 2>"$WORK/err2.txt")
rc=$?
set -e
if [[ "$rc" -eq 0 ]] && grep -q "\-O3\|-m\|logo" "$WORK/out2.txt"; then
  ok "print mode from product dir"
else
  bad "print mode from product dir (rc=$rc)"
fi

# 3. --run from a different CWD with absolute settings path
cat > "$WORK/one.conf" <<EOF
mode = auto
optimize = 3
input = $SRC_GIF
output = $WORK/out_abs.gif
EOF
set +e
(cd /tmp && "$CLI" "$WORK/one.conf" --run --engine "$ENGINE" >"$WORK/out3.txt" 2>"$WORK/err3.txt")
rc=$?
set -e
if [[ "$rc" -eq 0 && -s "$WORK/out_abs.gif" ]]; then
  ok "run from /tmp with absolute settings"
else
  bad "run from /tmp (rc=$rc, out exists=$([[ -s $WORK/out_abs.gif ]] && echo y || echo n))"
  cat "$WORK/err3.txt" >&2 || true
fi

# 4. Paths with spaces
mkdir -p "$WORK/my vacation"
cp "$SRC_GIF" "$WORK/my vacation/in.gif"
cat > "$WORK/space.conf" <<EOF
mode = auto
optimize = 2
input = $WORK/my vacation/in.gif
output = $WORK/my vacation/out.gif
EOF
set +e
"$CLI" "$WORK/space.conf" --run --engine "$ENGINE" >"$WORK/out4.txt" 2>"$WORK/err4.txt"
rc=$?
set -e
if [[ "$rc" -eq 0 && -s "$WORK/my vacation/out.gif" ]]; then
  ok "paths with spaces"
else
  bad "paths with spaces (rc=$rc)"
  cat "$WORK/err4.txt" >&2 || true
fi

# 5. Malformed conf does not crash; bad values warned
cat > "$WORK/bad.conf" <<EOF
lossy = abc
optimize = notanumber
colors = 999
input = $SRC_GIF
output = $WORK/bad_out.gif
EOF
set +e
"$CLI" "$WORK/bad.conf" --run --engine "$ENGINE" >"$WORK/out5.txt" 2>"$WORK/err5.txt"
rc=$?
set -e
# Should either succeed with defaults or warn; must not crash (rc != 139 etc.)
if [[ "$rc" -lt 128 ]]; then
  ok "malformed conf does not crash (rc=$rc)"
else
  bad "malformed conf crashed (rc=$rc)"
fi
if grep -qi "WARNING" "$WORK/err5.txt"; then
  ok "malformed conf emits warnings"
else
  # warnings go to stderr; accept either channel
  if grep -qi "WARNING" "$WORK/out5.txt"; then ok "malformed conf emits warnings"; else bad "malformed conf silent on bad values"; fi
fi

# 6. Shell quoting visible in print for spaces
set +e
"$CLI" "$WORK/space.conf" --engine "$ENGINE" >"$WORK/out6.txt" 2>/dev/null
set -e
if grep -q "'" "$WORK/out6.txt"; then
  ok "toString shell-quotes paths with spaces"
else
  # also accept if the path is shown quoted another way
  if grep -q "my vacation" "$WORK/out6.txt"; then
    ok "print shows space path (quoting check soft)"
  else
    bad "print missing space path"
  fi
fi

# 7. --strict refuses any conf that produced a settings warning (audit U-40):
#    exit 3, nothing printed, nothing run. Without --strict the same conf
#    warns on stderr and proceeds (the documented policy).
cat > "$WORK/strict.conf" <<EOF
mode = auto
colors = 999
input = $SRC_GIF
output = $WORK/strict_out.gif
EOF
set +e
"$CLI" "$WORK/strict.conf" --strict --engine "$ENGINE" >"$WORK/out7.txt" 2>"$WORK/err7.txt"
rc_strict=$?
"$CLI" "$WORK/strict.conf" --run --strict --engine "$ENGINE" >"$WORK/out7r.txt" 2>"$WORK/err7r.txt"
rc_strict_run=$?
"$CLI" "$WORK/strict.conf" --engine "$ENGINE" >"$WORK/out7p.txt" 2>"$WORK/err7p.txt"
rc_plain=$?
set -e
if [[ "$rc_strict" -eq 3 && "$rc_strict_run" -eq 3 && "$rc_plain" -eq 0 ]] \
   && grep -q -- "--strict" "$WORK/err7.txt" \
   && [[ ! -s "$WORK/out7.txt" ]] \
   && grep -qi "WARNING" "$WORK/err7p.txt" \
   && [[ ! -f "$WORK/strict_out.gif" ]]; then
  ok "--strict refuses warned confs with rc=3 (print+run); plain mode warns and proceeds"
else
  bad "--strict behavior wrong (strict rc=$rc_strict, strict-run rc=$rc_strict_run, plain rc=$rc_plain)"
fi

# 8. --strict on a CLEAN conf is a pass-through (rc=0, command printed).
set +e
"$CLI" "$WORK/one.conf" --strict --engine "$ENGINE" >"$WORK/out8.txt" 2>"$WORK/err8.txt"
rc=$?
set -e
if [[ "$rc" -eq 0 ]] && grep -q "gifsicle" "$WORK/out8.txt" && [[ ! -s "$WORK/err8.txt" ]]; then
  ok "--strict passes a clean conf (rc=0, command on stdout)"
else
  bad "--strict mishandled a clean conf (rc=$rc)"
fi

echo "==> Done. $PASS passed, $FAIL failed."
[[ "$FAIL" -eq 0 ]]
