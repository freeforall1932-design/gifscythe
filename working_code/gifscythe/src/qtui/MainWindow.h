// Gifscythe main window - the GUI shell over the gifsicle engine.
//
// This is a MINIMAL Qt6 Widgets starting point (modeled on the "engine control
// layer in, GUI out" structure). It proves the GUI ↔ core wiring:
//   1. Pick input GIF(s) with a file dialog.
//   2. The core layer (GifsicleSettings + GifsicleCommand) turns the current
//      UI state into the exact gifsicle command line.
//   3. The "live command" pane shows that command (full terminal control).
//   4. "Run" executes it against the bundled gifsicle engine (QProcess).
//
// Built only where Qt6 is installed (see scripts/build.sh). This file is
// standard Qt6 Widgets + QProcess only, to keep compile risk low.

#ifndef GIFSYCYTHE_MAINWINDOW_H
#define GIFSYCYTHE_MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QProcess>

class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QLineEdit;
class QLabel;

#include "core/GifsicleSettings.h"
#include "core/GifsicleCommand.h"

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);

  // Current settings derived from the widget state -> feed to core layer.
  gs::Settings currentSettings() const;

 private slots:
  void chooseInputs();
  void refreshCommand();
  void runCommand();

 private:
  QListWidget* inputList_;
  QPlainTextEdit* commandPane_;
  QPushButton* runButton_;
  QSpinBox* optimizeSpin_;
  QSpinBox* lossySpin_;
  QLineEdit* outputEdit_;
  QLabel* statusLabel_;

  QString enginePath_;

  void chooseOutput();
  void updateStatus(const QString& message);

  QStringList inputs_;
};

#endif  // GIFSYCYTHE_MAINWINDOW_H
