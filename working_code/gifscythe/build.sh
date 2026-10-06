#!/usr/bin/env bash
#
# build.sh - One-command build for Gifscythe.
#
#   ./build.sh            build engine + CLI + unit tests (always)
#   ./build.sh --gui      also build the Qt6 GUI (fail if Qt6 missing)
#   ./build.sh --all      same as --gui (engine + CLI + tests + GUI)
#
# Output:
#   build/gifscythe-cli
#   build/test_gifsicle_command
#   build/test_input_admission   (GS-205 / P1-27, S37)
#   build/gifscythe          (GUI, if requested and Qt6 present — cmake path)
#   build/gui/gifscythe      (GUI, if built via qmake)
#   release/<v>/gifsicle     (engine)
#
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$self/build"
LOG="$BUILD_DIR/build.log"

# Parse before touching logs, generated headers, hooks or build outputs.
want_gui=0
engine_arg=""
want_help=0
for arg in "$@"; do
  case "$arg" in
    --gui|--all) want_gui=1 ;;
    --windows) engine_arg="--windows" ;;
    -h|--help) want_help=1 ;;
    *) echo "ERROR: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done
if [[ "$want_help" == 1 ]]; then
  echo "usage: build.sh [--gui|--all] [--windows]"
  exit 0
fi
mkdir -p "$BUILD_DIR"
: > "$LOG"
version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" \
  | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
version="${version:-0.1.0}"

fail() { echo "ERROR: $*" >&2; exit 1; }

build_gui_cmake() {
  local bdir="$BUILD_DIR/cmake"
  mkdir -p "$bdir"
  if cmake -S "$self" -B "$bdir" -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON >>"$LOG" 2>&1 \
     && cmake --build "$bdir" >>"$LOG" 2>&1; then
    if [[ -x "$bdir/gifscythe" ]]; then
      cp "$bdir/gifscythe" "$BUILD_DIR/gifscythe"
      gui_path="$BUILD_DIR/gifscythe"
      return 0
    elif [[ -x "$bdir/gifscythe.exe" ]]; then
      cp "$bdir/gifscythe.exe" "$BUILD_DIR/gifscythe.exe"
      gui_path="$BUILD_DIR/gifscythe.exe"
      return 0
    fi
    # cmake may have skipped GUI if Qt missing
    if grep -q "Qt6 NOT found" "$LOG" 2>/dev/null; then
      return 1
    fi
  fi
  return 1
}

build_gui_qmake() {
  local qmake_bin=""
  if command -v qmake6 >/dev/null 2>&1; then qmake_bin=qmake6
  elif command -v qmake >/dev/null 2>&1; then qmake_bin=qmake
  else return 1
  fi
  mkdir -p "$BUILD_DIR/gui"
  (
    cd "$BUILD_DIR/gui"
    "$qmake_bin" "$self/gifscythe.pro" >>"$LOG" 2>&1
    # Prefer make, fall back to mingw32-make
    if command -v make >/dev/null 2>&1; then make >>"$LOG" 2>&1
    elif command -v mingw32-make >/dev/null 2>&1; then mingw32-make >>"$LOG" 2>&1
    else return 1
    fi
  ) || return 1
  if [[ -x "$BUILD_DIR/gui/gifscythe" ]]; then
    gui_path="$BUILD_DIR/gui/gifscythe"; return 0
  fi
  if [[ -x "$BUILD_DIR/gui/gifscythe.exe" ]]; then
    gui_path="$BUILD_DIR/gui/gifscythe.exe"; return 0
  fi
  return 1
}

# GS-210 (S37): which GUI build tool wins when a host has BOTH.
#
# The old dispatch was `build_gui_qmake || build_gui_cmake`, i.e. qmake FIRST —
# the opposite of what this repo documents everywhere else (gifscythe.pro's own
# header: "Preferred build path is CMake"; CI builds the GUI with CMake only).
# On a host with qmake installed the preferred, CI-verified path was therefore
# never exercised. CMake first, qmake as the fallback.
#
# The dispatcher is a function on purpose: tests/test_build_options.py sources
# this script (the execution guard at the bottom keeps a source from building
# anything), replaces the two builders with recorders, and asserts the ORDER —
# the behaviour, not the source text.
build_gui() {
  if build_gui_cmake; then return 0; fi
  if build_gui_qmake; then return 0; fi
  return 1
}


main() {
# 0. Bootstrap the repo's git hooks. Git does not copy .githooks/ on clone, so
#    the pre-push documentation gate is inert until this runs. Building is the
#    one thing every session does, which is what makes the bootstrap reliable;
#    scripts/bootstrap_hooks.sh is a no-op when it is already set, or when the
#    developer chose a different hooksPath on purpose. A failure here must not
#    fail a build.
"$self/scripts/bootstrap_hooks.sh" || echo "   (hook bootstrap skipped)"

# Keep src/core/version.h in sync with VERSION.md for direct g++ builds.
# This is the ONLY writer of the in-tree copy since U-15 (CMake generates into
# its build tree from build_support/version.h.in and never touches src/).
# Header text must stay byte-identical to what CMake generates from
# build_support/version.h.in, otherwise the two generators dirty each other's
# output.
cat > "$self/src/core/version.h" <<EOF
// Generated from VERSION.md — do not edit by hand.
#ifndef GIFSCYTHE_CORE_VERSION_H
#define GIFSCYTHE_CORE_VERSION_H

#define GS_VERSION "$version"
#define GS_VERSION_STR "$version"

#endif  // GIFSCYTHE_CORE_VERSION_H
EOF

# 1. Engine
echo "==> [1/4] Building gifsicle engine..."
"$self/scripts/build_engine.sh" $engine_arg || fail "engine build failed"

# Audit U-31: <filesystem> lived in a separate libstdc++fs on GCC 7 and 8, so
# EngineLocator.h (which uses std::filesystem) failed to LINK on those hosts even
# though it compiled. GCC >= 9 folded it into libstdc++, where -lstdc++fs is a
# no-op at best and an unknown-library error on some toolchains. Detect it
# instead of assuming either way.
STDCXXFS=""
fs_probe="$BUILD_DIR/.fs_probe.cpp"
printf '#include <filesystem>\nint main(){ return std::filesystem::exists("x") ? 0 : 1; }\n' \
  > "$fs_probe"
if ! g++ -std=c++17 "$fs_probe" -o "$BUILD_DIR/.fs_probe" >/dev/null 2>&1; then
  if g++ -std=c++17 "$fs_probe" -o "$BUILD_DIR/.fs_probe" -lstdc++fs >/dev/null 2>&1; then
    STDCXXFS="-lstdc++fs"
    echo "    (this toolchain keeps <filesystem> in libstdc++fs; adding -lstdc++fs)"
  fi
fi
rm -f "$fs_probe" "$BUILD_DIR/.fs_probe"

# 2. CLI
echo "==> [2/4] Building CLI driver (gifscythe-cli)..."
g++ -std=c++17 -Wall -Wextra -pedantic -O2 -I"$self/src" \
  -o "$BUILD_DIR/gifscythe-cli" "$self/src/cli/main.cpp" $STDCXXFS \
  || fail "CLI build failed"

# 3. Unit tests
echo "==> [3/4] Running unit tests..."
g++ -std=c++17 -Wall -Wextra -pedantic -O2 -I"$self/src" \
  -o "$BUILD_DIR/test_gifsicle_command" "$self/tests/test_gifsicle_command.cpp" $STDCXXFS \
  || fail "unit test compile failed"
"$BUILD_DIR/test_gifsicle_command" || fail "unit tests failed"
# GS-205 / P1-27 (S37): the input-admission rule has its own Qt-free suite, so the
# rule the GUI merely CALLS is pinned on every platform this script builds for.
g++ -std=c++17 -Wall -Wextra -pedantic -O2 -I"$self/src" \
  -o "$BUILD_DIR/test_input_admission" "$self/tests/test_input_admission.cpp" $STDCXXFS \
  || fail "input-admission test compile failed"
"$BUILD_DIR/test_input_admission" || fail "input-admission tests failed"

# 4. GUI (only when requested)
echo "==> [4/4] Qt6 GUI..."
gui_built=0
gui_path=""

if [[ "$want_gui" == "1" ]]; then
  # Remove stale GUI binaries FIRST: otherwise a leftover binary from an
  # earlier successful build would pass the -x probe and build.sh would
  # claim "GUI built" even when Qt6 is now missing and the target was
  # skipped (silent-failure class; found during COMPILED_AUDIT C7 verify).
  rm -f "$BUILD_DIR/gifscythe" "$BUILD_DIR/gifscythe.exe" \
        "$BUILD_DIR/cmake/gifscythe" "$BUILD_DIR/cmake/gifscythe.exe" \
        "$BUILD_DIR/gui/gifscythe" "$BUILD_DIR/gui/gifscythe.exe"
  if build_gui; then
    gui_built=1
    echo "   GUI built: $gui_path"
  else
    echo "   GUI build FAILED. Last log lines:" >&2
    tail -n 40 "$LOG" >&2 || true
    fail "Qt6 GUI was requested (--gui/--all) but could not be built. Install Qt6 (qt6-base-dev) or see $LOG"
  fi
else
  # Opportunistic attempt is NOT done — default path is engine+CLI+tests only.
  echo "   GUI not requested (pass --all or --gui to build it)."
fi

# The repo holds no image files (N-36): rebuild the upstream test images from their text form so the example
# config and every suite find them (build/fixtures/). Not fatal - the suites call fixtures.sh themselves.
"$self/scripts/fixtures.sh" >/dev/null 2>&1 || echo "   note: could not materialise build/fixtures (see scripts/fixtures.sh)"

echo ""
echo "==> Build complete (v$version)."
echo "   Engine:  $self/release/$version/gifsicle"
echo "   CLI:     $BUILD_DIR/gifscythe-cli"
if [[ "$gui_built" == "1" ]]; then
  echo "   GUI:     $gui_path"
fi
}

# Sourceable for tests; executing it builds.
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
