#!/usr/bin/env bash
# Shared, fail-closed stager. Source only from package_portable/system.sh.
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
repo_root="$(cd "$self/../.." && pwd)"
engine_cli_only=0
want_help=0
windows=0
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) windows=1 ;; esac
for arg in "$@"; do
  case "$arg" in
    --engine-cli-only) engine_cli_only=1 ;;
    --windows) windows=1 ;;
    -h|--help) want_help=1 ;;
    *) echo "ERROR: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done
if [[ "$want_help" == 1 ]]; then
  echo "usage: package_${package_kind}.sh [--engine-cli-only] [--windows]"
  exit 0
fi
fail() { echo "ERROR: $*" >&2; exit 1; }
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
[[ -n "$version" ]] || fail "missing product version"
release="$self/release/$version"
out="$release/Gifscythe"
[[ "$package_kind" == system ]] && out="$release/Gifscythe-system"
# Old contents must not masquerade as this attempt; publish only a verified tree.
mkdir -p "$release"
rm -rf "$out"
stage="$(mktemp -d "$release/.package-${package_kind}.XXXXXX")"
trap '[[ -z "$stage" ]] || rm -rf "$stage"' EXIT
ext=""; [[ "$windows" == 1 ]] && ext=".exe"
required=()
copy_required() { # exact name + ordered same-target candidates
  local name="$1"; shift
  local cand
  for cand in "$@"; do
    [[ -f "$cand" && -s "$cand" ]] || continue
    case "$name" in gifsicle|gifscythe-cli|gifscythe) [[ -x "$cand" ]] || continue ;; esac
    cp "$cand" "$stage/$name"
    required+=("$name")
    return 0
  done
  fail "$name missing or empty (no usable same-target candidate)"
}
copy_required "gifsicle$ext" "$release/gifsicle$ext"
copy_required "gifscythe-cli$ext" "$self/build/gifscythe-cli$ext"
scope="engine + CLI (no GUI, by request)"
if [[ "$engine_cli_only" == 0 ]]; then
  gui_candidates=()
  for dir in build build/gui build/cmake build-cmake; do
    gui_candidates+=("$self/$dir/gifscythe$ext")
  done
  if [[ "$windows" == 1 ]]; then
    gui_candidates+=("$self/build-win/gifscythe.exe" "$self/build-win/Release/gifscythe.exe")
  fi
  copy_required "gifscythe$ext" "${gui_candidates[@]}"
  scope="engine + CLI + Qt GUI"
  if [[ "$package_kind" == portable && "$windows" == 1 ]]; then
    command -v windeployqt >/dev/null 2>&1 || fail "windeployqt is required for a portable Windows GUI package"
    windeployqt --release --no-translations "$stage/gifscythe.exe" || fail "windeployqt failed"
    required+=(Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll platforms/qwindows.dll)
  fi
fi
copy_required VERSION.md "$self/VERSION.md"
copy_required README.md "$self/README.md"
copy_required LICENSE "$repo_root/LICENSE" "$repo_root/COPYING"
copy_required COPYING.gifsicle "$repo_root/COPYING.gifsicle" "$repo_root/reference_code/gifsicle/COPYING"
runtime="Qt runtime is not bundled on this target; GUI requires system Qt6."
if [[ "$package_kind" == portable && "$windows" == 1 && "$engine_cli_only" == 0 ]]; then
  runtime="Qt runtime deployed by windeployqt; clean-Windows smoke is still required."
fi
cat > "$stage/README.txt" <<EOF
Gifscythe $version — $package_kind package
Contents: $scope
$runtime
Run gifscythe-cli for the command-line interface; GUI is present only in GUI packages.
gifsicle is GPL v2-only — see COPYING.gifsicle. See LICENSE for the first-party notice.
EOF
required+=(README.txt)
printf '%s\n' "${required[@]}" > "$stage/MANIFEST.txt"
required+=(MANIFEST.txt)
for name in "${required[@]}"; do
  [[ -f "$stage/$name" && -s "$stage/$name" ]] || fail "package incomplete: missing or empty $name"
done
mv "$stage" "$out"
stage=""
echo "Package created: $out"
echo "  scope: $scope"
echo "  verified present and non-empty: ${required[*]}"
