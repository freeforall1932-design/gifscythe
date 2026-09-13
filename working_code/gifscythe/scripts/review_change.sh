#!/usr/bin/env bash
#
# review_change.sh - read a change before accepting it. Never take a diff blindly.
#
#   ./scripts/review_change.sh                    review the working tree vs HEAD
#   ./scripts/review_change.sh --commit <sha>     review one commit
#   ./scripts/review_change.sh --range A..B       review a range
#   ./scripts/review_change.sh --patch FILE       review a patch file (git-apply format)
#   ./scripts/review_change.sh --pr N             review PR N (needs gh)
#   --report                                      report only; always exit 0
#
# WHY THIS EXISTS
#   sweep_stale.sh checks whether the DOCS still describe reality. Nothing checked
#   whether a CHANGE is any good. Two real regressions got through this repo before
#   that was noticed: PR #16 reworded the handoff header and silently switched the
#   base-commit claim out of gate G10's reach (G10 then matched 0 files and passed
#   vacuously for five PRs), and a patch arrived claiming "all 35 narrative status
#   lines match the 52 register rows" when the file had 44. Both are catchable by
#   measuring instead of reading.
#
#   RULE: every flag carries EVIDENCE - a measured number, the offending line, a
#   file:line. A suspicion with no measurement is not printed. This script never
#   edits anything; it reports and exits non-zero so a human must look.
#
#   R1  check logic changed - a gate/sweep verdict or matcher line was added,
#       removed or edited. Not automatically "bad": it means the change must be
#       re-proven by mutation test before it is accepted.
#   R2  vacuous matcher - a matcher a check depends on matches NOTHING in the
#       corpus, so that check passes without ever reading anything.
#   R3  countable claim - added prose states a count that can be measured here,
#       and the measurement disagrees.
#   R4  executable bit - a script gained or lost +x, or a new script lacks it.
#   R5  docs to update - which docs this change obliges you to re-check, derived
#       from what the diff touched, not from a fixed list.
#
set -uo pipefail
export LC_ALL=C

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="$(cd "$self/../.." && pwd)"
cd "$root"

MODE="worktree"; ARG=""; REPORT=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --commit) MODE="commit"; ARG="${2:-}"; shift ;;
    --range)  MODE="range";  ARG="${2:-}"; shift ;;
    --patch)  MODE="patch";  ARG="${2:-}"; shift ;;
    --pr)     MODE="pr";     ARG="${2:-}"; shift ;;
    --report) REPORT=1 ;;
    -h|--help) sed -n '2,40p' "${BASH_SOURCE[0]}"; exit 0 ;;
    *) echo "review_change.sh: unknown argument '$1'" >&2; exit 2 ;;
  esac
  shift
done
[[ -n "$ARG" || "$MODE" == "worktree" ]] || { echo "review_change.sh: $MODE needs an argument" >&2; exit 2; }

PASS=0; FAIL=0; SKIP=0
ok()   { echo "  PASS [$1] $2"; PASS=$((PASS + 1)); }
bad()  { echo "  FAIL [$1] $2"; FAIL=$((FAIL + 1)); }
skip() { echo "  SKIP [$1] $2"; SKIP=$((SKIP + 1)); }
note() { echo "         $*"; }

# ---------------------------------------------------------------------------
# Get the diff under review.
# ---------------------------------------------------------------------------
DIFF=""
case "$MODE" in
  worktree) DIFF="$(git diff HEAD 2>/dev/null)"; LABEL="working tree vs HEAD" ;;
  commit)   DIFF="$(git show --format= "$ARG" 2>/dev/null)"; LABEL="commit $ARG" ;;
  range)    DIFF="$(git diff "$ARG" 2>/dev/null)"; LABEL="range $ARG" ;;
  patch)    [[ -f "$ARG" ]] || { echo "review_change.sh: no such patch file: $ARG" >&2; exit 2; }
            DIFF="$(cat "$ARG")"; LABEL="patch $ARG" ;;
  pr)       command -v gh >/dev/null 2>&1 || { echo "review_change.sh: --pr needs gh" >&2; exit 2; }
            DIFF="$(gh pr diff "$ARG" 2>/dev/null)"; LABEL="PR #$ARG" ;;
esac

echo "==> Change review: $LABEL"
if [[ -z "$DIFF" ]]; then
  echo "  (no changes to review)"
  echo "==> Done. 0 passed, 0 failed, 0 skipped."
  exit 0
fi

# Corpus: same definition as sweep_stale.sh (git, minus dated snapshots + the log).
EXCLUDED='^(docs/archive/|docs/audit/POST_S7_AUDIT\.md|docs/audit/CONSOLIDATED_AUDIT_2026-09-10\.md|docs/audit/FIX_PICK_2026-09-10\.md|docs/audit/REMEDIATION_2026-09-10\.md|docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12\.md|reference_code/|IMPROVEMENT_LOG\.md$)'
mapfile -t ALL_MD < <(git ls-files '*.md')
CURRENT_DOCS=()
for f in "${ALL_MD[@]}"; do [[ "$f" =~ $EXCLUDED ]] || CURRENT_DOCS+=("$f"); done

# Files the diff touches, and the added/removed lines.
mapfile -t CHANGED < <(grep -E '^diff --git ' <<<"$DIFF" | sed 's|^diff --git a/||; s| b/.*$||')
ADDED="$(grep -E '^\+' <<<"$DIFF" | grep -vE '^\+\+\+')"
REMOVED="$(grep -E '^-' <<<"$DIFF" | grep -vE '^---')"

# ---------------------------------------------------------------------------
# R1. CHECK LOGIC CHANGED. Evidence: the changed line itself.
# ---------------------------------------------------------------------------
CHECKFILES='(check_docs\.sh|sweep_stale\.sh|verify_audit\.sh|pr_preflight\.sh|review_change\.sh|^\.githooks/)'
r1_files=()
for f in "${CHANGED[@]}"; do [[ "$f" =~ $CHECKFILES ]] && r1_files+=("$f"); done
if [[ ${#r1_files[@]} -eq 0 ]]; then
  skip "R1" "no gate/sweep/hook script in this change"
else
  r1_hits="$(awk '
    /^diff --git / { f=$3; sub(/^a\//,"",f); keep = (f ~ /(check_docs\.sh|sweep_stale\.sh|verify_audit\.sh|pr_preflight\.sh|review_change\.sh|^\.githooks\/)/) }
    keep && /^[+-]/ && !/^(\+\+\+|---)/ {
      if ($0 ~ /(ok|bad|skip) "[A-Z]+[0-9]/ || $0 ~ /grep -o[iE]*[ ]*'"'"'/ || $0 ~ /^(VOLATILE|EVIDENCE|PRESCRIPTIVE|EXCLUDED|SWEEP)=/) print f": "$0
    }' <<<"$DIFF")"
  if [[ -z "$r1_hits" ]]; then
    ok "R1" "$(IFS=,; echo "${r1_files[*]}") changed but no verdict/matcher line moved"
  else
    n="$(wc -l <<<"$r1_hits")"
    bad "R1" "$n check-logic line(s) changed in $(IFS=,; echo "${r1_files[*]}") - a check that is edited is a check that must be re-proven"
    note "evidence: mutation-test it (inject the failure it exists to catch, confirm FAIL) before accepting."
    while IFS= read -r l; do note "$(printf '%s' "$l" | cut -c1-118)"; done <<<"$(head -8 <<<"$r1_hits")"
  fi
fi

# ---------------------------------------------------------------------------
# R2. VACUOUS MATCHER. A check whose matcher matches nothing passes without
#     reading anything. Regexes are READ OUT OF the scripts where possible, so
#     this probe cannot drift away from the gate it is testing.
# ---------------------------------------------------------------------------
r2_bad=""
probe() { # name, regex, grep-flags
  local name="$1" re="$2" flags="${3:--E}" hits=0 f
  for f in "${CURRENT_DOCS[@]}"; do
    [[ -f "$f" ]] || continue
    c="$(grep -c $flags -- "$re" "$f" 2>/dev/null || true)"
    hits=$((hits + ${c:-0}))
  done
  if [[ "$hits" -eq 0 ]]; then
    r2_bad+="$name (0 matches across ${#CURRENT_DOCS[@]} docs); "
  else
    note "$name: $hits match(es) across ${#CURRENT_DOCS[@]} current-state docs"
  fi
}
# Extraction is ANCHORED on distinctive content, not on "the first grep line":
# check_docs.sh has three `grep -oE '...' <<<"$content"` lines, and taking the
# first one silently probed the wrong matcher (it reported 1827 hits for a
# pattern that really has 1). Each extraction is followed by a sanity check on
# the extracted text, so a future refactor degrades to "probe skipped" instead
# of quietly measuring something else.
extract_quoted() { # file, awk-regex identifying the line
  awk -v pat="$2" '$0 ~ pat { s=$0; sub(/.*grep -o[iE]+ ./,"",s); sub(/. <<<.*/,"",s); print s; exit }' "$1"
}
g10_re="$(extract_quoted "$self/scripts/check_docs.sh" 'based on\|base commit')"
# The probe must mirror the gate EXACTLY, including case-sensitivity. Hardcoding
# -iE here made the probe report 1 hit for a matcher the gate could not use at
# all (the gate is case-sensitive and the header says "Based on"), which is the
# one thing this probe exists to notice.
g10_flags="-E"
if grep -qE "grep -oiE '\(based on\|base commit" "$self/scripts/check_docs.sh" 2>/dev/null; then
  g10_flags="-iE"
fi
s2_re="$(sed -n "s/^  s2_re='\(.*\)'$/\1/p" "$self/scripts/sweep_stale.sh" | head -1)"
s3_re="$(sed -n "s/^VOLATILE='\(.*\)'$/\1/p" "$self/scripts/sweep_stale.sh" | head -1)"
if [[ -n "$g10_re" && "$g10_re" == *"base commit"* ]]; then
  probe "G10 base-commit matcher ($g10_flags, as the gate invokes it)" "$g10_re" "$g10_flags"
else
  note "G10: could not extract the base-commit matcher from check_docs.sh - probe skipped (not silently guessed)"
fi
if [[ -n "$s2_re" && "$s2_re" == *"DONE"* ]]; then probe "S2 register-tally matcher" "$s2_re"
else note "S2: could not extract s2_re from sweep_stale.sh - probe skipped"; fi
if [[ -n "$s3_re" && "$s3_re" == *"green"* ]]; then probe "S3 volatile-claim matcher" "$s3_re"
else note "S3: could not extract VOLATILE from sweep_stale.sh - probe skipped"; fi
probe "S5 narrative **Status:** blocks" '^\*\*Status:\*\*'
probe "G16 web plan template state" '^\*\*Template state:\*\* (SKELETON|WORKING PLAN)'
if [[ -z "$r2_bad" ]]; then
  ok "R2" "every probed matcher matches something - no check is passing vacuously"
else
  bad "R2" "vacuous matcher(s): $r2_bad"
  note "a matcher with 0 hits means that gate/sweep rule cannot fail. Either the doc wording"
  note "moved away from the pattern (PR #16 did this to G10) or the pattern is wrong."
fi

# ---------------------------------------------------------------------------
# R3. COUNTABLE CLAIM. Evidence: claimed vs measured.
# ---------------------------------------------------------------------------
measure_status_lines() { grep -c '^\*\*Status:\*\*' "$root/COMPILED_AUDIT.md" 2>/dev/null || echo 0; }
measure_register_rows() { awk '/^## 5\./{f=1;next} /^## 6\./{f=0} f' "$root/COMPILED_AUDIT.md" 2>/dev/null | grep -cE '^\| \*\*U-[0-9]+\*\* \|' || echo 0; }
r3_bad=""
while IFS= read -r line; do
  [[ -n "$line" ]] || continue
  # A count the line is correcting is not a count it asserts. IMPROVEMENT_LOG
  # quotes the false claim in order to refute it, and flagging that would train
  # reviewers to ignore R3. This is a heuristic and is documented as one: a bare
  # false claim with no contradiction marker on the same line is still flagged,
  # which is the case that matters.
  if grep -qiE 'claim|assert|quote|refut|not true|was false|were false|incorrect|correcting|misstat|does not match|did not|wrong' <<<"$line"; then
    continue
  fi
  # "<n> narrative status lines" / "<n> status lines"
  while read -r claimed; do
    [[ -n "$claimed" ]] || continue
    real="$(measure_status_lines)"
    [[ "$claimed" == "$real" ]] || r3_bad+="claims $claimed narrative status lines, measured $real; "
  done < <(grep -oE '[0-9]+ (narrative )?status lines' <<<"$line" | grep -oE '^[0-9]+')
  while read -r claimed; do
    [[ -n "$claimed" ]] || continue
    real="$(measure_register_rows)"
    [[ "$claimed" == "$real" ]] || r3_bad+="claims $claimed register rows, measured $real; "
  done < <(grep -oE '[0-9]+ register rows' <<<"$line" | grep -oE '^[0-9]+')
done <<<"$ADDED"
if [[ -z "$r3_bad" ]]; then
  ok "R3" "no added line states a measurable count that disagrees with the repo"
else
  bad "R3" "$r3_bad"
  note "evidence: measured just now from COMPILED_AUDIT.md. A count in prose is a claim; fix the"
  note "number or delete it - prose counts are exactly what goes stale."
fi

# ---------------------------------------------------------------------------
# R4. EXECUTABLE BIT. Evidence: the mode line, or the bit on disk.
# ---------------------------------------------------------------------------
r4_bad=""
while IFS= read -r f; do
  [[ -n "$f" ]] || continue
  case "$f" in *.sh) [[ -x "$root/$f" ]] || r4_bad+="$f is not executable; " ;; esac
done < <(grep -E '^diff --git ' <<<"$DIFF" | sed 's|^diff --git a/||; s| b/.*$||')
if grep -qE '^(old|new) mode ' <<<"$DIFF"; then
  r4_bad+="the diff changes a file mode: $(grep -E '^(old|new) mode ' <<<"$DIFF" | tr '\n' ' '); "
fi
if [[ -z "$r4_bad" ]]; then
  ok "R4" "no executable bit lost and no file-mode change"
else
  bad "R4" "$r4_bad"
  note "a gate script without +x makes G17 (and the pre-push hook) fail closed on a fresh clone."
fi

# ---------------------------------------------------------------------------
# R5. DOCS THIS CHANGE OBLIGES YOU TO UPDATE. Derived, not a fixed list.
# ---------------------------------------------------------------------------
echo "== R5: docs to update for this change"
touched() { local x; for x in "${CHANGED[@]}"; do [[ "$x" == "$1" ]] && return 0; done; return 1; }
for d in SESSION_HANDOFF.md IMPROVEMENT_LOG.md WORKLIST.md; do
  if touched "$d"; then note "  [touched]  $d - re-read it, do not assume the edit is complete"
  else note "  [MISSING]  $d - not in this change; update it or say in the PR body why not"; fi
done
# docs that reference a changed non-doc file
for f in "${CHANGED[@]}"; do
  case "$f" in *.md) continue ;; esac
  base="$(basename "$f")"
  refs="$(for d in "${CURRENT_DOCS[@]}"; do grep -lF "$base" "$d" 2>/dev/null; done | tr '\n' ' ')"
  [[ -n "$refs" ]] && note "  [referenced] $f is named in: $refs"
done
if touched "STATUS.md"; then note "  [touched]  STATUS.md - it is generated; edit COMPILED_AUDIT.md §5 then run check_docs.sh --emit"
else note "  [check]    STATUS.md - if this change closes or opens a finding, its row must move too"; fi
ok "R5" "doc obligations listed above"

# ---------------------------------------------------------------------------
# Summary: what changed (measured) and what the author CLAIMS was fixed.
# ---------------------------------------------------------------------------
echo "== Summary"
ndoc=0; ncode=0
for f in "${CHANGED[@]}"; do case "$f" in *.md) ndoc=$((ndoc+1));; *) ncode=$((ncode+1));; esac; done
note "files: ${#CHANGED[@]} changed ($ndoc doc, $ncode non-doc); +$(wc -l <<<"$ADDED") / -$(wc -l <<<"$REMOVED") lines"
if [[ "$MODE" == "commit" ]]; then
  note "author's claim (NOT verified by this script): $(git log -1 --format=%s "$ARG" 2>/dev/null)"
elif [[ "$MODE" == "pr" ]]; then
  note "author's claim (NOT verified by this script): $(gh pr view "$ARG" --json title -q .title 2>/dev/null)"
fi
note "'what was fixed' is never inferred from a diff. Verify it: run the gate, the sweep, and"
note "mutation-test anything R1 flagged. Only then write it into the docs R5 listed."

echo "==> Done. $PASS passed, $FAIL failed, $SKIP skipped."
[[ "$REPORT" == "1" ]] && exit 0
[[ "$FAIL" -eq 0 ]]
