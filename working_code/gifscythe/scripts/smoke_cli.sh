#!/usr/bin/env bash
#
# smoke_cli.sh - Integration smoke tests for gifscythe-cli.
# Coverage: missing engine exit code, strict CLI parsing, PATH-only engine
# discovery, CWD independence, paths with spaces, binary stdout purity,
# output-target refusal, batch vs merge semantics, malformed conf warnings,
# --strict refusal, explode frame verification (audit U-17: real frames counted,
# lying engine refused, empty-output basename prefix followed), Batch-with-no-
# output refusal (GS-201 / P0-5: --run must not rewrite source GIFs).
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

# 2. Unknown and incomplete CLI options must fail closed (audit U-23).
set +e
"$CLI" "$self/examples/animation.conf" --rnu >"$WORK/out2u.txt" 2>"$WORK/err2u.txt"
rc_unknown=$?
"$CLI" "$self/examples/animation.conf" --engine >"$WORK/out2m.txt" 2>"$WORK/err2m.txt"
rc_missing=$?
set -e
if [[ "$rc_unknown" -eq 2 ]] && grep -q "unknown argument '--rnu'" "$WORK/err2u.txt"; then
  ok "unknown CLI option is rejected with rc=2"
else
  bad "unknown CLI option was not rejected honestly (rc=$rc_unknown)"
fi
if [[ "$rc_missing" -eq 2 ]] && grep -q -- "--engine requires a path" "$WORK/err2m.txt"; then
  ok "--engine without a value is rejected with rc=2"
else
  bad "missing --engine value was not rejected honestly (rc=$rc_missing)"
fi

# 3. Print mode works from product dir
set +e
(cd "$self" && "$CLI" examples/animation.conf >"$WORK/out2.txt" 2>"$WORK/err2.txt")
rc=$?
set -e
if [[ "$rc" -eq 0 ]] && grep -q "\-O3\|-m\|logo" "$WORK/out2.txt"; then
  ok "print mode from product dir"
else
  bad "print mode from product dir (rc=$rc)"
fi

# 4. --run from a different CWD with absolute settings path
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

# 5. Paths with spaces
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

# 6. In --run mode stdout must remain a byte-pure GIF stream (audit U-04).
# The CLI's progress commentary belongs on stderr; compare its stdout with a
# direct engine invocation so even a valid-looking GIF with trailing text fails.
cat > "$WORK/stdout.conf" <<EOF
mode = auto
input = $SRC_GIF
EOF
set +e
"$CLI" "$WORK/stdout.conf" --run --engine "$ENGINE" >"$WORK/stdout.gif" 2>"$WORK/err_stdout.txt"
rc=$?
set -e
"$ENGINE" "$SRC_GIF" >"$WORK/direct.gif" 2>/dev/null
if [[ "$rc" -eq 0 && -s "$WORK/stdout.gif" ]] && cmp -s "$WORK/stdout.gif" "$WORK/direct.gif" \
   && grep -q "running (argv exec, no shell)" "$WORK/err_stdout.txt"; then
  ok "--run stdout is byte-pure engine output; commentary stays on stderr"
else
  bad "--run stdout was contaminated or engine output changed (rc=$rc)"
fi

# 7. PATH-only engine discovery must execute the first matching PATH entry
# (audit U-05 / P2-4), even when the packaged/release candidates are absent.
mkdir -p "$WORK/path-only" "$WORK/path-app"
cat > "$WORK/path-only/gifsicle" <<EOF
#!/bin/sh
exec "$ENGINE" "\$@"
EOF
chmod +x "$WORK/path-only/gifsicle"
# Run a copy of the CLI outside the product tree so its packaged/dev search
# candidates are absent; PATH must be the successful discovery route.
cp "$CLI" "$WORK/path-app/gifscythe-cli"
cat > "$WORK/path.conf" <<EOF
mode = auto
input = $SRC_GIF
output = $WORK/path_out.gif
EOF
set +e
(cd "$WORK" && env -u GS_ENGINE PATH="$WORK/path-only" "$WORK/path-app/gifscythe-cli" "$WORK/path.conf" --run >"$WORK/out_path.txt" 2>"$WORK/err_path.txt")
rc=$?
set -e
if [[ "$rc" -eq 0 && -s "$WORK/path_out.gif" ]] \
   && grep -Fq "$WORK/path-only/gifsicle" "$WORK/err_path.txt"; then
  ok "PATH-only engine is discovered and executed"
else
  bad "PATH-only engine discovery failed (rc=$rc)"
fi

# 8. The CLI must refuse an output that aliases its input before starting the
# engine (audit U-01 / P2-4), and must leave the source bytes untouched.
cp "$SRC_GIF" "$WORK/self.gif"
cp "$WORK/self.gif" "$WORK/self.before.gif"
cat > "$WORK/self.conf" <<EOF
mode = auto
input = $WORK/self.gif
output = $WORK/self.gif
EOF
set +e
"$CLI" "$WORK/self.conf" --run --engine "$ENGINE" >"$WORK/out_self.txt" 2>"$WORK/err_self.txt"
rc=$?
set -e
if [[ "$rc" -eq 2 ]] && cmp -s "$WORK/self.gif" "$WORK/self.before.gif" \
   && grep -q "planned output is not safe" "$WORK/err_self.txt"; then
  ok "output target equal to source is refused before engine start"
else
  bad "unsafe output target was not refused honestly (rc=$rc)"
fi

# 9. Malformed conf does not crash; bad values warned
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

# 10. Shell quoting visible in print for spaces
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

# 11. --strict refuses any conf that produced a settings warning (audit U-40):
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

# 12. --strict on a CLEAN conf is a pass-through (rc=0, command printed).
set +e
"$CLI" "$WORK/one.conf" --strict --engine "$ENGINE" >"$WORK/out8.txt" 2>"$WORK/err8.txt"
rc=$?
set -e
if [[ "$rc" -eq 0 ]] && grep -q "gifsicle" "$WORK/out8.txt" && [[ ! -s "$WORK/err8.txt" ]]; then
  ok "--strict passes a clean conf (rc=0, command on stdout)"
else
  bad "--strict mishandled a clean conf (rc=$rc)"
fi

# 12b. A half-parsed position pair is ignored (audit U-53): plain print must
#      not emit -p X,0, and --strict still refuses the warned conf.
cat > "$WORK/pos_half.conf" <<EOF
mode = auto
position_x = 12
position_y = nope
input = $SRC_GIF
EOF
set +e
"$CLI" "$WORK/pos_half.conf" --engine "$ENGINE" >"$WORK/out8b.txt" 2>"$WORK/err8b.txt"
rc_pos_plain=$?
"$CLI" "$WORK/pos_half.conf" --strict --engine "$ENGINE" >"$WORK/out8bs.txt" 2>"$WORK/err8bs.txt"
rc_pos_strict=$?
set -e
if [[ "$rc_pos_plain" -eq 0 && "$rc_pos_strict" -eq 3 ]] \
   && ! grep -q -- ' -p ' "$WORK/out8b.txt" \
   && grep -q 'position needs both position_x and position_y' "$WORK/err8b.txt" \
   && grep -q -- '--strict' "$WORK/err8bs.txt"; then
  ok "U-53 half-parsed position pair is dropped; strict still refuses the warning"
else
  bad "U-53 position half-parse regressed (plain=$rc_pos_plain strict=$rc_pos_strict)"
fi

# 12c. crop 0x0 is legal engine syntax (audit U-62), so --strict must not stop it.
cat > "$WORK/crop0.conf" <<EOF
mode = auto
crop = true
crop_x = 2
crop_y = 2
crop_w = 0
crop_h = 0
input = $SRC_GIF
output = $WORK/crop0.gif
EOF
set +e
"$CLI" "$WORK/crop0.conf" --run --strict --engine "$ENGINE" >"$WORK/out8c.txt" 2>"$WORK/err8c.txt"
rc_crop0=$?
set -e
if [[ "$rc_crop0" -eq 0 && -s "$WORK/crop0.gif" ]] \
   && ! grep -q 'crop width/height must be > 0' "$WORK/err8c.txt"; then
  ok "U-62 crop 0x0 survives validation and runs under --strict"
else
  bad "U-62 crop 0x0 was still blocked (rc=$rc_crop0)"
fi

# 12d. Special frame selectors stay literal (audit U-60): #0 reaches the engine
#      unchanged, and the run succeeds with a one-frame output.
cat > "$WORK/frame_selector.conf" <<EOF
mode = auto
input = $SRC_GIF
input = #0
output = $WORK/frame0.gif
EOF
set +e
"$CLI" "$WORK/frame_selector.conf" --run --engine "$ENGINE" >"$WORK/out8d.txt" 2>"$WORK/err8d.txt"
rc_frame0=$?
set -e
INFO_FRAME0="$($ENGINE --info "$WORK/frame0.gif" 2>/dev/null || true)"
if [[ "$rc_frame0" -eq 0 && -s "$WORK/frame0.gif" ]] \
   && grep -q '1 image' <<<"$INFO_FRAME0" \
   && ! grep -q '/#0' "$WORK/err8d.txt"; then
  ok "U-60 frame selector #0 is not path-resolved and the run succeeds"
else
  bad "U-60 frame selector still broke run honesty (rc=$rc_frame0)"
fi

# 12e. output = - keeps stdout streaming semantics (audit U-61): the CLI must
#      not turn it into a literal file named '-'.
cat > "$WORK/stdout_dash.conf" <<EOF
mode = auto
input = $SRC_GIF
output = -
EOF
set +e
"$CLI" "$WORK/stdout_dash.conf" --run --engine "$ENGINE" >"$WORK/stdout_dash.gif" 2>"$WORK/err8e.txt"
rc_dash=$?
set -e
"$ENGINE" "$SRC_GIF" -o - >"$WORK/direct_dash.gif" 2>/dev/null
if [[ "$rc_dash" -eq 0 && -s "$WORK/stdout_dash.gif" ]] \
   && cmp -s "$WORK/stdout_dash.gif" "$WORK/direct_dash.gif" \
   && [[ ! -e "$WORK/-" ]] \
   && grep -q -- ' -o -' "$WORK/err8e.txt"; then
  ok "U-61 output=- streams to stdout and does not create a literal dash file"
else
  bad "U-61 output=- did not keep stdout semantics (rc=$rc_dash, dash-file=$([[ -e "$WORK/-" ]] && echo y || echo n))"
fi

# 12f. threads < -1 is warned and therefore refused by --strict (audit DS-09).
cat > "$WORK/threads_neg.conf" <<EOF
mode = auto
threads = -7
input = $SRC_GIF
EOF
set +e
"$CLI" "$WORK/threads_neg.conf" --strict --engine "$ENGINE" >"$WORK/out8f.txt" 2>"$WORK/err8f.txt"
rc_threads=$?
set -e
if [[ "$rc_threads" -eq 3 ]] && grep -q 'WARNING: threads=-7' "$WORK/err8f.txt"; then
  ok "DS-09 threads<-1 surfaces a warning and strict refusal"
else
  bad "DS-09 threads<-1 warning/refusal missing (rc=$rc_threads)"
fi

# 12g. Threads is a TRI-state (DS-06 / P0-2): -1 says nothing to the engine,
#      0 is a bare -j ("auto" = the engine's own default count), N is -jN.
#      Before P0-2 the first two collapsed, so "unset" silently ran 8 threads.
cat > "$WORK/threads_unset.conf" <<EOF
mode = auto
threads = -1
input = $SRC_GIF
EOF
cat > "$WORK/threads_auto.conf" <<EOF
mode = auto
threads = 0
input = $SRC_GIF
EOF
cat > "$WORK/threads_four.conf" <<EOF
mode = auto
threads = 4
input = $SRC_GIF
EOF
"$CLI" "$WORK/threads_unset.conf" --engine "$ENGINE" >"$WORK/tu.txt" 2>/dev/null
"$CLI" "$WORK/threads_auto.conf"  --engine "$ENGINE" >"$WORK/ta.txt" 2>/dev/null
"$CLI" "$WORK/threads_four.conf"  --engine "$ENGINE" >"$WORK/tf.txt" 2>/dev/null
if ! grep -q -- '-j' "$WORK/tu.txt" && grep -qE -- '-j( |$)' "$WORK/ta.txt" \
   && grep -q -- '-j4' "$WORK/tf.txt"; then
  ok "P0-2 threads: -1 no flag, 0 bare -j, 4 explicit -j4"
else
  bad "P0-2 threads tri-state wrong (unset/auto/four: $(grep -oE '\-j[0-9]*' "$WORK/tu.txt" "$WORK/ta.txt" "$WORK/tf.txt" | tr '\n' ' '))"
fi

# 12h. "Play once" is reachable end to end (U-63 / P1-40): the CLI prints
#      --no-loopcount AND the written GIF carries no loop extension at all
#      (gifsicle.1: --no-loopcount turns looping off; a count cannot do that).
cat > "$WORK/play_once.conf" <<EOF
mode = auto
loopcount = -2
input = $SRC_GIF
output = $WORK/once_out.gif
EOF
set +e
"$CLI" "$WORK/play_once.conf" --run --engine "$ENGINE" >"$WORK/lo.txt" 2>"$WORK/loerr.txt"
rc_lo=$?
set -e
"$ENGINE" --info "$WORK/once_out.gif" >"$WORK/loinfo.txt" 2>&1
cat > "$WORK/play_forever.conf" <<EOF
mode = auto
loopcount = 0
input = $SRC_GIF
output = $WORK/forever_out.gif
EOF
"$CLI" "$WORK/play_forever.conf" --run --engine "$ENGINE" >/dev/null 2>&1
"$ENGINE" --info "$WORK/forever_out.gif" >"$WORK/lfinfo.txt" 2>&1
# The command line goes to STDERR under --run (stdout must stay byte-pure, U-04),
# so the emitted flag is checked in a separate print-mode run instead.
"$CLI" "$WORK/play_once.conf" --engine "$ENGINE" >"$WORK/loprint.txt" 2>/dev/null
if [[ "$rc_lo" -eq 0 ]] && grep -q -- '--no-loopcount' "$WORK/loprint.txt" \
   && ! grep -qi 'loop' "$WORK/loinfo.txt" && grep -qi 'loop forever' "$WORK/lfinfo.txt"; then
  ok "U-63 play-once runs: no loop extension written (forever still loops)"
else
  bad "U-63 play-once wrong (rc=$rc_lo; printed=$(grep -o "\S*loopcount\S*" "$WORK/loprint.txt"|tr "\n" " "); once-loop=$(grep -ci loop "$WORK/loinfo.txt") forever-loop=$(grep -ci loop "$WORK/lfinfo.txt"))"
fi

# 12i. A number too wide for the model warns instead of wrapping (GS-206 /
#      P1-28). 4294967296 is 2^32: the old long->int cast made it 0, which the
#      engine reads as "loop forever" — a silent data-visible change.
cat > "$WORK/huge_loop.conf" <<EOF
mode = auto
loopcount = 4294967296
input = $SRC_GIF
EOF
set +e
"$CLI" "$WORK/huge_loop.conf" --engine "$ENGINE" >"$WORK/hl.txt" 2>"$WORK/hlerr.txt"
rc_hl=$?
"$CLI" "$WORK/huge_loop.conf" --strict --engine "$ENGINE" >/dev/null 2>"$WORK/hls.txt"
rc_hls=$?
set -e
if [[ "$rc_hl" -eq 0 && "$rc_hls" -eq 3 ]] \
   && grep -q "outside the range this build can hold in an int" "$WORK/hlerr.txt" \
   && ! grep -q -- '--loopcount' "$WORK/hl.txt"; then
  ok "P1-28 oversized loopcount warns (and --strict refuses); no wrapped value reaches the engine"
else
  bad "P1-28 oversized loopcount not caught (rc=$rc_hl strict=$rc_hls)"
fi

# 12j. A relative input that only exists in the CWD is announced, not silently
#      honoured (U-73 / P1-43): the conf's own directory is the documented
#      anchor, and the old fallback made the same conf mean different things from
#      two directories. Non-strict warns and proceeds; --strict refuses with 3.
mkdir -p "$WORK/u73conf" "$WORK/u73cwd"
cp "$SRC_GIF" "$WORK/u73cwd/logo.gif"
cat > "$WORK/u73conf/a.conf" <<EOF
mode = auto
input = logo.gif
EOF
set +e
(cd "$WORK/u73cwd" && "$CLI" "$WORK/u73conf/a.conf" --engine "$ENGINE") >"$WORK/u73.txt" 2>"$WORK/u73err.txt"
rc73=$?
(cd "$WORK/u73cwd" && "$CLI" "$WORK/u73conf/a.conf" --strict --engine "$ENGINE") >/dev/null 2>"$WORK/u73s.txt"
rc73s=$?
set -e
if [[ "$rc73" -eq 0 && "$rc73s" -eq 3 ]] \
   && grep -q "resolves against the CWD, not the settings file" "$WORK/u73err.txt" \
   && grep -q "$WORK/u73cwd/logo.gif" "$WORK/u73.txt"; then
  ok "U-73 CWD-resolved input warns (and --strict refuses) instead of silently diverging"
else
  bad "U-73 CWD fallback not announced (rc=$rc73 strict=$rc73s)"
fi

# 12k. Batch with N>1 inputs and ONE output is refused (U-74 / P1-43). Measured
#      on the bundled engine: `gifsicle -b a.gif b.gif -o out.gif` exits 0 and
#      out.gif is a byte copy of b.gif — a.gif's result simply does not exist.
cp "$SRC_GIF" "$WORK/u74a.gif"; cp "$SRC_GIF1" "$WORK/u74b.gif"
md5a_before="$(md5sum "$WORK/u74a.gif" | cut -d' ' -f1)"
cat > "$WORK/u74.conf" <<EOF
mode = batch
input = $WORK/u74a.gif
input = $WORK/u74b.gif
output = $WORK/u74out.gif
optimize = 2
EOF
set +e
"$CLI" "$WORK/u74.conf" --run --engine "$ENGINE" >/dev/null 2>"$WORK/u74err.txt"
rc74=$?
set -e
if [[ "$rc74" -eq 2 && ! -e "$WORK/u74out.gif" ]] \
   && grep -q "only the LAST input" "$WORK/u74err.txt" \
   && [[ "$(md5sum "$WORK/u74a.gif" | cut -d' ' -f1)" == "$md5a_before" ]]; then
  ok "U-74 Batch + N inputs + 1 output refused (rc=2), nothing written, sources intact"
else
  bad "U-74 Batch merge-shape not refused (rc=$rc74, out=$([[ -e $WORK/u74out.gif ]] && echo y || echo n))"
fi

# 12l. One input + Batch + output still runs: the refusal above is about the
#      N-to-1 shape, not about Batch with an output (which is what U-01 plans).
cat > "$WORK/u74ok.conf" <<EOF
mode = batch
input = $WORK/u74b.gif
output = $WORK/u74ok.gif
optimize = 2
EOF
set +e
"$CLI" "$WORK/u74ok.conf" --run --engine "$ENGINE" >/dev/null 2>"$WORK/u74okerr.txt"
rc74ok=$?
set -e
if [[ "$rc74ok" -eq 0 && -s "$WORK/u74ok.gif" ]] \
   && [[ "$(md5sum "$WORK/u74b.gif" | cut -d' ' -f1)" == "$(md5sum "$SRC_GIF1" | cut -d' ' -f1)" ]]; then
  ok "U-74 does not over-refuse: Batch + 1 input + output runs and leaves the source alone"
else
  bad "U-74 over-refused the legal single-input batch shape (rc=$rc74ok)"
fi

# 12m. The advisory contract is greppable (DS-08 / P3-5): a warned run still
#      exits 0 — that is deliberate and unchanged — so it also ends with ONE
#      `WARNING-SUMMARY:` line a script can test. A clean run prints none, and
#      --strict refuses before the summary exists.
cat > "$WORK/warnsum.conf" <<EOF
mode = auto
unoptimize = maybe
input = $SRC_GIF
EOF
set +e
"$CLI" "$WORK/warnsum.conf" --engine "$ENGINE" >/dev/null 2>"$WORK/ws.txt"
"$CLI" "$WORK/warnsum.conf" --run --engine "$ENGINE" >/dev/null 2>"$WORK/wsrun.txt"
"$CLI" "$WORK/warnsum.conf" --strict --engine "$ENGINE" >/dev/null 2>"$WORK/wss.txt"
cat > "$WORK/clean.conf" <<EOF
mode = auto
input = $SRC_GIF
EOF
"$CLI" "$WORK/clean.conf" --engine "$ENGINE" >/dev/null 2>"$WORK/wsclean.txt"
set -e
if grep -q "WARNING-SUMMARY: parse=1 validation=0 mode=advisory outcome=print-continued" "$WORK/ws.txt" \
   && grep -q "WARNING-SUMMARY: parse=1 validation=0 mode=advisory outcome=run-continued" "$WORK/wsrun.txt" \
   && grep -q "WARNING: settings key 'unoptimize' value 'maybe'" "$WORK/ws.txt" \
   && ! grep -q "WARNING-SUMMARY" "$WORK/wsclean.txt" \
   && ! grep -q "WARNING-SUMMARY" "$WORK/wss.txt"; then
  ok "DS-08 greppable warning summary (print/run), absent when clean and under --strict"
else
  bad "DS-08 warning summary wrong (print:$(grep -c WARNING-SUMMARY "$WORK/ws.txt") run:$(grep -c WARNING-SUMMARY "$WORK/wsrun.txt") clean:$(grep -c WARNING-SUMMARY "$WORK/wsclean.txt") strict:$(grep -c WARNING-SUMMARY "$WORK/wss.txt"))"
fi

# 12n. Engine discovery survives a symlink install (U-65 / P1-41). The real
#      binary lives in $WORK/inst with its engine beside it; $WORK/link holds a
#      symlink used as the command. argv[0] alone points at $WORK/link, where
#      there is no engine — the old code searched THAT directory and gave up.
mkdir -p "$WORK/inst" "$WORK/link"
cp "$CLI" "$WORK/inst/gifscythe-cli"
cp "$ENGINE" "$WORK/inst/gifsicle"
ln -sf "$WORK/inst/gifscythe-cli" "$WORK/link/gifscythe-cli"
cat > "$WORK/symlink.conf" <<EOF
mode = auto
input = $SRC_GIF
output = $WORK/symlink_out.gif
EOF
set +e
"$WORK/link/gifscythe-cli" "$WORK/symlink.conf" --run >"$WORK/sl.txt" 2>"$WORK/slerr.txt"
rc_sl=$?
set -e
if [[ "$rc_sl" -eq 0 && -s "$WORK/symlink_out.gif" ]] \
   && grep -q "$WORK/inst/gifsicle" "$WORK/slerr.txt" \
   && grep -q "# Engine source: bundled/release" "$WORK/slerr.txt"; then
  ok "U-65 symlinked CLI still finds the engine beside the REAL binary (OS path, not argv[0])"
else
  bad "U-65 symlink install lost the engine (rc=$rc_sl; $(tail -2 "$WORK/slerr.txt" | head -1))"
fi

# 12o. release/current is honoured, and it is honoured BEFORE the versioned
#      directory (U-66 / P1-41): one pin, every surface. Both candidate engines
#      exist here so the assertion is about ORDER, not about availability.
mkdir -p "$WORK/pin/release/current" "$WORK/pin/release/0.1.0"
cp "$ENGINE" "$WORK/pin/release/current/gifsicle"
cp "$ENGINE" "$WORK/pin/release/0.1.0/gifsicle"
cat > "$WORK/pin.conf" <<EOF
mode = auto
input = $SRC_GIF
output = $WORK/pin_out.gif
EOF
set +e
(cd "$WORK/pin" && "$CLI" "$WORK/pin.conf" --run) >"$WORK/pin.txt" 2>"$WORK/pinerr.txt"
rc_pin=$?
set -e
if [[ "$rc_pin" -eq 0 && -s "$WORK/pin_out.gif" ]] \
   && grep -q "release/current/gifsicle" "$WORK/pinerr.txt"; then
  ok "U-66 release/current wins over release/<version> for the CLI too"
else
  bad "U-66 current pin not preferred (rc=$rc_pin; $(grep -o 'release/[^ ]*gifsicle' "$WORK/pinerr.txt" | head -1))"
fi

# 13. Explode E2E (audit U-17): real engine writes prefix.NNN frames and the
#    CLI reports the verified count on stderr. logo.gif has 12 frames.
mkdir -p "$WORK/ex"
cat > "$WORK/explode.conf" <<EOF
mode = explode
input = $SRC_GIF
output = $WORK/ex/f
EOF
set +e
"$CLI" "$WORK/explode.conf" --run --engine "$ENGINE" >"$WORK/out9.txt" 2>"$WORK/err9.txt"
rc=$?
set -e
if [[ "$rc" -eq 0 && -s "$WORK/ex/f.000" && -s "$WORK/ex/f.011" ]] \
   && grep -q "12 frame(s)" "$WORK/err9.txt"; then
  ok "explode writes frames and reports the verified count (rc=0, f.000..f.011)"
else
  bad "explode E2E (rc=$rc, f.000=$([[ -s $WORK/ex/f.000 ]] && echo y || echo n), stderr=$(tail -1 "$WORK/err9.txt"))"
fi

# 14. Explode with a LYING engine (exits 0, writes nothing) must NOT exit 0
#     (audit U-17: rc=0 + zero frames used to mean success).
printf '#!/bin/sh\nexit 0\n' > "$WORK/lie.sh"
chmod +x "$WORK/lie.sh"
rm -f "$WORK"/ex/f.*
set +e
"$CLI" "$WORK/explode.conf" --run --engine "$WORK/lie.sh" >"$WORK/out10.txt" 2>"$WORK/err10.txt"
rc=$?
set -e
if [[ "$rc" -eq 1 ]] && grep -q "wrote no frames" "$WORK/err10.txt" \
   && grep -q "$WORK/ex/f" "$WORK/err10.txt"; then
  ok "lying engine (rc=0, zero frames) is refused: exit 1, error names the prefix"
else
  bad "lying-engine explode not refused honestly (rc=$rc, stderr=$(tail -2 "$WORK/err10.txt" | head -1))"
fi

# 15. Explode with EMPTY output (U-76 / P1-43). It used to inherit the engine's
#     own fallback — `<input basename>.NNN` in the CWD, extension and all, so
#     logo.gif produced logo.gif.000 next to wherever the CLI happened to run,
#     while the desktop and the web wrote <stem>_frame.NNN beside the input.
#     Now the NAME is shared (`<stem>_frame`) while the DIRECTORY stays the CWD,
#     which is both the engine's convention and safe: beside-the-input wrote 12
#     frames into reference_code/ the first time it was tried, so it is refused
#     here and left as owner decision OD-18. The input is copied into $WORK/src
#     so the assertion "nothing new appears next to the input" is meaningful.
mkdir -p "$WORK/cwd" "$WORK/src"
cp "$SRC_GIF" "$WORK/src/logo.gif"
cat > "$WORK/explode_noout.conf" <<EOF
mode = explode
input = $WORK/src/logo.gif
EOF
set +e
(cd "$WORK/cwd" && "$CLI" "$WORK/explode_noout.conf" --run --engine "$ENGINE" >"$WORK/out11.txt" 2>"$WORK/err11.txt")
rc=$?
set -e
if [[ "$rc" -eq 0 && -s "$WORK/cwd/logo_frame.000" && -s "$WORK/cwd/logo_frame.011" ]] \
   && grep -q "12 frame(s)" "$WORK/err11.txt" \
   && grep -q "no explode prefix given" "$WORK/err11.txt" \
   && [[ -z "$(ls -A "$WORK/src" | grep -v '^logo.gif$' | tr -d '\n')" ]]; then
  ok "explode with empty output writes <CWD>/logo_frame.NNN (name matches the desktop; input dir untouched)"
else
  bad "explode empty-output prefix rule (rc=$rc, frames=$([[ -s $WORK/cwd/logo_frame.000 ]] && echo y || echo n), src=$(ls -A "$WORK/src" | tr '\n' ' '))"
fi

# 16. Multi-input explode is REFUSED by --run (N-05): the engine exits 0 but
#     scatters every input except the last into the CWD. Print mode keeps its
#     documented policy: warn, then print.
cat > "$WORK/explode_multi.conf" <<EOF
mode = explode
input = $SRC_GIF
input = $SRC_GIF1
output = $WORK/ex/m
EOF
set +e
(cd "$WORK" && "$CLI" "$WORK/explode_multi.conf" --run --engine "$ENGINE" >"$WORK/out12.txt" 2>"$WORK/err12.txt")
rc_run=$?
"$CLI" "$WORK/explode_multi.conf" --engine "$ENGINE" >"$WORK/out12p.txt" 2>"$WORK/err12p.txt"
rc_print=$?
set -e
if [[ "$rc_run" -eq 2 ]] && grep -q "scatters frames" "$WORK/err12.txt" \
   && ! ls "$WORK"/m.* >/dev/null 2>&1 && ! ls "$WORK"/logo.gif.[0-9]* >/dev/null 2>&1 \
   && ! ls "$WORK"/logo1.gif.[0-9]* >/dev/null 2>&1; then
  ok "multi-input explode refused by --run (rc=2), no frames written anywhere"
else
  bad "multi-input explode not refused honestly (rc=$rc_run)"
fi
if [[ "$rc_print" -eq 0 ]] && grep -q "WARNING: mode=explode" "$WORK/err12p.txt" \
   && grep -q -- "-e" "$WORK/out12p.txt"; then
  ok "multi-input explode print mode warns (documented policy) and still prints"
else
  bad "multi-input explode print-mode policy wrong (rc=$rc_print)"
fi

# 17. Batch with no output is REFUSED by --run (GS-201 / P0-5): CLI Batch
#     maps to engine -b, and with no output key the planner is skipped, so
#     the engine would rewrite the source GIF. Print mode still prints.
cp "$SRC_GIF" "$WORK/batch_src.gif"
cp "$WORK/batch_src.gif" "$WORK/batch_src.before.gif"
cat > "$WORK/batch_noout.conf" <<EOF
mode = batch
input = $WORK/batch_src.gif
EOF
set +e
"$CLI" "$WORK/batch_noout.conf" --run --engine "$ENGINE" >"$WORK/out13.txt" 2>"$WORK/err13.txt"
rc_batch=$?
"$CLI" "$WORK/batch_noout.conf" --engine "$ENGINE" >"$WORK/out13p.txt" 2>"$WORK/err13p.txt"
rc_batch_print=$?
set -e
if [[ "$rc_batch" -eq 2 ]] && cmp -s "$WORK/batch_src.gif" "$WORK/batch_src.before.gif" \
   && grep -q "Batch with no output" "$WORK/err13.txt" \
   && grep -q "in-place -b" "$WORK/err13.txt"; then
  ok "Batch with no output refused by --run (rc=2), source GIF untouched"
else
  bad "Batch with no output not refused honestly (rc=$rc_batch)"
fi
if [[ "$rc_batch_print" -eq 0 ]] && grep -q -- "-b" "$WORK/out13p.txt"; then
  ok "Batch with no output print mode still prints (documented policy)"
else
  bad "Batch with no output print-mode policy wrong (rc=$rc_batch_print)"
fi

# 18. GS-207: invalid explicit environment overrides must never fall back to
# the real release engine or the working PATH engine. Print also fails clearly.
mkdir -p "$WORK/engine choice"
printf 'not executable\n' > "$WORK/not-executable"
chmod 600 "$WORK/not-executable"
for invalid in "$WORK/missing-engine" "$WORK/engine choice" "$WORK/not-executable" "gifsicle" "   "; do
  set +e
  (cd "$WORK" && GS_ENGINE="$invalid" PATH="$WORK/path-only" "$CLI" "$WORK/stdout.conf" --run \
    >"$WORK/env_run.out" 2>"$WORK/env_run.err")
  rc_run=$?
  (cd "$WORK" && GS_ENGINE="$invalid" PATH="$WORK/path-only" "$CLI" "$WORK/stdout.conf" \
    >"$WORK/env_print.out" 2>"$WORK/env_print.err")
  rc_print=$?
  set -e
  if [[ "$rc_run" -eq 1 && "$rc_print" -eq 1 && ! -s "$WORK/env_run.out" && ! -s "$WORK/env_print.out" ]] \
     && grep -q 'GS_ENGINE.*refusing automatic fallback' "$WORK/env_run.err" \
     && grep -Fq -- "$invalid" "$WORK/env_run.err" \
     && grep -q 'GS_ENGINE' "$WORK/env_print.err"; then
    ok "GS-207 invalid GS_ENGINE refused in print+run: [$invalid]"
  else bad "GS-207 override fell back or lacked a diagnostic: [$invalid] (run=$rc_run print=$rc_print)"; fi
done

# A distinct real-engine wrapper proves the selected override actually runs;
# check both absolute and CWD-relative paths containing spaces.
cat > "$WORK/engine choice/selected engine" <<EOF
#!/bin/sh
printf selected >> "$WORK/selected.log"
exec "$ENGINE" "\$@"
EOF
chmod +x "$WORK/engine choice/selected engine"
for chosen in "$WORK/engine choice/selected engine" "engine choice/selected engine"; do
  rm -f "$WORK/selected.log"
  set +e
  (cd "$WORK" && GS_ENGINE="$chosen" "$CLI" "$WORK/stdout.conf" --run \
    >"$WORK/env_valid.gif" 2>"$WORK/env_valid.err")
  rc=$?
  set -e
  if [[ "$rc" -eq 0 && -s "$WORK/selected.log" ]] && cmp -s "$WORK/env_valid.gif" "$WORK/direct.gif" \
     && grep -q '# Engine source: GS_ENGINE' "$WORK/env_valid.err"; then
    ok "GS-207 exact override selected with binary-pure stdout: [$chosen]"
  else bad "GS-207 valid override failed (rc=$rc): [$chosen]"; fi
done

# CLI --engine retains highest priority; print mode may display a prospective
# explicit --engine path without requiring that path to exist.
set +e
GS_ENGINE="$WORK/missing-engine" "$CLI" "$WORK/stdout.conf" --run --engine "$ENGINE" \
  >"$WORK/flag.gif" 2>"$WORK/flag.err"
rc_flag=$?
GS_ENGINE="$WORK/missing-engine" "$CLI" "$WORK/stdout.conf" --engine "$WORK/prospective" \
  >"$WORK/flag-print.out" 2>"$WORK/flag-print.err"
rc_flag_print=$?
set -e
if [[ "$rc_flag" -eq 0 && "$rc_flag_print" -eq 0 ]] && cmp -s "$WORK/flag.gif" "$WORK/direct.gif" \
   && grep -q '# Engine source: --engine' "$WORK/flag.err" \
   && grep -Fq "$WORK/prospective" "$WORK/flag-print.out"; then
  ok "GS-207 --engine takes precedence over invalid GS_ENGINE (run and prospective print)"
else bad "GS-207 --engine precedence regressed"; fi

# Empty is deliberately equivalent to unset, including PATH-only discovery.
set +e
(cd "$WORK" && GS_ENGINE='' PATH="$WORK/path-only" "$WORK/path-app/gifscythe-cli" "$WORK/stdout.conf" --run \
  >"$WORK/env_empty.gif" 2>"$WORK/env_empty.err")
rc=$?
set -e
if [[ "$rc" -eq 0 ]] && cmp -s "$WORK/env_empty.gif" "$WORK/direct.gif" \
   && grep -q '# Engine source: PATH' "$WORK/env_empty.err"; then
  ok "GS-207 empty override preserves PATH discovery"
else bad "GS-207 empty override did not discover PATH engine"; fi

# GS-203 file-output postconditions. Fake engines are real processes; an old
# valid output must not make an exit-zero/no-write process appear successful.
if "$self/scripts/test_output_verify.sh" >"$WORK/verifier-unit.log" 2>&1; then
  ok "GS-203 core output snapshot/signature assertions"
else bad "GS-203 core output verifier: $(cat "$WORK/verifier-unit.log")"; fi
printf '#!/bin/sh\nexit 0\n' > "$WORK/no-write-engine"
chmod +x "$WORK/no-write-engine"
cat > "$WORK/bogus-engine" <<'EOF'
#!/bin/sh
while [ "$#" -gt 0 ]; do
  if [ "$1" = -o ]; then shift; printf 'not a GIF' > "$1"; exit 0; fi
  shift
done
exit 1
EOF
chmod +x "$WORK/bogus-engine"
for mode in auto merge batch; do
  cat > "$WORK/verify.conf" <<EOF
mode = $mode
input = $SRC_GIF
output = $WORK/verify.gif
EOF
  for shape in missing stale bogus; do
    rm -f "$WORK/verify.gif"
    engine="$WORK/no-write-engine"
    [[ "$shape" == stale ]] && cp "$SRC_GIF" "$WORK/verify.gif"
    [[ "$shape" == bogus ]] && engine="$WORK/bogus-engine"
    set +e
    "$CLI" "$WORK/verify.conf" --run --engine "$engine" >"$WORK/verify.out" 2>"$WORK/verify.err"
    rc=$?
    set -e
    if [[ "$rc" == 1 ]] && grep -q 'output verification failed' "$WORK/verify.err" \
       && grep -Fq "$WORK/verify.gif" "$WORK/verify.err"; then
      ok "GS-203 $mode refuses $shape output after engine exit zero"
    else bad "GS-203 $mode/$shape wrong verdict (rc=$rc)"; fi
  done
done

# U-59 / P0-7 (P0 data loss): a run that dies mid-write must never damage a
# PRE-EXISTING output. The engine writes direct to `-o <target>`, so a cancel or
# a failed run used to leave a truncated file over the last good result. The
# engine must write a partial beside the target and the target may only be
# replaced on verified success.
U59_GOOD="$WORK/u59-good.gif"
U59_TARGET="$WORK/u59.gif"
U59_PARTIAL="$WORK/u59.gif.gs-partial"
cp "$SRC_GIF" "$U59_GOOD"
cat > "$WORK/u59.conf" <<EOF
mode = auto
input = $SRC_GIF
output = $U59_TARGET
EOF

# (a) engine writes garbage to -o then exits 0: the run must be refused AND the
#     pre-existing target must survive byte-identical, with no partial left.
cat > "$WORK/u59-bogus-engine" <<'EOF'
#!/bin/sh
while [ "$#" -gt 0 ]; do
  if [ "$1" = -o ]; then shift; printf 'not a GIF' > "$1"; exit 0; fi
  shift
done
exit 1
EOF
chmod +x "$WORK/u59-bogus-engine"
cp "$U59_GOOD" "$U59_TARGET"
rm -f "$U59_PARTIAL"
set +e
"$CLI" "$WORK/u59.conf" --run --engine "$WORK/u59-bogus-engine" >"$WORK/u59a.out" 2>"$WORK/u59a.err"
u59_rc=$?
set -e
if [[ "$u59_rc" == 1 ]] && cmp -s "$U59_GOOD" "$U59_TARGET" && [[ ! -e "$U59_PARTIAL" ]]; then
  ok "U-59 refused run leaves the pre-existing output byte-identical"
else
  bad "U-59 refused run damaged the pre-existing output (rc=$u59_rc cmp=$(cmp -s "$U59_GOOD" "$U59_TARGET" && echo same || echo DIFFERS) partial=$([[ -e "$U59_PARTIAL" ]] && echo left || echo none))"
fi

# (b) engine is killed by a signal mid-write: rc follows 128+signum (U-32) and
#     the pre-existing target must survive byte-identical.
cat > "$WORK/u59-die-engine" <<'EOF'
#!/bin/sh
while [ "$#" -gt 0 ]; do
  if [ "$1" = -o ]; then shift; printf 'partial' > "$1"; kill -TERM $$; fi
  shift
done
exit 0
EOF
chmod +x "$WORK/u59-die-engine"
cp "$U59_GOOD" "$U59_TARGET"
rm -f "$U59_PARTIAL"
set +e
"$CLI" "$WORK/u59.conf" --run --engine "$WORK/u59-die-engine" >"$WORK/u59b.out" 2>"$WORK/u59b.err"
u59_rc=$?
set -e
if [[ "$u59_rc" -ne 0 ]] && cmp -s "$U59_GOOD" "$U59_TARGET" && [[ ! -e "$U59_PARTIAL" ]]; then
  ok "U-59 engine signalled mid-write leaves the pre-existing output intact (rc=$u59_rc)"
else
  bad "U-59 signalled run damaged the pre-existing output (rc=$u59_rc cmp=$(cmp -s "$U59_GOOD" "$U59_TARGET" && echo same || echo DIFFERS) partial=$([[ -e "$U59_PARTIAL" ]] && echo left || echo none))"
fi

# (c) the audit's literal scenario: Cancel (SIGTERM to the CLI) while the engine
#     is still writing. The last good output must keep its OLD bytes and no
#     partial may be left behind.
cat > "$WORK/u59-slow-engine" <<EOF
#!/bin/sh
echo \$\$ > "$WORK/u59-engine.pid"
while [ "\$#" -gt 0 ]; do
  if [ "\$1" = -o ]; then shift; printf 'truncated' > "\$1"; sleep 8; exit 0; fi
  shift
done
exit 1
EOF
chmod +x "$WORK/u59-slow-engine"
cp "$U59_GOOD" "$U59_TARGET"
rm -f "$U59_PARTIAL"
# NOTE: this block runs with `set -e` active (case 1 turns it on), and every
# command here is allowed to fail — a SIGTERM'd job makes `wait` return 143,
# which would otherwise abort the whole suite. Hence the explicit set +e window.
set +e
"$CLI" "$WORK/u59.conf" --run --engine "$WORK/u59-slow-engine" >"$WORK/u59c.out" 2>"$WORK/u59c.err" &
u59_pid=$!
u59_seen=0
for _ in $(seq 1 200); do
  # break as soon as the engine has touched SOMETHING: the partial (fixed build)
  # or the target itself (unfixed build, where the damage has already happened)
  if [[ -e "$U59_PARTIAL" ]] || ! cmp -s "$U59_GOOD" "$U59_TARGET"; then u59_seen=1; break; fi
  sleep 0.05
done
kill -TERM "$u59_pid" 2>/dev/null
wait "$u59_pid" 2>/dev/null
u59_rc=$?
if [[ -f "$WORK/u59-engine.pid" ]]; then kill "$(cat "$WORK/u59-engine.pid")" 2>/dev/null; fi
sleep 0.2
set -e
if [[ "$u59_seen" == 1 ]] && cmp -s "$U59_GOOD" "$U59_TARGET"; then
  ok "U-59 cancel mid-write keeps the last good output (old bytes intact)"
else
  bad "U-59 cancel mid-write damaged the pre-existing output (seen=$u59_seen rc=$u59_rc cmp=$(cmp -s "$U59_GOOD" "$U59_TARGET" && echo same || echo DIFFERS))"
fi

# (d) the partial a hard kill cannot clean up (the CLI was SIGTERM'd, so it ran
#     no cleanup) must be swept by the NEXT guarded run, and must never be
#     mistaken for an output: the run still produces the real target.
if [[ -e "$U59_PARTIAL" ]]; then
  set +e
  "$CLI" "$WORK/u59.conf" --run --engine "$ENGINE" >"$WORK/u59d.out" 2>"$WORK/u59d.err"
  u59_rc=$?
  set -e
  if [[ "$u59_rc" == 0 ]] && [[ ! -e "$U59_PARTIAL" ]] && [[ -s "$U59_TARGET" ]]; then
    ok "U-59 next run sweeps the partial a hard kill left behind"
  else
    bad "U-59 stale partial survived the next run (rc=$u59_rc partial=$([[ -e "$U59_PARTIAL" ]] && echo left || echo gone))"
  fi
else
  ok "U-59 cancel left no partial behind at all"
fi

# U-81 / P1-46: the explode frame verifier must honour the same exemptions the
# ordinary output verifier documents. `-o -` streams and `--info` writes text, so
# there are no frames to count — ungated, both were downgraded to rc=1 "engine
# exited 0 but wrote no frames" over an honest run.
cat > "$WORK/u81-stream.conf" <<EOF
mode = explode
input = $SRC_GIF
output = -
EOF
# MEASURED on the bundled 1.96: `gifsicle -e -o - in.gif` writes ZERO bytes to
# stdout and drops in.gif.000..011 into the CWD (rc=0). The intake note called
# this "an honest stdout run" — it is not; it is N-05's scatter class, so the CLI
# refuses it with a named reason instead of either lying or scattering.
u81_cwd="$WORK/u81cwd"
mkdir -p "$u81_cwd"
set +e
(cd "$u81_cwd" && "$CLI" "$WORK/u81-stream.conf" --run --engine "$ENGINE" >"$WORK/u81s.out" 2>"$WORK/u81s.err")
u81_rc=$?
set -e
u81_scatter="$(ls "$u81_cwd" 2>/dev/null | wc -l | tr -d ' ')"
if [[ "$u81_rc" -eq 2 ]] && grep -q "cannot stream to stdout" "$WORK/u81s.err" \
   && [[ "$u81_scatter" == 0 ]]; then
  ok "U-81 explode + output=- is refused by name and scatters nothing into the CWD"
else
  bad "U-81 explode output=- wrong contract (rc=$u81_rc expected 2; files_in_cwd=$u81_scatter; err=$(tail -1 "$WORK/u81s.err"))"
fi

# info=true + explode is a validation WARNING (Validate.h: --info cannot be
# combined with a mode option), so this runs in advisory mode; the point is that
# the explode verifier no longer turns the honest run into rc=1.
cat > "$WORK/u81-info.conf" <<EOF
mode = explode
input = $SRC_GIF
info = true
EOF
set +e
"$CLI" "$WORK/u81-info.conf" --run --engine "$ENGINE" >"$WORK/u81i.out" 2>"$WORK/u81i.err"
u81_rc=$?
set -e
# Also MEASURED, and also not what the intake note said: the engine ITSELF
# refuses this combination ("'--info' suppresses normal output, can't use with an
# output mode"), rc=1 with its own reason. So the honest contract is rc=1 from the
# engine — what must NOT happen is the frame verifier adding its own false
# "wrote no frames" on top of an honest engine refusal.
if [[ "$u81_rc" -eq 1 ]] && grep -q "suppresses normal output" "$WORK/u81i.err" \
   && ! grep -q "wrote no frames" "$WORK/u81i.err"; then
  ok "U-81 explode + info reports the ENGINE's own refusal, not a false frame failure"
else
  bad "U-81 explode+info wrong contract (rc=$u81_rc expected 1 with the engine's reason; err=$(tail -1 "$WORK/u81i.err"))"
fi

# U-83 / P3-15: batch + an explicit output + ONE input was neither refused nor
# pinned — the planner classifies it as the merge shape, so argv carries both -b
# and -o. Pin what the engine actually does: the output is written to the -o
# target and the SOURCE is left byte-identical.
cp "$SRC_GIF" "$WORK/u83-src.gif"
u83_before="$WORK/u83-src-before.gif"
cp "$WORK/u83-src.gif" "$u83_before"
cat > "$WORK/u83.conf" <<EOF
mode = batch
input = $WORK/u83-src.gif
output = $WORK/u83-out.gif
EOF
set +e
"$CLI" "$WORK/u83.conf" --run --engine "$ENGINE" >"$WORK/u83.out" 2>"$WORK/u83.err"
u83_rc=$?
set -e
if [[ "$u83_rc" -eq 0 ]] && [[ -s "$WORK/u83-out.gif" ]] && cmp -s "$u83_before" "$WORK/u83-src.gif"; then
  ok "U-83 batch + output + one input writes the -o target and leaves the source intact"
else
  bad "U-83 batch+output single input unpinned behaviour (rc=$u83_rc out=$([[ -s $WORK/u83-out.gif ]] && echo yes || echo no) source=$(cmp -s "$u83_before" "$WORK/u83-src.gif" && echo intact || echo CHANGED))"
fi

echo "==> Done. $PASS passed, $FAIL failed."
[[ "$FAIL" -eq 0 ]]
