// Gifscythe main window - the GUI shell over the gifsicle engine.
//
// Layout (XNConvert-style flow):
//   Tab 1 "Input"   — animation queue (add / folder / drag-drop / remove /
//                     clear / move up-down / sort / filter)
//   Tab 2 "Actions" — SettingsPanel + on-demand Before/After Preview splitter
//   Tab 3 "Output"  — Destination (Source folder vs Folder), Filename template
//                     with `{` token popup, Format + Settings…,
//                     Options (collision policy, keep folder structure,
//                     same-format size guard), and verified Files on disk
//   Tab 4 "Status"  — W-32 per-operation result table, command preview, log
//   Tab 5 "Settings"— engine and session file paths
//   Tab 6 "About"   — version block and plain-language guide
//   Footer          — Presets ribbon, status, progress, Stop/Cancel, Convert
//
// CORRECTED 2026-09-27. This comment used to claim the GUI "Never blocks the
// UI thread (S3-7 rule)". It does block, in five bounded places; U-12 was
// closed on measurement in S37 with T26 as its regression guard:
//     ~MainWindow()              process_->waitForFinished(2000)
//     runCommand()  batch branch process_->waitForStarted(5000)
//     runCommand() single branch process_->waitForStarted(5000)
//     cancelRun()                process_->waitForFinished(3000)
//     killPreview()              previewProcess_->waitForFinished(1000)
// What IS true: nothing blocks for the DURATION of a run. The engine runs in a
// separate QProcess and completion arrives via the finished() signal; the waits
// above bound START and CANCEL only.

#ifndef GIFSCYTHE_MAINWINDOW_H
#define GIFSCYTHE_MAINWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QPixmap>
#include <QProcess>
#include <QStringList>
#include <QVector>

#include <ctime>
#include <map>
#include <ostream>
#include <string>

class QAction;
class QCheckBox;
class QComboBox;
class QEvent;
class QFileInfo;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;
class QToolButton;
class DropListWidget;
class PreviewPanel;
class SettingsPanel;

#include "core/Collision.h"
#include "core/ExplodeVerify.h"
#include "core/FfmpegCommand.h"
#include "core/FfmpegLocator.h"
#include "core/GifFacts.h"
#include "core/GifsicleCommand.h"
#include "core/GifsicleSettings.h"
#include "core/NameTemplate.h"
#include "core/OutputPlan.h"
#include "core/OutputVerify.h"

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  struct FfmpegFormatPrefs {
    int webpQuality = -1;  // -1 = codec default (75); else 0..100 (higher = better)
    int h264Quality = -1;  // -1 = codec default (CRF 20); else 0..100 (higher = better)
    int h265Quality = -1;  // -1 = codec default (CRF 24); else 0..100 (higher = better)
    int vp9Quality = -1;   // -1 = codec default (CRF 32); else 0..100 (higher = better)
    int av1Quality = -1;   // -1 = codec default (CRF 32); else 0..100 (higher = better)
    int fps = 0;           // 0 = keep source timing (-1 in FfmpegEncodeRequest); else 1..120
    bool loop = true;      // true -> animate forever (loop=0), false -> play once (loop=1)
  };

  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  gs::Settings currentSettings() const;

  // ---- Folder admission + presets + Output state (public for offscreen tests) ----
  void appendInputFolder(const QString& rootDir);
  bool savePresetByName(const QString& rawName);
  void setCollisionPolicy(gs::CollisionPolicy policy);
  gs::CollisionPolicy collisionPolicy() const { return collisionPolicy_; }
  FfmpegFormatPrefs ffmpegFormatPrefs() const { return ffmpegPrefs_; }
  void setFfmpegFormatPrefs(const FfmpegFormatPrefs& prefs);
  void openFormatSettings();

  // ---- Session persistence (S7 / OD-26) ----
  QString sessionFilePath() const;
  void loadSessionState();
  bool saveSessionState();

 protected:
  void closeEvent(QCloseEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private slots:
  void chooseInputs();
  void chooseInputFolder();
  void removeSelected();
  void clearQueue();
  void applyQueueFilter();
  void removeFilteredItems(bool matching);
  void removeQueueRows(QList<int> rows);
  void applyQueueSort();
  void refreshCommand();
  void runCommand();
  void cancelRun();
  void onProcessFinished(int exitCode, QProcess::ExitStatus status);
  void onProcessError(QProcess::ProcessError error);
  void onFilesDropped(const QStringList& files);

 private:
  void discardPendingPartial();
  QWidget* buildInputTab();
  QWidget* buildActionsTab();
  QWidget* buildOutputTab();
  QWidget* buildStatusTab();
  QWidget* buildSettingsTab();
  QWidget* buildAboutTab();
  void buildBottomBar(class QWidget* central, class QVBoxLayout* root);

  // ---- Status tab result rows (W-32, slice A) ----
  void beginResultRows(const QStringList& files);
  void beginSingleResultRow(const QString& label, qint64 beforeBytes);
  void setResultState(int row, const QString& state, const QString& detail,
                      qint64 afterBytes = -1);
  void markRemainingSkipped(int fromRow);
  void refreshResultSummary();
  void refreshTabTitles();

  // ---- Output / Destination / Naming / Collision helpers ----
  void chooseBatchDir();
  void openOutputFolder();
  void updateStatus(const QString& message);
  QString presetDir() const;
  void refreshPresetList();
  void savePresetAs();
  void loadSelectedPreset();
  void deleteSelectedPreset();
  void serializeGuiState(std::ostream& oss, const char* headerComment) const;
  void applyGuiState(const gs::Settings& loaded,
                     const std::map<std::string, std::string>& guiKeys);

  void appendInputs(const QStringList& files);
  void setBusy(bool busy);
  void moveCurrent(int delta);
  bool destinationIncomplete() const;
  QString targetDirForInput(const QString& input) const;
  gs::NameContext templateContextFor(const QString& input, int index,
                                     gs::OutputFormat fmt,
                                     std::time_t now) const;
  QString defaultOutputFor(const QString& input) const;
  QString singleTargetOutput() const;
  QString explodeTargetPrefix() const;
  QStringList plannedBatchTargets() const;
  gs::OutputPlan planBatch() const;
  QString renderedOutputName(const QFileInfo& input, int index = 0) const;
  bool templateIsConstant() const;
  void updateTokenPopup();
  void acceptSelectedToken();
  void refreshTemplateNote();
  void refreshCollisionButton();
  void refreshSizeGuardState();

  struct AskCollisionOutcome {
    bool cancelled = false;
    gs::CollisionPolicy policy = gs::CollisionPolicy::Skip;
    bool applyToAll = false;
  };
  AskCollisionOutcome promptCollisionAsk(const QString& sourcePath,
                                         const QString& targetPath,
                                         bool allowApplyAll);

  bool ensureEngine();
  // ---- N-62 / N-65: the FFmpeg sidecar & per-format parameters ----
  gs::OutputFormat selectedOutputFormat() const;
  void refreshFfmpegStatus(bool relocate = false);
  void applyOutputFormatToken(const QString& token);
  gs::FfmpegEncodeRequest buildFfmpegRequest(const QString& input,
                                             const QString& partial,
                                             gs::OutputFormat format,
                                             const FfmpegFormatPrefs& prefs) const;
  bool configureEncoder(const gs::Settings& one, const QString& input, const QString& partial,
                        gs::OutputFormat format, QString* error);
  void refreshQueueLabel();
  // ---- Slice C: the Input grid ----
  gs::GifFacts factsFor(const QString& path);
  QPixmap thumbnailFor(const QString& path);
  void applyQueueItemFacts(QListWidgetItem* item, const QString& path);
  void updateQueueDetails();
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
  void markPreviewStale();
  void startPreview();
  void onSelectionChanged();
  void killPreview();
  void invalidatePreview();
  void cleanupPreviewFiles(const QString& keep = QString());
  QString selectedInput() const;

  // Widgets
  QTabWidget* tabs_ = nullptr;
  DropListWidget* inputList_ = nullptr;
  QLabel* queueCountLabel_ = nullptr;
  QPushButton* addFolderButton_ = nullptr;
  QLineEdit* queueFilterEdit_ = nullptr;
  QToolButton* queueFilterMenuButton_ = nullptr;
  QMenu* queueFilterMenu_ = nullptr;
  QAction* deleteFilteredAction_ = nullptr;
  QAction* deleteNonFilteredAction_ = nullptr;
  QToolButton* thumbnailSizeButton_ = nullptr;
  QLabel* queueSelectionDetails_ = nullptr;
  QToolButton* previewToggleButton_ = nullptr;
  QComboBox* presetCombo_ = nullptr;
  QComboBox* formatCombo_ = nullptr;
  QPushButton* formatSettingsButton_ = nullptr;
  QLabel* formatNoteLabel_ = nullptr;
  QLabel* ffmpegStatusLabel_ = nullptr;
  QVector<QToolButton*> sortButtons_;
  QHash<QString, gs::GifFacts> factsCache_;
  QHash<QString, QPixmap> thumbCache_;
  // N-63: clean input path -> clean root folder path when added via Add folder…
  QHash<QString, QString> inputRoots_;
  int queueSortKey_ = SortQueueOrder;
  bool queueSortAscending_ = true;
  QStringList manualOrder_;
  SettingsPanel* settingsPanel_ = nullptr;

  // Output tab controls (N-61, N-63, N-64, N-65)
  QRadioButton* destSourceRadio_ = nullptr;
  QRadioButton* destFolderRadio_ = nullptr;
  QLineEdit* batchDirEdit_ = nullptr;
  QPushButton* batchDirBrowse_ = nullptr;
  bool syncingDestControls_ = false;

  QLineEdit* nameTemplateEdit_ = nullptr;
  QToolButton* tokenMenuButton_ = nullptr;
  QListWidget* tokenPopup_ = nullptr;
  // Old preset/session files may contain this; new UI defaults to 1.
  int legacyStartIndex_ = 1;
  QLabel* templateNoteLabel_ = nullptr;

  QPushButton* collisionButton_ = nullptr;
  QMenu* collisionMenu_ = nullptr;
  bool collisionMenuOpen_ = false;
  gs::CollisionPolicy collisionPolicy_ = gs::CollisionPolicy::Ask;
  QCheckBox* keepStructureCheck_ = nullptr;
  QCheckBox* discardLargerCheck_ = nullptr;

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
  QVector<qint64> resultBeforeBytes_;
  int resultActiveRow_ = -1;

  // Engine / run state
  QString enginePath_;
  gs::FfmpegLocation ffmpegLocation_;
  QString ffmpegPath_;
  bool ffmpegLocated_ = false;
  gs::OutputFormat activeFormat_ = gs::OutputFormat::Gif;
  FfmpegFormatPrefs ffmpegPrefs_;
  FfmpegFormatPrefs activeFfmpegPrefs_;
  bool activeDiscardLarger_ = false;
  QString activeDestRoot_;  // non-empty when Folder destination was active at run start
  bool activeRootExists_ = false;
  QStringList inputs_;
  QProcess* process_ = nullptr;
  bool busy_ = false;
  bool cancelling_ = false;
  QString pendingOutput_;
  QString pendingPartial_;
  gs::OutputSnapshot partialSnapshot_;
  QStringList batchTargets_;
  QVector<bool> batchSkipMask_;
  QVector<bool> batchDiscardedMask_;
  QStringList lastOutputPaths_;
  QString processStderr_;
  QString processStdout_;
  int batchIndex_ = -1;
  int actionsTabIndex_ = -1;
  QStringList batchQueue_;
  gs::Settings batchSettings_;
  gs::Mode batchMode_ = gs::Mode::Batch;
  std::vector<gs::ExplodeFileState> explodeSnapshot_;

  // Preview state
  QProcess* previewProcess_ = nullptr;
  int previewSeq_ = 0;
  QString previewDir_;
};

#endif  // GIFSCYTHE_MAINWINDOW_H
