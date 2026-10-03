#!/usr/bin/env bash
#
# fixtures.sh - materialise the test images from their TEXT form.  (S34, N-36)
#
#   ./scripts/fixtures.sh [DIR]     prints DIR (default: working_code/gifscythe/build/fixtures)
#
# The repo holds no binary files (the owner removed every image in commit fe4f0a7, and that includes the
# four gifsicle test GIFs the engine tests, smoke tests, oracle, glue harness and CI steps read). The two
# upstream test images are kept as base64 text in tests/fixtures/*.b64 with their sha256 in
# tests/fixtures/SHA256SUMS; this decodes them into DIR and verifies the checksums, so a test always reads
# the byte-identical upstream logo.gif / logo1.gif (every expectation - 12 frames, 8703 B -> 8637 B under
# -O3 - was measured on exactly these bytes). Idempotent and quick; nothing is written outside DIR.
#
# Needs: base64 and sha256sum (GNU coreutils; Git for Windows ships both) or `shasum`.
# CRLF-proof on purpose: GNU base64 -d rejects a CR, and a Windows checkout turns text into CRLF unless
# .gitattributes says otherwise (it does, for tests/fixtures; this is the second line of defence).
# Exit: 0 DIR holds both files and they match / 1 a checksum does not match / 2 a tool is missing.
#
set -uo pipefail
export LC_ALL=C

self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
src="$self/tests/fixtures"
dest="${1:-$self/build/fixtures}"

if command -v sha256sum >/dev/null 2>&1; then sha() { sha256sum "$1" | cut -d' ' -f1; }
elif command -v shasum >/dev/null 2>&1; then sha() { shasum -a 256 "$1" | cut -d' ' -f1; }
else echo "fixtures.sh: no sha256sum or shasum on PATH" >&2; exit 2; fi
command -v base64 >/dev/null 2>&1 || { echo "fixtures.sh: no base64 on PATH" >&2; exit 2; }
[[ -f "$src/SHA256SUMS" ]] || { echo "fixtures.sh: $src/SHA256SUMS is missing" >&2; exit 2; }

mkdir -p "$dest" || exit 2
status=0
while read -r want name; do
  want="${want%$'\r'}"; name="${name%$'\r'}"   # a Windows checkout may hand us CRLF text files
  [[ -n "${name:-}" ]] || continue
  if [[ -f "$dest/$name" && "$(sha "$dest/$name")" == "$want" ]]; then continue; fi   # already good
  tr -d '\r' < "$src/$name.b64" | base64 -d > "$dest/$name.tmp" 2>/dev/null || { echo "fixtures.sh: cannot decode $src/$name.b64" >&2; rm -f "$dest/$name.tmp"; exit 2; }
  if [[ "$(sha "$dest/$name.tmp")" != "$want" ]]; then
    echo "fixtures.sh: $name decodes to the wrong bytes (sha256 mismatch against SHA256SUMS)" >&2
    rm -f "$dest/$name.tmp"; status=1; continue
  fi
  mv -f "$dest/$name.tmp" "$dest/$name"
done < "$src/SHA256SUMS"
printf '%s\n' "$dest"
exit "$status"
