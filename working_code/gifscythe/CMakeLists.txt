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

# Generate version.h into the build tree and also refresh the committed fallback.
configure_file(
  "${CMAKE_SOURCE_DIR}/src/core/version.h.in"
  "${CMAKE_BINARY_DIR}/generated/core/version.h"
  @ONLY
)
configure_file(
  "${CMAKE_SOURCE_DIR}/src/core/version.h.in"
  "${CMAKE_SOURCE_DIR}/src/core/version.h"
  @ONLY
)

# ---- Engine control layer (header-only INTERFACE) ----
add_library(gifscythe_core INTERFACE)
target_include_directories(gifscythe_core INTERFACE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_BINARY_DIR}/generated"
)

# ---- CLI driver ----
add_executable(gifscythe-cli src/cli/main.cpp)
target_link_libraries(gifscythe-cli PRIVATE gifscythe_core)

# ---- Unit tests ----
enable_testing()
add_executable(test_gifsicle_command tests/test_gifsicle_command.cpp)
target_link_libraries(test_gifsicle_command PRIVATE gifscythe_core)
add_test(NAME gifsicle_command COMMAND test_gifsicle_command)

# ---- Qt6 GUI (optional) ----
option(BUILD_GUI "Build the Qt6 GUI" ON)
if(BUILD_GUI)
  find_package(Qt6 COMPONENTS Widgets QUIET)
  if(Qt6_FOUND)
    message(STATUS "Qt6 found -> building Gifscythe GUI")
    qt_standard_project_setup()
    qt_add_executable(gifscythe
      src/qtui/main.cpp
      src/qtui/MainWindow.cpp
      src/qtui/MainWindow.h
      src/qtui/DropListWidget.h
    )
    target_link_libraries(gifscythe PRIVATE gifscythe_core Qt6::Widgets)
    target_include_directories(gifscythe PRIVATE src "${CMAKE_BINARY_DIR}/generated")
  else()
    message(STATUS "Qt6 NOT found -> GUI skipped. Build it where Qt6 is installed.")
  endif()
endif()

# ---- Engine build hook (bash required; skipped cleanly on pure Windows generators) ----
find_program(GS_BASH bash)
if(GS_BASH)
  add_custom_target(build_engine
    COMMAND "${GS_BASH}" "${CMAKE_SOURCE_DIR}/scripts/build_gifsicle.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Building bundled gifsicle engine"
    USES_TERMINAL
  )
else()
  message(STATUS "bash not found -> build_engine target skipped; use scripts/build_gifsicle.sh")
endif()
