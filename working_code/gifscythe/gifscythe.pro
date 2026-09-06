# gifscythe.pro - qmake project for the Gifscythe Qt6 GUI.
#
# Build the GUI where Qt is installed:
#   qmake6 gifscythe.pro && make
# or
#   mkdir build && cd build && qmake6 .. && make
#
# The engine control layer (src/core/) is header-only and shared with the GUI
# via include path. The gifsicle engine binary is produced by
# scripts/build_gifsicle.sh (subprocess, not linked in).

QT += widgets
CONFIG += c++17
TARGET = gifscythe
TEMPLATE = app
VERSION = 0.1.0

SOURCES += \
    src/qtui/main.cpp \
    src/qtui/MainWindow.cpp

HEADERS += \
    src/qtui/MainWindow.h \
    src/core/GifsicleSettings.h \
    src/core/GifsicleCommand.h \
    src/core/SettingsIO.h

INCLUDEPATH += src
