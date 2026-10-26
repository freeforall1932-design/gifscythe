#!/usr/bin/env bash
#
# fetch_ffmpeg.sh — put the portable FFmpeg sidecar next to the app.
#
#   ./scripts/fetch_ffmpeg.sh                 fetch the build for this machine
#   ./scripts/fetch_ffmpeg.sh --check         say what is pinned / present
#   ./scripts/fetch_ffmpeg.sh --accept-unpinned
#                                             install a fresh download and record
#                                             its SHA-256 as the pin (trust on
#                                             first use, deliberate second step)
#   ./scripts/fetch_ffmpeg.sh --pin           re-pin what is already staged
#
# WHY A SCRIPT AND NOT A COMMITTED BINARY
#   An FFmpeg build is 70-200 MB per platform. Committing three of them bloats
#   every clone and every CI checkout forever, and this repository does not put
#   large binaries in git. The product still SHIPS them: this script stages a
#   build into vendor/ffmpeg/<platform>/bin/, packaging copies that next to the
#   executable, and src/core/FfmpegLocator.h prefers it over anything installed
#   on the machine. A user unzips the package and it works - no PATH surgery, no
#   "is ffmpeg installed" dialog.
#
# TRUST
#   A download that is not checked is not a dependency, it is a gamble. Every
#   install is verified against the SHA-256 recorded in vendor/ffmpeg/PINS.txt.
#   An unknown download is REFUSED unless --accept-unpinned, which prints the
#   hash, installs it, and records it as the pin. That keeps the pin a decision
#   somebody made rather than a side effect of running a script.
#
# NOTHING IS FETCHED UNLESS ASKED
#   The build works without FFmpeg (GIF needs none of this). Only APNG/WebP/MP4/
#   WebM need it, and the GUI disables exactly those formats when it is absent -
#   rather than offering a button that fails at the end of a batch.

set -uo pipefail

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vendor="$self/vendor/ffmpeg"
pins="$vendor/PINS.txt"

os="$(uname -s)"
arch="$(uname -m)"
case "$os" in
  Linux*)  platform="linux64" ;;
  Darwin*) platform="macos-$( [[ "$arch" == "arm64" ]] && echo arm64 || echo x64 )" ;;
  MINGW*|MSYS*|CYGWIN*) platform="win64" ;;
  *) echo "fetch_ffmpeg: unsupported host '$os' - install ffmpeg yourself and set GS_FFMPEG" >&2; exit 2 ;;
esac

# BtbN publishes a dated autobuild per day; each carries its full ffmpeg git
# version in the asset name, so the NAME is discovered from the release instead
# of guessed (a guessed name is a 404, and a 404 during packaging looks like a
# network problem rather than a bug).
tag="${GS_FFMPEG_TAG:-}"
api="https://api.github.com/repos/BtbN/FFmpeg-Builds/releases"
if [[ -z "$tag" ]]; then meta="$api/latest"; else meta="$api/tags/$tag"; fi

if command -v curl >/dev/null 2>&1; then
  get() { curl -fsSL --retry 3 --retry-delay 2 "$1"; }
  dl()  { curl -fL --retry 3 --retry-delay 2 -o "$2" "$1"; }
elif command -v wget >/dev/null 2>&1; then
  get() { wget -qO- "$1"; }
  dl()  { wget -qO "$2" "$1"; }
else
  echo "neither curl nor wget is available" >&2; exit 1
fi

sha_of() {
  if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | awk '{print $1}';
  else shasum -a 256 "$1" | awk '{print $1}'; fi
}
pinned_sha() { [[ -f "$pins" ]] && awk -v k="$1" '$1 == k { print $2 }' "$pins" | head -1; }

discover_asset() {
  # First GPL archive for this platform, never the "-shared" variant: a shared
  # build needs a pile of sibling libraries next to it, which defeats the point
  # of a sidecar that a package can copy as one file.
  "$@" >/dev/null 2>&1 || true
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  sed -n '2,32p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  exit 0
fi

asset="$(get "$meta" | tr ',' '\n' | grep -o '"name": *"[^"]*"' | sed 's/.*"name": *"//; s/"$//' \
         | grep -- "-$platform-gpl" | grep -v -- "-shared" | head -1)"
if [[ -z "$asset" ]]; then
  echo "fetch_ffmpeg: could not find a GPL build for '$platform' in $meta" >&2
  echo "  (checked the release's asset list; nothing matched '-$platform-gpl')" >&2
  exit 1
fi
url="$(get "$meta" | tr ',' '\n' | grep -o '"browser_download_url": *"[^"]*"' \
       | sed 's/.*"browser_download_url": *"//; s/"$//' | grep -F "/$asset")"
[[ -n "$url" ]] || { echo "fetch_ffmpeg: no download URL for $asset" >&2; exit 1; }

exe="ffmpeg"; [[ "$platform" == win64 ]] && exe="ffmpeg.exe"
target="$vendor/$platform"
staged="$target/bin/$exe"

if [[ "${1:-}" == "--check" ]]; then
  echo "host     : $os $arch -> $platform"
  echo "asset    : $asset"
  sha="$(pinned_sha "$asset" || true)"
  echo "pinned   : ${sha:-(none recorded)}"
  if [[ -f "$staged" ]]; then
    echo "sidecar  : $staged ($(wc -c <"$staged" | tr -d ' ') bytes)"
  else
    echo "sidecar  : absent (GIF works; APNG/WebP/MP4/WebM stay disabled)"
  fi
  exit 0
fi

if [[ "${1:-}" == "--pin" ]]; then
  [[ -f "$staged" ]] || { echo "nothing staged to pin at $staged" >&2; exit 1; }
  mkdir -p "$vendor"; touch "$pins"
  grep -v "^$asset " "$pins" > "$pins.tmp" || true
  printf '%s %s %s\n' "$asset" "$(sha_of "$staged")" "$platform" >> "$pins.tmp"
  mv "$pins.tmp" "$pins"
  echo "pinned $asset"
  exit 0
fi

case "${1:-}" in
  ""|--accept-unpinned) ;;
  *) echo "fetch_ffmpeg: unknown option '$1' (see --help)" >&2; exit 2 ;;
esac

echo "==> FFmpeg sidecar for $platform"
echo "    $asset"
echo "    $url"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
dl "$url" "$tmp/$asset" || { echo "download failed" >&2; exit 1; }
got="$(sha_of "$tmp/$asset")"
want="$(pinned_sha "$asset" || true)"

if [[ -n "$want" && "$got" != "$want" ]]; then
  echo "REFUSING: SHA-256 mismatch for $asset" >&2
  echo "  pinned: $want" >&2
  echo "  got   : $got" >&2
  echo "  The release was re-published, or the download was tampered with." >&2
  exit 1
fi
if [[ -z "$want" ]]; then
  if [[ "${1:-}" != "--accept-unpinned" ]]; then
    echo "REFUSING: no pinned SHA-256 for this asset in $pins" >&2
    echo "  downloaded: $got" >&2
    echo "  If you trust this build, run again with --accept-unpinned;" >&2
    echo "  that installs it AND records the hash, so later installs are checked." >&2
    exit 1
  fi
  mkdir -p "$vendor"; touch "$pins"
  grep -v "^$asset " "$pins" > "$pins.tmp" || true
  printf '%s %s %s\n' "$asset" "$got" "$platform" >> "$pins.tmp"
  mv "$pins.tmp" "$pins"
  echo "==> recorded pin for $asset: $got"
fi

case "$asset" in
  *.zip) unzip -q "$tmp/$asset" -d "$tmp/x" ;;
  *)     mkdir -p "$tmp/x" && tar -xf "$tmp/$asset" -C "$tmp/x" ;;
esac
src_bin="$(find "$tmp/x" -type f -name "$exe" -path '*/bin/*' | head -1)"
[[ -n "$src_bin" ]] || src_bin="$(find "$tmp/x" -type f -name "$exe" | head -1)"
[[ -n "$src_bin" ]] || { echo "archive did not contain $exe" >&2; exit 1; }

mkdir -p "$target/bin"
cp "$src_bin" "$target/bin/$exe"
chmod +x "$target/bin/$exe" 2>/dev/null || true
echo "==> staged $target/bin/$exe"
"$target/bin/$exe" -version 2>/dev/null | head -1 || echo "    (could not run it here; not fatal)"
echo "    Package it with: ./scripts/package_portable.sh --require-ffmpeg"
echo "    The GUI prefers this copy over anything on PATH (src/core/FfmpegLocator.h)."
