#!/usr/bin/env bash
#
# build_qt6_local.sh - provision a minimal Qt6 from source in a sandbox that has
# no apt/root, so the Qt offscreen harness can be COMPILED AND RUN locally
# instead of waiting for CI.
#
# Why this exists (S36): SESSION_HANDOFF.md said "no Qt6 build" was possible
# here, so the S35 rows U-58/U-70/U-72 shipped PARTIAL with unproven Qt hunks.
# It is possible: qtbase builds Core + Gui + Widgets + the offscreen platform
# plugin + the qgif image-format plugin with bundled zlib/pcre/freetype/libpng/
# harfbuzz, no system dev packages needed, in about 15 minutes on 2 cores.
#
# Usage:
#   ./scripts/build_qt6_local.sh                 # /home/user/build/gifscythe-qt6
#   QT6_ROOT=/some/dir ./scripts/build_qt6_local.sh
#
# Then build and run the harness against it (the paths this script prints):
#
#   cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON \
#         -DCMAKE_PREFIX_PATH="$QT6_ROOT/qtbase/build"
#   cmake --build build-cmake -j2
#   export LD_LIBRARY_PATH="$QT6_ROOT/qtbase/build/lib"
#   export QT_PLUGIN_PATH="$QT6_ROOT/qtbase/build/plugins"
#   export QT_QPA_PLATFORM=offscreen
#   export GS_TEST_REF_DIR="$(./scripts/fixtures.sh)"
#   ./build-cmake/test_gui_offscreen
#
# Two traps this script already avoids, both measured in S36:
#   1. The offscreen platform plugin must be built explicitly; without it Qt
#      aborts at QApplication with "Could not find the Qt platform plugin".
#   2. The qgif image-format plugin is NOT in a module build's default target
#      set either. Without it QMovie cannot decode a GIF at all, so T12/T20/T23
#      fail on the *environment*, not on the code (6 such failures before the
#      plugin was built, 0 after). Build it and keep it.
#
# Output lives under a path whose component is named "build" (not snapshotted,
# not part of the repo): it is a toolchain, not a deliverable.
set -euo pipefail

QT6_ROOT="${QT6_ROOT:-$HOME/build/gifscythe-qt6}"
QT6_TAG="${QT6_TAG:-v6.8.3}"
SRC="$QT6_ROOT/qtbase"
BUILD="$QT6_ROOT/build"

say() { printf '%s\n' "$*"; }

# cmake + ninja are not preinstalled in the sandbox; pip is the only package
# source that answers here (apt mirrors are blocked, and pip needs
# --break-system-packages because the system Python is PEP 668 managed).
if ! command -v cmake >/dev/null 2>&1 || ! command -v ninja >/dev/null 2>&1; then
  python3 -m pip install --user --break-system-packages --quiet cmake ninja
  export PATH="$HOME/.local/bin:$PATH"
fi
command -v cmake >/dev/null 2>&1 || { say "FAIL: cmake unavailable after pip install"; exit 1; }
command -v ninja >/dev/null 2>&1 || { say "FAIL: ninja unavailable after pip install"; exit 1; }
say "cmake: $(cmake --version | head -1)"
say "ninja: $(ninja --version)"

mkdir -p "$QT6_ROOT"
if [ ! -d "$SRC/.git" ]; then
  say "cloning qtbase $QT6_TAG (shallow) -> $SRC"
  git clone --depth 1 --branch "$QT6_TAG" https://github.com/qt/qtbase.git "$SRC"
else
  say "reusing existing $SRC"
fi

# Configure. The feature set is the minimum the harness needs: no X11, no
# OpenGL, no dbus/glib/openssl/fontconfig, bundled third-party libs (the
# sandbox has no -dev packages). -no-feature-printsupport and -DFEATURE_*=OFF
# keep the build small; none of them are used by the GUI or the harness.
# NOTE: qtbase's configure builds in the CURRENT directory, so the build tree
# must exist first and configure must run from inside it. Run from anywhere
# else and it silently writes a Qt build tree into YOUR cwd (measured the hard
# way in S36: it dropped ~1150 files into working_code/gifscythe).
mkdir -p "$BUILD"
if [ ! -f "$BUILD/CMakeCache.txt" ]; then
  ( cd "$BUILD" && "$SRC/configure" -prefix "$QT6_ROOT/prefix" -release -shared \
    -no-opengl -no-xcb -no-egl -no-linuxfb -no-feature-vulkan \
    -no-feature-dbus -no-feature-glib -no-feature-openssl \
    -no-feature-fontconfig -no-feature-cups -no-feature-printsupport \
    -qt-zlib -qt-pcre -qt-libpng -qt-freetype -qt-harfbuzz \
    -nomake examples -nomake tests \
    -DFEATURE_sql=OFF -DFEATURE_testlib=OFF \
    2>&1 | tail -20 )
fi

cd "$BUILD"
# The module libs the harness links, the host tools its AUTOMOC steps need,
# the offscreen platform plugin, and the qgif image-format plugin.
cmake --build . --target Core -j2
cmake --build . --target moc rcc uic -j2
cmake --build . --target Gui -j2
cmake --build . --target Widgets -j2
cmake --build . --target QOffscreenIntegrationPlugin -j2
cmake --build . --target QGifPlugin -j2

say ""
say "Qt6 ready. Module libs:   $BUILD/lib"
say "            plugins:     $BUILD/plugins  (platforms + imageformats)"
say "            cmake config: $BUILD/lib/cmake/Qt6"
say ""
say "Build and run the harness:"
say "  cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON -DCMAKE_PREFIX_PATH=$BUILD"
say "  cmake --build build-cmake -j2"
say "  LD_LIBRARY_PATH=$BUILD/lib QT_PLUGIN_PATH=$BUILD/plugins QT_QPA_PLATFORM=offscreen \\"
say "    GS_TEST_REF_DIR=\"\$(./scripts/fixtures.sh)\" ./build-cmake/test_gui_offscreen"
