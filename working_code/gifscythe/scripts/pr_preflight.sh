#!/usr/bin/env bash
#
# pr_preflight.sh - the PR/merge companion to the stale-claim sweep.
#
#   ./scripts/pr_preflight.sh                 offline: doc gate + sweep + clean tree
#   ./scripts/pr_preflight.sh --online        also print repo/run/PR state via gh
#   ./scripts/pr_preflight.sh --body /tmp/pr_body.md   write the PR body skeleton
#
# The sweep runs "every session, at PR create and merge". This script makes
# that one command instead of a promise. It is offline by default and degrades
# to SKIP (never FAIL) when gh is absent or unauthenticated.
#
#   P1  runs check_docs.sh and prints its "==> Done" line
#   P2  runs sweep_stale.sh
#   P3  fails if the working tree is dirty (uncommitted work is lost on cutoff)
#   P3b fails if HEAD is ahead of origin (unpushed commits are not in the repo)
#   P4  (--online only) prints repo, main tip + latest main run, branch tip +
#       latest branch run, and the PR state for the branch
#   P5  writes a "## What this is / ## Evidence / ## Not in this PR" skeleton
#       to the --body path
#   P6  (--online only) checks SESSION_HANDOFF.md's "**Docs synced through:** PR #n"
#       line against the newest MERGED PR. If a merge landed after the last doc
#       sync, the docs are silently behind - nobody recorded what changed. Fails
#       and names the PRs that have to be reviewed and written up.
#
# Exits 1 if any check failed.
#
set -uo pipefail
export LC_ALL=C

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="$(cd "$self/../.." && pwd)"
cd "$root"

ONLINE=0
BODY=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --online) ONLINE=1 ;;
    --body) BODY="${2:-}"; shift ;;
    --body=*) BODY="${1#--body=}" ;;
    -h|--help) sed -n '2,30p' "${BASH_SOURCE[0]}"; exit 0 ;;
    *) echo "pr_preflight.sh: unknown argument '$1' (try --online / --body FILE)" >&2; exit 2 ;;
  esac
  shift
done

FAIL=0
note() { echo "         $*"; }

echo "==> PR/merge preflight (documentation gate + stale-claim sweep + tree + PR state)"

# ---------------------------------------------------------------------------
# P1. the documentation gate must be green before the PR boundary.
# ---------------------------------------------------------------------------
echo "== P1: documentation gate (check_docs.sh)"
if gate_out="$( "$self/scripts/check_docs.sh" 2>&1 )"; then
  grep -E '^==> Done' <<<"$gate_out" | tail -1 | sed 's/^/         /'
  echo "  PASS [P1] check_docs.sh green"
else
  echo "$gate_out" | tail -25 | sed 's/^/         /'
  echo "  FAIL [P1] check_docs.sh failed - fix and re-run before creating or merging"
  FAIL=$((FAIL + 1))
fi

# ---------------------------------------------------------------------------
# P2. the stale-claim sweep (runs every session, at PR create and merge).
# ---------------------------------------------------------------------------
echo "== P2: stale-claim sweep (sweep_stale.sh)"
if sweep_out="$( "$self/scripts/sweep_stale.sh" 2>&1 )"; then
  grep -E '^==> Done' <<<"$sweep_out" | tail -1 | sed 's/^/         /'
  echo "  PASS [P2] sweep_stale.sh green"
else
  echo "$sweep_out" | grep -E 'FAIL \[S|action:' | sed 's/^/         /'
  echo "  FAIL [P2] sweep_stale.sh found stale claims - date or mark them, then re-run"
  FAIL=$((FAIL + 1))
fi

# ---------------------------------------------------------------------------
# P3. no uncommitted changes may ride along into a PR.
# ---------------------------------------------------------------------------
echo "== P3: working tree"
if [[ -n "$(git status --porcelain 2>/dev/null)" ]]; then
  echo "  FAIL [P3] working tree is dirty - commit NOW before create/merge (session cut-off loses uncommitted work)"
  git status --porcelain | head -20 | sed 's/^/         /'
  FAIL=$((FAIL + 1))
else
  echo "  PASS [P3] working tree clean"
fi

echo "== P3b: unpushed commits"
if ! git rev-parse --abbrev-ref --symbolic-full-name '@{u}' >/dev/null 2>&1; then
  echo "  FAIL [P3b] branch has no upstream - git push -u origin HEAD before create/merge (unpushed commits are not in the repo)"
  FAIL=$((FAIL + 1))
else
  ahead="$(git rev-list --count '@{u}..HEAD' 2>/dev/null || echo 0)"
  if [[ "$ahead" -gt 0 ]]; then
    echo "  FAIL [P3b] $ahead unpushed commit(s) - git push before create/merge (unpushed commits are not in the repo)"
    FAIL=$((FAIL + 1))
  else
    echo "  PASS [P3b] HEAD is not ahead of origin"
  fi
fi

# ---------------------------------------------------------------------------
# P4. online PR/run state. Doc sentences about "main tip" / "latest run" must
#     be written from THESE values, never from memory. Offline without --online,
#     and SKIP (not FAIL) when gh is missing or unauthenticated.
# ---------------------------------------------------------------------------
echo "== P4: online PR state"
if [[ "$ONLINE" != "1" ]]; then
  echo "  SKIP [P4] offline (no --online) - add --online to fetch repo/run/PR state"
elif ! command -v gh >/dev/null 2>&1; then
  echo "  SKIP [P4] gh is not installed - offline mode"
elif ! gh auth status >/dev/null 2>&1; then
  echo "  SKIP [P4] gh is not authenticated - offline mode (connect GitHub to use --online)"
else
  repo="$(gh repo view --json nameWithOwner -q .nameWithOwner 2>/dev/null || echo '?')"
  branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null)"
  main_tip="$(gh api "repos/{owner}/{repo}/commits/main" -q .sha 2>/dev/null | cut -c1-7 || git rev-parse --short=7 origin/main 2>/dev/null || echo '?')"
  main_run="$(gh run list --branch main --limit 1 --json databaseId,conclusion -q '.[0] | "\(.databaseId) \(.conclusion)"' 2>/dev/null || echo '?')"
  branch_tip="$(git rev-parse --short=7 HEAD 2>/dev/null)"
  branch_run="$(gh run list --branch "$branch" --limit 1 --json databaseId,conclusion -q '.[0] | "\(.databaseId) \(.conclusion)"' 2>/dev/null || echo '? (none yet)')"
  pr_state="$(gh pr list --head "$branch" --state all --limit 1 --json number,state,title -q '.[0] | "#\(.number) \(.state) - \(.title)"' 2>/dev/null || echo '(no PR yet)')"
  echo "         repo:              $repo"
  echo "         main tip:          $main_tip    latest main run: $main_run"
  echo "         branch:            $branch"
  echo "         branch tip:        $branch_tip    latest branch run: $branch_run"
  echo "         PR state:          $pr_state"
  note "write doc sentences about tips/runs/PR from these values - never from memory"
  echo "  PASS [P4] online PR state printed"
fi

# ---------------------------------------------------------------------------
# P5. PR body skeleton (optional --body path).
# ---------------------------------------------------------------------------
if [[ -n "$BODY" ]]; then
  echo "== P5: PR body skeleton -> $BODY"
  mkdir -p "$(dirname "$BODY")" 2>/dev/null || true
  cat > "$BODY" <<'EOF'
## What this is

<!-- what this PR changes and why -->

## Evidence

<!-- gate numbers, run ids, verification steps - written from the P4 values -->

## Not in this PR

<!-- explicitly out of scope -->
EOF
  echo "  PASS [P5] wrote PR body skeleton to $BODY"
else
  echo "  SKIP [P5] no --body path given - skipping the body skeleton"
fi

# ---------------------------------------------------------------------------
# P6. HANDOFF SYNC vs NEWEST MERGED PR.
#     SESSION_HANDOFF.md names the PR it was last synced through. If a merge
#     landed after that, the docs are silently behind: nobody wrote down what
#     changed, what got fixed, what got implemented. Comparing a branch name
#     against a sha is not enough on its own - the point is to notice that the
#     *session* the handoff belongs to is no longer the last one merged.
# ---------------------------------------------------------------------------
echo "== P6: handoff sync vs newest merged PR"
HANDOFF_MD="$root/SESSION_HANDOFF.md"
if [[ "$ONLINE" != "1" ]]; then
  echo "  SKIP [P6] offline (no --online) - cannot read PR state"
elif ! command -v gh >/dev/null 2>&1; then
  echo "  SKIP [P6] gh is not installed - offline mode"
elif ! gh auth status >/dev/null 2>&1; then
  echo "  SKIP [P6] gh is not authenticated - offline mode"
elif [[ ! -f "$HANDOFF_MD" ]]; then
  echo "  SKIP [P6] SESSION_HANDOFF.md not found at $HANDOFF_MD"
else
  synced_line="$(grep -m1 -E '^\*\*Docs synced through:\*\*' "$HANDOFF_MD" || true)"
  if [[ -z "$synced_line" ]]; then
    echo "  FAIL [P6] SESSION_HANDOFF.md has no '**Docs synced through:** PR #n - branch X - merged as Y' line"
    note "action: add one naming the newest merge these docs describe, so the next session can detect a skipped sync"
    FAIL=$((FAIL+1))
  else
    synced_pr="$(grep -oE 'PR #[0-9]+' <<<"$synced_line" | head -1 | grep -oE '[0-9]+' || true)"
    synced_branch="$(grep -oE '`[^`]+`' <<<"$synced_line" | head -1 | tr -d '`' || true)"
    last_row="$(gh pr list --state merged --limit 1 --json number,headRefName,mergeCommit \
                  -q '.[0] | "\(.number) \(.headRefName) \(.mergeCommit.oid[0:7])"' 2>/dev/null || true)"
    last_pr="${last_row%% *}"; rest="${last_row#* }"
    last_branch="${rest%% *}"; last_sha="${rest##* }"
    if [[ -z "$last_pr" || ! "$last_pr" =~ ^[0-9]+$ ]]; then
      echo "  SKIP [P6] could not read the newest merged PR from gh"
    elif [[ -z "$synced_pr" ]]; then
      echo "  FAIL [P6] the '**Docs synced through:**' line names no PR number"
      FAIL=$((FAIL+1))
    elif (( last_pr > synced_pr )); then
      echo "  FAIL [P6] docs are BEHIND: handoff is synced through PR #$synced_pr (branch ${synced_branch:-?}) but the newest merged PR is #$last_pr (branch $last_branch, merged $last_sha)"
      note "a merge landed with no doc sync. For each PR below: read 'gh pr diff <n>', then write into"
      note "SESSION_HANDOFF.md + IMPROVEMENT_LOG.md what changed / what was fixed / what was implemented,"
      note "and only then move the '**Docs synced through:**' line to #$last_pr."
      gh pr list --state merged --limit "$((last_pr - synced_pr))" \
         --json number,headRefName,title \
         -q '.[] | "           unreviewed: PR #\(.number)  \(.headRefName)  \(.title)"' 2>/dev/null || true
      FAIL=$((FAIL+1))
    elif (( synced_pr > last_pr )); then
      echo "  FAIL [P6] handoff claims to be synced through PR #$synced_pr, but the newest MERGED PR is #$last_pr - that PR is not merged yet"
      FAIL=$((FAIL+1))
    else
      echo "  PASS [P6] handoff is synced through PR #$synced_pr, which is the newest merged PR"
      if [[ -n "$synced_branch" && "$synced_branch" != "$last_branch" ]]; then
        note "mismatch: handoff names branch '$synced_branch' but PR #$last_pr came from '$last_branch' - fix the branch label"
      fi
    fi
  fi
fi

echo "==> Done."
[[ "$FAIL" -eq 0 ]]
