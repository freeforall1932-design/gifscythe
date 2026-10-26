#!/usr/bin/env bash
#
# fetch_ffmpeg.sh — put the portable FFmpeg sidecar next to the app.
#
#   ./scripts/fetch_ffmpeg.sh            fetch the pinned build for this host
#   ./scripts/fetch_ffmpeg.sh --check    say what is pinned and what is present
#   ./scripts/fetch_ffmpeg.sh --pin      record the SHA-256 of what is already there
#
# WHY A SCRIPT RATHER THAN A COMMITTED BINARY
#   An FFmpeg build is 60-120 MB per platform. Committing three of them would
#   bloat every clone and every CI checkout forever, and the repository's own
#   budget rules keep large binaries out of git. The product still ships them:
#   this script stages them into `vendor/ffmpeg/<platform>/`, packaging copies
#   that folder next to the executable, and `src/core/FfmpegLocator.h` prefers
#   that sidecar over anything installed on the machine. So a user unzips and it
#   works; nobody downloads anything by hand.
#
# TRUST
#   A download that is not checked is not a dependency, it is a gamble. The
#   version and the SHA-256 both live in `vendor/ffmpeg/PINS.txt`; a mismatch is
#   a hard failure, and `--pin` is a deliberate two-step so a pin can never be
#   recorded by accident.
#
# NOTHING IS FETCHED UNLESS ASKED
#   The build works without FFmpeg (GIF via the bundled gifsicle engine). This
#   script is only reached when someone wants the video/APNG/WebP formats, or by
#   the release job.

set -uo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vendor="$self/vendor/ffmpeg"
pins="$vendor/PINS.txt"

os="$(uname -s)"
arch="$(uname -m)"

case "$os" in
  Linux*)  platform="linux-x64" ;;
  Darwin*) platform="macos-$( [[ "$arch" == "arm64" ]] && echo arm64 || echo x64 )" ;;
  MINGW*|MSYS*|CYGWIN*) platform="windows-x64" ;;
  *) echo "fetch_ffmpeg: unsupported host '$os' - install ffmpeg yourself and set GS_FFMPEG" >&2; exit 2 ;;
esac

# BtbN's builds are the reference static builds for every platform this project
# ships on, and they are versioned by tag. The tag is the PIN.
default_tag="autobuild-2026-10-25-13-00"   # a dated tag, never "latest": a rolling tag cannot be pinned
repo="https://github.com/BtbN/FFmpeg-Builds/releases/download"

case "$platform" in
  linux-x64)    asset="ffmpeg-master-latest-linux64-gpl.tar.xz"; dir="ffmpeg-master-latest-linux64-gpl" ;;
  windows-x64)  asset="ffmpeg-master-latest-win64-gpl.zip";     dir="ffmpeg-master-latest-win64-gpl" ;;
  macos-arm64)  asset="ffmpeg-master-latest-macos-arm64-gpl.tar.xz"; dir="ffmpeg-master-latest-macos-arm64-gpl" ;;
  macos-x64)    asset="ffmpeg-master-latest-macos-x64-gpl.tar.xz";   dir="ffmpeg-master-latest-macos-x64-gpl" ;;
esac

target="$vendor/$platform"
exe="ffmpeg"; [[ "$platform" == windows-x64 ]] && exe="ffmpeg.exe"
staged="$target/bin/$exe"

pinned_sha() { # $1 = asset name
  [[ -f "$pins" ]] || return 1
  awk -v k="$1" '$1 == k { print $2 }' "$pins" | head -1
}

usage() {
  sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

case "${1:-}" in
  -h|--help) usage; exit 0 ;;
esac

if [[ "${1:-}" == "--check" ]]; then
  echo "platform : $platform"
  echo "asset    : $asset"
  echo "pin file : $pins"
  sha="$(pinned_sha "$asset" || true)"
  if [[ -n "$sha" ]]; then echo "pinned   : $sha"; else echo "pinned   : (none recorded)"; fi
  if [[ -f "$staged" ]]; then
    echo "sidecar  : $staged ($(wc -c <"$staged" | tr -d ' ') bytes)"
    echo "  run it : \"$staged\" -version | head -1"
  else
    echo "sidecar  : absent (GIF still works; APNG/WebP/MP4/WebM stay disabled)"
  fi
  exit 0
fi

if [[ "${1:-}" == "--pin" ]]; then
  [[ -f "$staged" ]] || { echo "nothing staged to pin at $staged" >&2; exit 1; }
  mkdir -p "$vendor"
  touch "$pins"
  grep -v "^$asset " "$pins" > "$pins.tmp" 2>/dev/null || true
  if command -v sha256sum >/dev/null 2>&1; then
    sum="$(sha256sum "$staged" | awk '{print $1}')"
  else
    sum="$(shasum -a 256 "$staged" | awk '{print $1}')"
  fi
  printf '%s %s %s\n' "$asset" "$sum" "$platform" >> "$pins.tmp"
  mv "$pins.tmp" "$pins"
  echo "pinned $asset -> $sum"
  exit 0
fi

tag="${GS_FFMPEG_TAG:-$default_tag}"
url="$repo/$tag/$asset"
mkdir -p "$target"

echo "==> FFmpeg sidecar for $platform"
echo "    $url"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

if command -v curl >/dev/null 2>&1; then
  curl -fL --retry 3 --retry-delay 2 -o "$tmp/$asset" "$url" || { echo "download failed" >&2; exit 1; }
elif command -v wget >/dev/null 2>&1; then
  wget -O "$tmp/$asset" "$url" || { echo "download failed" >&2; exit 1; }
else
  echo "neither curl nor wget is available" >&2; exit 1
fi

if command -v sha256sum >/dev/null 2>&1; then
  got="$(sha256sum "$tmp/$asset" | awk '{print $1}')"
else
  got="$(shasum -a 256 "$tmp/$asset" | awk '{print $1}')"
fi
want="$(pinned_sha "$asset" || true)"
if [[ -z "$want" ]]; then
  echo "REFUSING: no pinned SHA-256 for $asset in $pins." >&2
  echo "  Downloaded   : $got" >&2
  echo "  If you trust this tag, record it deliberately:  $0 --pin" >&2
  echo "  (that is a two-step on purpose: a pin recorded by accident pins nothing)" >&2
  exit 1
fi
if [[ "$got" != "$want" ]]; then
  echo "REFUSING: SHA-256 mismatch for $asset" >&2
  echo "  pinned: $want" >&2
  echo "  got   : $got" >&2
  exit 1
fi

case "$asset" in
  *.zip) unzip -q "$tmp/$asset" -d "$tmp/x" ;;
  *)     tar -xf "$tmp/$asset" -C "$tmp" && mv "$tmp/$dir" "$tmp/x" ;;
esac

mkdir -p "$target/bin"
src_bin="$tmp/x/bin/$exe"
[[ -f "$src_bin" ]] || src_bin="$(find "$tmp/x" -name "$exe" -type f | head -1)"
[[ -f "$src_bin" ]] || { echo "archive did not contain $exe" >&2; exit 1; }
cp "$src_bin" "$target/bin/$exe"
chmod +x "$target/bin/$exe" 2>/dev/null || true

echo "==> staged $target/bin/$exe"
"$target/bin/$exe" -version 2>/dev/null | head -1 || echo "    (could not run it here; that is not fatal)"
echo "    The GUI prefers this copy over anything on PATH (see src/core/FfmpegLocator.h)."
