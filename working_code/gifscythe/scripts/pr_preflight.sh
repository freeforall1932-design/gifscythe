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
#   P3  fails if the working tree is dirty
#   P4  (--online only) prints repo, main tip + latest main run, branch tip +
#       latest branch run, and the PR state for the branch
#   P5  writes a "## What this is / ## Evidence / ## Not in this PR" skeleton
#       to the --body path
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
  echo "  FAIL [P3] working tree is dirty - commit or stash before the PR boundary"
  git status --porcelain | head -20 | sed 's/^/         /'
  FAIL=$((FAIL + 1))
else
  echo "  PASS [P3] working tree clean"
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

echo "==> Done."
[[ "$FAIL" -eq 0 ]]
