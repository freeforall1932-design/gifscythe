// Gifscythe main window - the GUI shell over the gifsicle engine.
//
// Layout (XNConvert-style flow):
//   Tab 1 "Input"   — animation queue (add / drag-drop / remove / clear,
//                     per-file size, total count/size label)
//   Tab 2 "Actions" — SettingsPanel: every whole-GIF control gifsicle has
//   Tab 3 "Output"  — Save-as (Merge/single-file Batch), batch output
//                     folder, open-folder action, honest per-mode summary
//   Right pane      — PreviewPanel: debounced async before/after preview
//   Bottom          — live one-way command pane, progress, run/cancel, status
//
// Run semantics are UNCHANGED from the verified MVP (COMPILED_AUDIT §6.B):
//   * Batch default (E4): N inputs -> N outputs, auto <name>_opt.gif next to
//     each input, or in the chosen batch folder; single input + explicit
//     Save-as uses that path (E1).
//   * Merge requires an output file; empty output REFUSES (B12, no silent
//     stdout loss).
//   * Explode auto-prefixes <stem>_frame (E2).
//   * Async QProcess, cancel, honest failure statuses, output verified to
//     exist and be non-empty before "complete" is claimed.
//
// Preview pipeline: any control/queue/selection change restarts a 1200 ms
// debounce timer; on fire, a SEPARATE QProcess re-encodes the selected file
// to a temp dir. Never blocks the UI thread (S3-7 rule); main runs pause
// previewing and kill any in-flight preview process.

#ifndef GIFSCYTHE_MAINWINDOW_H
#define GIFSCYTHE_MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QProcess>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTabWidget;
class QTimer;
class DropListWidget;
class PreviewPanel;
class SettingsPanel;

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
  // UI construction
  QWidget* buildInputTab();
  QWidget* buildOutputTab();
  void buildBottomBar(class QWidget* central, class QVBoxLayout* root);

  // Queue helpers
  void chooseOutput();
  void chooseBatchDir();
  void openOutputFolder();
  void updateStatus(const QString& message);
  void appendInputs(const QStringList& files);
  void setBusy(bool busy);
  QString defaultOutputFor(const QString& input) const;
  bool ensureEngine();
  void refreshQueueLabel();
  void refreshOutputSummary();
  QString batchDir() const;

  // Preview pipeline
  void schedulePreview();
  void startPreview();
  void onSelectionChanged();
  void killPreview();
  QString selectedInput() const;

  // Widgets
  QTabWidget* tabs_ = nullptr;
  DropListWidget* inputList_ = nullptr;
  QLabel* queueCountLabel_ = nullptr;
  SettingsPanel* settingsPanel_ = nullptr;
  QLineEdit* outputEdit_ = nullptr;
  QLineEdit* batchDirEdit_ = nullptr;
  QPushButton* openDirButton_ = nullptr;
  QLabel* outputSummaryLabel_ = nullptr;
  PreviewPanel* previewPanel_ = nullptr;
  QPlainTextEdit* commandPane_ = nullptr;
  QPushButton* runButton_ = nullptr;
  QPushButton* cancelButton_ = nullptr;
  QPushButton* removeButton_ = nullptr;
  QPushButton* clearButton_ = nullptr;
  QLabel* statusLabel_ = nullptr;
  QProgressBar* progressBar_ = nullptr;

  // Engine / run state
  QString enginePath_;
  QStringList inputs_;
  QProcess* process_ = nullptr;        // objectName "engineProcess"
  bool busy_ = false;
  QString pendingOutput_;
  int batchIndex_ = -1;
  QStringList batchQueue_;
  gs::Mode batchMode_ = gs::Mode::Batch;

  // Preview state
  QProcess* previewProcess_ = nullptr;  // objectName "previewProcess"
  QTimer* previewTimer_ = nullptr;      // single-shot debounce (1200 ms)
  int previewSeq_ = 0;                  // stale-completion guard
  QString previewDir_;                  // temp dir for preview outputs
};

#endif  // GIFSCYTHE_MAINWINDOW_H
