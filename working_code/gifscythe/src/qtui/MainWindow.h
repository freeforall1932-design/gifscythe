// Gifscythe main window - the GUI shell over the gifsicle engine.
//
// Layout (XNConvert-style flow):
//   Tab 1 "Input"   — animation queue (add / drag-drop / remove / clear /
//                     move up-down, per-file size, total count/size label)
//   Tab 2 "Actions" — SettingsPanel: every whole-GIF control gifsicle has
//   Tab 3 "Output"  — Save-as (Merge/single-file Batch), batch output
//                     folder, name template ({name}_opt.gif default),
//                     open-folder action, honest per-mode summary
//   Right pane      — PreviewPanel: debounced async before/after preview
//   Bottom          — live one-way command pane, progress, run/cancel, status
//
// Run semantics are UNCHANGED from the verified MVP (COMPILED_AUDIT §6.B):
//   * Batch default (E4): N inputs -> N outputs, auto <name>_opt.gif next to
//     each input (or the chosen batch folder / name template); single input +
//     explicit Save-as uses that path (E1).
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
//
// Session persistence (S7 — closes the OFFLINE_BUILD_REVIEW §6 gap and audit
// row 15): on close, the Actions-tab state is written through the core
// SettingsIO serializer plus two GUI-only keys (batch_dir, name_template) to
// %AppConfig%/gifscythe.conf (override: GS_SETTINGS_PATH env var). On start,
// the file is loaded back (first launch = defaults, no error). Deliberately
// NOT persisted: the queue (files move between sessions) and the Save-as
// field (a per-run choice — restoring it could silently overwrite a stale
// path). Load warnings surface in the status bar instead of being dropped.

#ifndef GIFSCYTHE_MAINWINDOW_H
#define GIFSCYTHE_MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QProcess>

class QLabel;
class QLineEdit;
class QFileInfo;
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

  // ---- Session persistence (S7) ----
  // Where settings are stored: $GS_SETTINGS_PATH when set (tests/portable),
  // else QStandardPaths::AppConfigLocation + "/gifscythe.conf" (empty if the
  // platform has no writable config location — persistence then no-ops).
  QString sessionFilePath() const;
  // Apply a previously saved session (no-op on first launch). Called by the
  // constructor; public for the offscreen harness.
  void loadSessionState();
  // Write the current Actions state + GUI keys. Returns false only when a
  // path exists but writing failed (closeEvent surfaces that honestly).
  bool saveSessionState();

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
  void moveCurrent(int delta);
  QString defaultOutputFor(const QString& input) const;
  QString renderedOutputName(const QFileInfo& input) const;
  bool templateIsConstant() const;
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
  QLineEdit* nameTemplateEdit_ = nullptr;
  QPushButton* openDirButton_ = nullptr;
  QLabel* outputSummaryLabel_ = nullptr;
  PreviewPanel* previewPanel_ = nullptr;
  QPlainTextEdit* commandPane_ = nullptr;
  QPushButton* runButton_ = nullptr;
  QPushButton* cancelButton_ = nullptr;
  QPushButton* removeButton_ = nullptr;
  QPushButton* clearButton_ = nullptr;
  QPushButton* moveUpButton_ = nullptr;
  QPushButton* moveDownButton_ = nullptr;
  QLabel* statusLabel_ = nullptr;
  QProgressBar* progressBar_ = nullptr;

  // Engine / run state
  QString enginePath_;
  QStringList inputs_;
  QProcess* process_ = nullptr;        // objectName "engineProcess"
  bool busy_ = false;
  bool cancelling_ = false;            // set while cancelRun() kills the engine
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
