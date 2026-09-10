#!/usr/bin/env bash
#
# package_portable.sh - Build the portable release folder.
#
#   ./scripts/package_portable.sh                     # GUI REQUIRED (default)
#   ./scripts/package_portable.sh --engine-cli-only   # explicitly no GUI
#
# FAILS CLOSED (audit U-02 / U-08). The previous version printed
# "Note: Qt GUI not found; packaging engine and CLI only." and then exited 0
# with a folder that contained no application — verified: SCRIPT_EXIT=0 and
# `ls …/Gifscythe/gifscythe` -> No such file or directory. A packager that
# cannot tell a release from a stub is worse than no packager.
#
# Now:
#   * staging dir is wiped first, so a stale folder cannot ride along;
#   * engine, CLI and (unless --engine-cli-only) the GUI are REQUIRED;
#   * a windeployqt failure is fatal, not a "Note:";
#   * the license set is asserted, not `if [[ -f ]]`-guarded;
#   * a final pass re-stats every required file and the script exits non-zero
#     if any is missing or empty.
#
set -euo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
version="${version:-0.1.0}"
out="$self/release/$version/Gifscythe"

engine_cli_only=0
for arg in "$@"; do
  case "$arg" in
    --engine-cli-only) engine_cli_only=1 ;;
    -h|--help)
      echo "usage: package_portable.sh [--engine-cli-only]"
      echo "  default: the Qt GUI is REQUIRED and its absence is fatal"
      echo "  --engine-cli-only: build the headless package on purpose"
      exit 0 ;;
    *) echo "ERROR: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done

fail() { echo "ERROR: $*" >&2; exit 1; }

# ---- 1. Clean staging (a stale folder must never be mistaken for a release) ----
rm -rf "$out"
mkdir -p "$out"

required=()   # paths that MUST exist and be non-empty when we finish

# ---- 2. Engine (always required) ----
engine=""
for cand in "$self/release/$version/gifsicle" "$self/release/$version/gifsicle.exe"; do
  [[ -f "$cand" ]] && { engine="$cand"; break; }
done
[[ -n "$engine" ]] || fail "engine missing from release/$version/ — run ./build.sh first"
cp "$engine" "$out/"
required+=("$out/$(basename "$engine")")

# ---- 3. CLI (always required — it used to be silently skipped) ----
cli=""
for cand in "$self/build/gifscythe-cli" "$self/build/gifscythe-cli.exe"; do
  [[ -x "$cand" ]] && { cli="$cand"; break; }
done
[[ -n "$cli" ]] || fail "gifscythe-cli missing from build/ — run ./build.sh first"
cp "$cli" "$out/"
required+=("$out/$(basename "$cli")")

# ---- 4. GUI (required unless the caller opted out) ----
gui=""
for cand in \
  "$self/build/gifscythe" \
  "$self/build/gifscythe.exe" \
  "$self/build/gui/gifscythe" \
  "$self/build/gui/gifscythe.exe" \
  "$self/build/cmake/gifscythe" \
  "$self/build/cmake/gifscythe.exe" \
  "$self/build-win/gifscythe" \
  "$self/build-win/gifscythe.exe" \
  "$self/build-win/Release/gifscythe.exe" \
  "$self/build-win/Debug/gifscythe.exe"
do
  [[ -f "$cand" ]] && { gui="$cand"; break; }
done

if [[ -z "$gui" ]]; then
  if [[ "$engine_cli_only" == "1" ]]; then
    echo "NOTE: --engine-cli-only given; packaging engine + CLI with no GUI (by request)."
  else
    fail "Qt GUI not found. Build it (./build.sh --all) or pass --engine-cli-only
       to build the headless package on purpose. Looked in:
         build/ build/gui/ build/cmake/ build-win/ build-win/Release/ build-win/Debug/"
  fi
else
  cp "$gui" "$out/"
  required+=("$out/$(basename "$gui")")
  # Deploy the Qt runtime on Windows. A windeployqt failure is FATAL: shipping
  # a GUI with no Qt DLLs produces a folder that does not start on a clean
  # machine, which is exactly what gate C4/D3/D4 exists to catch.
  if command -v windeployqt >/dev/null 2>&1; then
    gui_base="$(basename "$gui")"
    windeployqt --release --no-translations "$out/$gui_base" \
      || fail "windeployqt failed for $gui_base — the package would not run on a clean machine"
  fi
fi

# ---- 5. Licenses (asserted, audit U-08) ----
repo_root="$(cd "$self/../.." && pwd)"
cp "$self/VERSION.md" "$out/" || fail "cannot copy VERSION.md"
cp "$self/README.md"  "$out/" || fail "cannot copy README.md"
required+=("$out/VERSION.md" "$out/README.md")

# Our own license text.
if [[ -f "$repo_root/LICENSE" ]]; then
  cp "$repo_root/LICENSE" "$out/"
elif [[ -f "$repo_root/COPYING" ]]; then
  cp "$repo_root/COPYING" "$out/LICENSE"
else
  fail "no LICENSE or COPYING at the repository root ($repo_root) — refusing to ship unlicensed"
fi
required+=("$out/LICENSE")

# gifsicle is GPL v2-only and MUST travel with the engine binary.
if [[ -f "$repo_root/COPYING.gifsicle" ]]; then
  cp "$repo_root/COPYING.gifsicle" "$out/"
elif [[ -f "$repo_root/reference_code/gifsicle/COPYING" ]]; then
  cp "$repo_root/reference_code/gifsicle/COPYING" "$out/COPYING.gifsicle"
else
  fail "no gifsicle GPL v2 text found (COPYING.gifsicle or reference_code/gifsicle/COPYING)"
fi
required+=("$out/COPYING.gifsicle")

# ---- 6. Release notes ----
scope="engine + CLI + Qt GUI"
[[ "$engine_cli_only" == "1" && -z "$gui" ]] && scope="engine + CLI (no GUI, by request)"
cat > "$out/README.txt" <<EOF
Gifscythe $version — portable release
Contents: $scope

Run gifscythe (GUI) or gifscythe-cli for the command-line interface.
The application does not install anything and can run from this folder.

gifsicle (the engine) is GPL v2-only — see COPYING.gifsicle.
EOF
required+=("$out/README.txt")

# ---- 7. Final assertion: every required file exists AND is non-empty ----
missing=""
for f in "${required[@]}"; do
  if [[ ! -e "$f" ]]; then
    missing+=$'\n'"  MISSING: $f"
  elif [[ ! -s "$f" ]]; then
    missing+=$'\n'"  EMPTY:   $f"
  fi
done
if [[ -n "$missing" ]]; then
  echo "ERROR: portable package is incomplete:$missing" >&2
  exit 1
fi

echo "Portable package created: $out"
echo "  scope: $scope"
echo "  verified present and non-empty:"
for f in "${required[@]}"; do
  printf '    %10d  %s\n' "$(stat -c%s "$f" 2>/dev/null || stat -f%z "$f")" "$(basename "$f")"
done
