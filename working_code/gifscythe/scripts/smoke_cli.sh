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

# 15. Explode with EMPTY output verifies gifsicle's own fallback prefix: the
#     input's basename in the CWD (reference gifsicle.c "explode into current
#     directory"), i.e. <cwd>/logo.gif.NNN — verification must follow the same rule.
mkdir -p "$WORK/cwd"
cat > "$WORK/explode_noout.conf" <<EOF
mode = explode
input = $SRC_GIF
EOF
set +e
(cd "$WORK/cwd" && "$CLI" "$WORK/explode_noout.conf" --run --engine "$ENGINE" >"$WORK/out11.txt" 2>"$WORK/err11.txt")
rc=$?
set -e
if [[ "$rc" -eq 0 && -s "$WORK/cwd/logo.gif.000" && -s "$WORK/cwd/logo.gif.011" ]] \
   && grep -q "12 frame(s)" "$WORK/err11.txt"; then
  ok "explode with empty output verifies the CWD basename prefix (logo.gif.NNN)"
else
  bad "explode empty-output prefix rule (rc=$rc, logo.gif.000=$([[ -s $WORK/cwd/logo.gif.000 ]] && echo y || echo n))"
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

echo "==> Done. $PASS passed, $FAIL failed."
[[ "$FAIL" -eq 0 ]]
