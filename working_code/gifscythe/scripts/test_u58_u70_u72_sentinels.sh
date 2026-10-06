#!/usr/bin/env bash
# test_u58_u70_u72_sentinels.sh — audit U-58 / P1-38, U-70 / P1-42 and
# U-72 / P1-42.
#
# All three patches in the S35 intake shipped a source-level sentinel as part
# of their proof plan (their Qt behaviour could not be compiled in the sandbox
# that took them). This is that set, with the defect in the 0005 sentinel
# fixed: as written it was `grep -c currentSettings()`, which FALSE-FAILS on
# the patch's own new comment — MainWindow.cpp's continuation comment names
# currentSettings() to explain what must NOT be re-read there. Every check
# below strips comments first, so prose about a pattern never counts as the
# pattern.
#
# U-70 is here because its Qt behaviour is NOT distinguishable on a UTF-8-clean
# host: on POSIX the wrapped and the bare std::string -> fs::path conversion are
# the same bytes, so only a Windows runner can see the difference in behaviour
# (tests/test_gui_offscreen.cpp T23 asserts the premise there under `#ifdef
# _WIN32`). The source invariant is the platform-independent proof, and it is
# the one that fails on the pre-fix tree.
#
# Legs:
#   S1 SENTINEL  U-58 / P1-38: no currentSettings() CALL inside
#                onProcessFinished; the batch-start snapshot is written once
#                (runCommand) and read once (the continuation); batchSettings_
#                is declared in MainWindow.h.
#   S2 SENTINEL  U-72 / P1-42: cancelRun() contains NO `cancelling_ = false;`
#                (the early clear the finding is about) and arms the latch
#                inside `if (engineWasRunning)`; onProcessFinished consumes it
#                FIRST — read-and-clear before the verdict branch.
#   S3 SENTINEL  U-70 / P1-42: every engine probe goes through
#                gs::u8path_compat (3 sites: ensureEngine x2 + startPreview),
#                startPreview() probes through the boundary exactly once, and
#                the bare `fs::path(enginePath_.toStdString())` form is absent.
#   S4 MUTATION  each sentinel re-run against its PRE-FIX source shape — built
#                here by patching a scratch copy with the exact lines the
#                patches removed/added — and MUST fail. A sentinel that cannot
#                fail on the code it exists to reject has no teeth (the
#                discipline review_change.sh R2 asks for, executable).
#
# No Qt, no compiler: pure text invariants over the committed sources, so it
# runs in this sandbox and on every CI platform. Optional overrides let the
# mutation legs (and a reviewer) point it elsewhere:
#   --mainwindow PATH   default working_code/gifscythe/src/qtui/MainWindow.cpp
#   --header     PATH   default working_code/gifscythe/src/qtui/MainWindow.h

set -euo pipefail
cd "$(dirname "$0")/.."

CPP=src/qtui/MainWindow.cpp
HDR=src/qtui/MainWindow.h
while [ $# -gt 0 ]; do
  case "$1" in
    --mainwindow) CPP="$2"; shift 2 ;;
    --header)     HDR="$2"; shift 2 ;;
    *) printf 'unknown argument: %s\n' "$1" >&2; exit 2 ;;
  esac
done

say() { printf '%s\n' "$*"; }

for f in "$CPP" "$HDR"; do
  if [ ! -f "$f" ]; then
    say "FAIL: $f not found — run from working_code/gifscythe/ or pass --mainwindow/--header"
    exit 2
  fi
done

# ---- comment filter + function extraction ---------------------------------
# strip_comments: drop //-to-end-of-line and /* ... */ so a comment that NAMES
# a pattern can never satisfy (or trip) a check. String literals containing
# "//" would be truncated too — harmless for token checks, and no check below
# looks for a token that only occurs inside a literal.
strip_comments() {
  sed -e 's:/\*.*\*/::g' -e 's://.*::' "$1"
}

# extract_fn FILE 'START' — the function body from the line starting with
# START to the first column-0 closing brace.
extract_fn() {
  awk -v pat="$2" 'index($0, pat) == 1 { infn = 1 } infn { print } infn && $0 == "}" { exit }' "$1"
}

# ---- S1: U-58 batch-settings snapshot -------------------------------------
check_u58() { # $1 = MainWindow.cpp, $2 = MainWindow.h
  local cpp="$1" hdr="$2" rc=0
  local on_finished run_cmd
  on_finished="$(extract_fn "$cpp" 'void MainWindow::onProcessFinished(')"
  run_cmd="$(extract_fn "$cpp" 'void MainWindow::runCommand(')"

  # Vacuity guards: an empty/failed extraction must not look like a pass.
  if ! printf '%s\n' "$on_finished" | grep -q 'batchIndex_'; then
    say "  S1: could not extract onProcessFinished() from $cpp"; return 1
  fi
  if ! printf '%s\n' "$run_cmd" | grep -q 'batchTargets_'; then
    say "  S1: could not extract runCommand() from $cpp"; return 1
  fi

  # The finding: continuation jobs re-read LIVE settings through
  # currentSettings() — a CALL here is the regression, a comment is not.
  local live_calls
  live_calls="$(printf '%s\n' "$on_finished" | strip_comments /dev/stdin | grep -c 'currentSettings()' || true)"
  if [ "$live_calls" != "0" ]; then
    say "  S1: onProcessFinished() still CALLS currentSettings() ($live_calls hit(s)) —"
    say "      the continuation must read the batch-start snapshot (U-58 / P1-38)."
    rc=1
  fi

  # Written once, in the batch branch of runCommand, beside the plan.
  local writes
  writes="$(printf '%s\n' "$run_cmd" | strip_comments /dev/stdin | grep -c 'batchSettings_ = settings;' || true)"
  if [ "$writes" != "1" ]; then
    say "  S1: expected exactly 1 'batchSettings_ = settings;' in runCommand(), found $writes."
    rc=1
  fi
  local total_writes
  total_writes="$(strip_comments "$cpp" | grep -c 'batchSettings_ = settings;' || true)"
  if [ "$total_writes" != "1" ]; then
    say "  S1: the snapshot must be written ONCE per file, found $total_writes."
    rc=1
  fi

  # Read once, in the continuation.
  local reads
  reads="$(printf '%s\n' "$on_finished" | strip_comments /dev/stdin | grep -c 'gs::Settings one = batchSettings_;' || true)"
  if [ "$reads" != "1" ]; then
    say "  S1: expected exactly 1 'gs::Settings one = batchSettings_;' in"
    say "      onProcessFinished(), found $reads."
    rc=1
  fi

  # Declared where the continuation can see it.
  if [ "$(strip_comments "$hdr" | grep -c 'gs::Settings batchSettings_;' || true)" != "1" ]; then
    say "  S1: 'gs::Settings batchSettings_;' not declared exactly once in $hdr."
    rc=1
  fi
  return "$rc"
}

# ---- S2: U-72 cancel latch -------------------------------------------------
check_u72() { # $1 = MainWindow.cpp
  local cpp="$1" rc=0
  local cancel on_finished
  cancel="$(extract_fn "$cpp" 'void MainWindow::cancelRun(')"
  on_finished="$(extract_fn "$cpp" 'void MainWindow::onProcessFinished(')"

  if ! printf '%s\n' "$cancel" | grep -q 'waitForFinished(3000)'; then
    say "  S2: could not extract cancelRun() from $cpp"; return 1
  fi
  if ! printf '%s\n' "$on_finished" | grep -q 'batchIndex_'; then
    say "  S2: could not extract onProcessFinished() from $cpp"; return 1
  fi

  local cancel_stripped on_stripped
  cancel_stripped="$(printf '%s\n' "$cancel" | strip_comments /dev/stdin)"
  on_stripped="$(printf '%s\n' "$on_finished" | strip_comments /dev/stdin)"

  # The finding in one line: the pre-fix code cleared the flag after the wait.
  if [ "$(printf '%s\n' "$cancel_stripped" | grep -c 'cancelling_ = false;' || true)" != "0" ]; then
    say "  S2: cancelRun() clears cancelling_ again — the early clear that lets a"
    say "      late finished() pop a spurious failure dialog (U-72 / P1-42)."
    rc=1
  fi

  # ...and it arms the latch only when a process was ACTUALLY running.
  if [ "$(printf '%s\n' "$cancel_stripped" | grep -c 'engineWasRunning' || true)" -lt 1 ] ||
     [ "$(printf '%s\n' "$cancel_stripped" | grep -c 'if (engineWasRunning) {' || true)" != "1" ]; then
    say "  S2: cancelRun() must arm the latch inside 'if (engineWasRunning) {'."
    rc=1
  fi
  if [ "$(printf '%s\n' "$cancel_stripped" | grep -c 'cancelling_ = true;' || true)" != "1" ]; then
    say "  S2: expected exactly 1 'cancelling_ = true;' in cancelRun()."
    rc=1
  fi

  # Exactly one clear in the whole file, and it is the consume-first one.
  if [ "$(strip_comments "$cpp" | grep -c 'cancelling_ = false;' || true)" != "1" ]; then
    say "  S2: expected exactly 1 'cancelling_ = false;' in $cpp (the consumption)."
    rc=1
  fi
  local read_line clear_line
  read_line="$(printf '%s\n' "$on_stripped" | grep -n 'const bool wasCancelling = cancelling_;' | head -n1 | cut -d: -f1 || true)"
  clear_line="$(printf '%s\n' "$on_stripped" | grep -n 'cancelling_ = false;' | head -n1 | cut -d: -f1 || true)"
  if [ -z "$read_line" ] || [ -z "$clear_line" ] || [ "$read_line" -ge "$clear_line" ]; then
    say "  S2: onProcessFinished() must read-and-consume the latch FIRST"
    say "      (one completion, one consumption), else the next genuine failure"
    say "      is swallowed."
    rc=1
  fi
  return "$rc"
}

# ---- S3: U-70 preview engine boundary -------------------------------------
check_u70() { # $1 = MainWindow.cpp
  local cpp="$1" rc=0
  local start_preview
  start_preview="$(extract_fn "$cpp" 'void MainWindow::startPreview(')"
  if ! printf '%s\n' "$start_preview" | grep -q 'previewSeq_'; then
    say "  S3: could not extract startPreview() from $cpp"; return 1
  fi

  # One boundary rule everywhere: 2 probes in ensureEngine + 1 in startPreview.
  local wrapped
  wrapped="$(strip_comments "$cpp" | grep -c 'gs::path_is_executable(gs::u8path_compat(enginePath_.toStdString()))' || true)"
  if [ "$wrapped" != "3" ]; then
    say "  S3: expected 3 wrapped engine probes (ensureEngine x2 + startPreview),"
    say "      found $wrapped — a probe bypasses the UTF-8 boundary (U-70 / P1-42)."
    rc=1
  fi

  # The finding's exact pre-fix shape, anywhere in the file.
  local bare
  bare="$(strip_comments "$cpp" | grep -cE 'path_is_executable\(fs::path\(enginePath_|path_is_executable\(enginePath_' || true)"
  if [ "$bare" != "0" ]; then
    say "  S3: a bare (unwrapped) engine probe is back ($bare hit(s)) — on a"
    say "      legacy-ACP Windows host that path is not the file (U-70 / P1-42)."
    rc=1
  fi

  # ...and the preview re-probe specifically obeys it.
  local preview_probes
  preview_probes="$(printf '%s\n' "$start_preview" | strip_comments /dev/stdin | grep -c 'gs::path_is_executable(gs::u8path_compat(enginePath_.toStdString()))' || true)"
  if [ "$preview_probes" != "1" ]; then
    say "  S3: startPreview() must probe through gs::u8path_compat exactly once,"
    say "      found $preview_probes."
    rc=1
  fi
  return "$rc"
}

# ---- S1 + S2 + S3 against the committed sources ----------------------------
if ! check_u58 "$CPP" "$HDR"; then
  say "S1 SENTINEL FAIL (U-58 / P1-38)"
  exit 1
fi
say "S1 sentinel: continuation reads the batch snapshot; currentSettings() not called there"

if ! check_u72 "$CPP"; then
  say "S2 SENTINEL FAIL (U-72 / P1-42)"
  exit 1
fi
say "S2 sentinel: latch armed only for a running engine, consumed once, never cleared in cancelRun"

if ! check_u70 "$CPP"; then
  say "S3 SENTINEL FAIL (U-70 / P1-42)"
  exit 1
fi
say "S3 sentinel: all 3 engine probes wrap u8path_compat; no bare probe in the file"

# ---- S4: mutation — the pre-fix shapes MUST be rejected --------------------
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# (a) U-58 pre-fix continuation: live re-read instead of the snapshot.
sed 's|^      gs::Settings one = batchSettings_;$|      auto settings = currentSettings();\n      gs::Settings one = settings;|' \
    "$CPP" > "$TMP/prefix_u58.cpp"
if ! grep -q 'auto settings = currentSettings();' "$TMP/prefix_u58.cpp"; then
  say "S4 MUTATION SETUP FAIL: could not build the pre-fix U-58 shape"
  exit 1
fi
if check_u58 "$TMP/prefix_u58.cpp" "$HDR" >/dev/null 2>&1; then
  say "S4 MUTATION FAIL: the U-58 sentinel accepts a live currentSettings()"
  say "   re-read in the continuation — it cannot catch the finding."
  exit 1
fi
say "S4 mutation a: pre-fix U-58 continuation (live re-read) rejected"

# (b) U-72 pre-fix early clear, back in cancelRun right after the 3 s wait.
sed 's|^    process_->waitForFinished(3000);$|    process_->waitForFinished(3000);\n    cancelling_ = false;|' \
    "$CPP" > "$TMP/prefix_u72.cpp"
if [ "$(grep -c 'cancelling_ = false;' "$TMP/prefix_u72.cpp")" -lt 2 ]; then
  say "S4 MUTATION SETUP FAIL: could not build the pre-fix U-72 shape"
  exit 1
fi
if check_u72 "$TMP/prefix_u72.cpp" >/dev/null 2>&1; then
  say "S4 MUTATION FAIL: the U-72 sentinel accepts the early clear it exists to reject."
  exit 1
fi
say "S4 mutation b: pre-fix U-72 early clear rejected"

# (c) the consume-after-verdict pit the patch's own comment names: clearing
# before reading lets the latch outlive one finished() and swallow the next one.
sed 's|^  const bool wasCancelling = cancelling_;$|@@READ@@|; s|^  cancelling_ = false;$|@@CLEAR@@|; s|^@@READ@@$|  cancelling_ = false;|; s|^@@CLEAR@@$|  const bool wasCancelling = cancelling_;|' \
    "$CPP" > "$TMP/pit_u72.cpp"
if [ "$(grep -c 'cancelling_ = false;' "$TMP/pit_u72.cpp")" -lt 1 ] ||
   ! grep -q 'const bool wasCancelling = cancelling_;' "$TMP/pit_u72.cpp"; then
  say "S4 MUTATION SETUP FAIL: could not build the U-72 consume-order pit"
  exit 1
fi
if check_u72 "$TMP/pit_u72.cpp" >/dev/null 2>&1; then
  say "S4 MUTATION FAIL: the U-72 sentinel accepts consume-after-verdict order."
  exit 1
fi
say "S4 mutation c: consume-order pit rejected"

# (d) U-70 pre-fix preview probe: the bare std::string -> fs::path conversion
# re-added beside the wrapped one (the count check still sees 3 wrapped probes,
# so only the bare-form check can catch this shape).
awk 'index($0, "void MainWindow::startPreview(") == 1 { infn = 1 }
     infn && !inserted && /u8path_compat\(enginePath_\.toStdString\(\)\)/ {
       print
       print "  if (!gs::path_is_executable(fs::path(enginePath_.toStdString()))) { return; }"
       inserted = 1
       next
     }
     { print }' "$CPP" > "$TMP/prefix_u70.cpp"
if [ "$(grep -cE 'path_is_executable\(fs::path\(enginePath_' "$TMP/prefix_u70.cpp" || true)" != "1" ]; then
  say "S4 MUTATION SETUP FAIL: could not build the pre-fix U-70 shape"
  exit 1
fi
if check_u70 "$TMP/prefix_u70.cpp" >/dev/null 2>&1; then
  say "S4 MUTATION FAIL: the U-70 sentinel accepts a bare engine probe in startPreview()."
  exit 1
fi
say "S4 mutation d: pre-fix U-70 bare preview probe rejected"

say "test_u58_u70_u72_sentinels: PASS (S1-S3 sentinels, S4 mutations a/b/c/d)"
