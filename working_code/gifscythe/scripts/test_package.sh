#!/usr/bin/env bash
#
# test_package.sh - NEGATIVE tests for package_portable.sh (audit U-02/U-08/U-14).
#
# The packager used to exit 0 for a folder with no application in it, and CI
# uploaded that folder without ever looking inside. These tests assert the
# opposite: an incomplete package MUST fail, and a complete one must contain
# exactly the promised files.
#
# The negative cases run against a minimal synthetic product tree in a temp
# dir, so they are fast and do not need Qt. The last case runs the REAL script
# against the REAL tree in --engine-cli-only mode.
#
set -uo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
version="${version:-0.1.0}"

PASS=0; FAIL=0
ok()  { echo "  PASS: $1"; PASS=$((PASS + 1)); }
bad() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

ROOT="$(mktemp -d)"
trap 'rm -rf "$ROOT"' EXIT

# ---- Build a minimal synthetic product tree -------------------------------
mk_tree() {
  local t="$ROOT/$1"
  rm -rf "$t"; mkdir -p "$t/working_code/gifscythe/scripts"
  mkdir -p "$t/working_code/gifscythe/release/$version"
  mkdir -p "$t/working_code/gifscythe/build"
  cp "$self/scripts/package_portable.sh" "$t/working_code/gifscythe/scripts/"
  printf '# Gifscythe Versioning\nCurrent version: %s\n' "$version" \
    > "$t/working_code/gifscythe/VERSION.md"
  printf '# Gifscythe\n' > "$t/working_code/gifscythe/README.md"
  printf '#!/bin/sh\necho fake engine\n' > "$t/working_code/gifscythe/release/$version/gifsicle"
  chmod +x "$t/working_code/gifscythe/release/$version/gifsicle"
  printf '#!/bin/sh\necho fake cli\n' > "$t/working_code/gifscythe/build/gifscythe-cli"
  chmod +x "$t/working_code/gifscythe/build/gifscythe-cli"
  printf 'GPL v2 text\n' > "$t/LICENSE"
  printf 'gifsicle GPL v2 text\n' > "$t/COPYING.gifsicle"
  echo "$t/working_code/gifscythe"
}

echo "==> Packaging negative tests"

# 1. No GUI, default mode -> MUST fail closed (this is the exact U-02 repro).
t="$(mk_tree nogui)"
if "$t/scripts/package_portable.sh" >"$ROOT/o1" 2>"$ROOT/e1"; then
  bad "no GUI + default mode exited 0 (U-02 regression)"
else
  if grep -q "Qt GUI not found" "$ROOT/e1"; then
    ok "no GUI + default mode fails closed and says why"
  else
    bad "no GUI failed but without an explanatory error"
  fi
fi

# 2. No GUI + --engine-cli-only -> succeeds, and the folder is what it claims.
t="$(mk_tree optout)"
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/o2" 2>"$ROOT/e2"; then
  pkg="$t/release/$version/Gifscythe"
  miss=""
  [[ -x "$pkg/gifsicle" ]]        || miss+=" engine"
  [[ -x "$pkg/gifscythe-cli" ]]   || miss+=" cli"
  [[ -s "$pkg/LICENSE" ]]         || miss+=" LICENSE"
  [[ -s "$pkg/COPYING.gifsicle" ]]|| miss+=" COPYING.gifsicle"
  [[ -s "$pkg/README.txt" ]]      || miss+=" README.txt"
  if [[ -z "$miss" ]]; then ok "--engine-cli-only produces a complete headless package";
  else bad "--engine-cli-only package missing:$miss"; fi
  if grep -q "no GUI, by request" "$pkg/README.txt"; then
    ok "headless package states its reduced scope in README.txt"
  else
    bad "headless README.txt does not disclose the missing GUI"
  fi
else
  bad "--engine-cli-only failed (rc=$?): $(tail -2 "$ROOT/e2")"
fi

# 3. Missing CLI -> MUST fail (it used to be silently skipped).
t="$(mk_tree nocli)"
rm -f "$t/build/gifscythe-cli"
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/o3" 2>"$ROOT/e3"; then
  bad "missing CLI still exited 0"
else
  grep -q "gifscythe-cli missing" "$ROOT/e3" \
    && ok "missing CLI is fatal" || bad "missing CLI failed without the right message"
fi

# 4. Missing engine -> MUST fail (was already fatal; pin it).
t="$(mk_tree noeng)"
rm -f "$t/release/$version/gifsicle"
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/o4" 2>"$ROOT/e4"; then
  bad "missing engine still exited 0"
else
  ok "missing engine is fatal"
fi

# 5. Missing license set -> MUST fail (audit U-08: a GPL v2 engine shipped with
#    no license text used to sail through because every copy was [[ -f ]]-guarded).
t="$(mk_tree nolic)"
# The license set lives at the REPO ROOT, i.e. two levels above the product dir.
rm -f "$t/../../LICENSE" "$t/../../COPYING.gifsicle"
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/o5" 2>"$ROOT/e5"; then
  bad "missing licenses still exited 0"
else
  grep -q "no LICENSE or COPYING" "$ROOT/e5" \
    && ok "missing license set is fatal" || bad "missing licenses failed without the right message"
fi

# 6. Stale staging must be wiped, not merged into.
t="$(mk_tree stale)"
mkdir -p "$t/release/$version/Gifscythe"
echo leftover > "$t/release/$version/Gifscythe/STALE.txt"
"$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/o6" 2>"$ROOT/e6"
if [[ -e "$t/release/$version/Gifscythe/STALE.txt" ]]; then
  bad "stale file survived the re-cut"
else
  ok "staging dir is wiped before packaging"
fi

# 7. Unknown argument -> non-zero (no silent ignore).
t="$(mk_tree badarg)"
if "$t/scripts/package_portable.sh" --nope >"$ROOT/o7" 2>"$ROOT/e7"; then
  bad "unknown packager argument exited 0"
else
  ok "unknown packager argument is rejected"
fi

# 8. The REAL script against the REAL tree, headless (engine + CLI are built by
#    ./build.sh, which this repo's CI always runs before packaging).
if [[ -x "$self/build/gifscythe-cli" && -f "$self/release/$version/gifsicle" ]]; then
  if "$self/scripts/package_portable.sh" --engine-cli-only >"$ROOT/o8" 2>"$ROOT/e8"; then
    if grep -q "verified present and non-empty" "$ROOT/o8"; then
      ok "real tree: headless package built and self-verified"
    else
      bad "real tree: package built but the assertion pass did not report"
    fi
  else
    bad "real tree: --engine-cli-only failed: $(tail -2 "$ROOT/e8")"
  fi
else
  echo "  SKIP: real-tree case (run ./build.sh first)"
fi

echo "==> Done. $PASS passed, $FAIL failed."
[[ "$FAIL" -eq 0 ]]
