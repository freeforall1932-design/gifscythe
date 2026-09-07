// Gifscythe main window - the GUI shell over the gifsicle engine.
//
// Proves the GUI ↔ core wiring:
//   1. Pick input GIF(s) with a file dialog or drag-and-drop.
//   2. Core layer turns UI state into the exact gifsicle command line.
//   3. Live command pane shows that command (full terminal control).
//   4. "Run" executes it against the bundled gifsicle engine (async QProcess).
//
// Built only where Qt6 is installed. Batch is the default queue mode;
// Merge is an explicit user choice.

#ifndef GIFSCYTHE_MAINWINDOW_H
#define GIFSCYTHE_MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QProcess>

class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QLineEdit;
class QLabel;
class QComboBox;
class QProgressBar;
class DropListWidget;

#include "core/GifsicleSettings.h"
#include "core/GifsicleCommand.h"

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  gs::Settings currentSettings() const;

 protected:
  void closeEvent(QCloseEvent* event) override;

 private slots:
  void chooseInputs();
  void removeSelected();
  void clearQueue();
  void refreshCommand();
  void runCommand();
  void cancelRun();
  void onProcessFinished(int exitCode, QProcess::ExitStatus status);
  void onProcessError(QProcess::ProcessError error);
  void onFilesDropped(const QStringList& files);

 private:
  void chooseOutput();
  void updateStatus(const QString& message);
  void appendInputs(const QStringList& files);
  void setBusy(bool busy);
  QString defaultOutputFor(const QString& input) const;
  bool ensureEngine();

  DropListWidget* inputList_ = nullptr;
  QPlainTextEdit* commandPane_ = nullptr;
  QPushButton* runButton_ = nullptr;
  QPushButton* cancelButton_ = nullptr;
  QPushButton* removeButton_ = nullptr;
  QPushButton* clearButton_ = nullptr;
  QSpinBox* optimizeSpin_ = nullptr;
  QSpinBox* lossySpin_ = nullptr;
  QLineEdit* outputEdit_ = nullptr;
  QComboBox* modeCombo_ = nullptr;
  QLabel* statusLabel_ = nullptr;
  QProgressBar* progressBar_ = nullptr;

  QString enginePath_;
  QStringList inputs_;
  QProcess* process_ = nullptr;
  bool busy_ = false;
  QString pendingOutput_;
  int batchIndex_ = -1;
  QStringList batchQueue_;
  gs::Mode batchMode_ = gs::Mode::Batch;
};

#endif  // GIFSCYTHE_MAINWINDOW_H
