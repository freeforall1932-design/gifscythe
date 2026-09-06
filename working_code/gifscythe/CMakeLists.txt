cmake_minimum_required(VERSION 3.16)
project(gifscythe VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Engine control layer (Qt-independent) ----
add_library(gifscythe_core STATIC)
target_sources(gifscythe_core PRIVATE
    src/core/SettingsIO.h   # header-only; kept for target completeness
)
target_include_directories(gifscythe_core PUBLIC src)
set_target_properties(gifscythe_core PROPERTIES
    PUBLIC_HEADER "src/core/GifsicleSettings.h;src/core/GifsicleCommand.h;src/core/SettingsIO.h")

# ---- CLI driver (power-user terminal interface + live pane) ----
add_executable(gifscythe-cli src/cli/main.cpp)
target_link_libraries(gifscythe-cli PRIVATE gifscythe_core)

# ---- Unit tests ----
enable_testing()
add_executable(test_gifsicle_command tests/test_gifsicle_command.cpp)
target_link_libraries(test_gifsicle_command PRIVATE gifscythe_core)
add_test(NAME gifsicle_command COMMAND test_gifsicle_command)

# ---- Qt6 GUI (optional; enabled only if Qt6 is found) ----
option(BUILD_GUI "Build the Qt6 GUI" ON)
if(BUILD_GUI)
  find_package(Qt6 COMPONENTS Widgets QUIET)
  if(Qt6_FOUND)
    message(STATUS "Qt6 found -> building Gifscythe GUI")
    qt_standard_project_setup()
    qt_add_executable(gifscythe src/qtui/main.cpp src/qtui/MainWindow.cpp)
    target_link_libraries(gifscythe PRIVATE gifscythe_core Qt6::Widgets)
    target_include_directories(gifscythe PRIVATE src)
  else()
    message(STATUS "Qt6 NOT found -> GUI skipped. Build it where Qt6 is installed.")
  endif()
endif()

# ---- Engine build/packaging hooks ----
# gifsicle engine is built by scripts/build_gifsicle.sh (native or --windows).
add_custom_target(build_engine
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_gifsicle.sh
    COMMENT "Building bundled gifsicle engine"
    USES_TERMINAL
)
