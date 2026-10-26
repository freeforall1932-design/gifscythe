#!/usr/bin/env bash
# Shared, fail-closed stager. Source only from package_portable/system.sh.
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
repo_root="$(cd "$self/../.." && pwd)"
engine_cli_only=0
want_help=0
windows=0
require_ffmpeg=0
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) windows=1 ;; esac
for arg in "$@"; do
  case "$arg" in
    --engine-cli-only) engine_cli_only=1 ;;
    --windows) windows=1 ;;
    --require-ffmpeg) require_ffmpeg=1 ;;
    -h|--help) want_help=1 ;;
    *) echo "ERROR: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done
if [[ "$want_help" == 1 ]]; then
  echo "usage: package_${package_kind}.sh [--engine-cli-only] [--windows] [--require-ffmpeg]"
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
    mkdir -p "$stage/$(dirname "$name")"
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
# Since 2026-10-26 every notice text lives in <repo>/legal/ (the product is
# UNLICENSED until the owner settles it); the third-party texts below are still
# obligations that travel with every package.
copy_required licenses/LICENSE.md-pl.proposed "$repo_root/legal/LICENSE.md-pl.proposed"
copy_required licenses/COPYING.gifsicle "$repo_root/legal/COPYING.gifsicle" "$repo_root/reference_code/gifsicle/COPYING"
copy_required licenses/COPYING.ms-pl "$repo_root/legal/COPYING.ms-pl"
copy_required licenses/COPYING.lgplv3 "$repo_root/legal/COPYING.lgplv3"
copy_required licenses/COPYING.gplv3 "$repo_root/legal/COPYING.gplv3"
# ---------------------------------------------------------------------------
# FFmpeg sidecar (N-62). The owner asked for FFmpeg IN the project, portable,
# with a search fallback only when the local copy is missing. So the copy that
# travels with the app is staged right next to the executable - exact path the
# locator looks at first - and it is OPTIONAL only because a checkout without
# vendor/ffmpeg/ must still be packageable (CI has no 156 MB sidecar and must
# not download one on every push). A release build passes --require-ffmpeg and
# then a missing sidecar is fatal.
ffmpeg_ext=""; [[ "$windows" == 1 ]] && ffmpeg_ext=".exe"
ffmpeg_platform=""
case "$(uname -s)" in
  Linux*)  ffmpeg_platform="linux-x64" ;;
  Darwin*) ffmpeg_platform="macos-$( [[ "$(uname -m)" == arm64 ]] && echo arm64 || echo x64 )" ;;
  MINGW*|MSYS*|CYGWIN*) ffmpeg_platform="windows-x64" ;;
esac
ffmpeg_bundled=0
ffmpeg_src="$self/vendor/ffmpeg/$ffmpeg_platform/bin/ffmpeg$ffmpeg_ext"
if [[ -f "$ffmpeg_src" && -s "$ffmpeg_src" && -x "$ffmpeg_src" ]]; then
  cp "$ffmpeg_src" "$stage/ffmpeg$ffmpeg_ext"
  required+=("ffmpeg$ffmpeg_ext")
  ffmpeg_bundled=1
  cat > "$stage/FFMPEG_NOTICE.txt" <<EOF
FFmpeg notice (Gifscythe $version, $package_kind package)
This package bundles FFmpeg as a separate program, next to the application
(ffmpeg$ffmpeg_ext). The application runs it as a subprocess; it is not linked
into the application binary.
FFmpeg is licensed under the GNU General Public License v3 - see COPYING.gplv3
in this folder, and https://ffmpeg.org/legal.html for the build options used.
Corresponding source for the bundled FFmpeg build: https://ffmpeg.org/download.html
The bundled build is a GPL build (no --enable-nonfree component is used).
The application prefers this bundled copy; it only searches PATH and the usual
install folders when this file is absent.
EOF
  required+=(FFMPEG_NOTICE.txt)
elif [[ "$require_ffmpeg" == 1 ]]; then
  fail "FFmpeg sidecar required but missing: $ffmpeg_src (run scripts/fetch_ffmpeg.sh)"
fi
runtime="Qt runtime is not bundled on this target; GUI requires system Qt6."
if [[ "$package_kind" == portable && "$windows" == 1 && "$engine_cli_only" == 0 ]]; then
  runtime="Qt runtime deployed by windeployqt; clean-Windows smoke is still required."
fi
# Qt notice (audit U-08): a GUI package links Qt dynamically, so it carries a
# generated QT_NOTICE.txt next to the staged LGPLv3/GPLv3 texts. Headless
# (--engine-cli-only) packages ship no Qt and carry no such notice.
if [[ "$engine_cli_only" == 0 ]]; then
  qt_ver="$(qmake6 -query QT_VERSION 2>/dev/null || qmake -query QT_VERSION 2>/dev/null || echo unknown)"
  cat > "$stage/QT_NOTICE.txt" <<EOF
Qt licensing notice (Gifscythe $version, $package_kind package)
This build of Gifscythe links the Qt libraries (version: $qt_ver) dynamically.
Qt is used under the GNU Lesser General Public License v3 - see COPYING.lgplv3
(companion GPLv3 text: COPYING.gplv3), unless you hold a commercial Qt license.
Qt sources: https://www.qt.io and https://code.qt.io/cgit/qt/qtbase.git
Relink: Qt is linked as shared libraries, so you may replace them with a
compatible build of your choice.
EOF
  required+=(QT_NOTICE.txt)
fi
cat > "$stage/README.txt" <<EOF
Gifscythe $version — $package_kind package
Contents: $scope
$runtime
Run gifscythe-cli for the command-line interface; GUI is present only in GUI packages.
Third-party notices are in licenses/: gifsicle is GPL v2-only (COPYING.gifsicle), Qt is LGPLv3
(COPYING.lgplv3 + companion COPYING.gplv3; GUI packages also carry QT_NOTICE.txt).
The first-party code is UNLICENSED: no licence is granted, and licenses/LICENSE.md-pl.proposed
is only a proposal (see the repository's legal/ folder).
EOF
# One README, two honest variants: a package either carries FFmpeg or it does
# not, and it must say which (a silent missing encoder is exactly the bug the
# owner called out).
if [[ "$ffmpeg_bundled" == 1 ]]; then
  cat >> "$stage/README.txt" <<EOF
FFmpeg is bundled: ffmpeg$ffmpeg_ext sits next to the executable, so APNG, WebP, MP4,
WebM and every other FFmpeg-backed format works without installing anything. The
application only searches for a system FFmpeg if that bundled file is removed.
EOF
else
  cat >> "$stage/README.txt" <<EOF
FFmpeg is NOT bundled in this build. GIF output is fully supported (the packaged
gifsicle engine needs no FFmpeg). The FFmpeg-backed formats will only appear as
available if an FFmpeg is found in this folder, on PATH, or in a usual install
location - see the application's own report of what it found and where.
EOF
fi
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
