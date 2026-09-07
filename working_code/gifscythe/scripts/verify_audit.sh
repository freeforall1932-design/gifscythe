#!/usr/bin/env bash
#
# verify_audit.sh - One-command regression suite for COMPILED_AUDIT.md §6.
#
# Runs everything that can be verified on a Linux machine (with or without
# Qt6) plus the offscreen GUI harness, and prints audit-ID-labelled results.
# Windows-CI-only items (C1-C5) and real-desktop items are reported as SKIP
# with a pointer. Run after any change; tick §6 boxes from this output.
#
#   ./scripts/verify_audit.sh
#
set -uo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$self"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' VERSION.md \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
version="${version:-0.1.0}"
ENGINE="$self/release/$version/gifsicle"

PASS=0; FAIL=0; SKIP=0
ok()   { echo "  PASS [$1] $2"; PASS=$((PASS + 1)); }
bad()  { echo "  FAIL [$1] $2"; FAIL=$((FAIL + 1)); }
skip() { echo "  SKIP [$1] $2"; SKIP=$((SKIP + 1)); }

echo "==> Gifscythe audit verification (COMPILED_AUDIT §6)"

# ---------- A: build ----------
echo "-- build (A1 prerequisite)"
if ./build.sh > /tmp/vs_build.log 2>&1; then ok "A1" "build.sh green (engine+CLI+unit tests)"; else bad "A1" "build.sh failed — see /tmp/vs_build.log"; fi

# ---------- A1: example conf end-to-end ----------
rm -f /tmp/gifscythe_demo.gif
if ./build/gifscythe-cli examples/animation.conf --run >/dev/null 2>&1 && [[ -s /tmp/gifscythe_demo.gif ]]; then
  ok "A1" "example conf --run -> /tmp/gifscythe_demo.gif ($(stat -c%s /tmp/gifscythe_demo.gif 2>/dev/null) bytes)"
else bad "A1" "example conf --run failed or empty output"; fi

# ---------- A2: missing engine exits non-zero ----------
err="$(./build/gifscythe-cli examples/animation.conf --run --engine /nope 2>&1 >/dev/null)"; rc=$?
if [[ "$rc" -ne 0 ]] && grep -q "ERROR: engine not found" <<<"$err"; then
  ok "A2" "missing engine -> rc=$rc + honest stderr"
else bad "A2" "missing engine rc=$rc stderr: $err"; fi

# ---------- A3: CWD independence ----------
work="$(mktemp -d)"; cat > "$work/one.conf" <<EOF
mode = auto
optimize = 3
input = $self/../../reference_code/gifsicle/logo.gif
output = $work/out_abs.gif
EOF
(cd /tmp && "$self/build/gifscythe-cli" "$work/one.conf" --run --engine "$ENGINE" >/dev/null 2>&1); rc=$?
if [[ "$rc" -eq 0 && -s "$work/out_abs.gif" ]]; then ok "A3" "run from /tmp with absolute paths"; else bad "A3" "CWD independence (rc=$rc)"; fi

# ---------- A4: spaces ----------
mkdir -p "$work/my vacation"; cp ../../reference_code/gifsicle/logo.gif "$work/my vacation/in.gif"
cat > "$work/space.conf" <<EOF
mode = auto
optimize = 2
input = $work/my vacation/in.gif
output = $work/my vacation/out.gif
EOF
printed="$(./build/gifscythe-cli "$work/space.conf" 2>/dev/null)"
if ./build/gifscythe-cli "$work/space.conf" --run >/dev/null 2>&1 && [[ -s "$work/my vacation/out.gif" ]] && grep -q "'" <<<"$printed"; then
  ok "A4" "paths with spaces run + quoted display"
else bad "A4" "paths with spaces"; fi

# ---------- A5: single version source ----------
if ! grep -rq '"0\.1\.' src/cli src/qtui 2>/dev/null; then
  ok "A5" "no hardcoded product version in src/cli|src/qtui (GS_VERSION only)"
else bad "A5" "hardcoded version string found"; fi
if grep -q "GS_VERSION \"$version\"" src/core/version.h; then
  ok "A5" "version.h synced with VERSION.md ($version)"
else bad "A5" "version.h out of sync"; fi

# ---------- A6/A7/A8/A9: unit suite ----------
if ./build/test_gifsicle_command | tail -1 | grep -q "ALL TESTS PASSED"; then
  ok "A6/A7/A8/A9" "unit suite green (malformed parse, bool/rotation, round-trip, prvalue)"
else bad "A6/A7/A8/A9" "unit suite failed"; fi

# ---------- A10: engine identity ----------
vline="$("$ENGINE" --version 2>/dev/null | head -1)"
if grep -q "1\.96" <<<"$vline" && ! grep -q "$version" <<<"$vline"; then
  ok "A10" "engine reports upstream identity: $vline"
else bad "A10" "engine version line: $vline"; fi

# ---------- A11/A12: scripts ----------
if ./scripts/test_engine.sh 2>&1 | tail -1 | grep -q "0 failed"; then ok "A11" "test_engine.sh 5/5"; else bad "A11" "test_engine.sh"; fi
if ./scripts/smoke_cli.sh 2>&1 | tail -1 | grep -q "0 failed"; then ok "A12" "smoke_cli.sh 7/7"; else bad "A12" "smoke_cli.sh"; fi

# ---------- C6/C7/C8: cmake + honest GUI dispatch ----------
if cmake -S . -B "$work/c6" -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1 \
   && cmake --build "$work/c6" >/dev/null 2>&1; then
  ok "C6" "cmake configure+build (INTERFACE core)"
else bad "C6" "cmake configure/build"; fi
out_plain="$(./build.sh 2>&1)"
if grep -q "GUI not requested" <<<"$out_plain"; then ok "C8" "default build.sh does not claim GUI"; else bad "C8" "default build.sh GUI claim"; fi
if command -v qmake6 >/dev/null 2>&1 || command -v qmake >/dev/null 2>&1; then
  if ./build.sh --all >/dev/null 2>&1; then ok "C7*" "Qt present: --all builds GUI (no-Qt path needs Qt hidden — see audit notes)"; else bad "C7*" "--all with Qt present"; fi
else
  if ./build.sh --all >/dev/null 2>&1; then bad "C7" "--all claimed success without Qt"; else ok "C7" "--all fails honestly without Qt"; fi
fi

# ---------- B: GUI offscreen harness ----------
if command -v cmake >/dev/null 2>&1 && (command -v qmake6 >/dev/null 2>&1 || [[ -d /usr/lib/x86_64-linux-gnu/cmake/Qt6 ]]); then
  cmake -S . -B "$work/gui" -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
  if cmake --build "$work/gui" --target test_gui_offscreen -j2 >/dev/null 2>&1; then
    if GS_ENGINE="$ENGINE" GS_TEST_REF_DIR="$self/../../reference_code/gifsicle" \
       QT_QPA_PLATFORM=offscreen "$work/gui/test_gui_offscreen" 2>/dev/null | tail -1 | grep -q "ALL GUI TESTS PASSED"; then
      ok "B1-B15" "offscreen GUI harness green (batch/merge/explode/cancel/close/dedupe/live pane)"
    else bad "B1-B15" "GUI harness failed"; fi
  else skip "B" "GUI harness build failed (Qt6 incomplete?)"; fi
else
  skip "B" "Qt6 not installed — run on a Qt machine or CI"
fi

# ---------- D: packaging ----------
if ./scripts/package_portable.sh >/dev/null 2>&1; then
  pkg="release/$version/Gifscythe"
  miss=""
  [[ -x "$pkg/gifsicle" ]] || miss+=" engine"
  [[ -x "$pkg/gifscythe-cli" ]] || miss+=" cli"
  [[ -f "$pkg/LICENSE" ]] || miss+=" LICENSE"
  [[ -f "$pkg/COPYING.gifsicle" ]] || miss+=" COPYING.gifsicle"
  if [[ -z "$miss" ]]; then ok "D1/D2" "portable package complete (engine+CLI+licenses$([[ -f $pkg/gifscythe ]] && echo '+GUI'))"; else bad "D1/D2" "package missing:$miss"; fi
else bad "D" "package_portable.sh failed"; fi

# ---------- E: new-pit probes ----------
if ! grep -rn "system(\|/bin/sh\|cmd\.exe\|sh -c" src/ 2>/dev/null | grep -v "not for system()\|no shell\|NEVER\|never" | grep -q .; then
  ok "E3" "no shell execution in src/"
else bad "E3" "shell execution pattern found in src/"; fi
if grep -rq "GIFSYCYTHE" src/ 2>/dev/null; then bad "E8" "GIFSYCYTHE typo present"; else ok "E8" "include guards GIFSCYTHE_*"; fi
if grep -q "1/100 s" ../../FEASIBILITY_REVIEW.md; then ok "E7" "FEASIBILITY delay documented as 1/100 s"; else bad "E7" "delay unit doc"; fi
if ! grep -q 'currentData.*Mode::Merge' src/qtui/MainWindow.cpp && grep -q "setCurrentIndex(0)" src/qtui/MainWindow.cpp; then
  ok "E4" "default mode stays Batch (combo index 0)"
else bad "E4" "default mode suspect"; fi
# E5: the --windows compile must force win32cfg.h ahead of any config.h.
# (Real proof is the cross-build + wine run; this guards the recipe text.)
if awk '/== "windows"/{f=1} f && /-include src\/win32cfg\.h/{found=1} END{exit !found}' scripts/build_gifsicle.sh \
   && ! { grep -B1 -A2 -- '-include src/win32cfg.h' scripts/build_gifsicle.sh | grep -q -- '-DVERSION='; }; then
  ok "E5" "windows engine line: -include win32cfg.h, no -DVERSION override"
else bad "E5" "windows engine config suspect"; fi

# ---------- Windows CI-only ----------
skip "C1-C5" "GitHub Actions linux/windows jobs — check https://github.com/freeforall1932-design/gifscythe/actions after push"
skip "D3/D4" "clean Windows machine smoke (windeployqt folder, double-click GUI)"

rm -rf "$work"
echo "==> Done. $PASS passed, $FAIL failed, $SKIP skipped."
[[ "$FAIL" -eq 0 ]]
