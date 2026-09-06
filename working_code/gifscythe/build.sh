#!/usr/bin/env bash
#
# build.sh - One-command build for Gifscythe.
#
#   ./build.sh            build engine + CLI + run unit tests (always)
#                         + GUI (only where Qt6 is installed)
#   ./build.sh --gui      build only the Qt6 GUI (skip engine/tests)
#   ./build.sh --all      build engine + GUI (requires Qt6)
#
# Designed so the next session needs no explanation:
#   - The gifsicle ENGINE always builds natively (or --windows cross-compile).
#   - The CORE + CLI always build.
#   - The Qt6 GUI builds only where Qt6 is present; otherwise a clear message
#     is printed and the rest still succeeds.
#
# Output:
#   build/core/    cli driver (gifscythe-cli)
#   build/gui/     GUI binary (gifscythe) if Qt6 present
#   release/<v>/   engine binary
#
# Everything compiles/links with whatever is available (gcc/g++ + Qt). No
# autotools needed. This runs on a machine with Qt6, or a bare one (GUI skipped).

set -uo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$self/../.." && pwd)"
BUILD_DIR="$self/build"
mkdir -p "$BUILD_DIR"

want_gui=0
case "${1:-}" in
  --gui)  want_gui=1;;
  --all)  want_gui=1;;
  "")     want_gui=auto;;
esac
[[ "${want_gui}" == "auto" ]] && want_gui=0

# 1. Build the gifsicle ENGINE.
echo "==> [1/4] Building gifsicle engine..."
"$self/scripts/build_gifsicle.sh" "${2:-}" || { echo "engine build failed"; exit 1; }

# 2. Build the CLI driver (engine control layer).
echo "==> [2/4] Building CLI driver (gifscythe-cli)..."
g++ -std=c++17 -O2 -I"$self/src" -o "$BUILD_DIR/gifscythe-cli" "$self/src/cli/main.cpp" \
  || { echo "CLI build failed"; exit 1; }

# 3. Run unit tests.
echo "==> [3/4] Running unit tests..."
g++ -std=c++17 -O2 -I"$self/src" -o "$BUILD_DIR/test_gifsicle_command" \
  "$self/tests/test_gifsicle_command.cpp" && "$BUILD_DIR/test_gifsicle_command" \
  || { echo "unit tests failed"; exit 1; }

# 4. Build the Qt6 GUI, only if Qt6 is available (or explicitly requested).
echo "==> [4/4] Checking for Qt6 to build the GUI..."
qt_found=0
if command -v qmake6 >/dev/null 2>&1; then
  qt_found=1
elif command -v qmake >/dev/null 2>&1; then
  qt_found=1
elif command -v cmake >/dev/null 2>&1; then
  # cmake path: let CMake find Qt6; if absent the target is skipped.
  ( cd "$BUILD_DIR" && cmake -S "$self" -B . -DBUILD_GUI=ON >/dev/null 2>&1 \
    && cmake --build . >/dev/null 2>&1 ) \
    && { echo "   GUI built via cmake"; qt_found=1; }
fi

if [[ "$qt_found" == "1" && "$want_gui" == "1" ]]; then
  echo "   Qt6 present; building GUI with qmake..."
  ( cd "$BUILD_DIR/gui" 2>/dev/null || mkdir -p "$BUILD_DIR/gui"; \
    cd "$BUILD_DIR/gui" && qmake6 "$self/gifscythe.pro" >/dev/null 2>&1 \
    && make >/dev/null 2>&1 ) \
    && echo "   GUI built: $BUILD_DIR/gui/gifscythe" \
    || echo "   GUI build failed (see Qt errors above)."
elif [[ "$want_gui" == "1" ]]; then
  echo "   Qt6 NOT found; GUI skipped. Install Qt6 (e.g. 'sudo apt install qt6-base-dev') or build on a Qt machine."
else
  echo "   Done (GUI not requested; pass --all or --gui to build it)."
fi

echo ""
echo "==> Build complete."
echo "   Engine:  $self/release/$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')/gifsicle"
echo "   CLI:     $BUILD_DIR/gifscythe-cli"
echo "   (GUI:    $BUILD_DIR/gui/gifscythe   if Qt6 was present)"
