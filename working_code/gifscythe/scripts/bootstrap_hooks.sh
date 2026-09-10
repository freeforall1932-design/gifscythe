#!/usr/bin/env bash
#
# bootstrap_hooks.sh - point this clone at the repo's own git hooks.
#
# Git does NOT copy .githooks/ when you clone, so .githooks/pre-push is inert
# until this runs. build.sh calls it, so a normal build is enough; run it by
# hand if you are not building:
#
#   working_code/gifscythe/scripts/bootstrap_hooks.sh
#
# Verify it took:
#   git config core.hooksPath     ->  .githooks
#
set -uo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="$(cd "$self/../.." && pwd)"

if ! git -C "$root" rev-parse --git-dir >/dev/null 2>&1; then
  echo "bootstrap_hooks: $root is not a git checkout - nothing to do." >&2
  exit 0
fi

current="$(git -C "$root" config core.hooksPath 2>/dev/null || true)"
if [[ "$current" == ".githooks" ]]; then
  echo "==> hooks already bootstrapped (core.hooksPath=.githooks)"
  exit 0
fi

# Never silently hijack a hooks path the developer set on purpose.
if [[ -n "$current" ]]; then
  echo "bootstrap_hooks: core.hooksPath is already '$current' - leaving it alone." >&2
  echo "  To use the repo's pre-push doc gate, run:" >&2
  echo "    git -C $root config core.hooksPath .githooks" >&2
  exit 0
fi

git -C "$root" config core.hooksPath .githooks \
  && echo "==> core.hooksPath set to .githooks (pre-push doc gate is now live)" \
  || { echo "bootstrap_hooks: could not set core.hooksPath." >&2; exit 1; }
