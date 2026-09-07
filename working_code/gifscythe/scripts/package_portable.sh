#!/usr/bin/env bash
# Create a portable release folder. Run after ./build.sh [--all] on a Qt machine.
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
version="${version:-0.1.0}"
out="$self/release/$version/Gifscythe"
mkdir -p "$out"

# Probe engine: gifsicle then gifsicle.exe
engine=""
for cand in "$self/release/$version/gifsicle" "$self/release/$version/gifsicle.exe"; do
  if [[ -f "$cand" ]]; then engine="$cand"; break; fi
done
[[ -n "$engine" ]] || { echo "Missing engine in release/$version/ (run ./build.sh first)" >&2; exit 1; }
cp "$engine" "$out/"

# CLI
if [[ -x "$self/build/gifscythe-cli" ]]; then
  cp "$self/build/gifscythe-cli" "$out/"
elif [[ -x "$self/build/gifscythe-cli.exe" ]]; then
  cp "$self/build/gifscythe-cli.exe" "$out/"
fi

# GUI — probe all known output locations
gui=""
for cand in \
  "$self/build/gifscythe" \
  "$self/build/gifscythe.exe" \
  "$self/build/gui/gifscythe" \
  "$self/build/gui/gifscythe.exe" \
  "$self/build/cmake/gifscythe" \
  "$self/build/cmake/gifscythe.exe"
do
  if [[ -f "$cand" ]]; then gui="$cand"; break; fi
done

if [[ -n "$gui" ]]; then
  cp "$gui" "$out/"
  # Deploy Qt runtime on Windows when windeployqt is available.
  if command -v windeployqt >/dev/null 2>&1; then
    gui_base="$(basename "$gui")"
    windeployqt --release --no-translations "$out/$gui_base" || \
      echo "Note: windeployqt reported errors; check the portable folder." >&2
  fi
else
  echo "Note: Qt GUI not found; packaging engine and CLI only."
fi

# Licenses + docs
cp "$self/VERSION.md" "$self/README.md" "$out/"
repo_root="$(cd "$self/../.." && pwd)"
if [[ -f "$repo_root/COPYING" ]]; then cp "$repo_root/COPYING" "$out/"; fi
if [[ -f "$repo_root/LICENSE" ]]; then cp "$repo_root/LICENSE" "$out/"; fi
# Always ship gifsicle's GPL v2 text alongside the engine.
if [[ -f "$repo_root/reference_code/gifsicle/COPYING" ]]; then
  cp "$repo_root/reference_code/gifsicle/COPYING" "$out/COPYING.gifsicle"
fi

cat > "$out/README.txt" <<EOF
Gifscythe $version — portable release

Run gifscythe (GUI) or gifscythe-cli for the command-line interface.
The application does not install anything and can run from this folder.

gifsicle (the engine) is GPL v2-only — see COPYING.gifsicle.
EOF

printf 'Portable package created: %s\n' "$out"
# Sanity: engine must be present
ls -la "$out" | head -20
