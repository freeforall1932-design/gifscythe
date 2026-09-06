#!/usr/bin/env bash
# Package a system-dependent build: application binaries only, no Qt runtime.
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
out="$self/release/$version/Gifscythe-system"
mkdir -p "$out"
for file in "$self/release/$version/gifsicle" "$self/build/gifscythe-cli" "$self/build/gui/gifscythe"; do
  if [[ -f "$file" ]]; then cp "$file" "$out/"; fi
done
cp "$self/VERSION.md" "$self/README.md" "$out/"
cat > "$out/README.txt" <<EOF
Gifscythe $version — system-dependent package

This package intentionally does not include Qt runtime libraries.
Use it on a system that already provides the required Qt6 runtime.
For a self-contained package, use the Gifscythe portable release instead.
EOF
printf 'System-dependent package created: %s\n' "$out"
