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
smoke_out="$(./scripts/smoke_cli.sh 2>&1 | tail -1)"
if grep -q "0 failed" <<<"$smoke_out"; then
  smoke_n="$(grep -oE '[0-9]+ passed' <<<"$smoke_out" | grep -oE '^[0-9]+')"
  ok "A12" "smoke_cli.sh ${smoke_n:-?}/${smoke_n:-?}"
else bad "A12" "smoke_cli.sh"; fi

# ---------- C6/C7/C8: cmake + honest GUI dispatch ----------
# Audit U-38: a missing toolchain is a SKIP, not a FAIL. This block used to
# call cmake unconditionally and report FAIL on a machine without it (verified:
# "19 passed, 1 failed (C6), 3 skipped"), while the [B] block 13 lines below
# guarded the same tool with `command -v cmake`. The headline number was
# therefore not reproducible off the author's machine.
if ! command -v cmake >/dev/null 2>&1; then
  skip "C6" "cmake not installed — run on a machine with cmake or in CI"
elif cmake -S . -B "$work/c6" -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1 \
   && cmake --build "$work/c6" >/dev/null 2>&1; then
  ok "C6" "cmake configure+build (INTERFACE core)"
else bad "C6" "cmake configure/build"; fi

# C9 (audit U-15 / P2-2): the CMake build must not need — or write — anything
# inside src/. Proven on a throwaway COPY of the source tree with the committed
# src/core/version.h fallback DELETED: configure+build must still succeed (so
# every target consumed the build-tree generated header) and the copy's src/
# must still lack version.h afterwards (so configure wrote nothing back). The
# old CMakeLists failed both halves: it wrote src/core/version.h at configure
# time and the sources resolved the in-tree copy first.
if ! command -v cmake >/dev/null 2>&1; then
  skip "C9" "cmake not installed — out-of-tree purity not re-checked"
else
  c9src="$work/c9src"
  mkdir -p "$c9src"
  for item in CMakeLists.txt VERSION.md build_support src tests; do
    cp -r "$self/$item" "$c9src/" 2>/dev/null || true
  done
  rm -f "$c9src/src/core/version.h"
  if cmake -S "$c9src" -B "$work/c9" -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF >/dev/null 2>&1 \
     && cmake --build "$work/c9" >/dev/null 2>&1 \
     && [[ ! -e "$c9src/src/core/version.h" ]] \
     && [[ -s "$work/c9/generated/core/version.h" ]] \
     && grep -q "GS_VERSION \"$version\"" "$work/c9/generated/core/version.h" \
     && "$work/c9/test_gifsicle_command" >/dev/null 2>&1; then
    ok "C9" "cmake is source-tree pure: builds with NO committed version.h and writes nothing into src/ (U-15)"
  else
    bad "C9" "cmake still needs or mutates src/core/version.h (U-15 regression)"
  fi
fi
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
  # fake_engine_exit0 is T7's lying-engine fixture (audit U-17) — the harness
  # requires it next to the test binary, so build both targets here.
  if cmake --build "$work/gui" --target test_gui_offscreen fake_engine_exit0 -j2 >/dev/null 2>&1; then
    if GS_ENGINE="$ENGINE" GS_TEST_REF_DIR="$self/../../reference_code/gifsicle" \
       QT_QPA_PLATFORM=offscreen "$work/gui/test_gui_offscreen" 2>/dev/null | tail -1 | grep -q "ALL GUI TESTS PASSED"; then
      ok "B1-B15" "offscreen GUI harness green (batch/merge/explode/cancel/close/dedupe/live pane)"
    else bad "B1-B15" "GUI harness failed"; fi
  else skip "B" "GUI harness build failed (Qt6 incomplete?)"; fi
else
  skip "B" "Qt6 not installed — run on a Qt machine or CI"
fi

# ---------- D: packaging ----------
# The packager now fails closed (audit U-02): the GUI is REQUIRED unless the
# caller opts out with --engine-cli-only. So this check first asks whether a GUI
# was actually built, and then asserts the package matches that scope instead of
# quietly accepting a GUI-less folder (the old check printed "portable package
# complete" for exactly that).
gui_built=0
for cand in build/gifscythe build/gifscythe.exe build/gui/gifscythe \
            build/cmake/gifscythe build-win/gifscythe.exe; do
  [[ -f "$cand" ]] && { gui_built=1; break; }
done
pkg_args=()
[[ "$gui_built" == "1" ]] || pkg_args=(--engine-cli-only)
if ./scripts/package_portable.sh ${pkg_args+"${pkg_args[@]}"} >/dev/null 2>&1; then
  pkg="release/$version/Gifscythe"
  miss=""
  [[ -x "$pkg/gifsicle" ]] || miss+=" engine"
  [[ -x "$pkg/gifscythe-cli" ]] || miss+=" cli"
  [[ -s "$pkg/LICENSE" ]] || miss+=" LICENSE"
  [[ -s "$pkg/COPYING.gifsicle" ]] || miss+=" COPYING.gifsicle"
  if [[ "$gui_built" == "1" ]]; then
    { [[ -f "$pkg/gifscythe" ]] || [[ -f "$pkg/gifscythe.exe" ]]; } || miss+=" GUI"
  fi
  if [[ -z "$miss" ]]; then
    ok "D1/D2" "portable package complete (engine+CLI+licenses$([[ $gui_built == 1 ]] && echo '+GUI' || echo ', headless by request'))"
  else bad "D1/D2" "package missing:$miss"; fi
else bad "D" "package_portable.sh failed${pkg_args:+ (headless mode)}"; fi

# D5: negative packaging tests — an incomplete package MUST fail (audit U-14:
# CI had no negative test, so a green build proved nothing about the contents).
if ./scripts/test_package.sh 2>&1 | tail -1 | grep -q "0 failed"; then
  ok "D5" "test_package.sh negative suite green (incomplete packages fail closed)"
else bad "D5" "test_package.sh reported failures"; fi

# ---------- E: new-pit probes ----------
# Audit U-30 follow-up: E3 is line-based, so a DOC COMMENT that merely mentions
# "/bin/sh" used to trip it (verified: my own ProcessRunner.h comment turned a
# green run into "22 passed, 1 failed"). Nothing can execute inside a `//` or
# `*` comment line, so those are exempted here — code lines still must not match.
if ! grep -rn "system(\|/bin/sh\|cmd\.exe\|sh -c" src/ 2>/dev/null \
     | grep -v "not for system()\|no shell\|NEVER\|never" \
     | grep -vE ':[0-9]+:[[:space:]]*(//|\*|/\*)' | grep -q .; then
  ok "E3" "no shell execution in src/"
else bad "E3" "shell execution pattern found in src/"; fi
if grep -rq "GIFSYCYTHE" src/ 2>/dev/null; then bad "E8" "GIFSYCYTHE typo present"; else ok "E8" "include guards GIFSCYTHE_*"; fi
if grep -q "1/100 s" ../../FEASIBILITY_REVIEW.md; then ok "E7" "FEASIBILITY delay documented as 1/100 s"; else bad "E7" "delay unit doc"; fi
# E4: first mode item is Batch and combo starts at index 0 (SettingsPanel.cpp
# since the 2026-09-07 tab retrofit; harness T1 enforces this at runtime too).
if ! grep -q 'currentData.*Mode::Merge' src/qtui/MainWindow.cpp src/qtui/SettingsPanel.cpp \
   && grep -q "setCurrentIndex(0)" src/qtui/SettingsPanel.cpp \
   && grep -q "gs::Mode::Batch" src/qtui/SettingsPanel.cpp; then
  ok "E4" "default mode stays Batch (combo index 0)"
else bad "E4" "default mode suspect"; fi
# E5: the --windows compile must force win32cfg.h ahead of any config.h.
# (Real proof is the cross-build + wine run; this guards the recipe text.)
if awk '/== "windows"/{f=1} f && /-include src\/win32cfg\.h/{found=1} END{exit !found}' scripts/build_engine.sh \
   && ! { grep -B1 -A2 -- '-include src/win32cfg.h' scripts/build_engine.sh | grep -q -- '-DVERSION='; }; then
  ok "E5" "windows engine line: -include win32cfg.h, no -DVERSION override"
else bad "E5" "windows engine config suspect"; fi

# F-10/U-39: the proposed-workflow copy is hand-maintained next to the live one
# and has already drifted once, so drift is a FAIL instead of a surprise.
# One declared exception: the CI bot token has no `workflows` scope, so a change
# to .github/ cannot always be pushed (docs/ci/README.md "Apply manually").
# In that state the copies differ ON PURPOSE, and docs/ci/PENDING_WORKFLOW_CHANGE.md
# records what is waiting and how to apply it. Declared drift is a SKIP; any
# other drift is still a FAIL. Delete the marker once the change is applied.
if diff -q ../../.github/workflows/build.yml ../../docs/ci/build.yml.proposed >/dev/null 2>&1; then
  ok "E9" ".github/workflows/build.yml and docs/ci/build.yml.proposed are byte-identical"
elif [[ -f ../../docs/ci/PENDING_WORKFLOW_CHANGE.md ]]; then
  skip "E9" "declared pending workflow change — see docs/ci/PENDING_WORKFLOW_CHANGE.md (needs a token with the workflows scope)"
else
  bad "E9" "workflow and docs/ci/build.yml.proposed have drifted"
fi

# ---------- F: documentation status gate ----------
# The docs ARE the context the next session starts from, so a stale doc is
# corrupted input rather than a cosmetic problem. STATUS.md is the single status
# register and scripts/check_docs.sh both emits it and enforces it.
#
# RECURSION NOTE: check_docs.sh runs THIS script to learn its real "N passed,
# N failed, N skipped" (so docs cannot quote a stale gate count). To stop the
# loop it invokes us with GS_SKIP_DOC_GATE=1, which suppresses this block.
# DOC_GATE_CHECKS declares how many checks this block contributes and
# check_docs.sh reads that number back out of this file, so the total the docs
# must quote stays derived from the repo instead of hardcoded in the checker.
DOC_GATE_CHECKS=2
if [[ "${GS_SKIP_DOC_GATE:-0}" == "1" ]]; then
  : # nested run from check_docs.sh — do not recurse
else
  if ./scripts/check_docs.sh --from-verify-audit > /tmp/vs_docs.log 2>&1; then
    ok "F1" "check_docs.sh green — STATUS.md register + all doc-consistency checks"
  else
    bad "F1" "check_docs.sh reported failures — see /tmp/vs_docs.log"
    tail -n 12 /tmp/vs_docs.log | sed 's/^/       /'
  fi
  # F2 is deliberately independent of check_docs.sh: if the checker itself is
  # broken, a missing or self-inconsistent register must still fail the suite.
  if [[ -s ../../STATUS.md ]] && \
     awk -F'|' '/^\|[ ]*[A-Z]+-[0-9]+[ ]*\|/ { s=$4; gsub(/[ \t]/,"",s); c[s]++; t++ }
                END { exit !(t > 0 && c["DONE"]+c["PARTIAL"]+c["OPEN"]+c["UNTRIAGED"] == t) }' \
        <(sed 's/\\|/\x01/g' ../../STATUS.md); then
    ok "F2" "STATUS.md present and its four state counts sum to its row count"
  else
    bad "F2" "STATUS.md missing, empty, or its state counts do not sum to the row count"
  fi
fi

# ---------- W: web demo parity (JS ⇄ C++) ----------
# web/ is not the product path, but it ships a SECOND copy of the command
# builder and — since audit U-30 — of the validation rules. Neither is checked
# by the C++ suites, so both are cross-checked against the real gifscythe-cli
# here; otherwise the two clients drift silently (audit U-03 proved they do).
if command -v node >/dev/null 2>&1; then
  if ( cd ../.. && node web/test/command.test.mjs 2>&1 | tail -1 | grep -q "ALL WEB COMMAND TESTS PASSED" ); then
    ok "W1" "web command parity (web/command.mjs == src/core/GifsicleCommand.h)"
  else bad "W1" "web command parity failed"; fi
  if ( cd ../.. && node web/test/validate.test.mjs 2>&1 | tail -1 | grep -q "ALL WEB VALIDATION TESTS PASSED" ); then
    ok "W2" "web validation parity (web/validate.mjs == src/core/Validate.h)"
  else bad "W2" "web validation parity failed"; fi
  # W3 drives the REAL server over HTTP: percent-encoding, latin1 header limits
  # and the 422 path. Mutation-tested — re-adding the double decode fails 4
  # cases, dropping encodeURIComponent fails 6, removing validate() fails 3.
  w3_out="$( cd ../.. && node web/test/transport.test.mjs 2>&1 )"
  if grep -q "ALL WEB TRANSPORT TESTS PASSED" <<<"$w3_out"; then
    w3_n="$(grep -c '^PASS' <<<"$w3_out")"
    ok "W3" "web transport end-to-end (${w3_n:-?} cases against a live server)"
  else bad "W3" "web transport tests failed"; fi
else
  skip "W1/W2" "node not installed — web parity tests skipped"
fi

# ---------- Windows CI-only ----------
skip "C1-C5" "GitHub Actions linux/windows jobs — check https://github.com/freeforall1932-design/gifscythe/actions after push"
skip "D3/D4" "clean Windows machine smoke (windeployqt folder, double-click GUI)"

rm -rf "$work"
echo "==> Done. $PASS passed, $FAIL failed, $SKIP skipped."
[[ "$FAIL" -eq 0 ]]
