#!/usr/bin/env bash
# Package a system-dependent build: application binaries only, no Qt runtime.
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
version="${version:-0.1.0}"
out="$self/release/$version/Gifscythe-system"
mkdir -p "$out"

for cand in \
  "$self/release/$version/gifsicle" \
  "$self/release/$version/gifsicle.exe" \
  "$self/build/gifscythe-cli" \
  "$self/build/gifscythe-cli.exe" \
  "$self/build/gifscythe" \
  "$self/build/gifscythe.exe" \
  "$self/build/gui/gifscythe" \
  "$self/build/gui/gifscythe.exe" \
  "$self/build/cmake/gifscythe" \
  "$self/build/cmake/gifscythe.exe"
do
  if [[ -f "$cand" ]]; then cp "$cand" "$out/"; fi
done

cp "$self/VERSION.md" "$self/README.md" "$out/"
repo_root="$(cd "$self/../.." && pwd)"
[[ -f "$repo_root/COPYING" ]] && cp "$repo_root/COPYING" "$out/"
[[ -f "$repo_root/reference_code/gifsicle/COPYING" ]] && \
  cp "$repo_root/reference_code/gifsicle/COPYING" "$out/COPYING.gifsicle"

cat > "$out/README.txt" <<EOF
Gifscythe $version — system-dependent package

This package intentionally does not include Qt runtime libraries.
Use it on a system that already provides the required Qt6 runtime.
For a self-contained package, use the Gifscythe portable release instead.
EOF
printf 'System-dependent package created: %s\n' "$out"
