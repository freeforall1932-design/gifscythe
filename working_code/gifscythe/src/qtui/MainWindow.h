// Gifscythe main window - the GUI shell over the gifsicle engine.
//
// Layout (XNConvert-style flow):
//   Tab 1 "Input"   — animation queue (add / drag-drop / remove / clear /
//                     move up-down), batch file-name pattern, and prominent Start
//   Tab 2 "Actions" — SettingsPanel: every whole-GIF control gifsicle has
//   Tab 3 "Output"  — Save-as (Merge/single-file Batch), batch output
//                     folder, verified files-on-disk list, open-file/folder actions
//   Tab 4 "Guide"   — plain-language glossary and workflow help
//   Right pane      — PreviewPanel: before/after, explicit Play/Stop, and a
//                     debounced settings preview (large GIFs opt out by default)
//   Bottom          — live one-way command pane, activity log, progress, Cancel, status
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
// to a temp dir. Main runs pause previewing and kill any in-flight preview
// process.
//
// CORRECTED 2026-09-27. This comment used to claim the GUI "Never blocks the
// UI thread (S3-7 rule)". It does block, in five bounded places; U-12 is still
// OPEN and this header is the comment a maintainer trusts:
//     ~MainWindow()              process_->waitForFinished(2000)
//     runCommand()  batch branch process_->waitForStarted(5000)
//     runCommand() single branch process_->waitForStarted(5000)
//     cancelRun()                process_->waitForFinished(3000)
//     killPreview()              previewProcess_->waitForFinished(1000)
//   killPreview() is also called from startPreview() and from setBusy(true),
//   i.e. on the UI thread during the first frame of every run.
// What IS true: nothing blocks for the DURATION of a run. The engine runs in a
// separate QProcess and completion arrives via the finished() signal; the waits
// above bound START and CANCEL only. Do not restore the old wording until the
// five sites are actually gone.
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
class QListWidget;
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
#include "core/OutputPlan.h"
#include "core/ExplodeVerify.h"
#include "core/OutputVerify.h"

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
  // AUD-02: remove THIS run's staging output - the partial file, or for
  // Explode the partial frame set (pendingPartial_ is then a prefix).
  void discardPendingPartial();
  // UI construction
  QWidget* buildInputTab();
  QWidget* buildOutputTab();
  QWidget* buildGuideTab();
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
  // Batch output planning (audit U-01): every target for the current queue,
  // and the core planner's verdict on that set. Both const, both used by the
  // summary label (to warn before Run) and by runCommand() (to refuse).
  QStringList plannedBatchTargets() const;
  gs::OutputPlan planBatch() const;
  QString renderedOutputName(const QFileInfo& input) const;
  bool templateIsConstant() const;
  bool ensureEngine();
  void refreshQueueLabel();
  void refreshOutputSummary();
  void refreshOutputFiles();
  QStringList plannedOutputPaths() const;
  void openSelectedOutput();
  QString batchDir() const;

  // Run log / progress helpers
  void appendLog(const QString& message);
  void captureProcessOutput();
  void resetRunLog();
  void showWrittenOutputPreview();

  // Preview pipeline
  void schedulePreview();
  void startPreview();
  void onSelectionChanged();
  void killPreview();
  // Preview invalidation + temp-file hygiene (audit U-34/U-47, fix-order
  // P1-10). invalidatePreview() advances previewSeq_ so every in-flight
  // preview becomes stale the MOMENT the selection/settings/queue change —
  // not only when a new eligible preview starts. cleanupPreviewFiles()
  // sweeps superseded/failed preview_*.gif out of the temp dir (all but the
  // file currently on display — QMovie reads it lazily).
  void invalidatePreview();
  void cleanupPreviewFiles(const QString& keep = QString());
  QString selectedInput() const;

  // Widgets
  QTabWidget* tabs_ = nullptr;
  DropListWidget* inputList_ = nullptr;
  QLabel* queueCountLabel_ = nullptr;
  SettingsPanel* settingsPanel_ = nullptr;
  QLineEdit* outputEdit_ = nullptr;
  QLineEdit* batchDirEdit_ = nullptr;
  QLineEdit* nameTemplateEdit_ = nullptr;
  // Browse buttons kept as members so setBusy() can lock the WHOLE output
  // group while a run is in flight (audit U-45, fix-order P1-23): a picker
  // can still setText() on a disabled QLineEdit, so disabling only the text
  // fields left the run's destination one dialog away from changing mid-run.
  QPushButton* outputBrowse_ = nullptr;
  QPushButton* batchDirBrowse_ = nullptr;
  QPushButton* openDirButton_ = nullptr;
  QLabel* outputSummaryLabel_ = nullptr;
  PreviewPanel* previewPanel_ = nullptr;
  QPlainTextEdit* commandPane_ = nullptr;
  QPlainTextEdit* logPane_ = nullptr;
  QListWidget* outputFiles_ = nullptr;
  QPushButton* runButton_ = nullptr;
  QPushButton* cancelButton_ = nullptr;
  QPushButton* openSelectedOutputButton_ = nullptr;
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
  QString pendingOutput_;              // user-visible final path/prefix
  QString pendingPartial_;             // U-59: guarded same-directory write path; empty for Explode
  gs::OutputSnapshot partialSnapshot_; // pre-run baseline for U-59 verification
  QStringList batchTargets_;           // planned per-file outputs (audit U-01)
  QStringList lastOutputPaths_;        // files from the last verified successful run
  QString processStderr_;              // stderr captured while the engine is running
  QString processStdout_;              // bounded stdout capture for the activity log
  int batchIndex_ = -1;
  QStringList batchQueue_;
  // U-58 / P1-38 (F:NF-01): the settings the batch was STARTED with. Batch
  // continuation jobs (onProcessFinished) used to call currentSettings()
  // live, so any Actions-tab state that changed mid-batch silently altered
  // every job after the first — the settings-twin of U-45's closed
  // destination hole: an unplanned mutation of a job plan that was computed
  // and collision-checked against DIFFERENT values. Written once in
  // runCommand()'s batch branch, read only while batchIndex_ >= 0.
  gs::Settings batchSettings_;
  gs::Mode batchMode_ = gs::Mode::Batch;
  // Explode frame verification (audit U-17 / P1-19): pre-run snapshot of the
  // files under the frame prefix, so onProcessFinished can tell frames THIS
  // run wrote from stale leftovers. Empty for every non-Explode run.
  std::vector<gs::ExplodeFileState> explodeSnapshot_;

  // Preview state
  QProcess* previewProcess_ = nullptr;  // objectName "previewProcess"
  QTimer* previewTimer_ = nullptr;      // single-shot debounce (1200 ms)
  int previewSeq_ = 0;                  // stale-completion guard
  bool forcePreview_ = false;           // explicit button may preview large GIFs
  QString previewDir_;                  // temp dir for preview outputs
  QString lastPreviewPath_;             // file currently shown as After (never swept)
};

#endif  // GIFSCYTHE_MAINWINDOW_H
