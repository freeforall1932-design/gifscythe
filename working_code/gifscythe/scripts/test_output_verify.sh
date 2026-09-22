#!/usr/bin/env bash
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
# Isolated binary and data; this test does not alter production output files.
g++ -std=c++17 -Wall -Wextra -pedantic -I"$self/src" "$self/tests/test_output_verify.cpp" -o "$work/test"
mkdir "$work/data"
"$work/test" "$work/data"
