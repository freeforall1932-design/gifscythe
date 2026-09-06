#!/usr/bin/env bash
# Create a portable release folder. Run after ./build.sh --all on a Qt machine.
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
out="$self/release/$version/Gifscythe"
engine="$self/release/$version/gifsicle"
mkdir -p "$out"
[[ -x "$engine" ]] || { echo "Missing engine: $engine (run ./build.sh first)" >&2; exit 1; }
cp "$engine" "$out/"
[[ -x "$self/build/gifscythe-cli" ]] && cp "$self/build/gifscythe-cli" "$out/"
if [[ -x "$self/build/gui/gifscythe" ]]; then cp "$self/build/gui/gifscythe" "$out/"; else echo "Note: Qt GUI not found; packaging engine and CLI only."; fi
cp "$self/VERSION.md" "$self/README.md" "$out/"
cat > "$out/README.txt" <<EOF
Gifscythe $version — portable release

Run gifscythe (Windows GUI) or gifscythe-cli for the command-line interface.
The application does not install anything and can run from this folder.
EOF
printf 'Portable package created: %s\n' "$out"
