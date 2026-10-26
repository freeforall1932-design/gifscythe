cmake_minimum_required(VERSION 3.16)

# Single version source: VERSION.md
file(READ "${CMAKE_SOURCE_DIR}/VERSION.md" _gs_version_md)
string(REGEX MATCH "Current version:[^\n]*([0-9]+\\.[0-9]+\\.[0-9]+)" _gs_ver_match "${_gs_version_md}")
set(GS_VERSION "${CMAKE_MATCH_1}")
if(NOT GS_VERSION)
  set(GS_VERSION "0.1.0")
endif()

project(gifscythe VERSION ${GS_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate version.h into the BUILD TREE ONLY (audit U-15 / fix-order P2-2).
# CMake used to also refresh the committed src/core/version.h fallback here,
# which made configure mutate the source tree: read-only checkouts failed,
# parallel build trees raced on one committed header, and `git status` was
# dirty after a mere configure. The fallback stays committed and is synced by
# build.sh alone; every CMake target consumes the generated copy below via
# the generated-FIRST include order on gifscythe_core.
configure_file(
  "${CMAKE_SOURCE_DIR}/build_support/version.h.in"
  "${CMAKE_BINARY_DIR}/generated/core/version.h"
  @ONLY
)

# ---- Engine control layer (header-only INTERFACE) ----
add_library(gifscythe_core INTERFACE)
target_include_directories(gifscythe_core INTERFACE
  "${CMAKE_BINARY_DIR}/generated"
  "${CMAKE_SOURCE_DIR}/src"
)

# ---- CLI driver ----
add_executable(gifscythe-cli src/cli/main.cpp)
target_link_libraries(gifscythe-cli PRIVATE gifscythe_core)

# ---- Unit tests ----
enable_testing()
add_executable(test_gifsicle_command tests/test_gifsicle_command.cpp)
target_link_libraries(test_gifsicle_command PRIVATE gifscythe_core)
add_test(NAME gifsicle_command COMMAND test_gifsicle_command)

# GS-205 / P1-27 (S37): the input-admission rule, Qt-free, so the rule itself
# is pinned by CI on Linux while the GUI only CALLS it (tests/test_input_admission.cpp).
add_executable(test_input_admission tests/test_input_admission.cpp)
target_link_libraries(test_input_admission PRIVATE gifscythe_core)
add_test(NAME input_admission COMMAND test_input_admission)

# Slice C: the Input grid's animation facts (frames / duration / loop) come
# from a Qt-free header walk; this pins the parser on every platform CI builds.
add_executable(test_gif_facts tests/test_gif_facts.cpp)
target_link_libraries(test_gif_facts PRIVATE gifscythe_core)
add_test(NAME gif_facts COMMAND test_gif_facts)

# Slice E2: the Output tab's Filename template is a Qt-free rule (tokens,
# format-owned extension, duplicate numbering), pinned on every platform.
add_executable(test_name_template tests/test_name_template.cpp)
target_link_libraries(test_name_template PRIVATE gifscythe_core)
add_test(NAME name_template COMMAND test_name_template)

# The FFmpeg sidecar locator: bundled copy first, PATH/installed only as a
# fallback. Fixture-driven so the precedence is proven, not assumed.
add_executable(test_ffmpeg_locator tests/test_ffmpeg_locator.cpp)
target_link_libraries(test_ffmpeg_locator PRIVATE gifscythe_core)
add_test(NAME ffmpeg_locator COMMAND test_ffmpeg_locator)

# What each format's encoder command must say, and what its output must look like
# afterwards (a still WebP under "animated WebP" is a failure, not a result).
add_executable(test_ffmpeg_command tests/test_ffmpeg_command.cpp)
target_link_libraries(test_ffmpeg_command PRIVATE gifscythe_core)
add_test(NAME ffmpeg_command COMMAND test_ffmpeg_command)

# Runs the real FFmpeg when the machine has one and prints SKIP when it does not
# (exit 0 either way), so "the formats work" is a measured claim, not a hope.
add_executable(test_ffmpeg_roundtrip tests/test_ffmpeg_roundtrip.cpp)
target_link_libraries(test_ffmpeg_roundtrip PRIVATE gifscythe_core)
add_test(NAME ffmpeg_roundtrip COMMAND test_ffmpeg_roundtrip)
set_tests_properties(ffmpeg_roundtrip PROPERTIES SKIP_RETURN_CODE 77)

# Lying-engine fixture for the harness's U-17 explode-verification test:
# exits 0 without writing anything. Tiny, dependency-free, built on every
# platform so T7 finds it next to test_gui_offscreen (see tests/fake_engine_exit0.cpp).
add_executable(fake_engine_exit0 tests/fake_engine_exit0.cpp)

# Writes corrupt bytes to -o and then fails; GUI U-59 regression fixture.
add_executable(fake_engine_partial_failure tests/fake_engine_partial_failure.cpp)

# Slow, argv-logging engine: makes each run's argv observable (U-58 / P1-38
# mid-batch mutation) and stays alive so Cancel can race a REAL process
# (U-72 / P1-42). Copies its input to -o so the promotion path accepts it.
add_executable(fake_engine_argv_sleep tests/fake_engine_argv_sleep.cpp)

# U-12 / P1-24 (S37): a fixture whose CHILD keeps the inherited pipes open after
# the parent is killed - without it, kill()+waitForFinished() returns in ~1 ms and
# the event-loop freeze cannot be measured at all (see the file's header).
add_executable(fake_engine_orphan_pipe tests/fake_engine_orphan_pipe.cpp)

# ---- Qt6 GUI (optional) ----
option(BUILD_GUI "Build the Qt6 GUI" ON)
if(BUILD_GUI)
  find_package(Qt6 COMPONENTS Widgets QUIET)
  if(Qt6_FOUND)
    message(STATUS "Qt6 found -> building Gifscythe GUI")
    qt_standard_project_setup()
    # The desktop executable is a GUI-subsystem application. Without WIN32,
    # MinGW marks it as a console-subsystem executable and Windows opens a
    # command window even when the user double-clicks Gifscythe. The engine
    # child is hidden separately in MainWindow::configureHeadless().
    qt_add_executable(gifscythe WIN32
      src/qtui/main.cpp
      src/qtui/MainWindow.cpp
      src/qtui/MainWindow.h
      src/qtui/DropListWidget.h
      src/qtui/SettingsPanel.cpp
      src/qtui/SettingsPanel.h
      src/qtui/PreviewPanel.cpp
      src/qtui/PreviewPanel.h
    )
    target_link_libraries(gifscythe PRIVATE gifscythe_core Qt6::Widgets)
    # generated FIRST (audit U-15): core/version.h must resolve to the build-tree
    # copy configured from VERSION.md, never to the committed src/ fallback.
    target_include_directories(gifscythe PRIVATE "${CMAKE_BINARY_DIR}/generated" src)

    # Offscreen GUI test harness (COMPILED_AUDIT 6.B evidence; runs in CI).
    qt_add_executable(test_gui_offscreen
      tests/test_gui_offscreen.cpp
      src/qtui/MainWindow.cpp
      src/qtui/MainWindow.h
      src/qtui/DropListWidget.h
      src/qtui/SettingsPanel.cpp
      src/qtui/SettingsPanel.h
      src/qtui/PreviewPanel.cpp
      src/qtui/PreviewPanel.h
    )
    target_link_libraries(test_gui_offscreen PRIVATE gifscythe_core Qt6::Widgets)
    target_include_directories(test_gui_offscreen PRIVATE "${CMAKE_BINARY_DIR}/generated" src)
    # Every fixture the harness looks for must be built WITH it: the audit gate
    # builds `--target test_gui_offscreen` only, so a fixture that is merely
    # declared (not a dependency) is missing at run time and its case fails on the
    # environment. T26's orphan-pipe fixture was exactly that when it was added.
    add_dependencies(test_gui_offscreen fake_engine_exit0 fake_engine_partial_failure
                     fake_engine_argv_sleep fake_engine_orphan_pipe)
    add_test(NAME gui_offscreen COMMAND test_gui_offscreen)
    set_tests_properties(gui_offscreen PROPERTIES
      ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
      TIMEOUT 600)
  else()
    message(STATUS "Qt6 NOT found -> GUI skipped. Build it where Qt6 is installed.")
  endif()
endif()

# ---- MinGW: static C++ runtime so binaries run without MinGW DLLs ----
# (libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll). Qt itself
# stays dynamic — windeployqt ships those DLLs for the GUI; the CLI and the
# unit tests must be fully self-contained. Verified under Wine 2026-09-07:
# dynamically linked exes fail silently on a machine without the runtime.
if(MINGW)
  target_link_options(gifscythe-cli PRIVATE -static)
  target_link_options(test_gifsicle_command PRIVATE -static)
  target_link_options(fake_engine_exit0 PRIVATE -static)
  target_link_options(fake_engine_partial_failure PRIVATE -static)
  target_link_options(fake_engine_argv_sleep PRIVATE -static)
  foreach(gui_tgt gifscythe test_gui_offscreen)
    if(TARGET ${gui_tgt})
      target_link_options(${gui_tgt} PRIVATE -static-libgcc -static-libstdc++)
    endif()
  endforeach()
endif()

# ---- Engine build hook (bash required; skipped cleanly on pure Windows generators) ----
find_program(GS_BASH bash)
if(GS_BASH)
  add_custom_target(build_engine
    COMMAND "${GS_BASH}" "${CMAKE_SOURCE_DIR}/scripts/build_engine.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Building bundled gifsicle engine"
    USES_TERMINAL
  )
else()
  message(STATUS "bash not found -> build_engine target skipped; use scripts/build_engine.sh")
endif()
