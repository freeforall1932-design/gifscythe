#!/usr/bin/env bash
#
# check_docs.sh - the documentation status gate AND the STATUS.md emitter.
#
#   ./scripts/check_docs.sh          check the committed STATUS.md is current
#   ./scripts/check_docs.sh --emit   regenerate STATUS.md from the repo
#
# WHY THIS EXISTS
#   This repo used to record status in five places (COMPILED_AUDIT.md,
#   WORKLIST.md, SESSION_HANDOFF.md, IMPROVEMENT_LOG.md, PROJECT_VISION.md) and
#   they drifted: a hand-run check found COMPILED_AUDIT.md's header claiming
#   "21 findings closed" while its own register held 31, and naming a branch
#   that was no longer checked out. Stale docs are corrupted input for the next
#   session, not a cosmetic problem.
#
#   So: ONE register (STATUS.md, four states, machine-parseable), and this gate,
#   which derives every expected value from the repo. Nothing here hardcodes an
#   expected count - a checker full of literals fails on every legitimate change
#   and gets deleted within a month.
#
# THE CENTRAL TRICK
#   --emit regenerates STATUS.md. Plain mode regenerates to a temp file and
#   DIFFS it against the committed one, so a hand-fudged roll-up cannot pass.
#
# Exits non-zero on any FAIL. Same ok/bad/skip + "==> N passed, N failed"
# shape as scripts/verify_audit.sh.
#
set -uo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="$(cd "$self/../.." && pwd)"
cd "$root"

STATUS_MD="$root/STATUS.md"
AUDIT_MD="$root/COMPILED_AUDIT.md"
WORKLIST_MD="$root/WORKLIST.md"
HANDOFF_MD="$root/SESSION_HANDOFF.md"
LOG_MD="$root/IMPROVEMENT_LOG.md"
VERIFY="$self/scripts/verify_audit.sh"

EMIT=0
NESTED="${GS_FROM_VERIFY_AUDIT:-0}"
NO_GATE_RUN=0
for arg in "$@"; do
  case "$arg" in
    --emit) EMIT=1 ;;
    --from-verify-audit) NESTED=1 ;;
    # CI uses this: the linux job already builds, packages and runs the web
    # suites, so re-running verify_audit.sh inside the doc gate would duplicate
    # four of its steps and roughly double job time (the same reason the whole
    # of verify_audit.sh is deliberately not a CI step). G6 then reports SKIP;
    # the pre-push hook and a local plain run still execute it.
    --no-gate-run) NO_GATE_RUN=1 ;;
    -h|--help) sed -n '2,30p' "${BASH_SOURCE[0]}"; exit 0 ;;
    *) echo "check_docs.sh: unknown argument '$arg' (try --emit)" >&2; exit 2 ;;
  esac
done

PASS=0; FAIL=0; SKIP=0
ok()   { echo "  PASS [$1] $2"; PASS=$((PASS + 1)); }
bad()  { echo "  FAIL [$1] $2"; FAIL=$((FAIL + 1)); }
skip() { echo "  SKIP [$1] $2"; SKIP=$((SKIP + 1)); }
note() { echo "         $*"; }

# The four states. One vocabulary, no synonyms, no legacy markers leaking into
# STATUS.md. Legacy markers in COMPILED_AUDIT.md are MAPPED here (see state_of).
STATE_DONE=DONE; STATE_PARTIAL=PARTIAL; STATE_OPEN=OPEN; STATE_UNTRIAGED=UNTRIAGED

# ---------------------------------------------------------------------------
# WHICH DOCS ARE "CURRENT STATE" (and which are excluded ON PURPOSE)
#
# Everything below is derived from git, never from a hardcoded file list.
#
# Excluded, with the reason in each case - these files keep historical numbers
# by design, so scanning them makes this gate scream forever and teaches people
# to ignore it:
#
#   docs/archive/**                                  dated review snapshots
#                                                    (pre-`docs/` layout; U-44
#                                                    moved them here and the
#                                                    policy is to keep their
#                                                    historical line refs)
#   docs/audit/POST_S7_AUDIT.md                      dated in-repo audit, S7
#   docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md      dated snapshot of the
#                                                    44-finding register
#   docs/audit/FIX_PICK_2026-09-10.md                dated pick rationale,
#                                                    already marked superseded
#   docs/audit/REMEDIATION_2026-09-10.md             dated S8 session record.
#                                                    ADDED TO THE POLICY LIST
#                                                    THIS SESSION (S9) for the
#                                                    same reason as the three
#                                                    above: it quotes the gate
#                                                    numbers AS S8 MEASURED
#                                                    THEM (23/0/5, correct at
#                                                    the time, before the CI
#                                                    workflow change landed).
#                                                    It carries a superseded
#                                                    banner instead.
#   reference_code/gifsicle*/**                      upstream third-party docs
#                                                    (read-only source material)
#
# IMPROVEMENT_LOG.md is special: only its NEWEST entry is current state. Older
# per-session entries are an append-only history and quote the numbers that were
# true when they were written.
# ---------------------------------------------------------------------------
excluded_doc() {
  case "$1" in
    docs/archive/*) return 0 ;;
    docs/audit/POST_S7_AUDIT.md) return 0 ;;
    docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md) return 0 ;;
    docs/audit/FIX_PICK_2026-09-10.md) return 0 ;;
    docs/audit/REMEDIATION_2026-09-10.md) return 0 ;;
    reference_code/gifsicle/*|reference_code/gifsicle-nested-*/*) return 0 ;;
  esac
  return 1
}

mapfile -t ALL_TRACKED_MD < <(git ls-files '*.md')
CURRENT_DOCS=()
for f in "${ALL_TRACKED_MD[@]}"; do
  excluded_doc "$f" || CURRENT_DOCS+=("$f")
done

# IMPROVEMENT_LOG.md: newest entry only. The entry format is the one this gate
# enforces (## S<n> - <theme>  (<date>)), with the older "## <date> (S<n>) ..."
# form still accepted so the gate works on a not-yet-migrated log.
newest_log_entry() {
  awk '
    /^## / { if (n) { print buf; exit } ; n=1; buf=$0; next }
    n { buf = buf "\n" $0 }
    END { if (n) print buf }
  ' "$LOG_MD"
}

# ---------------------------------------------------------------------------
# REGISTER PARSING
#
# Rows are markdown table rows. Cells may contain escaped pipes (\|) - COMPILED
# AUDIT.md does exactly that - so split on UNESCAPED pipes only: swap \| for a
# byte that cannot occur in a doc, split, swap back.
# ---------------------------------------------------------------------------
row_fields() {  # stdin: one table row -> stdout: one field per line
  sed 's/\\|/\x01/g' | awk -F'|' '{ for (i = 2; i < NF; i++) { gsub(/\x01/, "|", $i); gsub(/^[ \t]+|[ \t]+$/, "", $i); print $i } }' | sed 's/\x01/|/g'
}

register_rows() {  # $1 = file; emit every data row of every register table
  grep -E '^\|[[:space:]]*[A-Z]+-[0-9]+[[:space:]]*\|' "$1" 2>/dev/null || true
}

# state_of: map a COMPILED_AUDIT.md status cell onto the four-state vocabulary.
# This is the ONLY place legacy markers are known about.
state_of() {
  case "$1" in
    *"✅ FIXED"*)    printf '%s' "$STATE_DONE" ;;
    *"☑ CORRECTED"*) printf '%s' "$STATE_DONE" ;;
    *"◐ PARTIAL"*)   printf '%s' "$STATE_PARTIAL" ;;
    *"⬜ OPEN"*)     printf '%s' "$STATE_OPEN" ;;
    *)               printf 'UNKNOWN' ;;
  esac
}

# ---------------------------------------------------------------------------
# DERIVED INPUTS
# ---------------------------------------------------------------------------

# COMPILED_AUDIT.md section 5 (the 52-row master register).
audit_section() {  # $1 = section number
  awk -v want="## $1\\." '
    $0 ~ "^" want { on=1; next }
    on && /^## [0-9]+\./ { exit }
    on { print }
  ' "$AUDIT_MD"
}

# COMPILED_AUDIT.md section 6 -> "U-nn  Pn-m  <first sentence of the action>"
# Used to derive a real Next action instead of an invented one.
# NOTE: the Closes column is located by SHAPE, not by position - P1-18's action
# text contains a literal unescaped pipe ("WriteOnly|Truncate"), so counting
# fields silently drops U-16 from the map. Verified: that exact bug happened.
fix_order_map() {
  audit_section 6 | awk -F'|' '
    /^[|][ ]*P[0-9]-[0-9]+[ ]*[|]/ {
      aid=$2; gsub(/^[ \t*]+|[ \t*]+$/, "", aid)
      closes_idx = 0
      for (i = 3; i <= NF; i++) {
        c = $i; gsub(/[* \t]/, "", c)
        if (c ~ /^U-[0-9]+(,U-[0-9]+)*$/) { closes_idx = i; break }
      }
      if (!closes_idx) next
      act = ""
      for (i = 3; i < closes_idx; i++) act = act (act == "" ? "" : "|") $i
      gsub(/\*\*/, "", act); gsub(/^[ \t]+|[ \t]+$/, "", act); gsub(/[ \t]+/, " ", act)
      if (match(act, /\.[ ]/)) act = substr(act, 1, RSTART)
      if (length(act) > 110) act = substr(act, 1, 107) "..."
      gsub(/[|]/, "\\|", act)
      # Split "U-01,U-45" into ids. The character class must list the dash
      # LAST: [^U-0-9] reads "U-0" as a range, which gawk (the CI awk) rejects
      # with "Invalid range end" while mawk silently accepts - CI-linux died
      # on it (PR #13). [^0-9U-] is the same class, portable to both.
      n = split($closes_idx, ids, /[^0-9U-]+/)
      for (i = 1; i <= n; i++) {
        if (ids[i] ~ /^U-[0-9]+$/ && !(ids[i] in seen)) { seen[ids[i]]=1; print ids[i] "\t" aid "\t" act }
      }
    }
  '
}

FIXMAP="$(mktemp)"; TMP_REG="$(mktemp)"
trap 'rm -f "$FIXMAP" "$TMP_REG" 2>/dev/null' EXIT
fix_order_map > "$FIXMAP"

# Newest session + date, derived from the newest IMPROVEMENT_LOG.md entry.
# Both the emitted header and check L1 hang off this, so a session that forgets
# to log cannot quietly keep yesterday's date in the register header.
LOG_SESSION="$(newest_log_entry | head -1 | grep -oE '\bS[0-9]+' | head -1)"
LOG_DATE="$(newest_log_entry | head -1 | grep -oE '[0-9]{4}-[0-9]{2}-[0-9]{2}' | head -1)"

# ---------------------------------------------------------------------------
# THE EMITTER
# ---------------------------------------------------------------------------
# Emits the derived block (U-nn rows, straight out of COMPILED_AUDIT.md) and
# preserves the hand-maintained block (W/D/R + anything found this session)
# verbatim, because those rows have no other machine-readable home.

emit_u_rows() {
  audit_section 5 | awk -v fixmap="$FIXMAP" -v dash="—" -v sec5="§5" -v sec6="§6" '
    function trim(s) { gsub(/^[ \t]+|[ \t]+$/, "", s); return s }
    function esc(s)  { gsub(/\\[|]/, "|", s); gsub(/[|]/, "\\|", s); return s }
    function first_sentence(s,   t) {
      gsub(/\*\*/, "", s); gsub(/^[ \t]+|[ \t]+$/, "", s)
      gsub(/[ \t]+/, " ", s)
      if (match(s, /\.[ ]/)) { t = substr(s, 1, RSTART); if (length(t) > 25) return t }
      if (length(s) > 130) {
        # truncate on a word boundary, never mid-character (mawk is byte-based)
        t = substr(s, 1, 130); sp = 0
        for (i = length(t); i > 60; i--) if (substr(t, i, 1) == " ") { sp = i; break }
        return (sp ? substr(t, 1, sp - 1) : t) "..."
      }
      return s
    }
    function after_dash(s,   i) { i = index(s, dash); return i ? trim(substr(s, i + length(dash))) : "" }
    BEGIN {
      while ((getline line < fixmap) > 0) {
        split(line, p, "\t"); fa[p[1]] = p[2]; fact[p[1]] = p[3]
      }
      close(fixmap)
    }
    /^[|][ ]*\*\*U-[0-9]+\*\*[ ]*[|]/ {
      # cells may contain escaped pipes (\|): unescape, split, restore
      line = $0; gsub(/\\[|]/, "\001", line)
      n = split(line, f, "|")
      for (i = 1; i <= n; i++) { gsub(/\001/, "|", f[i]); f[i] = trim(f[i]) }
      id = f[2]; gsub(/\*/, "", id)
      # locate the Status cell by SHAPE (it is the one carrying a legacy marker),
      # not by position: a row with an unescaped pipe in its Finding text would
      # otherwise be read off-by-one and silently mis-mapped.
      status_idx = 0
      for (i = n; i >= 3; i--) if (f[i] ~ /\xe2\x9c\x85|\xe2\xac\x9c|\xe2\x97\x90|\xe2\x98\x91/) { status_idx = i; break }
      if (!status_idx) { print "NOSTATUSCELL " id > "/dev/stderr"; next }
      if (n != 7) print "NOTE " id ": register row has " n " cells, expected 7" > "/dev/stderr"
      status = f[status_idx]
      finding = ""
      for (i = 4; i < status_idx - 1; i++) finding = finding (finding == "" ? "" : "|") f[i]

      # --- state (legacy marker -> four-state vocabulary) ---
      st = "UNKNOWN"
      if (status ~ /✅ FIXED/)         st = "DONE"
      else if (status ~ /☑ CORRECTED/) st = "DONE"
      else if (status ~ /◐ PARTIAL/)   st = "PARTIAL"
      else if (status ~ /⬜ OPEN/)     st = "OPEN"
      if (st == "UNKNOWN") { print "UNKNOWNSTATE " id ": " status > "/dev/stderr"; next }

      # --- session: from "(S8)" in the status cell, else "Fixed by S7" prose ---
      sess = "-"
      if (match(status, /\(S[0-9]+\)/))  sess = substr(status, RSTART+1, RLENGTH-2)
      else if (match(finding, /S[0-9]+/)) sess = substr(finding, RSTART, RLENGTH)

      # --- proof / blocker ---
      note = after_dash(status)
      if (length(note) > 150) note = substr(note, 1, 147) "..."

      tier = (id in fa) ? fa[id] : ""
      if (st == "DONE") {
        if (note == "") note = "closed; no proof note in COMPILED_AUDIT.md " sec5 " - add one"
        next_act = "-"
      } else if (st == "PARTIAL") {
        if (note == "") { print "NOPARTIALNOTE " id > "/dev/stderr"; note = "PARTIAL with no note - fix the audit row" }
        next_act = (tier != "") ? tier ": " fact[id] : "close the gap named in Proof / Blocker"
      } else {
        note = (tier != "") ? "not started; scoped as " tier " in COMPILED_AUDIT.md " sec6 \
                            : "not started; NOT scoped in COMPILED_AUDIT.md " sec6
        next_act = (tier != "") ? tier ": " fact[id] : "scope it in COMPILED_AUDIT.md " sec6 ", then work it"
      }
      printf "| %s | %s | %s | %s | %s | %s |\n", id, esc(first_sentence(finding)), st, sess, esc(note), esc(next_act)
    }
  '
}

emit_hand_rows() {  # preserve the hand-maintained block verbatim
  [[ -f "$STATUS_MD" ]] || return 0
  awk '
    /<!-- BEGIN HAND-MAINTAINED/ { on=1; next }
    /<!-- END HAND-MAINTAINED/   { on=0 }
    on { print }
  ' "$STATUS_MD"
}

count_state() {  # $1 = state; counts rows across BOTH tables of a register file
  register_rows "$1" | while IFS= read -r r; do printf '%s\n' "$r" | row_fields | sed -n 3p; done \
    | grep -cx "$2"
}

emit_status_md() {  # $1 = output path
  local out="$1" hand u_rows all counts
  hand="$(emit_hand_rows)"
  u_rows="$(emit_u_rows)"
  all="$(printf '%s\n%s\n' "$u_rows" "$hand")"

  # Count off the unescaped third data cell, not off a text search - a row whose
  # Item text happens to contain "| DONE |" must not be counted twice.
  counts="$(printf '%s\n' "$all" | sed 's/\\|/\x01/g' \
    | awk -F'|' '/^\|[ ]*[A-Z]+-[0-9]+[ ]*\|/ { s=$4; gsub(/^[ \t]+|[ \t]+$/, "", s); c[s]++; t++ }
                 END { printf "%d %d %d %d %d", c["DONE"], c["PARTIAL"], c["OPEN"], c["UNTRIAGED"], t }')"
  read -r n_done n_part n_open n_untri n_total <<<"$counts"

  cat > "$out" <<EOF
# STATUS - the single status register

**One row per tracked item. Every item in exactly one of four states.**
This file is the roll-up; the detail stays where it already is.

| This file | The detail |
|---|---|
| **\`STATUS.md\`** (here) | the roll-up: one row per item, four states, machine-parseable. Answers *"how much is done?"* in one line. |
| \`COMPILED_AUDIT.md\` | the per-finding evidence for every \`U-nn\` row: source audit, verification mark, file:line, before/after. **Not** summarised away - the \`U-nn\` rows below are *generated from it*. |
| \`WORKLIST.md\` | the human task board (what to pick up next, in order). |
| \`SESSION_HANDOFF.md\` | context for the next session. |
| \`IMPROVEMENT_LOG.md\` | what each session did, newest first. |
| \`PROJECT_VISION.md\` | mission + hard constraints. |

**Do not hand-edit the generated block.** Run
\`working_code/gifscythe/scripts/check_docs.sh --emit\`. Plain
\`check_docs.sh\` regenerates to a temp file and diffs it against this file, so a
hand-fudged roll-up fails the gate.

## Vocabulary - four states, no synonyms

| State | Means | Rule |
|---|---|---|
| **DONE** | closed, with executed proof named in the row | Proof column names a command, a test id or a diff. |
| **PARTIAL** | partly closed | **The row must name what is still missing.** A bare \`PARTIAL\` is a gate failure. |
| **OPEN** | known, scoped, not started | Proof column carries the blocker; Next action carries the single next step. |
| **UNTRIAGED** | discovered, not yet scoped | The state that did not exist before. A new finding is recorded **in the session it is found**, here and as a pending line in \`WORKLIST.md\`. It may not sit unscoped past the next log entry (gate failure). |

**Session** = last session that touched the item, or \`-\` if untouched.
**Proof / Blocker** is never blank. **Next action** is \`-\` only for DONE.

**Counts (generated - do not edit by hand):** ${n_done} DONE · ${n_part} PARTIAL · ${n_open} OPEN · ${n_untri} UNTRIAGED · ${n_total} total
**Last regenerated:** ${LOG_SESSION:-(no log entry)} · ${LOG_DATE:-unknown} · by scripts/check_docs.sh --emit

## Register, part 1 - derived from \`COMPILED_AUDIT.md\` §5

<!-- BEGIN GENERATED from COMPILED_AUDIT.md - do not edit; run check_docs.sh --emit -->
| ID | Item | State | Session | Proof / Blocker | Next action |
|----|-------|-------|---------|-----------------|-------------|
$u_rows
<!-- END GENERATED -->

## Register, part 2 - hand-maintained (W worklist · D deferred · R risk · new findings)

These rows have no other machine-readable home, so they are written by hand and
preserved verbatim by \`--emit\`. Same schema, same vocabulary, same rules.

<!-- BEGIN HAND-MAINTAINED - sessions edit this block; --emit preserves it -->
$(printf '%s\n' "$hand")
<!-- END HAND-MAINTAINED -->

## How to add a row

* **New audit finding** - add it to \`COMPILED_AUDIT.md\` §5 (that is the detail),
  then \`check_docs.sh --emit\` picks it up here automatically. IDs are
  discovered, never hardcoded.
* **New worklist / deferred / risk item** - add a row to the hand-maintained
  block above with the next free ID in its namespace, then run \`--emit\` to
  refresh the header counts.
* **Anything you just discovered** - \`UNTRIAGED\`, this session, today, plus a
  pending line in \`WORKLIST.md\`. Scope it or close it before the next session.
EOF
}

# ===========================================================================
if [[ "$EMIT" == "1" ]]; then
  echo "==> check_docs.sh --emit (regenerating STATUS.md from the repo)"
  emit_status_md "$STATUS_MD"
  echo "  wrote $STATUS_MD"
  grep -E '^\*\*Counts' "$STATUS_MD"
  echo "==> Done. 0 passed, 0 failed (--emit only writes; run plain mode to check)."
  exit 0
fi

echo "==> Gifscythe documentation gate (STATUS.md register + doc consistency)"

# ---------------------------------------------------------------------------
# 0. STATUS.md must exist and must match what the emitter produces.
#    This is the check that makes the roll-up self-maintaining.
# ---------------------------------------------------------------------------
if [[ ! -f "$STATUS_MD" ]]; then
  bad "G0" "STATUS.md is missing - run scripts/check_docs.sh --emit"
else
  emit_status_md "$TMP_REG"
  if diff -u "$STATUS_MD" "$TMP_REG" > /tmp/check_docs_diff.txt 2>&1; then
    ok "G0" "STATUS.md is exactly what the emitter produces from the repo"
  else
    bad "G0" "STATUS.md has drifted from the repo (hand-edited, or --emit not re-run)"
    sed -n '1,25p' /tmp/check_docs_diff.txt | sed 's/^/         /'
    note "fix: run working_code/gifscythe/scripts/check_docs.sh --emit and commit"
  fi
fi

# ---------------------------------------------------------------------------
# 1. VOCABULARY - every state cell is one of the four.
# ---------------------------------------------------------------------------
if [[ -f "$STATUS_MD" ]]; then
  bad_states="$(register_rows "$STATUS_MD" | while IFS= read -r r; do
                  printf '%s\n' "$r" | row_fields | sed -n 3p
                done | grep -vxE "$STATE_DONE|$STATE_PARTIAL|$STATE_OPEN|$STATE_UNTRIAGED" | sort -u)"
  if [[ -z "$bad_states" ]]; then
    ok "G1" "every state cell is DONE/PARTIAL/OPEN/UNTRIAGED"
  else
    bad "G1" "state cells outside the four-state vocabulary: $(tr '\n' ' ' <<<"$bad_states")"
  fi

  # 1b. no legacy markers leaked into the register
  legacy="$(register_rows "$STATUS_MD" | grep -E '✅|⬜|◐|☑' || true)"
  if [[ -z "$legacy" ]]; then
    ok "G1b" "no legacy audit markers (✅/⬜/◐/☑) leaked into STATUS.md"
  else
    bad "G1b" "legacy audit markers in STATUS.md - map them in state_of() instead"
  fi
fi

# ---------------------------------------------------------------------------
# 3. PARTIAL MUST EXPLAIN - and every row must have a Proof/Blocker at all.
#    Generic filler counts as no explanation.
# ---------------------------------------------------------------------------
if [[ -f "$STATUS_MD" ]]; then
  partial_bad="$(register_rows "$STATUS_MD" | while IFS= read -r r; do
      f="$(printf '%s\n' "$r" | row_fields)"
      st="$(sed -n 3p <<<"$f")"; proof="$(sed -n 5p <<<"$f")"; id="$(sed -n 1p <<<"$f")"
      [[ "$st" == "$STATE_PARTIAL" ]] || continue
      case "$proof" in
        ""|"-"|"TBD"|"tbd"|"PARTIAL"|"partial"|"in progress"|"WIP"|"partially done"|"see above"|"n/a"|"N/A") echo "$id" ;;
      esac
    done)"
  if [[ -z "$partial_bad" ]]; then
    ok "G3" "every PARTIAL row names what is still missing"
  else
    bad "G3" "PARTIAL row(s) with no explanation: $(tr '\n' ' ' <<<"$partial_bad")"
  fi

  blank_proof="$(register_rows "$STATUS_MD" | while IFS= read -r r; do
      f="$(printf '%s\n' "$r" | row_fields)"
      proof="$(sed -n 5p <<<"$f")"; id="$(sed -n 1p <<<"$f")"
      [[ -z "$proof" || "$proof" == "-" ]] && echo "$id"
    done)"
  if [[ -z "$blank_proof" ]]; then
    ok "G3b" "no row has a blank Proof / Blocker"
  else
    bad "G3b" "blank Proof / Blocker: $(tr '\n' ' ' <<<"$blank_proof")"
  fi

  blank_next="$(register_rows "$STATUS_MD" | while IFS= read -r r; do
      f="$(printf '%s\n' "$r" | row_fields)"
      st="$(sed -n 3p <<<"$f")"; nx="$(sed -n 6p <<<"$f")"; id="$(sed -n 1p <<<"$f")"
      [[ "$st" == "$STATE_DONE" ]] && continue
      [[ -z "$nx" || "$nx" == "-" ]] && echo "$id"
    done)"
  if [[ -z "$blank_next" ]]; then
    ok "G3c" "every non-DONE row has a Next action"
  else
    bad "G3c" "non-DONE row(s) with no Next action: $(tr '\n' ' ' <<<"$blank_next")"
  fi

  # 3d. schema: exactly six cells per row, and IDs are namespaced
  schema_bad="$(register_rows "$STATUS_MD" | while IFS= read -r r; do
      n="$(printf '%s\n' "$r" | row_fields | wc -l)"
      [[ "$n" -eq 6 ]] || printf '%s(%s cells) ' "$r" "$n"
    done)"
  if [[ -z "$schema_bad" ]]; then
    ok "G3d" "every row has the six-cell schema | ID | Item | State | Session | Proof / Blocker | Next action |"
  else
    bad "G3d" "row(s) off-schema: $schema_bad"
  fi
fi

# ---------------------------------------------------------------------------
# 4. HEADER MATCHES TABLE
# ---------------------------------------------------------------------------
if [[ -f "$STATUS_MD" ]]; then
  hdr="$(grep -E '^\*\*Counts' "$STATUS_MD" | head -1)"
  h_done=$(grep -oE '[0-9]+ DONE' <<<"$hdr" | grep -oE '^[0-9]+')
  h_part=$(grep -oE '[0-9]+ PARTIAL' <<<"$hdr" | grep -oE '^[0-9]+')
  h_open=$(grep -oE '[0-9]+ OPEN' <<<"$hdr" | grep -oE '^[0-9]+')
  h_untri=$(grep -oE '[0-9]+ UNTRIAGED' <<<"$hdr" | grep -oE '^[0-9]+')
  h_total=$(grep -oE '[0-9]+ total' <<<"$hdr" | grep -oE '^[0-9]+')
  t_done=$(count_state "$STATUS_MD" "$STATE_DONE")
  t_part=$(count_state "$STATUS_MD" "$STATE_PARTIAL")
  t_open=$(count_state "$STATUS_MD" "$STATE_OPEN")
  t_untri=$(count_state "$STATUS_MD" "$STATE_UNTRIAGED")
  t_total=$(register_rows "$STATUS_MD" | wc -l | tr -d ' ')
  sum=$((t_done + t_part + t_open + t_untri))
  if [[ "${h_done:-x}" == "$t_done" && "${h_part:-x}" == "$t_part" && "${h_open:-x}" == "$t_open" \
     && "${h_untri:-x}" == "$t_untri" && "${h_total:-x}" == "$t_total" && "$sum" -eq "$t_total" ]]; then
    ok "G4" "header counts match the table ($t_done/$t_part/$t_open/$t_untri = $t_total)"
  else
    bad "G4" "header says ${h_done:-?}/${h_part:-?}/${h_open:-?}/${h_untri:-?} total ${h_total:-?}; table has $t_done/$t_part/$t_open/$t_untri total $t_total (states sum to $sum)"
  fi
fi

# ---------------------------------------------------------------------------
# 5. REGISTER <-> AUDIT DOC AGREE (every U-nn both ways, compatible state)
# ---------------------------------------------------------------------------
if [[ -f "$STATUS_MD" ]]; then
  audit_ids="$(audit_section 5 | grep -oE '\*\*U-[0-9]+\*\*' | grep -oE 'U-[0-9]+' | sort -u)"
  reg_ids="$(register_rows "$STATUS_MD" | while IFS= read -r r; do printf '%s\n' "$r" | row_fields | sed -n 1p; done | grep -E '^U-[0-9]+$' | sort -u)"
  missing="$(comm -23 <(printf '%s\n' "$audit_ids") <(printf '%s\n' "$reg_ids"))"
  extra="$(comm -13 <(printf '%s\n' "$audit_ids") <(printf '%s\n' "$reg_ids"))"
  if [[ -z "$missing" && -z "$extra" ]]; then
    ok "G5" "all $(wc -l <<<"$audit_ids" | tr -d ' ') U-nn audit findings appear in STATUS.md and vice versa"
  else
    bad "G5" "register/audit mismatch. In audit not STATUS: $(tr '\n' ' ' <<<"$missing")| In STATUS not audit: $(tr '\n' ' ' <<<"$extra")"
  fi

  # compatible state, per ID
  disagree=""
  while IFS= read -r id; do
    [[ -n "$id" ]] || continue
    cell="$(audit_section 5 | grep -E "^\|[[:space:]]*\*\*$id\*\*[[:space:]]*\|" | head -1)"
    [[ -n "$cell" ]] || continue
    a_state="$(printf '%s\n' "$cell" | row_fields | sed -n 5p | { read -r x; state_of "$x"; })"
    r_state="$(register_rows "$STATUS_MD" | grep -E "^\|[[:space:]]*$id[[:space:]]*\|" | head -1 | row_fields | sed -n 3p)"
    [[ "$a_state" == "$r_state" ]] || disagree+="$id(audit=$a_state,status=$r_state) "
  done <<< "$audit_ids"
  if [[ -z "$disagree" ]]; then
    ok "G5b" "every U-nn state in STATUS.md matches its COMPILED_AUDIT.md marker"
  else
    bad "G5b" "state disagreement between COMPILED_AUDIT.md and STATUS.md: $disagree"
  fi
fi

# ---------------------------------------------------------------------------
# 2. NO CONTRADICTIONS - no ID is DONE in one doc and OPEN in another.
# ---------------------------------------------------------------------------
if [[ -f "$STATUS_MD" && -f "$WORKLIST_MD" ]]; then
  contra=""
  # WORKLIST checkboxes that name IDs: [x] claims DONE, [ ] claims not-DONE.
  while IFS= read -r line; do
    [[ -n "$line" ]] || continue
    box="$(grep -oE '^\s*- \[.\]' <<<"$line" | grep -oE '\[.\]')"
    ids="$(grep -oE '\bU-[0-9]+\b' <<<"$line" | sort -u)"
    [[ -n "$ids" ]] || continue
    while IFS= read -r id; do
      [[ -n "$id" ]] || continue
      r_state="$(register_rows "$STATUS_MD" | grep -E "^\|[[:space:]]*$id[[:space:]]*\|" | head -1 | row_fields | sed -n 3p)"
      [[ -n "$r_state" ]] || continue
      if [[ "$box" == "[x]" && "$r_state" != "$STATE_DONE" ]]; then
        contra+="$id(WORKLIST ticked, STATUS=$r_state) "
      elif [[ "$box" == "[ ]" && "$r_state" == "$STATE_DONE" ]]; then
        contra+="$id(WORKLIST unticked, STATUS=DONE) "
      fi
    done <<< "$ids"
  done < <(grep -E '^[[:space:]]*- \[.\]' "$WORKLIST_MD")

  # ...and the audit register vs STATUS.md is covered by G5b above.
  if [[ -z "$contra" ]]; then
    ok "G2" "no ID is DONE in one doc and open in another (WORKLIST x STATUS x COMPILED_AUDIT)"
  else
    bad "G2" "contradictory status between docs: $contra"
  fi
fi

# ---------------------------------------------------------------------------
# 6. REAL GATE NUMBERS - run verify_audit.sh, compare with what docs quote.
# ---------------------------------------------------------------------------
GATE_PASS=""; GATE_FAIL=""; GATE_SKIP=""; GATE_ADD=0
# verify_audit.sh runs THIS script as its documentation gate. To stop the
# recursion it sets GS_FROM_VERIFY_AUDIT=1, and this run therefore measures
# verify_audit.sh with that one gate suppressed. verify_audit.sh declares how
# many checks its doc gate contributes (DOC_GATE_CHECKS) right next to the gate,
# and we read that back out of the file - so the total the docs must quote stays
# derived from the repo rather than baked in here.
if [[ "$NESTED" == "1" ]]; then
  skip "G6" "running inside verify_audit.sh - gate numbers not re-measured (recursion guard)"
elif [[ "$NO_GATE_RUN" == "1" ]]; then
  skip "G6" "--no-gate-run: verify_audit.sh not re-run here (CI already runs its steps)"
else
  if [[ -x "$VERIFY" ]]; then
    GATE_ADD="$(grep -oE '^DOC_GATE_CHECKS=[0-9]+' "$VERIFY" | grep -oE '[0-9]+' | head -1)"
    GATE_ADD="${GATE_ADD:-0}"
    gate_out="$(GS_SKIP_DOC_GATE=1 "$VERIFY" 2>&1)"
    gate_line="$(grep -E '^==> Done\. [0-9]+ passed' <<<"$gate_out" | tail -1)"
    if [[ -n "$gate_line" ]]; then
      GATE_PASS=$(grep -oE '[0-9]+ passed' <<<"$gate_line" | grep -oE '^[0-9]+')
      GATE_FAIL=$(grep -oE '[0-9]+ failed' <<<"$gate_line" | grep -oE '^[0-9]+')
      GATE_SKIP=$(grep -oE '[0-9]+ skipped' <<<"$gate_line" | grep -oE '^[0-9]+')
      EXP_PASS=$((GATE_PASS + GATE_ADD))
      note "verify_audit.sh (doc gate suppressed): $GATE_PASS passed, $GATE_FAIL failed, $GATE_SKIP skipped"
      note "verify_audit.sh declares DOC_GATE_CHECKS=$GATE_ADD, so the full run is $EXP_PASS passed, $GATE_FAIL failed, $GATE_SKIP skipped"
      # every current-state doc that quotes a "N passed, N failed, N skipped"
      # triple for verify_audit.sh must quote the full-run numbers
      wrong=""
      for f in "${CURRENT_DOCS[@]}"; do
        [[ "$f" == "IMPROVEMENT_LOG.md" ]] && content="$(newest_log_entry)" || content="$(cat "$f")"
        # Two filters, both needed, both learned the hard way this session:
        #
        #  (i) only claims ABOUT verify_audit.sh. check_docs.sh, test_engine.sh
        #      and the web suites print the same "N passed, N failed, N skipped"
        #      shape, and comparing those against the gate total is nonsense.
        # (ii) an explicit historical marker in the same sentence exempts a
        #      claim. These docs legitimately quote what a past session measured
        #      (that is the evidence for finding N-01). The marker must be
        #      explicit - a bare stale number is still a failure.
        # Prose wraps, so both tests look at a +/-1 line window.
        # Prose wraps: "23 PASS / 0 FAIL /\n5 SKIP" is one claim over two lines,
        # and a line-based match silently skipped it. Flatten the doc to a single
        # line and match with a context window on each side instead.
        flat="$(tr '\n' ' ' <<<"$content" | tr -s ' ')"
        while IFS= read -r ctx; do
          [[ -n "$ctx" ]] || continue
          # a claim naming a different runner is about that runner, even when it
          # sits next to a verify_audit.sh row in the same table
          grep -qiE 'check_docs|test_engine|test_package|smoke_cli|web/test' <<<"$ctx" && continue
          grep -qi 'verify_audit' <<<"$ctx" || continue
          # an explicit historical marker exempts a claim; a bare stale number
          # does not. These docs legitimately quote what a past session measured
          # (that is the evidence for finding N-01).
          grep -qiE 'historical|baseline|-era|at the time|still quoted|pre-application' <<<"$ctx" && continue
          nums=($(grep -oE '[0-9]+ (passed|PASS)[ ]*[,/] *[0-9]+ (failed|FAIL)[ ]*[,/] *[0-9]+ (skipped|SKIP)' <<<"$ctx" \
                    | head -1 | grep -oE '[0-9]+'))
          [[ "${#nums[@]}" -eq 3 ]] || continue
          if [[ "${nums[0]}" != "$EXP_PASS" || "${nums[1]}" != "$GATE_FAIL" || "${nums[2]}" != "$GATE_SKIP" ]]; then
            wrong+="$f claims ${nums[0]}/${nums[1]}/${nums[2]}; "
          fi
        done < <(grep -oE '.{0,70}[0-9]+ (passed|PASS)[ ]*[,/] *[0-9]+ (failed|FAIL)[ ]*[,/] *[0-9]+ (skipped|SKIP).{0,40}' <<<"$flat")
      done
      if [[ -z "$wrong" ]]; then
        ok "G6" "every current-state doc quotes the real gate numbers ($EXP_PASS/$GATE_FAIL/$GATE_SKIP)"
      else
        bad "G6" "stale gate numbers - $wrong real is $EXP_PASS passed, $GATE_FAIL failed, $GATE_SKIP skipped"
      fi
    else
      bad "G6" "verify_audit.sh printed no '==> Done. N passed' line"
    fi
  else
    skip "G6" "verify_audit.sh not found at $VERIFY"
  fi
fi

# ---------------------------------------------------------------------------
# 7. WORKFLOW COPIES - byte-identical unless a change is declared pending.
#    Mirrors verify_audit.sh gate E9 exactly, including the one exception.
# ---------------------------------------------------------------------------
wf="$root/.github/workflows/build.yml"; prop="$root/docs/ci/build.yml.proposed"
if [[ ! -f "$wf" || ! -f "$prop" ]]; then
  bad "G7" "missing workflow copy (.github/workflows/build.yml or docs/ci/build.yml.proposed)"
elif diff -q "$wf" "$prop" >/dev/null 2>&1; then
  ok "G7" ".github/workflows/build.yml and docs/ci/build.yml.proposed are byte-identical"
elif [[ -f "$root/docs/ci/PENDING_WORKFLOW_CHANGE.md" ]]; then
  skip "G7" "declared pending workflow change - see docs/ci/PENDING_WORKFLOW_CHANGE.md"
else
  bad "G7" "workflow and docs/ci/build.yml.proposed have drifted with no pending-change marker"
fi

# ---------------------------------------------------------------------------
# 8. REFERENCED FILES EXIST
# ---------------------------------------------------------------------------
# Backticked tokens that look like paths in THIS repo. Candidate roots, because
# docs are written from two working directories:
#   .                        repo root  (docs/, web/, working_code/, ...)
#   working_code/gifscythe   product    (src/, scripts/, tests/, examples/, ...)
#   reference_code/gifsicle  upstream engine (src/gifsicle.c, src/win32cfg.h)
# Anything git-ignored is skipped: those trees are auto-fetched or built, so
# their absence is not a documentation bug.
if [[ ${#CURRENT_DOCS[@]} -gt 0 ]]; then
  top="$(git ls-files | cut -d/ -f1 | sort -u | paste -sd'|' -)"
  sub="$(git ls-files 'working_code/gifscythe/*' | sed 's|^working_code/gifscythe/||' | cut -d/ -f1 | sort -u | paste -sd'|' -)"
  missing_paths=""
  while IFS= read -r cand; do
    [[ -n "$cand" ]] || continue
    p="${cand#* }"; src="${cand%% *}"
    p="${p#./}"; p="${p%/}"
    found=""
    for r in "." "working_code/gifscythe" "reference_code/gifsicle"; do
      [[ -e "$r/$p" ]] && { found=1; break; }
    done
    [[ -n "$found" ]] && continue
    # ignored on purpose (auto-fetched reference clones, build trees) - also try
    # the trailing-slash form, which is how directories are written in .gitignore
    git check-ignore -q "$p" 2>/dev/null && continue
    git check-ignore -q "$p/" 2>/dev/null && continue
    # documented allowances. Each one is a path that is SUPPOSED not to exist;
    # delete the entry when that stops being true.
    case "$p" in
      # the build_gifsicle.sh shim was deliberately removed (S7). Docs mention it
      # only to say "do not reintroduce it" - the mention is the point.
      scripts/build_gifsicle.sh) continue ;;
      # hypothetical release dirs used to demonstrate the version-sort fix (U-26)
      release/[0-9]*) continue ;;
      # (S11: the working_code/gifscythe/build_support allowance that lived here
      # was deleted when U-15 closed - the directory now exists for real and
      # holds version.h.in, the CMake template moved out of src/.)
      # upstream tarball directory name, cited in the reference manifest
      reference_code/gifsicle-1.96) continue ;;
    esac
    missing_paths+="$p ($src); "
  done < <(
    for f in "${CURRENT_DOCS[@]}"; do
      [[ "$f" == "IMPROVEMENT_LOG.md" ]] && content="$(newest_log_entry)" || content="$(cat "$f")"
      grep -oE '`[^`]*`' <<<"$content" | tr -d '`' | sed "s|^|$f |"
    done \
    | grep -E '^[^ ]+ (\./)?[A-Za-z0-9_.+-]+/[A-Za-z0-9_./+-]*$' \
    | grep -vE '<|\{|\*|\.\.|https?:' \
    | grep -E "^[^ ]+ (\./)?($top|$sub)(/|\$)" \
    | awk '{print $1" "$2}' | sort -u
  )
  if [[ -z "$missing_paths" ]]; then
    ok "G8" "every repo path referenced by a current-state doc exists"
  else
    bad "G8" "doc(s) point at files that do not exist: $missing_paths"
  fi
fi

# ---------------------------------------------------------------------------
# 9. CHECK( COUNTS - compare like with like, and say which you mean.
#    grep -c counts LINES. grep -o 'CHECK(' | wc -l counts OCCURRENCES.
#    Neither is the RUNTIME count: loops expand checks.
# ---------------------------------------------------------------------------
harness="$self/tests/test_gui_offscreen.cpp"
unit="$self/tests/test_gifsicle_command.cpp"
if [[ -f "$harness" && -f "$unit" ]]; then
  h_occ=$(grep -o 'CHECK(' "$harness" | wc -l | tr -d ' ')
  h_line=$(grep -c 'CHECK(' "$harness" | tr -d ' ')
  u_occ=$(grep -o 'CHECK(' "$unit" | wc -l | tr -d ' ')
  u_line=$(grep -c 'CHECK(' "$unit" | tr -d ' ')
  note "source OCCURRENCES of CHECK( - harness $h_occ, unit $u_occ"
  note "source LINES containing CHECK(  - harness $h_line, unit $u_line (differs when a line holds two)"

  badcount=""
  # (a) any doc claim of "N `CHECK(` sites" must equal the OCCURRENCE count
  for f in "${CURRENT_DOCS[@]}"; do
    [[ "$f" == "IMPROVEMENT_LOG.md" ]] && content="$(newest_log_entry)" || content="$(cat "$f")"
    while IFS= read -r n; do
      [[ -n "$n" ]] || continue
      case "$n" in
        "$h_occ"|"$u_occ") ;;
        *) badcount+="$f claims $n CHECK( sites (real: harness $h_occ / unit $u_occ OCCURRENCES); " ;;
      esac
    done < <(grep -oE '[0-9]+ `CHECK\(` sites' <<<"$content" | grep -oE '^[0-9]+')
  done
  # (b) the unit RUNTIME count is measurable here - compare it
  if [[ -x "$self/build/test_gifsicle_command" ]]; then
    u_run="$("$self/build/test_gifsicle_command" 2>/dev/null | grep -oE '[0-9]+ checks, [0-9]+ failures' | tail -1)"
    u_run_n="${u_run%% *}"
    note "unit RUNTIME count (measured just now): ${u_run:-not printed}"
    if [[ -n "$u_run_n" ]]; then
      for f in "${CURRENT_DOCS[@]}"; do
        [[ "$f" == "IMPROVEMENT_LOG.md" ]] && content="$(newest_log_entry)" || content="$(cat "$f")"
        # only lines that are about the UNIT suite; the GUI harness has its own
        # runtime count and is checked separately in (c).
        while IFS= read -r n; do
          [[ -n "$n" ]] || continue
          [[ "$n" == "$u_run_n" ]] || badcount+="$f claims $n unit checks (runtime measured $u_run_n); "
        done < <(grep -iE 'unit|build\.sh|test_gifsicle_command' <<<"$content" \
                   | grep -oE '[0-9]+ checks, [0-9]+ failures' | grep -oE '^[0-9]+' | sort -u)
      done
    fi
  else
    skip "G9b" "build/test_gifsicle_command not built - unit RUNTIME count not re-measured (run ./build.sh)"
  fi
  # (c) the GUI harness RUNTIME count needs Qt6 + cmake. Without them we do NOT
  #     accept a bare runtime number: it must be labelled as a past measurement.
  if command -v cmake >/dev/null 2>&1 && { command -v qmake6 >/dev/null 2>&1 || [[ -d /usr/lib/x86_64-linux-gnu/cmake/Qt6 ]]; }; then
    note "cmake + Qt6 present: the harness runtime count can be re-measured (see verify_audit.sh gate B)"
    ok "G9c" "Qt toolchain present - harness runtime count is measurable on this machine"
  else
    unlabelled=""
    for f in "${CURRENT_DOCS[@]}"; do
      [[ "$f" == "IMPROVEMENT_LOG.md" ]] && content="$(newest_log_entry)" || content="$(cat "$f")"
      # a harness runtime count is only honest if a measurement caveat sits in
      # the same sentence - and these are wrapped prose, so scan a +/-2 line
      # window rather than the single physical line.
      while IFS= read -r ln; do
        [[ -n "$ln" ]] || continue
        win="$(awk -v n="$ln" 'NR>=n-2 && NR<=n+2' <<<"$content")"
        grep -qiE 'measur|S7|historical|last |sandbox|not run|not been re|CI-only|CI only|corroborat|cannot|no Qt|no cmake' <<<"$win" \
          || unlabelled+="$f line $ln: $(awk -v n="$ln" 'NR==n' <<<"$content" | cut -c1-80); "
      done < <(grep -nE 'harness' <<<"$content" | grep -E '[0-9]{3} check' | cut -d: -f1)
    done
    if [[ -z "$unlabelled" ]]; then
      ok "G9c" "no cmake/Qt6 here: every harness runtime count in the docs is labelled as a past measurement"
    else
      bad "G9c" "harness runtime count quoted as current without a measurement caveat: $unlabelled"
    fi
  fi
  if [[ -z "$badcount" ]]; then
    ok "G9" "every CHECK( count in the docs is the right kind of count"
  else
    bad "G9" "CHECK( count mismatch - $badcount"
  fi
fi

# ---------------------------------------------------------------------------
# 10. BRANCH / SHA FRESHNESS - a doc naming the base commit must name the real
#     one. Derived from git, never from a literal.
# ---------------------------------------------------------------------------
# "The real one" is main's tip, NOT HEAD: a session commits on top of main, so
# comparing against HEAD made this gate fail on every single commit - the exact
# failure mode that gets a checker deleted. Found by mutation-testing.
main_ref=""
for cand in origin/main main; do
  git rev-parse --verify -q "$cand" >/dev/null 2>&1 && { main_ref="$cand"; break; }
done
if [[ -z "$main_ref" ]]; then
  # S10: CI checks out with fetch-depth 1, so neither origin/main nor main
  # exists in the runner's clone and every base-commit claim is "unknown to
  # this clone" — the gate failed CI on its very first run there (PR #13).
  # With no main ref there is nothing to compare against: SKIP honestly
  # instead of guessing. Local clones and the pre-push hook (full history)
  # still enforce this.
  skip "G10" "no origin/main or main ref in this clone (shallow CI checkout) - base-commit claims are enforced locally and by the pre-push hook, not here"
else
full_main="$(git rev-parse "$main_ref" 2>/dev/null)"
main_sha="$(git rev-parse --short=7 "$main_ref" 2>/dev/null)"
branch_now="$(git rev-parse --abbrev-ref HEAD 2>/dev/null)"
stale_base=""
for f in "${CURRENT_DOCS[@]}"; do
  [[ "$f" == "IMPROVEMENT_LOG.md" ]] && content="$(newest_log_entry)" || content="$(cat "$f")"
  # "based on `main` commit `xxxxxxx`" / "base commit `xxxxxxx`" / "of `main` (`xxxxxxx`)"
  while IFS= read -r sha; do
    [[ -n "$sha" ]] || continue
    [[ "$sha" == "$main_sha" || "$sha" == "$full_main" ]] && continue
    # a sha this repo does not know at all, or one that is not HEAD -> stale
    if git cat-file -e "${sha}^{commit}" 2>/dev/null; then
      stale_base+="$f names base $sha ($main_ref is $main_sha); "
    else
      stale_base+="$f names base $sha, unknown to this clone ($main_ref is $main_sha); "
    fi
  done < <(grep -oE '(based on|base commit|base of|branched from|merge of PR #[0-9]+[,:]?) `?main`?[^`]*`[0-9a-f]{7,40}`' <<<"$content" \
             | grep -oE '[0-9a-f]{7,40}' | sort -u)
done
if [[ -z "$stale_base" ]]; then
  ok "G10" "every doc naming the base commit names the real one ($main_ref = $main_sha; branch is $branch_now)"
else
  bad "G10" "stale base commit - $stale_base"
fi
fi

# ---------------------------------------------------------------------------
# 11. LOG IS CURRENT - the newest IMPROVEMENT_LOG.md entry must not be older
#     than the newest commit that touched a NON-DOC file. This is what turns
#     "update the docs every session" from advice into a gate.
# ---------------------------------------------------------------------------
if [[ -z "$LOG_DATE" ]]; then
  bad "G11" "IMPROVEMENT_LOG.md newest entry has no (YYYY-MM-DD) date to compare"
else
  nondoc_date="$(git log -1 --format=%ad --date=short -- \
      $(git ls-files | grep -v '\.md$' | tr '\n' ' ') 2>/dev/null)"
  if [[ -z "$nondoc_date" ]]; then
    skip "G11" "no commit touching a non-doc file found (shallow clone?) - log date is $LOG_DATE"
  elif [[ "$LOG_DATE" < "$nondoc_date" ]]; then
    bad "G11" "IMPROVEMENT_LOG.md newest entry is $LOG_DATE but a non-doc file changed on $nondoc_date - the docs are behind the code"
  else
    ok "G11" "log entry $LOG_DATE is not older than the newest non-doc commit ($nondoc_date)"
  fi
fi

# ---------------------------------------------------------------------------
# 12. NOTHING SITS UNTRIAGED - an UNTRIAGED row from a session older than the
#     newest log entry means a finding was left unscoped across a session
#     boundary. That is the failure this whole system exists to prevent.
# ---------------------------------------------------------------------------
if [[ -f "$STATUS_MD" ]]; then
  if [[ -z "$LOG_SESSION" ]]; then
    bad "G12" "cannot check UNTRIAGED age: IMPROVEMENT_LOG.md has no '## S<n>' entry"
  else
    cur_n="${LOG_SESSION#S}"
    stale_untri=""
    while IFS= read -r r; do
      [[ -n "$r" ]] || continue
      f="$(printf '%s\n' "$r" | row_fields)"
      st="$(sed -n 3p <<<"$f")"; sess="$(sed -n 4p <<<"$f")"; id="$(sed -n 1p <<<"$f")"
      [[ "$st" == "$STATE_UNTRIAGED" ]] || continue
      if [[ "$sess" == "-" || -z "$sess" ]]; then
        stale_untri+="$id(UNTRIAGED with no session - name the session that found it); "
        continue
      fi
      n="${sess#S}"
      [[ "$n" =~ ^[0-9]+$ ]] || { stale_untri+="$id(session '$sess' is not S<n>); "; continue; }
      [[ "$n" -lt "$cur_n" ]] && stale_untri+="$id(UNTRIAGED since $sess, newest session is $LOG_SESSION); "
    done < <(register_rows "$STATUS_MD")
    if [[ -z "$stale_untri" ]]; then
      ok "G12" "no UNTRIAGED row is older than the newest logged session ($LOG_SESSION)"
    else
      bad "G12" "finding(s) left unscoped across a session boundary: $stale_untri"
    fi
  fi
fi

# ---------------------------------------------------------------------------
# 13. THE RULES ARE WRITTEN WHERE A NEW SESSION READS THEM
#     The rules only stick if they live in files, not in a conversation.
# ---------------------------------------------------------------------------
rules_missing=""
for f in "$HANDOFF_MD" "$WORKLIST_MD" "$root/docs/release/RELEASE_PROCEDURE.md"; do
  [[ -f "$f" ]] || { rules_missing+="${f#$root/} missing; "; continue; }
  for needle in 'check_docs.sh' 'UNTRIAGED' 'STATUS.md'; do
    grep -q "$needle" "$f" || rules_missing+="${f#$root/} does not mention $needle; "
  done
done
if [[ -z "$rules_missing" ]]; then
  ok "G13" "SESSION_HANDOFF.md, WORKLIST.md and RELEASE_PROCEDURE.md all carry the status rules"
else
  bad "G13" "status rules not written where a new session reads them: $rules_missing"
fi

# 13b. the pre-push hook exists and bootstrap is wired
if [[ -x "$root/.githooks/pre-push" ]] && grep -q 'check_docs.sh' "$root/.githooks/pre-push"; then
  ok "G14" ".githooks/pre-push runs check_docs.sh"
else
  bad "G14" ".githooks/pre-push is missing or does not run check_docs.sh"
fi
if [[ "$(git config core.hooksPath 2>/dev/null)" == ".githooks" ]]; then
  ok "G15" "core.hooksPath is .githooks in this clone - the pre-push hook is live"
else
  bad "G15" "core.hooksPath is not .githooks - run scripts/bootstrap_hooks.sh (the hook is NOT live)"
fi

echo "==> Done. $PASS passed, $FAIL failed, $SKIP skipped."
[[ "$FAIL" -eq 0 ]]
