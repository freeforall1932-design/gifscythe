// main.cpp - entry point for the Gifscythe Qt6 GUI.

#include "MainWindow.h"
#include <QApplication>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Gifscythe"));
  QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

  MainWindow window;
  window.show();
  return app.exec();
}
