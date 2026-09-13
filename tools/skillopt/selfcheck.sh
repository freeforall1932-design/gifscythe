#!/usr/bin/env bash
# selfcheck.sh — mechanical proof that the SkillOpt integration still satisfies the
# conditions it was accepted on (docs/planning/SKILLOPT_INTEGRATION_QUERY.md §3):
# quarantined, optional and inert, MIT notice intact — plus pin consistency and
# work-tree hygiene. No gate calls this script; a session runs it by hand, and any
# check passes whether or not the pinned subtree has been materialized.
#
# Exit 0 = every check passed or skipped; exit 2 = at least one FAIL, with the fix
# printed next to it.
set -uo pipefail
export LC_ALL=C.UTF-8 LC_CTYPE=C.UTF-8 2>/dev/null || { export LC_ALL=C LC_CTYPE=C; }

HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)
PARENT=$(cd -- "$HERE/../.." >/dev/null 2>&1 && pwd -P)
SUBREL="tools/skillopt/upstream"
SRC="$HERE/upstream"
PASS=0 FAIL=0 SKIP=0
Z=""; R=""; N=""; if [ -t 1 ]; then Z=$'\033[32m'; R=$'\033[31m'; N=$'\033[0m'; fi
ok()   { PASS=$((PASS+1)); printf '%s  PASS%s %s — %s\n' "$Z" "$N" "$1" "$2"; }
bad()  { FAIL=$((FAIL+1)); printf '%s  FAIL%s %s — %s\n' "$R" "$N" "$1" "$2"; }
skip() { SKIP=$((SKIP+1)); printf '  SKIP   %s — %s\n' "$1" "$2"; }
hd()   { printf '\n== %s\n' "$1"; }

# quoted, case-insensitive, counted search; prints the number of hits
hits_in() { # <pattern> <file...>
  local n=0 f
  for f in "$@"; do
    [ -f "$f" ] || continue
    n=$((n + $(grep -ic "$1" "$f" 2>/dev/null | awk '{s+=$1} END{print s+0}')))
  done
  printf '%s' "$n"
}

hd "1. quarantine: nothing in the shipped trees mentions SkillOpt"
Q_HITS=$(grep -ril --exclude-dir=.git "skillopt" "$PARENT/working_code" "$PARENT/web" "$PARENT/.github/workflows" "$PARENT/docs/ci" 2>/dev/null | tr '\n' ' ')
if [ -z "${Q_HITS// /}" ]; then
  ok "product + CI trees clean" "no reference under working_code/, web/, .github/workflows/, docs/ci/"
else
  bad "product tree names SkillOpt" "remove it from: $Q_HITS (SkillOpt must stay outside the shipped tree; see §3 of the integration query)"
fi

hd "2. packaging: the installers stage an explicit allow-list, so tools/ cannot leak in"
PKG_DIR="$PARENT/working_code/gifscythe/scripts"
LEAK=""
for s in package_common.sh package_linux.sh package_windows.sh; do
  [ -f "$PKG_DIR/$s" ] || continue
  c=$(hits_in "skillopt" "$PKG_DIR/$s"); t=$(grep -c "tools/" "$PKG_DIR/$s" 2>/dev/null || true)
  if [ "${c:-0}" -ne 0 ] || [ "${t:-0}" -ne 0 ]; then LEAK="$LEAK $s(skillopt=$c,tools=$t)"; fi
done
if [ -z "$LEAK" ]; then
  ok "staging allow-list clean" "package_common.sh / package_linux.sh / package_windows.sh stage no tools/ path and no SkillOpt reference"
else
  bad "a packager references tools/ or SkillOpt" "$LEAK — the subtree must never be staged into a package"
fi

hd "3. gate indifference: no gate, build step, or pre-submit script may depend on SkillOpt"
GATES="$PKG_DIR/build.sh $PKG_DIR/check_docs.sh $PKG_DIR/verify_audit.sh $PKG_DIR/sweep_stale.sh $PKG_DIR/pr_preflight.sh $PKG_DIR/review_change.sh $PARENT/.githooks/pre-push"
GHITS=$(for f in $GATES; do [ -f "$f" ] || continue; c=$(grep -ic skillopt "$f" 2>/dev/null || true); [ "${c:-0}" -gt 0 ] && printf '%s(%s) ' "$(basename "$f")" "$c"; done)
if [ -z "$GHITS" ]; then
  ok "no gate reads SkillOpt" "build.sh, check_docs.sh, verify_audit.sh, sweep_stale.sh, pr_preflight.sh, review_change.sh, .githooks/pre-push: zero references"
else
  bad "a gate depends on SkillOpt" "$GHITS — revert it; the integration must stay optional (an uninitialized submodule would otherwise fail the gate)"
fi

hd "4. pin consistency"
REC_IDX=$(git -C "$PARENT" ls-files -s -- "$SUBREL" 2>/dev/null | awk '$1=="160000"{print $2}')
REC_HEAD=$(git -C "$PARENT" ls-tree HEAD -- "$SUBREL" 2>/dev/null | awk '{print $3}')
REC_GITLINK=${REC_IDX:-$REC_HEAD}
REC_FILE=$(sed -n 's/^commit=//p' "$HERE/upstream.pin" 2>/dev/null | head -1)
if [ -z "$REC_GITLINK" ]; then
  bad "no gitlink for $SUBREL" "neither the index nor HEAD records a submodule commit — the pin is what makes this reproducible; re-add the submodule"
elif ! printf '%s' "$REC_GITLINK" | grep -Eq '^[0-9a-f]{40}$'; then
  bad "gitlink is not a 40-hex commit" "got: $REC_GITLINK"
elif [ -n "$REC_IDX" ] && [ -n "$REC_HEAD" ] && [ "$REC_IDX" != "$REC_HEAD" ]; then
  bad "staged gitlink differs from HEAD" "index $REC_IDX vs HEAD $REC_HEAD — move the pin and upstream.pin in one commit, not across two"
elif [ "$REC_GITLINK" != "$REC_FILE" ]; then
  bad "upstream.pin disagrees with the gitlink" "pin file says ${REC_FILE:-unset}, git records $REC_GITLINK — fix upstream.pin in the same commit that moves the gitlink"
elif [ -z "$REC_FILE" ]; then
  bad "upstream.pin has no commit line" "the pin needs a machine-readable upstream.pin"
else
  ok "pin single-sourced" "the gitlink and upstream.pin both record $REC_GITLINK"
fi
if [ -e "$SRC/.git" ]; then
  MAT=$(git -C "$SRC" rev-parse HEAD 2>/dev/null || echo "")
  if [ "$MAT" = "$REC_GITLINK" ]; then
    ok "subtree matches the pin" "materialized at $MAT ($(git -C "$SRC" describe --tags --always 2>/dev/null || echo '?'))"
  else
    bad "subtree is off the pin" "materialized at ${MAT:-unresolvable}, expected $REC_GITLINK — run: ./skillopt.sh init (it aligns the subtree)"
  fi
else
  skip "subtree not materialized" "normal in a fresh clone or CI run; the pin above is what a session fetches"
fi

hd "5. license notice"
if [ -e "$SRC/LICENSE" ]; then
  if head -1 "$SRC/LICENSE" | grep -qi "MIT License"; then ok "upstream LICENSE present at the pin" "$SRC/LICENSE is MIT"; else bad "unexpected license at the pin" "$(head -1 "$SRC/LICENSE")"; fi
  if cmp -s "$SRC/LICENSE" "$HERE/LICENSE.SkillOpt"; then
    ok "the in-repo copy is verbatim" "LICENSE.SkillOpt is byte-identical to the pinned upstream LICENSE"
  else
    bad "LICENSE.SkillOpt drifted" "re-copy it from the pinned tree: cp upstream/LICENSE LICENSE.SkillOpt"
  fi
else
  if [ -s "$HERE/LICENSE.SkillOpt" ] && head -1 "$HERE/LICENSE.SkillOpt" | grep -qi "MIT License"; then
    skip "upstream LICENSE not fetched" "the notice is preserved verbatim in LICENSE.SkillOpt (MIT, Copyright (c) 2026 Microsoft Corporation)"
  else
    bad "no MIT notice anywhere" "LICENSE.SkillOpt is missing or not MIT — the attribution requirement has no fallback"
  fi
fi

hd "6. hygiene: a local install can never dirty the parent work tree"
# A generated directory that git can see would make every SkillOpt session dirty, and the
# dirty-tree gate (check_docs.sh G18 / pr_preflight P3) fails on ANY change. So each one is
# either reported ignored by git (checked when it exists) or covered by an explicit
# .gitignore rule (checked when a fresh clone has not created it yet).
gen_ignored() { # <path-relative-to-repo-root>  -> 0 if git will not see it
  if [ -d "$PARENT/$1" ]; then git -C "$PARENT" check-ignore -q "$1"; return $?; fi
  grep -qxF "/$1/" "$PARENT/.gitignore"
}
for d in .venv wheelhouse state runs outputs .skillopt; do
  rel="tools/skillopt/$d"
  if [ -d "$PARENT/$rel" ]; then how="git reports it ignored"; else how=".gitignore rule /$rel/ present"; fi
  if gen_ignored "$rel"; then
    ok "$rel never tracked" "$how"
  else
    bad "$rel is visible to git" "add /$rel/ to .gitignore — otherwise every session that installs SkillOpt fails the dirty-tree gate"
  fi
done
if [ -e "$SRC/.git" ]; then
  DIRTY=$(git -C "$SRC" status --porcelain | wc -l | tr -d ' ')
  if [ "$DIRTY" = "0" ]; then
    ok "pinned tree not written to" "upstream/ reports no changes of its own (its own .gitignore covers build/ and *.egg-info/, so an in-tree pip install could not leak to the parent either)"
  else
    bad "the pinned tree carries $DIRTY local change(s)" "SkillOpt tooling must not patch the subtree in place; clean it with: git -C $SUBREL clean -xqfd  (and register custom code in this directory instead)"
  fi
  IGN_LOCAL=$(git -C "$PARENT" config --get "submodule.$SUBREL.ignore" 2>/dev/null || true)
  IGN_FILE=$(git -C "$PARENT" config -f "$PARENT/.gitmodules" "submodule.$SUBREL.ignore" 2>/dev/null || true)
  if [ "${IGN_FILE:-}" = "dirty" ]; then
    ok "submodule ignore = dirty" "droppings inside the subtree stay out of the parent status, while a HEAD that drifts off the pin still shows (local config: ${IGN_LOCAL:-inherited from .gitmodules on init})"
  else
    bad "submodule ignore setting in .gitmodules" "expected 'dirty', got '${IGN_FILE:-unset}' — with 'all' a drifted pin would be hidden, without it a local run would fail the dirty-tree gate"
  fi
else
  skip "subtree work-tree checks" "nothing is materialized to inspect"
fi

printf '\n==> Done. %d passed, %d failed, %d skipped.\n' "$PASS" "$FAIL" "$SKIP"
if [ "$FAIL" -ne 0 ]; then
  printf '    FAIL: the SkillOpt integration no longer meets the conditions it was accepted on.\n'
  exit 2
fi
