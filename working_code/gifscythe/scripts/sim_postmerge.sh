#!/usr/bin/env bash
#
# sim_postmerge.sh - what will main's docs gate say after this branch merges?  (S34)
#
#   ./scripts/sim_postmerge.sh                                   a merge commit, now
#   ./scripts/sim_postmerge.sh --style squash --date "2026-10-05 09:00"
#   ./scripts/sim_postmerge.sh --style rebase
#   ./scripts/sim_postmerge.sh --base origin/main --head HEAD --keep
#
# Why. Main's push run went red the moment a PR merged four times (N-26), while the same tree was green
# on the PR's own run: G10 compares the docs' base sha with main's tip and the tip's first parent, and
# G11 compares the log date with the newest non-doc commit - both depend on the SHAPE of the merge and
# on the DAY it happens, and neither can be seen from a PR's green run. This builds the commit GitHub
# would create on top of <base> in a scratch clone (full history, like the CI `docs` job), points
# origin/main at it, builds the engine (G9b re-measures the unit count) and runs the gate exactly as CI
# does:  ./build.sh  then  check_docs.sh --no-gate-run.
#
#   --style merge|squash|rebase   what the owner clicks (default merge). rebase is known to fail G10.
#   --date  "YYYY-MM-DD [HH:MM]" the UTC moment of the merge (default: now). G11 reads this day.
#   --base  REF                   main as it is when the merge happens (default origin/main)
#   --head  REF                   the PR head (default HEAD; uncommitted changes are NOT included)
#   --keep                        keep the scratch clone and print its path
#
# Exit: 0 the docs gate is green after that merge / 1 it is not / 2 usage or setup problem /
#       3 the merge would conflict (the branch is stale: merge main into it, re-anchor the base lines)
# Nothing is written into the checkout; the scratch clone lives under $TMPDIR.
#
set -uo pipefail
export LC_ALL=C

here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="$(cd "$here/../.." && pwd)"

STYLE=merge
DATE=""
BASE=origin/main
HEAD_REF=HEAD
KEEP=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --style) STYLE="${2:-}"; shift ;;
    --date) DATE="${2:-}"; shift ;;
    --base) BASE="${2:-}"; shift ;;
    --head) HEAD_REF="${2:-}"; shift ;;
    --keep) KEEP=1 ;;
    -h|--help) sed -n '2,32p' "${BASH_SOURCE[0]}"; exit 0 ;;
    *) echo "sim_postmerge.sh: unknown argument '$1' (see --help)" >&2; exit 2 ;;
  esac
  shift
done
case "$STYLE" in merge|squash|rebase) ;; *) echo "sim_postmerge.sh: --style must be merge, squash or rebase" >&2; exit 2 ;; esac

cd "$root" || exit 2
if [[ "$(git rev-parse --is-shallow-repository)" == "true" ]]; then
  echo "sim_postmerge.sh: this clone is shallow - G10 and G11 skip in shallow clones, so the answer would mean nothing. git fetch --unshallow --tags --prune origin" >&2
  exit 2
fi
base_sha="$(git rev-parse --verify "$BASE^{commit}" 2>/dev/null)" || { echo "sim_postmerge.sh: cannot resolve --base $BASE" >&2; exit 2; }
head_sha="$(git rev-parse --verify "$HEAD_REF^{commit}" 2>/dev/null)" || { echo "sim_postmerge.sh: cannot resolve --head $HEAD_REF" >&2; exit 2; }
[[ -n "$(git status --porcelain)" ]] && echo "note: uncommitted changes are NOT part of the simulation (it merges commits)"
[[ -n "$DATE" ]] || DATE="$(date -u '+%Y-%m-%d %H:%M:%S')"
iso="$(date -u -d "$DATE" '+%Y-%m-%dT%H:%M:%S+00:00' 2>/dev/null)" || { echo "sim_postmerge.sh: cannot read --date '$DATE'" >&2; exit 2; }

tmp="$(mktemp -d "${TMPDIR:-/tmp}/gs-sim-postmerge.XXXXXX")"
[[ "$KEEP" == "1" ]] || trap 'rm -rf "$tmp"' EXIT
repo="$tmp/repo"
git clone -q --no-local "$root" "$repo" 2>/dev/null || { echo "sim_postmerge.sh: could not clone $root" >&2; exit 2; }
# remote-tracking refs are not cloned: bring base and head across explicitly
git -C "$repo" fetch -q "$root" "+refs/remotes/*:refs/remotes/src/*" "+refs/heads/*:refs/heads/src/*" 2>/dev/null
for s in "$base_sha" "$head_sha"; do
  git -C "$repo" cat-file -e "$s^{commit}" 2>/dev/null || { echo "sim_postmerge.sh: commit ${s:0:7} did not reach the scratch clone" >&2; exit 2; }
done

cd "$repo" || exit 2
git config user.name "sim-postmerge"
git config user.email "sim-postmerge@example.invalid"
export GIT_AUTHOR_DATE="$iso" GIT_COMMITTER_DATE="$iso"
git checkout -q --detach "$base_sha"
conflict=0
case "$STYLE" in
  merge)  git merge -q --no-ff -m "Merge pull request #0 from sim/branch" "$head_sha" >/dev/null 2>&1 || conflict=1 ;;
  squash) { git merge -q --squash "$head_sha" >/dev/null 2>&1 && git commit -q -m "Squashed pull request (#0)"; } || conflict=1 ;;
  rebase) { git checkout -q --detach "$head_sha" && git rebase -q "$base_sha" >/dev/null 2>&1; } || conflict=1 ;;
esac
if [[ "$conflict" == "1" ]]; then
  echo "CONFLICT: GitHub would not merge this cleanly ($STYLE of ${head_sha:0:7} onto ${base_sha:0:7}) - the branch is stale."
  echo "          Merge main into the branch, then re-anchor the base lines to main's tip (G10)."
  exit 3
fi
git checkout -q -B main
git update-ref refs/remotes/origin/main HEAD
echo "==> simulated $STYLE of ${head_sha:0:7} onto ${base_sha:0:7} dated $iso -> main = $(git rev-parse --short HEAD) (parents: $(git log -1 --format=%p))"

cd "$repo/working_code/gifscythe" || exit 2
./build.sh >"$tmp/build.log" 2>&1 || echo "note: ./build.sh failed in the scratch clone (see $tmp/build.log) - G9b will say so"
./scripts/check_docs.sh --no-gate-run >"$tmp/gate.log" 2>&1
rc=$?
grep -E '^\s+(FAIL|SKIP) \[' "$tmp/gate.log" | cut -c1-230
grep -E '\[G(8|10|11)\]' "$tmp/gate.log" | cut -c1-230
tail -n 1 "$tmp/gate.log"
if [[ "$rc" == "0" ]]; then
  echo "RESULT: the docs gate is GREEN after the simulated $STYLE on $(date -u -d "$DATE" '+%Y-%m-%d')."
else
  echo "RESULT: the docs gate is RED after the simulated $STYLE on $(date -u -d "$DATE" '+%Y-%m-%d') - main's docs job would fail."
fi
[[ "$KEEP" == "1" ]] && echo "scratch clone kept: $repo"
exit "$rc"
