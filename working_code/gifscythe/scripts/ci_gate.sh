#!/usr/bin/env bash
#
# ci_gate.sh - does this workflow run need to do the work, or is another run already covering it?  (S34)
#
# The workflow triggers on `push` (every branch) AND `pull_request`, so a push to a branch with an open
# PR started the same jobs twice: eight checks per commit on the PR page, and double the runner minutes.
# Dropping the `push` trigger would end that, but it would also end CI on a branch that has no PR yet -
# the loop the agent sessions depend on (push, read the run, fix, open the PR; run 36966494878 named
# N-30 before PR #10 existed). So the push run stays, and this gate makes it skip itself exactly when
# the PR's own run covers the commit.
#
#   skip=true  only when ALL of these hold:
#     - the event is a push to a branch other than main (main, tags and pull_request events always run)
#     - an open PR has that branch as its head
#     - GitHub reports that PR as mergeable: a PR with merge conflicts gets NO pull_request run, so the
#       push run is the only CI it has
#   skip=false in every other case, and for every doubt: the API failing, `gh` missing, mergeable still
#   `null` after the retries. A duplicate run costs minutes; a missing run costs the proof.
#
# Inputs (the workflow passes them from the github context):
#   EVENT   github.event_name          REF     github.ref
#   BRANCH  github.ref_name            REPO    github.repository       OWNER  github.repository_owner
#   GH_TOKEN                           GS_GATE_SLEEP  seconds between `mergeable` polls (default 4; 0 in tests)
# Output: `skip=true|false` appended to $GITHUB_OUTPUT (printed to stdout too), plus a notice annotation.
# Exit:   always 0 - the gate must never be the reason a run turns red. Test: tests/test_ci_gate.py
#
set -uo pipefail

EVENT="${EVENT:-}"
REF="${REF:-}"
BRANCH="${BRANCH:-}"
REPO="${REPO:-}"
OWNER="${OWNER:-}"
SLEEP="${GS_GATE_SLEEP:-4}"

skip=false
why="not a push to a branch other than main"

if [[ "$EVENT" == "push" && "$REF" == refs/heads/* && "$REF" != "refs/heads/main" ]]; then
  why="push to a branch with no open pull request"
  if ! command -v gh >/dev/null 2>&1; then
    why="gh is not available - running to be safe"
  elif ! prs="$(gh api "repos/$REPO/pulls?state=open&head=$OWNER:$BRANCH" --jq '.[].number' 2>&1)"; then
    why="could not list pull requests (${prs//$'\n'/ }) - running to be safe"
  elif [[ -n "$prs" ]]; then
    why="open PR(s) found but none is mergeable yet"
    for pr in $prs; do
      mergeable="null"
      # GitHub computes mergeability in the background after a push: null means "not yet", so ask again.
      for _ in 1 2 3 4 5 6 7 8; do
        mergeable="$(gh api "repos/$REPO/pulls/$pr" --jq '.mergeable' 2>/dev/null || echo error)"
        [[ "$mergeable" != "null" ]] && break
        sleep "$SLEEP"
      done
      if [[ "$mergeable" == "true" ]]; then
        skip=true
        why="PR #$pr is open and mergeable - its pull_request run covers this commit"
        break
      fi
      why="PR #$pr is open but mergeable=$mergeable - its pull_request run may not exist, so this push runs"
    done
  fi
fi

echo "skip=$skip"
echo "ci_gate: skip=$skip ($why)"
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
  echo "skip=$skip" >> "$GITHUB_OUTPUT"
fi
if [[ -n "${GITHUB_ACTIONS:-}" ]]; then
  echo "::notice title=CI gate::skip=$skip - $why"
fi
exit 0
