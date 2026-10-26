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
#include <QVector>

class QLabel;
class QComboBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QFileInfo;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTabWidget;
class QTableWidget;
class QTimer;
class QToolButton;
class DropListWidget;
class PreviewPanel;
class SettingsPanel;

#include <QHash>
#include <QPixmap>

#include "core/GifFacts.h"
#include "core/GifsicleSettings.h"
#include "core/GifsicleCommand.h"
#include "core/OutputPlan.h"
#include "core/ExplodeVerify.h"
#include "core/FfmpegCommand.h"
#include "core/FfmpegLocator.h"
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
  // Slice C: the reference's "Add folder…". Scans the chosen folder (one level,
  // name-sorted) and hands every entry to the SAME admission rule as a drop —
  // a folder is a shortcut for selecting files, never a way around GS-205.
  void chooseInputFolder();
  void removeSelected();
  void clearQueue();
  // Slice C: the Quick-filter box and the sort header. Both act on the VIEW,
  // except sorting, which reorders the real queue because Merge concatenates
  // in queue order (S3-9) — a sort the engine ignores would be a lie.
  void applyQueueFilter();
  void applyQueueSort();
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
  // Owner, 2026-10-24: Actions is a two-pane splitter - the action stack and
  // the Preview - exactly as the reference screenshot shows.
  QWidget* buildActionsTab();
  QWidget* buildOutputTab();
  // W-32 / slice A (2026-10-22): the Status tab - one row per operation, per
  // the contract in docs/planning/W32_RESULT_CONTRACT.md.
  QWidget* buildStatusTab();
  // OD-21 = a (2026-10-22): the reference's remaining chrome. Settings carries
  // the app-level facts the engine/version probes already produce; About
  // carries the version/licence block and the plain-language guide, which used
  // to be a fourth tab the reference does not have.
  QWidget* buildSettingsTab();
  QWidget* buildAboutTab();
  void buildBottomBar(class QWidget* central, class QVBoxLayout* root);

  // ---- Status tab result rows (W-32, slice A) ----
  // beginResultRows() installs one `pending` row per queued input (Batch) or a
  // single row for the N->1 / 1->N modes, exactly as the contract's
  // cardinality table requires. Every later call moves ONE row; nothing
  // recomputes the table from the live queue, because the queue can change
  // while a run is in flight and the rows must describe the RUN.
  void beginResultRows(const QStringList& files);
  void beginSingleResultRow(const QString& label, qint64 beforeBytes);
  void setResultState(int row, const QString& state, const QString& detail,
                      qint64 afterBytes = -1);
  void markRemainingSkipped(int fromRow);
  void refreshResultSummary();
  void refreshTabTitles();

  // Queue helpers
  void chooseOutput();
  void chooseBatchDir();
  void openOutputFolder();
  void updateStatus(const QString& message);
  // ---- Presets (reference footer, owner 2026-10-24) ----
  // A preset is a saved Actions state plus the GUI-only batch_dir /
  // name_template keys, stored as one .conf per preset next to the session
  // file. Real files, so a preset survives a restart and can be inspected.
  QString presetDir() const;
  void refreshPresetList();
  void savePresetAs();
  void loadSelectedPreset();
  void deleteSelectedPreset();
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
  // ---- N-62: the FFmpeg sidecar -----------------------------------------
  // Which container the Format combo is asking for. Read from the item's token
  // data, not from a hard-coded index, so reordering the list cannot silently
  // change what a run encodes.
  gs::OutputFormat selectedOutputFormat() const;
  // Update the Format controls from the cached locator answer. `relocate` also
  // re-runs the search (start-up, and Format > Settings… after the user
  // installed FFmpeg); a plain selection change must NOT re-scan the disk.
  void refreshFfmpegStatus(bool relocate = false);
  // Select a format by its stable token ("webp", "mp4-h264", ...). Unknown or
  // absent tokens mean GIF: a preset written before formats existed must not
  // suddenly encode something else.
  void applyOutputFormatToken(const QString& token);
  // Configure process_ for one job. GIF -> the bundled gifsicle engine with the
  // Actions-tab settings; every other format -> FFmpeg argv from the core
  // builder. Returns false with a reason when the job cannot be run at all.
  bool configureEncoder(const gs::Settings& one, const QString& input, const QString& partial,
                        gs::OutputFormat format, QString* error);
  void refreshQueueLabel();
  // ---- Slice C: the Input grid ------------------------------------------
  // Facts are read once per path (GifFacts.h, Qt-free) and cached; the grid,
  // the selection strip and the sort header all read the same numbers, so the
  // caption, the tooltip and the sort key cannot disagree about a file.
  gs::GifFacts factsFor(const QString& path);
  QPixmap thumbnailFor(const QString& path);
  void applyQueueItemFacts(QListWidgetItem* item, const QString& path);
  void updateQueueDetails();
  // Queue sort keys, in the reference's column order. QueueOrder is the
  // historical behaviour (insertion order = merge order) and stays the default.
  enum QueueSort {
    SortQueueOrder = 0,
    SortName,
    SortSize,
    SortModified,
    SortFrames,
    SortDuration
  };
  void updateSortButtonLabels();
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
  // ---- Slice C widgets (Input grid) ----
  QPushButton* addFolderButton_ = nullptr;
  QLineEdit* queueFilterEdit_ = nullptr;
  QLabel* queueSelectionDetails_ = nullptr;
  QToolButton* previewToggleButton_ = nullptr;  // Actions splitter handle
  // Output > Format (owner, 2026-10-24): the reference's combo + per-format
  // Settings..., with the entries this build cannot encode disabled.
  QComboBox* presetCombo_ = nullptr;
  QComboBox* formatCombo_ = nullptr;
  QPushButton* formatSettingsButton_ = nullptr;
  QLabel* formatNoteLabel_ = nullptr;
  QLabel* ffmpegStatusLabel_ = nullptr;
  QVector<QToolButton*> sortButtons_;
  QHash<QString, gs::GifFacts> factsCache_;
  QHash<QString, QPixmap> thumbCache_;
  int queueSortKey_ = SortQueueOrder;
  bool queueSortAscending_ = true;
  // The order the user established by adding and by Move Up/Down, kept apart
  // from the sorted VIEW order. Without this, "Queue order" would only mean
  // "stop sorting from here on" and could never restore what a sort displaced —
  // and queue order is the Merge order, so that would be a semantic lie.
  QStringList manualOrder_;
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
  // Status tab (W-32, slice A)
  QTableWidget* resultTable_ = nullptr;
  QLabel* resultHeaderLabel_ = nullptr;
  QLabel* resultSummaryLabel_ = nullptr;
  QVector<qint64> resultBeforeBytes_;   // per row, captured when the row began
  int resultActiveRow_ = -1;            // row a running engine belongs to

  // Engine / run state
  QString enginePath_;
  // N-62: what the locator found (kept, not just the path, because "not found"
  // and "found on PATH" are different stories the UI has to tell).
  gs::FfmpegLocation ffmpegLocation_;
  QString ffmpegPath_;
  bool ffmpegLocated_ = false;  // has the search run at least once this session
  // The format the IN-FLIGHT run encodes. Batch continuation jobs and the
  // post-run verifier read this, never the live combo, for the same reason
  // batchSettings_ exists (U-58): a plan must not mutate under the run.
  gs::OutputFormat activeFormat_ = gs::OutputFormat::Gif;
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
  int actionsTabIndex_ = -1;  // index of the Actions tab (slice D: title carries the count)
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
