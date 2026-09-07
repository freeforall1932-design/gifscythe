# gifscythe.pro - qmake project for the Gifscythe Qt6 GUI (secondary to CMake).
#
# Preferred build path is CMake (see CMakeLists.txt / build.sh --all).
# This .pro is kept for environments that only have qmake.
#
#   qmake6 gifscythe.pro && make
#
# The engine control layer (src/core/) is header-only and shared via INCLUDEPATH.
# The gifsicle engine binary is produced by scripts/build_gifsicle.sh (subprocess).

QT += widgets
CONFIG += c++17
TARGET = gifscythe
TEMPLATE = app

# Version is informational here; runtime uses src/core/version.h (from VERSION.md).
VERSION = 0.1.0

SOURCES += \
    src/qtui/main.cpp \
    src/qtui/MainWindow.cpp \
    src/qtui/SettingsPanel.cpp \
    src/qtui/PreviewPanel.cpp

HEADERS += \
    src/qtui/MainWindow.h \
    src/qtui/DropListWidget.h \
    src/qtui/SettingsPanel.h \
    src/qtui/PreviewPanel.h \
    src/core/GifsicleSettings.h \
    src/core/GifsicleCommand.h \
    src/core/SettingsIO.h \
    src/core/EngineLocator.h \
    src/core/Validate.h \
    src/core/version.h

INCLUDEPATH += src
