#!/usr/bin/env bash
#
# sweep_stale.sh - catch documents that were true when written and false when read.
#
#   ./scripts/sweep_stale.sh           check current-state docs; exit 1 on any stale claim
#   ./scripts/sweep_stale.sh --report  list findings only; always exit 0
#
# WHY THIS EXISTS
#   check_docs.sh keeps STATUS.md self-consistent with the repo, but a doc can
#   still say "CI green" (true when a session wrote it) and be read a week later
#   when main is red. Those stale claims are the input the NEXT session starts
#   from, so they are a correctness bug, not a cosmetic one. This sweep is the
#   complement to the gate: it scans the same "current-state" .md set, but for
#   claims that are only checkable against *reality* (workflow copies, quoted
#   register tallies, volatile "green/red/merged" wording, retired demo scoping,
#   and narrative-vs-register state), and names file:line + the fix. It NEVER
#   edits a doc - it reports.
#
#   Every rule was mutation-tested: inject the staleness, the rule must FAIL.
#
# Same ok/bad/skip + "==> N passed, N failed" shape as check_docs.sh. LC_ALL=C
# (byte-deterministic), no network, no writes.
#
set -uo pipefail
export LC_ALL=C

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="$(cd "$self/../.." && pwd)"
cd "$root"

REPORT=0
for arg in "$@"; do
  case "$arg" in
    --report) REPORT=1 ;;
    -h|--help) sed -n '2,32p' "${BASH_SOURCE[0]}"; exit 0 ;;
    *) echo "sweep_stale.sh: unknown argument '$arg' (try --report)" >&2; exit 2 ;;
  esac
done

PASS=0; FAIL=0; SKIP=0
ok()   { echo "  PASS [$1] $2"; PASS=$((PASS + 1)); }
bad()  { echo "  FAIL [$1] $2"; FAIL=$((FAIL + 1)); }
skip() { echo "  SKIP [$1] $2"; SKIP=$((SKIP + 1)); }
note() { echo "         $*"; }

STATUS_MD="$root/STATUS.md"
AUDIT_MD="$root/COMPILED_AUDIT.md"
WF="$root/.github/workflows/build.yml"
PROP="$root/docs/ci/build.yml.proposed"
MARKER="$root/docs/ci/PENDING_WORKFLOW_CHANGE.md"

# WHICH DOCS ARE "CURRENT STATE"
# Same source of truth as check_docs.sh (git, not a hardcoded list), but with
# one extra exclusion: docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md is a
# dated intake snapshot, and the append-only IMPROVEMENT_LOG.md quotes numbers
# that were true when each entry was written. Only dated snapshots + the log are
# excluded; every other tracked .md is live state and must survive the sweep.
EXCLUDED='^(docs/archive/|docs/audit/POST_S7_AUDIT\.md|docs/audit/CONSOLIDATED_AUDIT_2026-09-10\.md|docs/audit/FIX_PICK_2026-09-10\.md|docs/audit/REMEDIATION_2026-09-10\.md|docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12\.md|reference_code/|IMPROVEMENT_LOG\.md$)'

mapfile -t ALL_MD < <(git ls-files '*.md')
CURRENT_DOCS=()
for f in "${ALL_MD[@]}"; do
  [[ "$f" =~ $EXCLUDED ]] || CURRENT_DOCS+=("$f")
done

echo "==> Stale-claim sweep (current-state docs: true-when-written vs false-when-read)"

# ---------------------------------------------------------------------------
# S1. MARKER vs REALITY - the pending-workflow marker must track the workflow
#     copies. Class: finding N-01 (marker outlived the change it described).
# ---------------------------------------------------------------------------
if [[ ! -f "$WF" || ! -f "$PROP" ]]; then
  skip "S1" "workflow copies missing - cannot compare .github/workflows/build.yml to docs/ci/build.yml.proposed"
elif diff -q "$WF" "$PROP" >/dev/null 2>&1; then
  if [[ -f "$MARKER" ]]; then
    bad "S1" "workflow copies are byte-identical but docs/ci/PENDING_WORKFLOW_CHANGE.md still exists (class: finding N-01) - action: delete the marker in the commit that applies the change"
  else
    ok "S1" "workflow copies byte-identical and no stale pending-change marker"
  fi
else
  if [[ -f "$MARKER" ]]; then
    ok "S1" "workflow copies differ and docs/ci/PENDING_WORKFLOW_CHANGE.md declares the drift"
  else
    bad "S1" "workflow copies differ but docs/ci/PENDING_WORKFLOW_CHANGE.md is absent - action: create the marker (or re-sync the copies) so the drift is declared"
  fi
fi

# ---------------------------------------------------------------------------
# S2. QUOTED REGISTER COUNTS - a "<n> DONE ... <n> UNTRIAGED" four-cell tally
#     quoted in a current-state doc must equal STATUS.md's counts line. Only
#     the single-line four-cell form is matched, so dated/wrapped tallies
#     ("As of S13: ...") are left alone by design.
# ---------------------------------------------------------------------------
if [[ ! -f "$STATUS_MD" ]]; then
  skip "S2" "STATUS.md absent - no reference counts to compare against"
else
  counts_line="$(grep -E '^\*\*Counts' "$STATUS_MD" | head -1)"
  ref_done=$(grep -oE '[0-9]+ DONE' <<<"$counts_line" | grep -oE '^[0-9]+')
  ref_part=$(grep -oE '[0-9]+ PARTIAL' <<<"$counts_line" | grep -oE '^[0-9]+')
  ref_open=$(grep -oE '[0-9]+ OPEN' <<<"$counts_line" | grep -oE '^[0-9]+')
  ref_untri=$(grep -oE '[0-9]+ UNTRIAGED' <<<"$counts_line" | grep -oE '^[0-9]+')
  s2_bad=""
  for f in "${CURRENT_DOCS[@]}"; do
    [[ -f "$f" ]] || continue
    while IFS= read -r ln; do
      nums=($(grep -oE '[0-9]+[[:space:]]*DONE[^0-9]*[0-9]+[[:space:]]*PARTIAL[^0-9]*[0-9]+[[:space:]]*OPEN[^0-9]*[0-9]+[[:space:]]*UNTRIAGED' <<<"$ln" | grep -oE '[0-9]+'))
      [[ "${#nums[@]}" -eq 4 ]] || continue
      if [[ "${nums[0]}" != "$ref_done" || "${nums[1]}" != "$ref_part" || "${nums[2]}" != "$ref_open" || "${nums[3]}" != "$ref_untri" ]]; then
        s2_bad+="$f quotes ${nums[0]}/${nums[1]}/${nums[2]}/${nums[3]} (register says ${ref_done}/${ref_part}/${ref_open}/${ref_untri}); "
      fi
    done < "$f"
  done
  if [[ -z "$s2_bad" ]]; then
    ok "S2" "every quoted four-cell register tally matches STATUS.md's counts line"
  else
    bad "S2" "$s2_bad action: update the stale quote or re-run check_docs.sh --emit"
  fi
fi

# ---------------------------------------------------------------------------
# S3. VOLATILE CLAIMS NEED EVIDENCE - "CI green" / "is red" / "not yet merged"
#     etc. are only honest if a +/-1-line window carries EVIDENCE (run id,
#     PR #n, a date, a sha, a session, or an explicit historical marker) or
#     PRESCRIPTIVE wording. Word boundaries matter ("is red" must not match
#     "is redundant"); "gate"/"rule" are deliberately NOT prescriptive.
# ---------------------------------------------------------------------------
VOLATILE='\brelease-red\b|\bstill red\b|\bis red\b|\bmerge outstanding\b|\bopen against\b|\bgreen on both jobs\b|\bCI green\b|\bmain is green\b|\bstays red\b|\bnot yet merged\b'
EVIDENCE='[0-9]{6,}|PR[[:space:]#]+[0-9]+|[0-9]{4}-[0-9]{2}-[0-9]{2}|[0-9a-f]{7,}|S[0-9]+|superseded|historical|original report|at the time|recorded'
# PRESCRIPTIVE is case-SENSITIVE on purpose: "Stay with C++17" must not read as
# prescriptive "stays". "gate"/"rule" are deliberately absent (they caused a
# false negative on "Main is red and the gate is unhappy").
PRESCRIPTIVE='\bstays?\b|\bkeep|\bremain|\bmust\b|\bshould\b|\bwhen[ :]|\bfloor\b'

s3_findings=()
while IFS= read -r f; do
  [[ -f "$f" ]] || continue
  while IFS= read -r hit; do
    [[ -n "$hit" ]] || continue
    ln="${hit%%:*}"; content="${hit#*:}"
    [[ "$ln" =~ ^[0-9]+$ ]] || continue
    win="$(sed -n "$((ln-1)),$((ln+1))p" "$f" 2>/dev/null)"
    { grep -qiE "$EVIDENCE" <<<"$win" || grep -qE "$PRESCRIPTIVE" <<<"$win"; } && continue
    s3_findings+=("$f:$ln: $(printf '%s' "$content" | cut -c1-90)")
  done < <(grep -nE "$VOLATILE" "$f" 2>/dev/null)
done < <(printf '%s\n' "${CURRENT_DOCS[@]}")

if [[ ${#s3_findings[@]} -eq 0 ]]; then
  ok "S3" "no undated volatile claim (release-red / is red / CI green / not yet merged / ...) without evidence or prescriptive wording"
else
  bad "S3" "${s3_findings[0]} ... action: date each claim (S<session>/YYYY-MM-DD/run id/PR #n) or mark it historical/superseded"
  for l in "${s3_findings[@]:1}"; do note "$l"; done
fi

# ---------------------------------------------------------------------------
# S4. RETIRED CLAIMS NEED A MARKER - "demo only" / "not the product path"
#     etc. were retired when the owner made the web build a supported product
#     surface (S14). A doc still using them must carry a supersession marker
#     within +/-2 lines.
# ---------------------------------------------------------------------------
RETIRED='demo only|not the product path|is a demo|demo/parity harness|not the product'
SUPERSESSION='superseded|historical|was scoped|original report|S14|decision'

s4_findings=()
while IFS= read -r f; do
  [[ -f "$f" ]] || continue
  while IFS= read -r hit; do
    [[ -n "$hit" ]] || continue
    ln="${hit%%:*}"; content="${hit#*:}"
    [[ "$ln" =~ ^[0-9]+$ ]] || continue
    win="$(sed -n "$((ln-2)),$((ln+2))p" "$f" 2>/dev/null)"
    grep -qiE "$SUPERSESSION" <<<"$win" && continue
    s4_findings+=("$f:$ln: $(printf '%s' "$content" | cut -c1-90)")
  done < <(grep -nE "$RETIRED" "$f" 2>/dev/null)
done < <(printf '%s\n' "${CURRENT_DOCS[@]}")

if [[ ${#s4_findings[@]} -eq 0 ]]; then
  ok "S4" "no retired 'demo only / not the product path' claim without a supersession marker"
else
  bad "S4" "${s4_findings[0]} ... action: mark it superseded/historical (or name the S14 decision that retired it)"
  for l in "${s4_findings[@]:1}"; do note "$l"; done
fi

# ---------------------------------------------------------------------------
# S5. NARRATIVE vs REGISTER - a COMPILED_AUDIT.md narrative "**Status:**" block
#     that claims a finding FIXED/CORRECTED/DONE/RESOLVED while that finding's
#     §5 register row is not ✅/☑ is a lie waiting to be read. (This rule is
#     what caught U-06 and U-08 saying "FIXED (S8)" while §5 said PARTIAL.)
#     python3 heredoc; falls back to a note if COMPILED_AUDIT.md is absent.
# ---------------------------------------------------------------------------
if [[ ! -f "$AUDIT_MD" ]]; then
  skip "S5" "COMPILED_AUDIT.md absent - narrative-vs-register cross-check skipped"
elif ! command -v python3 >/dev/null 2>&1; then
  skip "S5" "python3 not available - narrative-vs-register cross-check skipped"
else
  s5_findings="$(python3 - "$AUDIT_MD" <<'PY'
import re, sys
lines = open(sys.argv[1], encoding='utf-8', errors='replace').read().splitlines()
glyphs = ['\u2705','\u2611','\u25d0','\u2b1c','\u23f8']  # ✅ ☑ ◐ ⬜ ⏸
fixed  = {'\u2705','\u2611'}

# §5 register: U-nn -> status glyph (rightmost glyph-leading cell in the row)
reg = {}
on = False
for ln in lines:
    if re.match(r'^## 6\.', ln):
        break
    if re.match(r'^## 5\.', ln):
        on = True
        continue
    if not on:
        continue
    m = re.match(r'^\|\s*\*\*(U-\d+)\*\*\s*\|', ln)
    if not m:
        continue
    glyph = None
    for c in ln.split('|'):
        c = c.strip()
        if c and c[0] in glyphs:
            glyph = c[0]
    if glyph:
        reg[m.group(1)] = glyph

# narrative blocks: "**Status:**" ... blank line / ---
word = re.compile(r'\b(FIXED|CORRECTED|DONE|RESOLVED)\b')
blocks, cur = [], None
for ln in lines:
    if ln.startswith('**Status:**'):
        cur = [ln]
    elif cur is not None:
        if ln.strip() == '' or ln.strip().startswith('---'):
            blocks.append('\n'.join(cur)); cur = None
        else:
            cur.append(ln)
if cur is not None:
    blocks.append('\n'.join(cur))

out = []
for blk in blocks:
    if not any(g in blk for g in fixed):
        continue
    if not word.search(blk):
        continue
    for uid in sorted(set(re.findall(r'\bU-\d+\b', blk))):
        g = reg.get(uid)
        if g is not None and g not in fixed:
            out.append(uid)
for uid in out:
    print(uid)
PY
)"
  if [[ -z "$s5_findings" ]]; then
    ok "S5" "no narrative claims FIXED/CORRECTED/DONE/RESOLVED while its §5 register row is not ✅/☑"
  else
    bad "S5" "narrative-vs-register mismatch: $(tr '\n' ' ' <<<"$s5_findings") - action: correct the narrative **Status:** line(s) to match §5 (see COMPILED_AUDIT.md)"
  fi
fi

# ---------------------------------------------------------------------------
# S6. REPORT-ONLY (never fails) - surface pending/outstanding/TODO wording so
#     a session sees what is still open, minus anything already marked
#     superseded/historical/at-the-time.
# ---------------------------------------------------------------------------
s6_hits=""
while IFS= read -r f; do
  [[ -f "$f" ]] || continue
  while IFS= read -r hit; do
    [[ -n "$hit" ]] || continue
    ln="${hit%%:*}"; content="${hit#*:}"
    [[ "$ln" =~ ^[0-9]+$ ]] || continue
    win="$(sed -n "$((ln-1)),$((ln+1))p" "$f" 2>/dev/null)"
    grep -qiE 'superseded|historical|at the time|evidence level' <<<"$win" && continue
    s6_hits+="$f:$ln: $(printf '%s' "$content" | cut -c1-70)\n"
  done < <(grep -nE '\b(pending|outstanding|TODO|still open|to be applied|next session)\b' "$f" 2>/dev/null)
done < <(printf '%s\n' "${CURRENT_DOCS[@]}")

note "report-only (informational, never a failure): pending/outstanding/TODO wording still in current-state docs"
printf '%b' "$s6_hits" | head -12 | sed 's/^/         /'

echo "==> Done. $PASS passed, $FAIL failed, $SKIP skipped."
if [[ "$REPORT" == "1" ]]; then
  exit 0
fi
[[ "$FAIL" -eq 0 ]]
