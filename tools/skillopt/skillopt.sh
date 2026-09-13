#!/usr/bin/env bash
# skillopt.sh — the one entry point for the in-repo SkillOpt integration
# (owner decision OD-15 = a, recorded session S18). README.md in this directory says
# what this directory is for and what it must never be used for.
#
# Subcommands
#   status                 what is pinned, materialized, installed (read-only)
#   init                   create tools/skillopt/.venv and install the PINNED tree into it
#     --offline            install from tools/skillopt/wheelhouse instead of the network
#   wheelhouse             fill tools/skillopt/wheelhouse with the pinned wheel and its
#                          dependencies, so a later `init --offline` needs no network
#   run -- <cmd> [args..]  run an installed SkillOpt CLI with this directory as the
#                          working directory, so upstream's relative write paths
#                          (outputs/, .skillopt/) land here and not in the product tree
#   selftest               upstream's own suite against the pinned tree + ./selfcheck.sh;
#                          the offline acceptance test for this integration (needs pytest
#                          in the venv: $VENV/bin/pip install pytest, once)
#
# Invariants, because the repo's rules require them:
#   * no gate, build step, or CI job calls this script — a session runs it by hand
#   * the pinned subtree is never written to: the install builds from a copy in $TMPDIR
#   * no credential is read, printed, or stored here; `run` only passes the caller's
#     environment through to the CLI
set -uo pipefail
export LC_ALL=C.UTF-8 LC_CTYPE=C.UTF-8 2>/dev/null || { export LC_ALL=C LC_CTYPE=C; }

HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)
PARENT=$(git -C "$HERE" rev-parse --show-toplevel 2>/dev/null || echo "")
SUB="tools/skillopt/upstream"        # the subtree path inside the parent repo
SRC="$HERE/upstream"
VENV="$HERE/.venv"
WH="$HERE/wheelhouse"
STATE="$HERE/state"
PINFILE="$HERE/upstream.pin"

Z=""; R=""; N=""; if [ -t 1 ]; then Z=$'\033[32m'; R=$'\033[31m'; N=$'\033[0m'; fi
say()  { printf '%s\n' "$*"; }
info() { printf '    %s\n' "$*"; }
ok()   { printf '%s  ok%s   %s\n' "$Z" "$N" "$*"; }
bad()  { printf '%s  FAIL%s %s\n' "$R" "$N" "$*"; }
skip() { printf '  skip   %s\n' "$*"; }
die()  { printf '%serror:%s %s\n' "$R" "$N" "$*" >&2; exit 2; }

pin() { if [ -f "$PINFILE" ]; then sed -n "s/^$1=//p" "$PINFILE" | head -1; fi; }

pip_tool() {
  if [ -x "$VENV/bin/pip" ]; then printf '%s' "$VENV/bin/pip"
  elif python3 -m pip --version >/dev/null 2>&1; then printf '%s' "python3 -m pip"
  else die "no pip available (python3 -m pip --version fails)"; fi
}

# The recorded pin is the gitlink in HEAD; upstream.pin is its human-readable twin.
recorded_sha() {
  local s=""
  if [ -n "$PARENT" ]; then s=$(git -C "$PARENT" ls-tree HEAD -- "$SUB" 2>/dev/null | awk '{print $3}'); fi
  if [ -z "$s" ]; then s=$(pin commit); fi
  printf '%s' "$s"
}
materialized_sha() {
  if [ -e "$SRC/.git" ]; then git -C "$SRC" rev-parse HEAD 2>/dev/null || true; fi
}

ensure_tree() {
  [ -n "$PARENT" ] || die "tools/skillopt is not inside a git work tree"
  if [ ! -e "$SRC/pyproject.toml" ]; then
    say "==> materializing the pinned subtree (~6 MB at this pin, shallow)"
    git -C "$PARENT" submodule update --init --depth 1 -- "$SUB" 2>/dev/null \
      || git -C "$PARENT" submodule update --init -- "$SUB" \
      || die "submodule init failed (offline, or no route to github.com?). The pinned commit is $(recorded_sha)"
  fi
  [ -e "$SRC/pyproject.toml" ] || die "$SRC is still empty after init"
  # Hygiene: keep build droppings inside the subtree out of the PARENT's status, while
  # still reporting a HEAD that has drifted off the pin. .gitmodules carries the same
  # setting; set it locally too for clones that were initialized before it landed.
  if [ "$(git -C "$PARENT" config --get "submodule.$SUB.ignore" 2>/dev/null || true)" != "dirty" ]; then
    git -C "$PARENT" config "submodule.$SUB.ignore" dirty || true
  fi
}

align_to_pin() {
  local rec mat
  rec=$(recorded_sha); mat=$(materialized_sha)
  if [ -n "$mat" ] && [ "$mat" != "$rec" ]; then
    say "==> subtree is at $mat, aligning it to the recorded pin $rec"
    git -C "$SRC" fetch --depth 1 origin "$rec" 2>/dev/null || git -C "$SRC" fetch -q origin "$rec" 2>/dev/null || true
    git -C "$SRC" checkout -q "$rec" || die "could not check out the pinned commit $rec"
  fi
}

build_wheel() {
  # Build the pinned tree into a wheel FROM A COPY: an in-tree `pip install ./upstream`
  # leaves skillopt.egg-info/ and build/ inside the pinned tree, which we promise not to do.
  local out="$1" tmp pip
  pip=$(pip_tool)
  tmp=$(mktemp -d "${TMPDIR:-/tmp}/skillopt-pinned.XXXXXX") || die "mktemp failed"
  mkdir -p "$out" "$tmp/src"
  say "==> building a wheel from the pinned tree (copy under $tmp)"
  if command -v tar >/dev/null 2>&1; then
    ( cd "$SRC" && tar cf - --exclude=./.git . ) | ( cd "$tmp/src" && tar xf - ) || die "copy failed"
  else
    cp -R "$SRC/." "$tmp/src/" || die "copy failed"
  fi
  $pip wheel -q --no-deps -w "$out" "$tmp/src" || { rm -rf "$tmp"; die "pip wheel failed"; }
  rm -rf "$tmp"
}

status_report() {
  local rec mat vver ip n=0 v
  rec=$(recorded_sha); mat=$(materialized_sha)
  say "  pin             $rec ($(pin describe), version $(pin version), $(pin url))"
  if [ -n "$mat" ]; then
    if [ "$mat" = "$rec" ]; then ok   "subtree materialized at the pin"; \
    else bad "subtree is at $mat, NOT the recorded pin $rec (run: init)"; fi
    info "subtree work tree: $(git -C "$SRC" status --porcelain | wc -l) changed path(s) vs its own HEAD"
  else
    skip "subtree not materialized (normal for a fresh clone or CI) — init fetches it"
  fi
  if [ -x "$VENV/bin/python" ]; then
    vver=$("$VENV/bin/python" -c 'import skillopt;print(skillopt.__version__)' 2>/dev/null || echo "broken")
    say "  venv            present, skillopt $vver ($(du -sh "$VENV" 2>/dev/null | awk '{print $1}'))"
    if [ -f "$STATE/installed-pin" ]; then
      ip=$(cat "$STATE/installed-pin" 2>/dev/null || true)
      if [ "$ip" = "$rec" ]; then ok   "the venv was built from the recorded pin"; \
      else bad "the venv was built from ${ip:-unknown}, the pin is $rec (run: init)"; fi
    else
      skip "no install stamp at state/installed-pin"
    fi
  else
    say "  venv            absent — init takes ~12 s with network, ~6 s from the wheelhouse"
  fi
  if [ -d "$WH" ]; then
    say "  wheelhouse      $(find "$WH" -maxdepth 1 -name '*.whl' 2>/dev/null | wc -l) wheels, $(du -sh "$WH" 2>/dev/null | awk '{print $1}') — init --offline available"
  else
    say "  wheelhouse      absent — 'wheelhouse' makes init --offline possible without a network"
  fi
  for v in OPENAI_API_KEY AZURE_OPENAI_API_KEY ANTHROPIC_API_KEY OPTIMIZER_API_KEY TARGET_API_KEY; do
    if [ -n "${!v:-}" ]; then n=$((n+1)); fi
  done
  info "model credential env vars present in this shell: $n of 5 checked (a train/eval run needs a backend configured; status and selfcheck do not)"
}

case "${1:-status}" in
  status|"" )
    say "==> SkillOpt in-repo integration (OD-15 = a, session S18)"
    status_report
    exit 0 ;;
  help|--help|-h )
    awk 'NR>1 && /^#/{sub(/^# ?/,""); print; next} NR>1{exit}' "$0"
    say ""; say "==> current state"
    status_report
    exit 0 ;;
  init )
    shift || true
    offline=""
    if [ "${1:-}" = "--offline" ]; then offline=1; shift || true; fi
    [ $# -eq 0 ] || die "init accepts only --offline"
    command -v python3 >/dev/null 2>&1 || die "python3 is required (upstream needs $(pin requires_python))"
    ensure_tree
    align_to_pin
    if [ ! -d "$VENV" ]; then
      say "==> creating $VENV"
      python3 -m venv "$VENV" || die "python3 -m venv failed"
    fi
    mkdir -p "$STATE"
    if [ -n "$offline" ]; then
      [ -d "$WH" ] || die "--offline needs $WH — build it once with: $(basename "$0") wheelhouse"
      say "==> installing the pinned wheel from the wheelhouse (no network)"
      "$VENV/bin/pip" install -q --disable-pip-version-check --no-index --find-links "$WH" skillopt \
        || die "offline install failed — is the pinned wheel in $WH?"
    else
      build_wheel "$STATE/wheels"
      say "==> installing skillopt and its dependencies into the venv"
      "$VENV/bin/pip" install -q --disable-pip-version-check --find-links "$STATE/wheels" skillopt \
        || die "install failed"
    fi
    printf '%s\n' "$(recorded_sha)" > "$STATE/installed-pin"
    say "==> verifying"
    "$VENV/bin/python" -c 'import skillopt, scripts.train, scripts.eval_only; print("    import ok: skillopt", skillopt.__version__)' \
      || die "the venv cannot import skillopt / scripts.train / scripts.eval_only"
    "$VENV/bin/skillopt-train" --help >/dev/null 2>&1 || die "skillopt-train --help failed"
    ok "installed $(recorded_sha) into .venv; the pinned tree has $(git -C "$SRC" status --porcelain | wc -l) changed path(s) of its own"
    say "next: $(basename "$0") run -- skillopt-sleep status   (no network needed)"
    say "      ./selfcheck.sh                                   (proves the quarantine and the pin)"
    ;;
  wheelhouse )
    ensure_tree
    align_to_pin
    mkdir -p "$WH"
    build_wheel "$WH"
    say "==> fetching the pinned version's dependencies into $WH so init --offline needs no network"
    pip=$(pip_tool)
    $pip download -q -d "$WH" "$SRC" || die "pip download failed (offline?) — place wheels in $WH by hand for init --offline"
    ok "wheelhouse ready: $(find "$WH" -maxdepth 1 -name '*.whl' | wc -l) wheels, $(du -sh "$WH" | awk '{print $1}'), git-ignored and never committed"
    ;;
  run )
    shift || true
    if [ "${1:-}" = "--" ]; then shift || true; fi
    [ $# -gt 0 ] || die "usage: $(basename "$0") run -- <command> [args]  e.g. run -- skillopt-sleep status"
    [ -x "$VENV/bin/$1" ] || die "no $1 in $VENV/bin — run: $(basename "$0") init"
    say "==> running '$*' with $HERE as the working directory"
    ( cd "$HERE" && exec "$VENV/bin/$1" "${@:2}" )
    ;;
  selftest )
    # Upstream's own test suite, run against the pinned tree and the installed package.
    # This is the offline acceptance test for the integration: no API key, no network.
    ensure_tree
    align_to_pin
    [ -x "$VENV/bin/python" ] || die "no venv yet — run: $(basename "$0") init"
    "$VENV/bin/python" -m pytest --version >/dev/null 2>&1 \
      || die "pytest is not in the venv — one-time: $VENV/bin/pip install pytest"
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/skillopt-selftest.XXXXXX") || die "mktemp failed"
    ( cd "$SRC" && tar cf - --exclude=./.git . ) | ( cd "$tmp" && tar xf - ) || { rm -rf "$tmp"; die "copy failed"; }
    say "==> running the pinned tree's own suite in $tmp (no credentials, no network)"
    ( cd "$tmp" && PYTHONPATH="$tmp" "$VENV/bin/python" -m pytest tests -q 2>&1 | tail -3 )
    rc=$?
    say "==> and the repo's own SkillOpt checks"
    bash "$HERE/selfcheck.sh" || rc=$?
    rm -rf "$tmp"
    [ $rc -eq 0 ] && ok "selftest passed: the pinned upstream works here and the quarantine holds"
    exit $rc
    ;;
  * )
    die "unknown subcommand '${1:-}' (status | init [--offline] | wheelhouse | run -- <cmd> | selftest | help)" ;;
esac
