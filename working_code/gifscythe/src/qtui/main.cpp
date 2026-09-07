// main.cpp - entry point for the Gifscythe Qt6 GUI.

#include "MainWindow.h"
#include "core/version.h"
#include <QApplication>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Gifscythe"));
  QApplication::setApplicationVersion(QStringLiteral(GS_VERSION));

  MainWindow window;
  window.show();
  return app.exec();
}
