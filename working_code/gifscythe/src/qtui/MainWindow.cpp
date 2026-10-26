#include "MainWindow.h"
#include "DropListWidget.h"
#include "PreviewPanel.h"
#include "SettingsPanel.h"

#include "core/Collision.h"
#include "core/EngineLocator.h"
#include "core/FfmpegCommand.h"
#include "core/FfmpegLocator.h"
#include "core/InputAdmission.h"
#include "core/NameTemplate.h"
#include "core/OutputName.h"
#include "core/OutputPlan.h"
#include "core/OutputVerify.h"
#include "core/SettingsIO.h"
#include "core/Validate.h"
#include "core/version.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QBrush>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QImageReader>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <ctime>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

// Gifsicle is a console executable, but the desktop app is not a console
// workflow. Without this modifier Windows creates a second console window for
// every optimization/preview process, which looks like Gifscythe is opening a
// terminal and is especially distracting for large files. Keep the subprocess
// fully observable through QProcess while hiding its console window.
void configureHeadless(QProcess* process) {
  if (!process) return;
  process->setProcessChannelMode(QProcess::SeparateChannels);
#ifdef Q_OS_WIN
  process->setCreateProcessArgumentsModifier(
      [](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
        args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow = SW_HIDE;
      });
#endif
}

// The Command pane must show the command that will ACTUALLY run. FFmpeg jobs
// are a different program with a different argv (N-62), so the pane would be a
// work of fiction if it kept printing the gifsicle line.
QString shellishJoin(const QString& program, const std::vector<std::string>& args) {
  QStringList parts;
  if (program.contains(QLatin1Char(' ')))
    parts << QStringLiteral("\"%1\"").arg(program);
  else
    parts << program;
  // A string literal rather than a character literal: the doc-gate's comment
  // masker does not understand chars, and a bare quote inside one desynchronises
  // its line map (found the hard way - it made five old findings look new).
  const QString kQuote = QStringLiteral("\"");
  for (const auto& a : args) {
    const QString arg = QString::fromStdString(a);
    if (arg.isEmpty() || arg.contains(QLatin1Char(' ')) || arg.contains(kQuote))
      parts << QStringLiteral("\"%1\"").arg(arg);
    else
      parts << arg;
  }
  return parts.join(QLatin1Char(' '));
}

QString humanSize(qint64 bytes) {
  if (bytes >= 1024 * 1024)
    return QStringLiteral("%1 MB").arg(double(bytes) / (1024.0 * 1024.0), 0, 'f', 2);
  if (bytes >= 1024)
    return QStringLiteral("%1 KB").arg(double(bytes) / 1024.0, 0, 'f', 1);
  return QStringLiteral("%1 B").arg(bytes);
}

// Slice C: a duration a person can read. Deliberately explicit for the three
// states the grid must distinguish, because they mean different things and a
// single "0 s" would blur them:
//   0 ms            -> "no delay"   (multi-frame GIF with zero delays: fast, legal)
//   under a minute  -> "2.40 s"
//   over a minute   -> "1m 05s"
QString formatDuration(qint64 ms) {
  if (ms <= 0) return QStringLiteral("no delay");
  if (ms < 60000) return QStringLiteral("%1 s").arg(double(ms) / 1000.0, 0, 'f', 2);
  const qint64 total_seconds = (ms + 500) / 1000;
  return QStringLiteral("%1m %2s")
      .arg(total_seconds / 60)
      .arg(total_seconds % 60, 2, 10, QLatin1Char('0'));
}

// Default output filename template (N-61 / OD-23, measured from XnConvert 1.118.0).
// {name} remains supported in NameTemplate.h as the pre-2026 alias so earlier
// session and preset files still render as intended.
const char* kDefaultNameTemplate = gs::kDefaultNameTemplate;

QString cleanDirPath(const QString& raw) {
  const QString trimmed = raw.trimmed();
  if (trimmed.isEmpty()) return {};
  return QDir::cleanPath(QFileInfo(trimmed).absoluteFilePath());
}

QString cleanFilePath(const QString& raw) {
  const QString trimmed = raw.trimmed();
  if (trimmed.isEmpty()) return {};
  return QDir::cleanPath(QFileInfo(trimmed).absoluteFilePath());
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("Gifscythe %1").arg(QStringLiteral(GS_VERSION)));
  resize(1180, 720);

  enginePath_ = QString::fromStdString(
      gs::locate_engine(QCoreApplication::applicationFilePath().toStdString()));

  process_ = new QProcess(this);
  process_->setObjectName(QStringLiteral("engineProcess"));
  configureHeadless(process_);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &MainWindow::onProcessFinished);
  connect(process_, &QProcess::errorOccurred, this, &MainWindow::onProcessError);
  connect(process_, &QProcess::readyReadStandardError, this,
          &MainWindow::captureProcessOutput);
  connect(process_, &QProcess::readyReadStandardOutput, this,
          &MainWindow::captureProcessOutput);

  // previewProcess_ is created fresh per preview run (see startPreview) so a
  // killed stale run can never be misread as the newest run's completion.

  previewTimer_ = new QTimer(this);
  previewTimer_->setSingleShot(true);
  previewTimer_->setInterval(1200);  // debounce: never re-encode per keystroke
  connect(previewTimer_, &QTimer::timeout, this, &MainWindow::startPreview);

  previewDir_ = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                    .filePath(QStringLiteral("gifscythe-preview-%1")
                                  .arg(QCoreApplication::applicationPid()));
  QDir().mkpath(previewDir_);

  // ---- Menu ----
  auto* openAct = new QAction(QStringLiteral("&Open GIF files..."), this);
  openAct->setShortcut(QKeySequence::Open);
  connect(openAct, &QAction::triggered, this, &MainWindow::chooseInputs);
  auto* quitAct = new QAction(QStringLiteral("&Quit"), this);
  quitAct->setShortcut(QKeySequence::Quit);
  connect(quitAct, &QAction::triggered, this, &QWidget::close);
  auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
  fileMenu->addAction(openAct);
  fileMenu->addSeparator();
  fileMenu->addAction(quitAct);

  // ---- Central layout ----
  auto* central = new QWidget(this);
  auto* root = new QVBoxLayout(central);

  // Owner, 2026-10-24: the reference keeps the Preview INSIDE the Actions tab
  // (docs/planning/reference/com.xnview.XnConvert-461adbcd7cdb0e2a7e01b211c63fb92c.png:
  // action stack on the left, "Preview" pane on the right with Before/After).
  // It used to be a sibling of the whole QTabWidget here, so it showed on
  // Input, Output, Status, Settings and About too - which is not the reference
  // and, on a small window, left no room for the tab content. It is created
  // here so buildActionsTab() can place it, and it lives nowhere else.
  previewPanel_ = new PreviewPanel(central);
  previewPanel_->setObjectName(QStringLiteral("previewPanel"));
  connect(previewPanel_, &PreviewPanel::previewRequested, this, [this]() {
    forcePreview_ = true;
    schedulePreview();
  });

  tabs_ = new QTabWidget(central);
  tabs_->setObjectName(QStringLiteral("mainTabs"));
  // OD-21 = a / N-55 (2026-10-22): the owner's XnConvert reference settles the
  // tab set as Input · Actions · Output · Status · Settings · About. "Guide"
  // was never part of it; its plain-language content moved into About, which
  // keeps every objectName the harness selects on (guideScroll/guideIntro/
  // guideDescription). Input carries its file count in the title, like the
  // reference ("Input: 10 file(s)"). See docs/planning/XN_CONVERT_REFERENCE.md.
  tabs_->addTab(buildInputTab(), QStringLiteral("Input: 0 file(s)"));
  actionsTabIndex_ = tabs_->addTab(buildActionsTab(), QStringLiteral("Actions"));
  tabs_->addTab(buildOutputTab(), QStringLiteral("Output"));
  tabs_->addTab(buildStatusTab(), QStringLiteral("Status"));
  tabs_->addTab(buildSettingsTab(), QStringLiteral("Settings"));
  tabs_->addTab(buildAboutTab(), QStringLiteral("About"));
  root->addWidget(tabs_, 1);

  buildBottomBar(central, root);
  setCentralWidget(central);

  // ---- Wiring: everything that changes settings refreshes the live pane,
  //      the output summary, and (debounced) the preview. ----
  connect(settingsPanel_, &SettingsPanel::changed, this, [this]() {
    forcePreview_ = false;
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
    schedulePreview();
  });
  // Slice D: the Actions tab carries "Actions [enabled/total]" in its title, the
  // way the reference does, and adding/removing/disabling a card DOES change the
  // generated command (a card that is out or off contributes defaults), so this
  // signal refreshes the same three surfaces changed() does - plus the title.
  connect(settingsPanel_, &SettingsPanel::actionCountsChanged, this, [this]() {
    forcePreview_ = false;
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
    schedulePreview();
    refreshTabTitles();
  });
  connect(batchDirEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
    if (!syncingDestControls_) {
      syncingDestControls_ = true;
      const QString trimmed = text.trimmed();
      if (!trimmed.isEmpty() && destFolderRadio_ && !destFolderRadio_->isChecked()) {
        destFolderRadio_->setChecked(true);
      } else if (trimmed.isEmpty() && destSourceRadio_ && !batchDirEdit_->hasFocus()) {
        destSourceRadio_->setChecked(true);
      }
      syncingDestControls_ = false;
    }
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
  });
  connect(nameTemplateEdit_, &QLineEdit::textChanged, this, [this]() {
    lastOutputPaths_.clear();
    refreshTemplateNote();
    refreshCommand();
    refreshOutputSummary();
  });
  connect(nameTemplateEdit_, &QLineEdit::textEdited, this, [this]() {
    updateTokenPopup();
  });
  connect(nameTemplateEdit_, &QLineEdit::cursorPositionChanged, this, [this](int, int) {
    if (tokenPopup_ && tokenPopup_->isVisible()) updateTokenPopup();
  });
  if (startIndexSpin_) {
    connect(startIndexSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
      lastOutputPaths_.clear();
      refreshTemplateNote();
      refreshCommand();
      refreshOutputSummary();
    });
  }
  connect(inputList_, &DropListWidget::filesDropped, this, &MainWindow::onFilesDropped);
  connect(inputList_, &QListWidget::itemSelectionChanged, this,
          &MainWindow::onSelectionChanged);
  connect(inputList_->model(), &QAbstractItemModel::rowsInserted, this,
          &MainWindow::refreshCommand);
  connect(inputList_->model(), &QAbstractItemModel::rowsRemoved, this,
          &MainWindow::refreshCommand);

  ensureEngine();
  loadSessionState();  // S7: restore the previous session (no-op on first launch)
  refreshCommand();
  refreshOutputSummary();
  refreshQueueLabel();
  updateQueueDetails();
}

MainWindow::~MainWindow() {
  killPreview();
  if (process_ && process_->state() != QProcess::NotRunning) {
    process_->kill();
    process_->waitForFinished(2000);
  }
  // Release the preview movie file handles BEFORE sweeping the temp dir:
  // on Windows a live QMovie delete-locks its GIF (S10, CI-windows T20).
  if (previewPanel_) previewPanel_->releaseMovies();
  QDir(previewDir_).removeRecursively();
}

QWidget* MainWindow::buildInputTab() {
  // Slice C (2026-10-24): the Input tab now follows the reference's shape —
  // toolbar, then a THUMBNAIL GRID, then the counts — instead of a flat text
  // list. Two things are deliberately different from the reference, both from
  // docs/planning/XN_CONVERT_PROGRAM_STUDY.md §11.1:
  //   * the caption and the tooltip carry ANIMATION facts (frames, duration,
  //     fps, loop), because XnConvert's Input tab has no such column anywhere;
  //   * sorting is a header of buttons over the real queue, not a table header
  //     over a proxy model, because Merge concatenates in queue order and a
  //     sort that the engine ignored would be a lie about the output.
  auto* page = new QWidget(tabs_);
  auto* left = new QVBoxLayout(page);

  // ---- toolbar (reference: Add files / Add folder / Remove / Remove all) ----
  auto* queueBtns = new QHBoxLayout();
  auto* addButton = new QPushButton(QStringLiteral("Add files…"), page);
  addButton->setObjectName(QStringLiteral("addButton"));
  addButton->setToolTip(QStringLiteral("Queue GIF files (drop them on the grid below instead, if you prefer)"));
  connect(addButton, &QPushButton::clicked, this, &MainWindow::chooseInputs);
  addFolderButton_ = new QPushButton(QStringLiteral("Add folder…"), page);
  addFolderButton_->setObjectName(QStringLiteral("addFolderButton"));
  addFolderButton_->setToolTip(
      QStringLiteral("Queue every GIF in a folder and its sub-folders (sorted by relative path).\n"
                     "Each file passes the same GIF header admission rule as a drop, and its "
                     "relative path is remembered for Keep folder structure."));
  connect(addFolderButton_, &QPushButton::clicked, this, &MainWindow::chooseInputFolder);
  removeButton_ = new QPushButton(QStringLiteral("Remove"), page);
  removeButton_->setObjectName(QStringLiteral("removeButton"));
  connect(removeButton_, &QPushButton::clicked, this, &MainWindow::removeSelected);
  clearButton_ = new QPushButton(QStringLiteral("Remove all"), page);
  clearButton_->setObjectName(QStringLiteral("clearButton"));
  connect(clearButton_, &QPushButton::clicked, this, &MainWindow::clearQueue);
  queueBtns->addWidget(addButton);
  queueBtns->addWidget(addFolderButton_);
  queueBtns->addWidget(removeButton_);
  queueBtns->addWidget(clearButton_);
  queueBtns->addStretch();

  // ---- Quick filter (reference: a funnel + a "Quick filter" line edit) ----
  // It hides rows; it never removes them, so the queue — and the batch plan —
  // is unchanged by typing. Hidden rows keep their row index, which is what
  // keeps inputs_ index-aligned with the list.
  auto* filterLabel = new QLabel(QStringLiteral("Filter"), page);
  queueFilterEdit_ = new QLineEdit(page);
  queueFilterEdit_->setObjectName(QStringLiteral("queueFilterEdit"));
  queueFilterEdit_->setPlaceholderText(QStringLiteral("Quick filter"));
  queueFilterEdit_->setClearButtonEnabled(true);
  queueFilterEdit_->setMaximumWidth(220);
  connect(queueFilterEdit_, &QLineEdit::textChanged, this, [this]() { applyQueueFilter(); });
  queueBtns->addWidget(filterLabel);
  queueBtns->addWidget(queueFilterEdit_);
  left->addLayout(queueBtns);

  // ---- sort header (the reference's columns, animation-aware) ----
  auto* sortRow = new QHBoxLayout();
  sortRow->addWidget(new QLabel(QStringLiteral("Sort"), page));
  struct SortSpec {
    const char* label;
    int key;
    const char* object_name;
  };
  const SortSpec specs[] = {
      {"Queue order", SortQueueOrder, "sortQueueOrder"},
      {"Name", SortName, "sortName"},
      {"Size", SortSize, "sortSize"},
      {"Modified", SortModified, "sortModified"},
      {"Frames", SortFrames, "sortFrames"},
      {"Duration", SortDuration, "sortDuration"},
  };
  for (const SortSpec& spec : specs) {
    auto* button = new QToolButton(page);
    button->setObjectName(QString::fromLatin1(spec.object_name));
    button->setText(QString::fromLatin1(spec.label));
    button->setCheckable(true);
    button->setChecked(spec.key == SortQueueOrder);
    button->setAutoRaise(true);
    button->setProperty("sortKey", spec.key);
    const int key = spec.key;
    connect(button, &QToolButton::clicked, this, [this, key]() {
      if (queueSortKey_ == key) {
        // Second click on the active key flips the direction — except for
        // Queue order, which has only one direction.
        if (key != SortQueueOrder) queueSortAscending_ = !queueSortAscending_;
      } else {
        queueSortKey_ = key;
        queueSortAscending_ = true;
      }
      updateSortButtonLabels();
      applyQueueSort();
    });
    sortButtons_.append(button);
    sortRow->addWidget(button);
  }
  sortRow->addStretch();
  left->addLayout(sortRow);

  // ---- the grid ----
  inputList_ = new DropListWidget(page);
  inputList_->setObjectName(QStringLiteral("queueList"));
  inputList_->setMinimumWidth(300);
  inputList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  // IconMode + a uniform cell = the reference's grid: image on a transparent
  // background, name underneath, one file per cell. No literal colours here:
  // the palette decides (the canvas rule — no #202020-style constants, so the
  // dark theme stays possible).
  inputList_->setViewMode(QListView::IconMode);
  inputList_->setIconSize(QSize(96, 96));
  inputList_->setGridSize(QSize(132, 152));
  inputList_->setResizeMode(QListView::Adjust);
  inputList_->setMovement(QListView::Static);
  inputList_->setUniformItemSizes(true);
  inputList_->setWordWrap(true);
  inputList_->setTextElideMode(Qt::ElideMiddle);
  inputList_->setSpacing(4);
  left->addWidget(inputList_, 1);

  // The reference has one selected cell and a preview bound to it; the facts
  // strip under the grid says the same thing in words for the keyboard user
  // and for anyone who cannot read the 96-px thumbnail.
  queueSelectionDetails_ = new QLabel(page);
  queueSelectionDetails_->setObjectName(QStringLiteral("queueSelectionDetails"));
  queueSelectionDetails_->setWordWrap(true);
  left->addWidget(queueSelectionDetails_);

  queueCountLabel_ = new QLabel(QStringLiteral("0 files"), page);
  queueCountLabel_->setObjectName(QStringLiteral("queueCountLabel"));
  left->addWidget(queueCountLabel_);

  // Queue order matters for Merge (frames concatenate in queue order) — S3-9.
  auto* orderRow = new QHBoxLayout();
  orderRow->addWidget(new QLabel(QStringLiteral("Order"), page));
  moveUpButton_ = new QPushButton(QStringLiteral("Move Up"), page);
  moveUpButton_->setObjectName(QStringLiteral("moveUpButton"));
  moveUpButton_->setToolTip(QStringLiteral("Move the selected file earlier in the queue "
                                           "(Merge concatenates in this order)."));
  connect(moveUpButton_, &QPushButton::clicked, this, [this]() { moveCurrent(-1); });
  moveDownButton_ = new QPushButton(QStringLiteral("Move Down"), page);
  moveDownButton_->setObjectName(QStringLiteral("moveDownButton"));
  moveDownButton_->setToolTip(QStringLiteral("Move the selected file later in the queue."));
  connect(moveDownButton_, &QPushButton::clicked, this, [this]() { moveCurrent(1); });
  orderRow->addWidget(moveUpButton_);
  orderRow->addWidget(moveDownButton_);
  orderRow->addStretch();
  left->addLayout(orderRow);

  // The primary action is NOT here: the reference puts Convert in the footer,
  // beside Close, where it is reachable from every tab (owner, 2026-10-24).
  // Output naming also moved to the Output tab (N-61 / OD-25 = a), matching
  // docs/planning/reference/4806388_6.jpg.
  return page;
}

QWidget* MainWindow::buildActionsTab() {
  // The reference's Actions tab is TWO panes, not one: the action stack on the
  // left, the Preview on the right, and a narrow column with a small arrow
  // handle between them that collapses the preview away (the reference shows
  // it as a single ">" at the vertical middle).
  //
  // Keeping the preview here, instead of beside the whole tab widget, is what
  // makes the other five tabs full-width - and it is what the owner's
  // screenshot actually shows.
  auto* page = new QWidget(tabs_);
  auto* lay = new QVBoxLayout(page);
  lay->setContentsMargins(0, 0, 0, 0);

  auto* splitter = new QSplitter(Qt::Horizontal, page);
  splitter->setObjectName(QStringLiteral("actionsSplitter"));
  splitter->setChildrenCollapsible(false);

  settingsPanel_ = new SettingsPanel(splitter);
  settingsPanel_->setObjectName(QStringLiteral("settingsPanel"));
  splitter->addWidget(settingsPanel_);

  // ---- the collapse handle (reference: a ">" at the middle of the divider) ----
  auto* handleColumn = new QWidget(splitter);
  handleColumn->setObjectName(QStringLiteral("actionsSplitterHandle"));
  auto* handleLayout = new QVBoxLayout(handleColumn);
  handleLayout->setContentsMargins(2, 0, 2, 0);
  handleLayout->addStretch();
  previewToggleButton_ = new QToolButton(handleColumn);
  previewToggleButton_->setObjectName(QStringLiteral("previewToggleButton"));
  previewToggleButton_->setAutoRaise(true);
  previewToggleButton_->setText(QStringLiteral("◀"));  // left-pointing: preview is shown
  previewToggleButton_->setToolTip(QStringLiteral("Hide the preview pane"));
  previewToggleButton_->setCheckable(true);
  previewToggleButton_->setChecked(true);
  connect(previewToggleButton_, &QToolButton::toggled, this, [this](bool shown) {
    if (!previewPanel_) return;
    previewPanel_->setVisible(shown);
    previewToggleButton_->setText(shown ? QStringLiteral("◀") : QStringLiteral("▶"));
    previewToggleButton_->setToolTip(shown ? QStringLiteral("Hide the preview pane")
                                           : QStringLiteral("Show the preview pane"));
  });
  handleLayout->addWidget(previewToggleButton_);
  handleLayout->addStretch();
  // A handle column, not a fixed width: the reference's arrow sits in its own
  // gutter, and the splitter still lets the user drag the real divider.
  handleColumn->setMaximumWidth(24);
  splitter->addWidget(handleColumn);

  splitter->addWidget(previewPanel_);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 0);
  splitter->setStretchFactor(2, 2);
  lay->addWidget(splitter);
  return page;
}

QWidget* MainWindow::buildOutputTab() {
  auto* page = new QWidget(tabs_);
  auto* lay = new QVBoxLayout(page);

  // ---- Output destination (N-61 / OD-25 = a + N-63) ------------------------
  // Matches XnConvert's top Output group (docs/planning/reference/4806388_6.jpg):
  // Source folder vs designated Folder + path + Browse. Save-as was dropped
  // (OD-25 = a): Destination + Filename explains every mode.
  auto* box = new QGroupBox(QStringLiteral("Output"), page);
  box->setObjectName(QStringLiteral("outputGroup"));
  auto* destLay = new QVBoxLayout(box);

  auto* destGroup = new QButtonGroup(box);
  destSourceRadio_ = new QRadioButton(QStringLiteral("Source folder"), box);
  destSourceRadio_->setObjectName(QStringLiteral("destSourceRadio"));
  destSourceRadio_->setChecked(true);
  destSourceRadio_->setToolTip(
      QStringLiteral("Write each result beside its own source file"));
  destGroup->addButton(destSourceRadio_);
  destLay->addWidget(destSourceRadio_);

  auto* folderRow = new QHBoxLayout();
  destFolderRadio_ = new QRadioButton(QStringLiteral("Folder"), box);
  destFolderRadio_->setObjectName(QStringLiteral("destFolderRadio"));
  destFolderRadio_->setToolTip(
      QStringLiteral("Write results into one designated folder (sub-folders recreated when Keep folder structure is on)"));
  destGroup->addButton(destFolderRadio_);
  batchDirEdit_ = new QLineEdit(box);
  batchDirEdit_->setObjectName(QStringLiteral("batchDirEdit"));
  batchDirEdit_->setPlaceholderText(
      QStringLiteral("Choose destination folder…"));
  batchDirBrowse_ = new QPushButton(QStringLiteral("Browse…"), box);
  batchDirBrowse_->setObjectName(QStringLiteral("browseBatchDirButton"));
  connect(batchDirBrowse_, &QPushButton::clicked, this, &MainWindow::chooseBatchDir);
  folderRow->addWidget(destFolderRadio_);
  folderRow->addWidget(batchDirEdit_, 1);
  folderRow->addWidget(batchDirBrowse_);
  destLay->addLayout(folderRow);

  connect(destSourceRadio_, &QRadioButton::toggled, this, [this](bool) {
    if (syncingDestControls_) return;
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
  });
  lay->addWidget(box);

  // ---- Middle row: Filename (left) + Format (right), side by side ----------
  auto* middleRow = new QHBoxLayout();

  auto* filenameBox = new QGroupBox(QStringLiteral("Filename"), page);
  filenameBox->setObjectName(QStringLiteral("filenameGroup"));
  auto* filenameForm = new QFormLayout(filenameBox);

  auto* tmplRow = new QHBoxLayout();
  nameTemplateEdit_ = new QLineEdit(filenameBox);
  nameTemplateEdit_->setObjectName(QStringLiteral("nameTemplateEdit"));
  nameTemplateEdit_->setText(QString::fromUtf8(kDefaultNameTemplate));
  nameTemplateEdit_->setToolTip(QStringLiteral(
      "Output filename template (extension is supplied by Format).\n"
      "Type '{' for live token completion, or click ▶ to pick from all tokens.\n"
      "Duplicate rendered names in the same folder are numbered (_1, _2, …)."));
  nameTemplateEdit_->installEventFilter(this);

  tokenMenuButton_ = new QToolButton(filenameBox);
  tokenMenuButton_->setObjectName(QStringLiteral("tokenMenuButton"));
  tokenMenuButton_->setText(QStringLiteral("▶"));
  tokenMenuButton_->setToolTip(QStringLiteral("Insert a filename token"));
  auto* tokenMenu = new QMenu(tokenMenuButton_);
  tokenMenu->setObjectName(QStringLiteral("tokenMenu"));
  for (const gs::TemplateToken& info : gs::template_tokens()) {
    const QString tok = QStringLiteral("{%1}").arg(QString::fromUtf8(info.name));
    const QString label = QStringLiteral("%1  —  %2")
                              .arg(tok, QString::fromUtf8(info.summary));
    QAction* act = tokenMenu->addAction(label);
    act->setData(tok);
    connect(act, &QAction::triggered, this, [this, tok]() {
      if (!nameTemplateEdit_ || busy_) return;
      nameTemplateEdit_->insert(tok);
      nameTemplateEdit_->setFocus();
    });
  }
  connect(tokenMenuButton_, &QToolButton::clicked, this, [this, tokenMenu]() {
    if (!tokenMenuButton_ || busy_) return;
    tokenMenu->popup(tokenMenuButton_->mapToGlobal(
        QPoint(0, tokenMenuButton_->height())));
  });
  tmplRow->addWidget(nameTemplateEdit_, 1);
  tmplRow->addWidget(tokenMenuButton_);
  filenameForm->addRow(QStringLiteral("Filename"), tmplRow);

  // `{`-triggered scrolling completion popup (N-61). WindowDoesNotAcceptFocus
  // keeps keystrokes flowing into nameTemplateEdit_ so typing `{fi` filters
  // live on Windows, Linux, and offscreen CI alike.
  tokenPopup_ = new QListWidget(this);
  tokenPopup_->setObjectName(QStringLiteral("tokenPopup"));
  tokenPopup_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                              Qt::WindowDoesNotAcceptFocus);
  tokenPopup_->setFocusPolicy(Qt::NoFocus);
  tokenPopup_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  tokenPopup_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tokenPopup_->setMaximumHeight(180);
  tokenPopup_->hide();
  connect(tokenPopup_, &QListWidget::itemClicked, this, [this](QListWidgetItem*) {
    acceptSelectedToken();
  });

  startIndexSpin_ = new QSpinBox(filenameBox);
  startIndexSpin_->setObjectName(QStringLiteral("startIndexSpin"));
  startIndexSpin_->setRange(0, 999999);
  startIndexSpin_->setValue(1);
  startIndexSpin_->setMaximumWidth(120);
  startIndexSpin_->setToolTip(
      QStringLiteral("Starting number for the {File Index} token"));
  filenameForm->addRow(QStringLiteral("Start index"), startIndexSpin_);

  templateNoteLabel_ = new QLabel(filenameBox);
  templateNoteLabel_->setObjectName(QStringLiteral("templateNoteLabel"));
  templateNoteLabel_->setWordWrap(true);
  filenameForm->addRow(QString(), templateNoteLabel_);
  middleRow->addWidget(filenameBox, 3);

  // ---- Format (the reference's Output > Format + Settings...) --------------
  auto* formatBox = new QGroupBox(QStringLiteral("Format"), page);
  formatBox->setObjectName(QStringLiteral("formatGroup"));
  auto* formatForm = new QFormLayout(formatBox);
  formatCombo_ = new QComboBox(formatBox);
  formatCombo_->setObjectName(QStringLiteral("formatCombo"));
  struct FormatSpec {
    const char* label;
    bool available;
    const char* why;
  };
  const FormatSpec formats[] = {
      {"Same as original (GIF)", true, nullptr},
      {"GIF", true, nullptr},
      {"APNG", false, "needs FFmpeg - not found (Settings... looks again)"},
      {"WebP (animated)", false, "needs FFmpeg - not found (Settings... looks again)"},
      {"MP4 - H.264", false, "needs FFmpeg - not found (Settings... looks again)"},
      {"MP4 - H.265/HEVC", false, "needs FFmpeg - not found (Settings... looks again)"},
      {"WebM - VP9", false, "needs FFmpeg - not found (Settings... looks again)"},
      {"WebM - AV1", false, "needs FFmpeg - not found (Settings... looks again)"},
  };
  const char* const formatTokens[] = {"gif", "gif", "apng", "webp",
                                      "mp4-h264", "mp4-h265", "webm-vp9", "webm-av1"};
  static_assert(sizeof(formats) / sizeof(formats[0]) == sizeof(formatTokens) / sizeof(formatTokens[0]),
                "every format entry needs a token");
  auto* model = qobject_cast<QStandardItemModel*>(formatCombo_->model());
  for (int i = 0; i < static_cast<int>(sizeof(formats) / sizeof(formats[0])); ++i) {
    const FormatSpec& spec = formats[i];
    formatCombo_->addItem(QString::fromLatin1(spec.label));
    const int row = formatCombo_->count() - 1;
    formatCombo_->setItemData(row, QString::fromLatin1(formatTokens[i]), Qt::UserRole);
    if (!spec.available) {
      if (model) {
        if (QStandardItem* item = model->item(row)) {
          item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
      }
      formatCombo_->setItemData(row, QString::fromLatin1(spec.why), Qt::ToolTipRole);
    }
  }
  formatCombo_->setToolTip(QStringLiteral(
      "Output container/codec.\n"
      "GIF is produced by the bundled gifsicle engine.\n"
      "Everything else is produced by FFmpeg: the copy shipped beside this program if there "
      "is one, otherwise one found on this computer."));
  formatSettingsButton_ = new QPushButton(QStringLiteral("Settings…"), formatBox);
  formatSettingsButton_->setObjectName(QStringLiteral("formatSettingsButton"));
  formatSettingsButton_->setToolTip(
      QStringLiteral("Per-format encoder settings and FFmpeg location (Format > Settings…)"));
  connect(formatSettingsButton_, &QPushButton::clicked, this, &MainWindow::openFormatSettings);
  auto* fmtRow = new QHBoxLayout();
  fmtRow->addWidget(formatCombo_, 1);
  fmtRow->addWidget(formatSettingsButton_);
  formatForm->addRow(QStringLiteral("Output format"), fmtRow);
  formatNoteLabel_ = new QLabel(formatBox);
  formatNoteLabel_->setObjectName(QStringLiteral("formatNote"));
  formatNoteLabel_->setWordWrap(true);
  formatForm->addRow(QString(), formatNoteLabel_);
  ffmpegStatusLabel_ = new QLabel(formatBox);
  ffmpegStatusLabel_->setObjectName(QStringLiteral("ffmpegStatus"));
  ffmpegStatusLabel_->setWordWrap(true);
  ffmpegStatusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  formatForm->addRow(QString(), ffmpegStatusLabel_);
  middleRow->addWidget(formatBox, 2);
  lay->addLayout(middleRow);

  // ---- Options (N-63 + N-64: existing file square, keep structure, size guard)
  auto* optionsBox = new QGroupBox(QStringLiteral("Options"), page);
  optionsBox->setObjectName(QStringLiteral("outputOptionsGroup"));
  auto* optionsForm = new QFormLayout(optionsBox);

  // Owner (2026-10-26): a single compact clickable square sized to the longest
  // option name plus a ⇅ glyph, with NO dropdown tail; displays the current
  // answer ("Ask each time" by default) and reveals the other options on click.
  // Deliberately NOT calling setMenu() so Qt's style paints zero menu-indicator
  // arrow/tail while keeping native palette rendering (QL006 = 0).
  collisionButton_ = new QPushButton(optionsBox);
  collisionButton_->setObjectName(QStringLiteral("collisionButton"));
  collisionButton_->setAutoDefault(false);
  collisionButton_->setDefault(false);
  collisionButton_->setToolTip(QStringLiteral(
      "What to do when an output file already exists on disk.\n"
      "Click to switch between Ask each time, Rename (numbered), Overwrite, and Skip."));
  collisionMenu_ = new QMenu(collisionButton_);
  collisionMenu_->setObjectName(QStringLiteral("collisionMenu"));
  auto* policyGroup = new QActionGroup(collisionMenu_);
  policyGroup->setExclusive(true);
  const gs::CollisionPolicy kPolicies[] = {
      gs::CollisionPolicy::Ask,
      gs::CollisionPolicy::Rename,
      gs::CollisionPolicy::Overwrite,
      gs::CollisionPolicy::Skip,
  };
  const QFontMetrics fm(collisionButton_->font());
  int maxPolicyWidth = 0;
  for (gs::CollisionPolicy pol : kPolicies) {
    const QString label = QString::fromUtf8(gs::collision_policy_name(pol));
    const QString token = QString::fromUtf8(gs::collision_policy_token(pol));
    const QString display = label + QStringLiteral("  \u21c5");
    maxPolicyWidth = std::max(maxPolicyWidth, fm.horizontalAdvance(display));
    QAction* act = collisionMenu_->addAction(label);
    act->setObjectName(QStringLiteral("collisionAction_") + token);
    act->setCheckable(true);
    act->setData(token);
    policyGroup->addAction(act);
    connect(act, &QAction::triggered, this, [this, pol]() {
      setCollisionPolicy(pol);
    });
  }
  collisionButton_->setFixedWidth(maxPolicyWidth + 28);
  connect(collisionButton_, &QPushButton::clicked, this, [this]() {
    if (!collisionButton_ || !collisionMenu_ || busy_) return;
    collisionMenu_->popup(
        collisionButton_->mapToGlobal(QPoint(0, collisionButton_->height())));
  });
  optionsForm->addRow(QStringLiteral("If an output file already exists"),
                      collisionButton_);

  keepStructureCheck_ = new QCheckBox(
      QStringLiteral("Keep folder structure (recreate sub-folders inside designated Folder)"),
      optionsBox);
  keepStructureCheck_->setObjectName(QStringLiteral("keepStructureCheck"));
  keepStructureCheck_->setChecked(true);  // Owner (2026-10-26): default ON
  keepStructureCheck_->setToolTip(QStringLiteral(
      "When enabled and Output is set to Folder, files added via Add folder… recreate "
      "their relative sub-folders inside the destination folder (e.g. in/a/b/c.gif → out/a/b/c_result.gif).\n"
      "Standalone files added individually stay flat."));
  connect(keepStructureCheck_, &QCheckBox::toggled, this, [this](bool) {
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
  });
  optionsForm->addRow(QString(), keepStructureCheck_);

  discardLargerCheck_ = new QCheckBox(
      QStringLiteral("Discard result if larger than original (same format only)"),
      optionsBox);
  discardLargerCheck_->setObjectName(QStringLiteral("discardLargerCheck"));
  discardLargerCheck_->setChecked(false);  // Owner (2026-10-26): default OFF
  discardLargerCheck_->setToolTip(QStringLiteral(
      "Applies only when output format matches source format (GIF → GIF).\n"
      "Disabled for APNG, WebP, MP4, and WebM because those formats serve different purposes."));
  optionsForm->addRow(QString(), discardLargerCheck_);
  lay->addWidget(optionsBox);

  // Locate the encoder NOW, before the user can pick a format, and keep the
  // combo + size-guard state + filename preview in step when selection changes.
  connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) {
            refreshFfmpegStatus(false);
            refreshSizeGuardState();
            refreshTemplateNote();
            lastOutputPaths_.clear();
            refreshCommand();
            refreshOutputSummary();
          });
  refreshCollisionButton();
  refreshFfmpegStatus(true);  // the first search of the session
  refreshSizeGuardState();
  refreshTemplateNote();

  outputSummaryLabel_ = new QLabel(page);
  outputSummaryLabel_->setObjectName(QStringLiteral("outputSummary"));
  outputSummaryLabel_->setWordWrap(true);
  lay->addWidget(outputSummaryLabel_);

  auto* filesBox = new QGroupBox(QStringLiteral("Files on disk"), page);
  filesBox->setObjectName(QStringLiteral("outputFilesGroup"));
  auto* filesLay = new QVBoxLayout(filesBox);
  auto* filesHint = new QLabel(QStringLiteral(
      "This is the real destination, not the temporary preview. Double-click a file to open it, "
      "or select one and choose Open file."), filesBox);
  filesHint->setWordWrap(true);
  filesHint->setObjectName(QStringLiteral("outputFilesHint"));
  filesLay->addWidget(filesHint);
  outputFiles_ = new QListWidget(filesBox);
  outputFiles_->setObjectName(QStringLiteral("outputFilesList"));
  outputFiles_->setMinimumHeight(110);
  outputFiles_->setToolTip(QStringLiteral("Verified output files and their full paths"));
  connect(outputFiles_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem*) { openSelectedOutput(); });
  filesLay->addWidget(outputFiles_, 1);
  auto* fileButtons = new QHBoxLayout();
  openDirButton_ = new QPushButton(QStringLiteral("Open output folder"), filesBox);
  openDirButton_->setObjectName(QStringLiteral("openDirButton"));
  connect(openDirButton_, &QPushButton::clicked, this, &MainWindow::openOutputFolder);
  openSelectedOutputButton_ = new QPushButton(QStringLiteral("Open selected file"), filesBox);
  openSelectedOutputButton_->setObjectName(QStringLiteral("openSelectedOutputButton"));
  connect(openSelectedOutputButton_, &QPushButton::clicked, this,
          &MainWindow::openSelectedOutput);
  fileButtons->addWidget(openSelectedOutputButton_);
  fileButtons->addWidget(openDirButton_);
  fileButtons->addStretch();
  filesLay->addLayout(fileButtons);
  lay->addWidget(filesBox, 1);
  return page;
}

// ---------------------------------------------------------------------------
// Status tab (W-32 / slice A, OD-21 = a 2026-10-22)
//
// The contract in docs/planning/W32_RESULT_CONTRACT.md is the source of truth:
// one row per OPERATION (not per input), six columns, named failure reasons, and
// the honesty rule that a row which did not succeed shows NO byte figures - not
// zero, because "0 bytes saved" for work that never ran is the lie the contract
// exists to prevent. Rows are installed once at Start from the planned jobs and
// are never re-derived from the live queue: the queue may change mid-run.
// ---------------------------------------------------------------------------
QWidget* MainWindow::buildStatusTab() {
  auto* page = new QWidget(tabs_);
  auto* lay = new QVBoxLayout(page);

  resultHeaderLabel_ = new QLabel(QStringLiteral("Processing list: 0 of 0 files"), page);
  resultHeaderLabel_->setObjectName(QStringLiteral("resultHeaderLabel"));
  lay->addWidget(resultHeaderLabel_);

  resultTable_ = new QTableWidget(0, 6, page);
  resultTable_->setObjectName(QStringLiteral("resultTable"));
  resultTable_->setHorizontalHeaderLabels({QStringLiteral("File"), QStringLiteral("Before"),
                                           QStringLiteral("After"), QStringLiteral("Change"),
                                           QStringLiteral("State"), QStringLiteral("Detail")});
  resultTable_->verticalHeader()->setVisible(false);
  resultTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  resultTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  resultTable_->setAlternatingRowColors(true);
  resultTable_->setWordWrap(false);
  resultTable_->horizontalHeader()->setStretchLastSection(true);
  resultTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  resultTable_->setColumnWidth(0, 260);
  resultTable_->setColumnWidth(1, 90);
  resultTable_->setColumnWidth(2, 90);
  resultTable_->setColumnWidth(3, 90);
  resultTable_->setColumnWidth(4, 90);
  lay->addWidget(resultTable_, 1);

  resultSummaryLabel_ = new QLabel(QStringLiteral("No run yet."), page);
  resultSummaryLabel_->setObjectName(QStringLiteral("resultSummaryLabel"));
  resultSummaryLabel_->setWordWrap(true);
  lay->addWidget(resultSummaryLabel_);

  auto* note = new QLabel(QStringLiteral(
      "One row per operation. Batch is one row per file; Merge is one row with every input; "
      "Explode is one row whose After is the sum of its frames. A row that did not succeed "
      "shows no sizes, because the work did not happen."), page);
  note->setObjectName(QStringLiteral("resultNoteLabel"));
  note->setWordWrap(true);
  note->setTextFormat(Qt::PlainText);
  lay->addWidget(note);

  // ---- the command preview and the activity log -------------------------
  // Owner, 2026-10-24: they used to sit under the whole window, which the
  // reference does not have and which made every tab look like a console. The
  // reference's Status tab IS the "what is happening" surface, so they live
  // here now - same object names (commandPane / logPane / clearLogButton), so
  // the honesty guarantees and every harness contract are unchanged.
  auto* commandTitle = new QLabel(
      QStringLiteral("Command preview · read-only · controls \u2192 exact engine arguments"), page);
  commandTitle->setObjectName(QStringLiteral("commandTitle"));
  lay->addWidget(commandTitle);
  commandPane_ = new QPlainTextEdit(page);
  commandPane_->setObjectName(QStringLiteral("commandPane"));
  commandPane_->setReadOnly(true);
  commandPane_->setMaximumHeight(64);
  commandPane_->setPlaceholderText(QStringLiteral("Add a GIF to generate a command\u2026"));
  lay->addWidget(commandPane_);

  auto* logHeader = new QHBoxLayout();
  auto* logTitle = new QLabel(QStringLiteral("Activity log"), page);
  logTitle->setObjectName(QStringLiteral("logTitle"));
  logHeader->addWidget(logTitle);
  auto* clearLog = new QPushButton(QStringLiteral("Clear"), page);
  clearLog->setObjectName(QStringLiteral("clearLogButton"));
  logHeader->addWidget(clearLog);
  logHeader->addStretch();
  lay->addLayout(logHeader);
  logPane_ = new QPlainTextEdit(page);
  logPane_->setObjectName(QStringLiteral("logPane"));
  logPane_->setReadOnly(true);
  logPane_->setMaximumBlockCount(250);
  logPane_->setMaximumHeight(120);
  logPane_->setPlaceholderText(QStringLiteral("Run messages will appear here\u2026"));
  connect(clearLog, &QPushButton::clicked, this, [this]() { resetRunLog(); });
  lay->addWidget(logPane_);
  return page;
}

// Settings tab (slice B): app-level facts, all read from the running build or
// the engine probe. Deliberately no editable controls yet - a settings field
// that nothing reads would be worse than none.
QWidget* MainWindow::buildSettingsTab() {
  auto* page = new QWidget(tabs_);
  auto* lay = new QVBoxLayout(page);

  auto* engineBox = new QGroupBox(QStringLiteral("Engine"), page);
  engineBox->setObjectName(QStringLiteral("settingsEngineGroup"));
  auto* engineForm = new QFormLayout(engineBox);
  auto* enginePath = new QLabel(enginePath_.isEmpty() ? QStringLiteral("(not located yet)") : enginePath_,
                                engineBox);
  enginePath->setObjectName(QStringLiteral("settingsEnginePath"));
  enginePath->setWordWrap(true);
  enginePath->setTextInteractionFlags(Qt::TextSelectableByMouse);
  engineForm->addRow(QStringLiteral("gifsicle"), enginePath);
  auto* engineVersion = new QLabel(QStringLiteral("probe on demand (status bar shows the result)"),
                                   engineBox);
  engineVersion->setObjectName(QStringLiteral("settingsEngineVersion"));
  engineVersion->setWordWrap(true);
  engineForm->addRow(QStringLiteral("Version"), engineVersion);
  lay->addWidget(engineBox);

  auto* filesBox = new QGroupBox(QStringLiteral("Files"), page);
  filesBox->setObjectName(QStringLiteral("settingsFilesGroup"));
  auto* filesForm = new QFormLayout(filesBox);
  auto* sessionPath = new QLabel(sessionFilePath().isEmpty()
                                     ? QStringLiteral("(no writable config location)")
                                     : sessionFilePath(),
                                 filesBox);
  sessionPath->setObjectName(QStringLiteral("settingsFilePath"));
  sessionPath->setWordWrap(true);
  sessionPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
  filesForm->addRow(QStringLiteral("Session file"), sessionPath);
  auto* openFolder = new QPushButton(QStringLiteral("Show settings folder"), filesBox);
  openFolder->setObjectName(QStringLiteral("settingsOpenFolderButton"));
  connect(openFolder, &QPushButton::clicked, this, [this]() {
    const QString path = sessionFilePath();
    if (path.isEmpty()) {
      updateStatus(QStringLiteral("No writable settings location on this system."));
      return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
  });
  filesForm->addRow(QString(), openFolder);
  lay->addWidget(filesBox);

  lay->addStretch();
  return page;
}

QWidget* MainWindow::buildAboutTab() {
  auto* page = new QWidget(tabs_);
  auto* outer = new QVBoxLayout(page);
  outer->setContentsMargins(0, 0, 0, 0);

  // The About block: the facts a user (or a bug report) needs. Everything here
  // is read from the binary or the engine probe, not typed in by hand.
  auto* about = new QGroupBox(QStringLiteral("Gifscythe"), page);
  about->setObjectName(QStringLiteral("aboutGroup"));
  auto* aboutLay = new QVBoxLayout(about);
  auto* ver = new QLabel(QStringLiteral("Version %1 · engine: gifsicle (LCDF) · licence: Ms-PL")
                             .arg(QStringLiteral(GS_VERSION)), about);
  ver->setObjectName(QStringLiteral("aboutVersion"));
  ver->setWordWrap(true);
  ver->setTextFormat(Qt::PlainText);
  aboutLay->addWidget(ver);
  auto* aboutNote = new QLabel(QStringLiteral(
      "Gifscythe is a desktop front end for the gifsicle GIF engine. Output is written "
      "beside a staging file, verified, and only then moved into place; the pictures you "
      "already have are never touched by a cancelled or failed run."), about);
  aboutNote->setObjectName(QStringLiteral("aboutNote"));
  aboutNote->setWordWrap(true);
  aboutNote->setTextFormat(Qt::PlainText);
  aboutLay->addWidget(aboutNote);
  outer->addWidget(about);

  auto* scroll = new QScrollArea(page);
  scroll->setObjectName(QStringLiteral("guideScroll"));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto* content = new QWidget(scroll);
  auto* lay = new QVBoxLayout(content);

  auto addSection = [content, lay](const QString& title, const QString& text) {
    auto* box = new QGroupBox(title, content);
    auto* boxLay = new QVBoxLayout(box);
    auto* label = new QLabel(text, box);
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    label->setObjectName(QStringLiteral("guideDescription"));
    boxLay->addWidget(label);
    lay->addWidget(box);
  };

  auto* intro = new QLabel(QStringLiteral(
      "A plain-language map of the terms behind Gifscythe. You do not need to know gifsicle "
      "commands to get a good result: start on Input, choose a few Actions, then press Start."),
      content);
  intro->setWordWrap(true);
  intro->setObjectName(QStringLiteral("guideIntro"));
  lay->addWidget(intro);
  addSection(QStringLiteral("Recommended first run"), QStringLiteral(
      "1. Add one or more GIFs on Input.\n"
      "2. Leave Optimization level at 3 for a lossless size reduction.\n"
      "3. If the file is still too large, try Lossy compression around 20–60 and check the written output.\n"
      "4. Use Resize only when you also want to change dimensions.\n"
      "5. Start writes a real file. The Output tab shows its exact path; the preview is only a viewer."));
  addSection(QStringLiteral("Compression terms"), QStringLiteral(
      "Optimization level (-O): removes redundant pixels and improves the GIF layout. Higher levels can take longer, "
      "but normally do not change the picture. Level 0 turns this pass off.\n\n"
      "Lossy compression: allows small color/detail changes so the file can become much smaller. 0 means lossless; "
      "higher numbers trade more visual fidelity for size. Inspect the After preview and the size report.\n\n"
      "Reduce colors: limits the palette used by each frame. Fewer colors usually mean a smaller file, but gradients "
      "can look banded. Dither adds a patterned blend of nearby colors to hide that banding."));
  addSection(QStringLiteral("Resize and image quality"), QStringLiteral(
      "Fit inside keeps the aspect ratio and never exceeds W×H. Touch W×H keeps the aspect ratio while fitting the "
      "requested box. Exact uses both dimensions and may stretch the animation. Width only and Height only change one "
      "dimension. Scale uses independent X and Y percentages.\n\n"
      "Resampling method is the algorithm used for new pixels. Point is sharp and fast; Lanczos and Mitchell are "
      "smoother, higher-quality choices that can take longer."));
  addSection(QStringLiteral("Animation terms"), QStringLiteral(
      "Frame delay is measured in 1/100 of a second because that is the GIF format unit: 10 means 0.10 seconds, not 10 ms.\n\n"
      "Loop forever repeats the animation. Play once removes the loop extension so players stop after one pass. "
      "Disposal says what a player should do with the previous frame; Keep original is the safest default.\n\n"
      "Threads controls parallel work, not the number of frames. Unchanged lets gifsicle choose its default; Auto asks "
      "for its automatic thread count."));
  addSection(QStringLiteral("Modes and outputs"), QStringLiteral(
      "Batch creates one optimized GIF per input, using the name pattern. Merge concatenates the queued GIFs into one "
      "animation and needs Save as. Explode writes individual frame GIFs under a prefix. Auto is the low-level mode for "
      "one explicit output.\n\n"
      "Output is not just a preview: Gifscythe verifies the file before saying Done. If Windows or another program keeps "
      "a destination open, the status and log say what happened and preserve verified work where possible."));
  addSection(QStringLiteral("Activity log and command"), QStringLiteral(
      "The command panel is a read-only, copyable view of the exact argv that will be executed. It is for transparency, "
      "not a second way to edit settings. The Activity log below it records start, progress, warnings, cancellation and "
      "engine errors. A moving progress bar means the engine is still working; for Batch, the bar advances once per file."));
  addSection(QStringLiteral("Preview controls"), QStringLiteral(
      "Before is the selected source. After is either a settings preview or, after a successful run, the actual file "
      "written to disk. Play and Stop only control playback; they never start another optimization. Turn off Auto-play "
      "if you are working with a very large GIF."));
  lay->addStretch();
  scroll->setWidget(content);
  outer->addWidget(scroll);
  return page;
}

// ---------------------------------------------------------------------------
// Presets — the reference footer's left-hand group.
//
// A preset is one file: the same key=value payload saveSessionState() writes,
// so it round-trips through gs::save_settings / gs::load_settings and cannot
// drift from the session format. Stored one-per-file so a preset can be read,
// edited or deleted by hand — a blob in one settings file would not be.
// ---------------------------------------------------------------------------

QString MainWindow::presetDir() const {
  const QString session = sessionFilePath();
  if (session.isEmpty()) return {};
  return QFileInfo(session).absolutePath() + QStringLiteral("/presets");
}

void MainWindow::refreshPresetList() {
  if (!presetCombo_) return;
  const QString previous = presetCombo_->currentText();
  QSignalBlocker blocker(presetCombo_);
  presetCombo_->clear();
  presetCombo_->addItem(QStringLiteral("(no preset)"));
  const QString dir = presetDir();
  if (!dir.isEmpty()) {
    const QDir folder(dir);
    const auto entries = folder.entryInfoList({QStringLiteral("*.conf")}, QDir::Files, QDir::Name);
    for (const QFileInfo& entry : entries) {
      presetCombo_->addItem(entry.completeBaseName());
    }
  }
  const int restored = presetCombo_->findText(previous);
  if (restored > 0) presetCombo_->setCurrentIndex(restored);
}

void MainWindow::serializeGuiState(std::ostream& oss, const char* headerComment) const {
  gs::Settings s;
  settingsPanel_->writeSessionInto(s);
  s.inputs.clear();
  s.output.clear();
  gs::save_settings(oss, s);
  oss << headerComment << "\n";

  const bool folderDest = destFolderRadio_ && destFolderRadio_->isChecked();
  if (folderDest) oss << "destination = folder\n";
  const QString bd = batchDirEdit_ ? batchDirEdit_->text().trimmed() : QString();
  if (!bd.isEmpty()) oss << "batch_dir = " << gs::encode_line_value(bd.toStdString()) << "\n";

  const QString tmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  if (!tmpl.isEmpty() && tmpl != QString::fromUtf8(kDefaultNameTemplate)) {
    oss << "name_template = " << gs::encode_line_value(tmpl.toStdString()) << "\n";
  }
  if (startIndexSpin_ && startIndexSpin_->value() != 1) {
    oss << "start_index = " << startIndexSpin_->value() << "\n";
  }

  const gs::OutputFormat fmt = selectedOutputFormat();
  if (fmt != gs::OutputFormat::Gif) {
    oss << "output_format = " << gs::output_format_token(fmt) << "\n";
  }
  if (ffmpegPrefs_.webpQuality >= 0) oss << "webp_quality = " << ffmpegPrefs_.webpQuality << "\n";
  if (ffmpegPrefs_.h264Quality >= 0) oss << "h264_quality = " << ffmpegPrefs_.h264Quality << "\n";
  if (ffmpegPrefs_.h265Quality >= 0) oss << "h265_quality = " << ffmpegPrefs_.h265Quality << "\n";
  if (ffmpegPrefs_.vp9Quality >= 0) oss << "vp9_quality = " << ffmpegPrefs_.vp9Quality << "\n";
  if (ffmpegPrefs_.av1Quality >= 0) oss << "av1_quality = " << ffmpegPrefs_.av1Quality << "\n";
  if (ffmpegPrefs_.fps > 0) oss << "ffmpeg_fps = " << ffmpegPrefs_.fps << "\n";
  if (!ffmpegPrefs_.loop) oss << "ffmpeg_loop = 0\n";

  if (collisionPolicy_ != gs::CollisionPolicy::Ask) {
    oss << "collision_policy = " << gs::collision_policy_token(collisionPolicy_) << "\n";
  }
  if (keepStructureCheck_ && !keepStructureCheck_->isChecked()) {
    oss << "keep_folder_structure = 0\n";
  }
  if (discardLargerCheck_ && discardLargerCheck_->isChecked()) {
    oss << "discard_if_larger = 1\n";
  }

  if (settingsPanel_) {
    const QString cards = settingsPanel_->cardState();
    if (!cards.isEmpty()) oss << "card_state = " << cards.toStdString() << "\n";
  }
}

void MainWindow::applyGuiState(const gs::Settings& loaded,
                               const std::map<std::string, std::string>& guiKeys) {
  settingsPanel_->readFrom(loaded);

  auto rawKey = [&guiKeys](const char* k) -> std::string {
    const auto it = guiKeys.find(k);
    return it == guiKeys.end() ? std::string() : it->second;
  };
  auto guiKey = [&guiKeys](const char* k) -> QString {
    const auto it = guiKeys.find(k);
    if (it == guiKeys.end()) return QString();
    std::string decoded = it->second;
    gs::decode_line_value(decoded);
    return QString::fromStdString(decoded);
  };
  auto parseBoundedInt = [&rawKey](const char* k, int defVal, int lo, int hi) -> int {
    const std::string raw = rawKey(k);
    if (raw.empty()) return defVal;
    int v = defVal;
    if (!gs::to_int_strict(raw, &v)) return defVal;
    return std::max(lo, std::min(hi, v));
  };

  const QString cards = guiKey("card_state");
  if (!cards.isEmpty()) settingsPanel_->setCardState(cards);

  const QString batch = guiKey("batch_dir");
  const std::string destTok = gs::lower(gs::trim(rawKey("destination")));
  syncingDestControls_ = true;
  if (batchDirEdit_) batchDirEdit_->setText(batch);
  if (destFolderRadio_ && destSourceRadio_) {
    const bool useFolder = (destTok == "folder") || (destTok.empty() && !batch.trimmed().isEmpty());
    destFolderRadio_->setChecked(useFolder);
    destSourceRadio_->setChecked(!useFolder);
  }
  syncingDestControls_ = false;

  if (nameTemplateEdit_) {
    const QString tmpl = guiKey("name_template");
    nameTemplateEdit_->setText(tmpl.isEmpty() ? QString::fromUtf8(kDefaultNameTemplate) : tmpl);
  }
  if (startIndexSpin_) {
    startIndexSpin_->setValue(parseBoundedInt("start_index", 1, 0, 999999));
  }

  FfmpegFormatPrefs prefs;
  prefs.webpQuality = parseBoundedInt("webp_quality", -1, -1, 100);
  prefs.h264Quality = parseBoundedInt("h264_quality", -1, -1, 100);
  prefs.h265Quality = parseBoundedInt("h265_quality", -1, -1, 100);
  prefs.vp9Quality = parseBoundedInt("vp9_quality", -1, -1, 100);
  prefs.av1Quality = parseBoundedInt("av1_quality", -1, -1, 100);
  prefs.fps = parseBoundedInt("ffmpeg_fps", 0, 0, 120);
  const std::string loopRaw = gs::lower(gs::trim(rawKey("ffmpeg_loop")));
  prefs.loop = !(loopRaw == "0" || loopRaw == "false" || loopRaw == "no");
  ffmpegPrefs_ = prefs;

  applyOutputFormatToken(guiKey("output_format"));

  setCollisionPolicy(gs::collision_policy_from_token(gs::lower(gs::trim(rawKey("collision_policy")))));
  if (keepStructureCheck_) {
    const std::string ks = gs::lower(gs::trim(rawKey("keep_folder_structure")));
    keepStructureCheck_->setChecked(!(ks == "0" || ks == "false" || ks == "no"));
  }
  if (discardLargerCheck_) {
    const std::string dl = gs::lower(gs::trim(rawKey("discard_if_larger")));
    discardLargerCheck_->setChecked(dl == "1" || dl == "true" || dl == "yes");
  }
  refreshSizeGuardState();
  refreshTemplateNote();
}

bool MainWindow::savePresetByName(const QString& rawName) {
  const QString dir = presetDir();
  if (dir.isEmpty()) {
    updateStatus(QStringLiteral("No writable settings folder — presets are unavailable here."));
    return false;
  }
  if (rawName.trimmed().isEmpty()) return false;
  QString safe = rawName.trimmed();
  safe.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
  if (!QDir().mkpath(dir)) {
    updateStatus(QStringLiteral("Could not create the preset folder: %1").arg(dir));
    return false;
  }
  std::ostringstream oss;
  serializeGuiState(oss, "# Gifscythe preset (GUI keys are ignored by the CLI)");

  const QString path = QDir(dir).filePath(safe + QStringLiteral(".conf"));
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    updateStatus(QStringLiteral("Could not write the preset: %1").arg(path));
    return false;
  }
  const QByteArray data = QByteArray::fromStdString(oss.str());
  if (file.write(data) != data.size() || !file.commit()) {
    file.cancelWriting();
    updateStatus(QStringLiteral("Could not save the preset: %1").arg(path));
    return false;
  }
  refreshPresetList();
  if (presetCombo_) {
    const int row = presetCombo_->findText(safe);
    if (row > 0) presetCombo_->setCurrentIndex(row);
  }
  updateStatus(QStringLiteral("Preset saved: %1").arg(safe));
  return true;
}

void MainWindow::savePresetAs() {
  const QString dir = presetDir();
  if (dir.isEmpty()) {
    updateStatus(QStringLiteral("No writable settings folder — presets are unavailable here."));
    return;
  }
  bool ok = false;
  const QString current = (presetCombo_ && presetCombo_->currentIndex() > 0)
                              ? presetCombo_->currentText()
                              : QString();
  const QString name = QInputDialog::getText(
      this, QStringLiteral("Save preset"), QStringLiteral("Preset name:"),
      QLineEdit::Normal, current, &ok);
  if (!ok || name.trimmed().isEmpty()) return;
  savePresetByName(name);
}

void MainWindow::loadSelectedPreset() {
  if (!presetCombo_ || presetCombo_->currentIndex() <= 0) return;
  const QString name = presetCombo_->currentText();
  const QString path = QDir(presetDir()).filePath(name + QStringLiteral(".conf"));
  std::vector<gs::LoadWarning> warnings;
  std::map<std::string, std::string> guiKeys;
  auto loaded = gs::load_settings_file(path.toStdString(), &warnings, &guiKeys);
  if (!loaded) {
    updateStatus(QStringLiteral("Could not read the preset %1 — it may have been removed.")
                     .arg(name));
    refreshPresetList();
    return;
  }
  applyGuiState(*loaded, guiKeys);

  lastOutputPaths_.clear();
  refreshCommand();
  refreshOutputSummary();
  schedulePreview();
  updateStatus(warnings.empty()
                   ? QStringLiteral("Preset applied: %1").arg(name)
                   : QStringLiteral("Preset applied: %1 — %2 warning(s), see the log")
                         .arg(name)
                         .arg(warnings.size()));
}

void MainWindow::deleteSelectedPreset() {
  if (!presetCombo_ || presetCombo_->currentIndex() <= 0) return;
  const QString name = presetCombo_->currentText();
  const QString path = QDir(presetDir()).filePath(name + QStringLiteral(".conf"));
  const auto answer = QMessageBox::question(
      this, QStringLiteral("Delete preset"),
      QStringLiteral("Delete the preset \"%1\"?").arg(name));
  if (answer != QMessageBox::Yes) return;
  if (!QFile::remove(path)) {
    updateStatus(QStringLiteral("Could not delete the preset: %1").arg(path));
    return;
  }
  refreshPresetList();
  updateStatus(QStringLiteral("Preset deleted: %1").arg(name));
}

void MainWindow::buildBottomBar(QWidget* central, QVBoxLayout* root) {
  // Owner, 2026-10-24: this is the reference's footer, left to right -
  //   Presets: [combo] [+] [x] ... [Convert] [Close]
  // The old bar carried the command preview and the activity log across the
  // whole window, which the reference does not have at all and which is why
  // the window did not look like XnConvert. Both moved into the Status tab
  // (same object names, so every harness contract still holds): Status is the
  // reference's "what is happening" tab, and a log belongs there.
  auto* bar = new QHBoxLayout();

  auto* presetsLabel = new QLabel(QStringLiteral("Presets:"), central);
  bar->addWidget(presetsLabel);
  presetCombo_ = new QComboBox(central);
  presetCombo_->setObjectName(QStringLiteral("presetCombo"));
  presetCombo_->setMinimumWidth(170);
  presetCombo_->setToolTip(
      QStringLiteral("Saved action sets. A preset stores every gifsicle option on the Actions "
                     "tab plus the batch folder and file-name pattern."));
  connect(presetCombo_, &QComboBox::activated, this, [this](int) { loadSelectedPreset(); });
  bar->addWidget(presetCombo_);

  auto* savePreset = new QToolButton(central);
  savePreset->setObjectName(QStringLiteral("presetSaveButton"));
  savePreset->setText(QStringLiteral("+"));
  savePreset->setToolTip(QStringLiteral("Save the current settings as a preset"));
  connect(savePreset, &QToolButton::clicked, this, &MainWindow::savePresetAs);
  bar->addWidget(savePreset);

  auto* deletePreset = new QToolButton(central);
  deletePreset->setObjectName(QStringLiteral("presetDeleteButton"));
  deletePreset->setText(QStringLiteral("\u00d7"));
  deletePreset->setToolTip(QStringLiteral("Delete the selected preset"));
  connect(deletePreset, &QToolButton::clicked, this, &MainWindow::deleteSelectedPreset);
  bar->addWidget(deletePreset);

  bar->addStretch();

  // The two controls that describe a run rather than a tab: what is happening
  // (status line) and how far it got (progress).
  statusLabel_ = new QLabel(central);
  statusLabel_->setObjectName(QStringLiteral("statusLabel"));
  statusLabel_->setWordWrap(false);
  statusLabel_->setMinimumWidth(200);
  bar->addWidget(statusLabel_, 1);

  progressBar_ = new QProgressBar(central);
  progressBar_->setObjectName(QStringLiteral("progressBar"));
  progressBar_->setRange(0, 0);  // indeterminate for one large engine run
  progressBar_->setFormat(QStringLiteral("Working\u2026"));
  progressBar_->setVisible(false);
  progressBar_->setMaximumWidth(220);
  bar->addWidget(progressBar_);

  // Slice F semantics unchanged: the primary control reads "Convert"; the
  // secondary one reads "Stop" while a run is in flight (setBusy swaps it) and
  // "Cancel" when idle. The U-72 latch is untouched.
  cancelButton_ = new QPushButton(QStringLiteral("Cancel"), central);
  cancelButton_->setObjectName(QStringLiteral("cancelButton"));
  cancelButton_->setEnabled(false);
  connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancelRun);
  bar->addWidget(cancelButton_);

  runButton_ = new QPushButton(QStringLiteral("Convert"), central);
  runButton_->setObjectName(QStringLiteral("runButton"));
  runButton_->setMinimumHeight(30);
  runButton_->setMinimumWidth(96);
  runButton_->setToolTip(QStringLiteral("Start processing the queued GIF files"));
  runButton_->setEnabled(false);
  connect(runButton_, &QPushButton::clicked, this, &MainWindow::runCommand);
  bar->addWidget(runButton_);

  auto* closeButton = new QPushButton(QStringLiteral("Close"), central);
  closeButton->setObjectName(QStringLiteral("closeButton"));
  connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
  bar->addWidget(closeButton);

  root->addLayout(bar);
  refreshPresetList();
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (busy_) {
    cancelRun();
  }
  killPreview();
  // S7: persist the session (Actions state + batch dir + name template).
  // A failed save is surfaced, never silent — but never blocks the close.
  if (!saveSessionState()) {
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Could not save your settings for the next session to:\n%1")
            .arg(sessionFilePath()));
  }
  QMainWindow::closeEvent(event);
}

bool MainWindow::ensureEngine() {
  if (gs::path_is_executable(gs::u8path_compat(enginePath_.toStdString()))) {
    updateStatus(QStringLiteral("Ready — engine: %1").arg(enginePath_));
    return true;
  }
  // Re-probe in case the user built the engine after launch.
  enginePath_ = QString::fromStdString(
      gs::locate_engine(QCoreApplication::applicationFilePath().toStdString()));
  if (gs::path_is_executable(gs::u8path_compat(enginePath_.toStdString()))) {
    updateStatus(QStringLiteral("Ready — engine: %1").arg(enginePath_));
    return true;
  }
  updateStatus(QStringLiteral("Engine not found — build with ./scripts/build_engine.sh"));
  runButton_->setEnabled(false);
  return false;
}

// ---------------------------------------------------------------------------
// N-62: the FFmpeg sidecar, from the GUI's side.
//
// The owner's one sentence was "add ffmpeg directly into the project ... just
// make it portable but add fallback if ffmpeg gone it will search for ffmpeg
// program on computer ... only called when ffmpeg local into the folder not
// found". All four of those steps live in core/FfmpegLocator.h; this is the part
// that (a) tells the truth about the answer and (b) refuses to offer a format it
// cannot write.
// ---------------------------------------------------------------------------
gs::OutputFormat MainWindow::selectedOutputFormat() const {
  if (!formatCombo_ || formatCombo_->currentIndex() < 0) return gs::OutputFormat::Gif;
  const QString token = formatCombo_->currentData(Qt::UserRole).toString();
  // "gif" is both "Same as original" and "GIF": they are the same encoder and
  // the same file, and pretending otherwise would be a lie in the session file.
  return gs::output_format_from_token(token.toStdString());
}

void MainWindow::applyOutputFormatToken(const QString& token) {
  if (!formatCombo_) return;
  const QString wanted = token.trimmed().isEmpty() ? QStringLiteral("gif") : token.trimmed();
  for (int row = 0; row < formatCombo_->count(); ++row) {
    if (formatCombo_->itemData(row, Qt::UserRole).toString() != wanted) continue;
    const QSignalBlocker block(formatCombo_);
    formatCombo_->setCurrentIndex(row);
    refreshFfmpegStatus(false);  // apply tooltips/notes for the restored choice
    refreshSizeGuardState();
    refreshTemplateNote();
    return;
  }
  // Unknown token: leave the current selection alone rather than guessing.
}

void MainWindow::setFfmpegFormatPrefs(const FfmpegFormatPrefs& prefs) {
  auto clampQ = [](int q) { return q < 0 ? -1 : std::min(100, q); };
  ffmpegPrefs_.webpQuality = clampQ(prefs.webpQuality);
  ffmpegPrefs_.h264Quality = clampQ(prefs.h264Quality);
  ffmpegPrefs_.h265Quality = clampQ(prefs.h265Quality);
  ffmpegPrefs_.vp9Quality = clampQ(prefs.vp9Quality);
  ffmpegPrefs_.av1Quality = clampQ(prefs.av1Quality);
  ffmpegPrefs_.fps = std::max(0, std::min(120, prefs.fps));
  ffmpegPrefs_.loop = prefs.loop;
  refreshFfmpegStatus(false);
  lastOutputPaths_.clear();
  refreshCommand();
  refreshOutputSummary();
}

void MainWindow::openFormatSettings() {
  refreshFfmpegStatus(true);  // re-scan in case FFmpeg was just installed
  const gs::OutputFormat fmt = selectedOutputFormat();

  QDialog dlg(this);
  dlg.setObjectName(QStringLiteral("formatSettingsDialog"));
  dlg.setWindowTitle(QStringLiteral("Format settings — %1")
                         .arg(QString::fromUtf8(gs::output_format_name(fmt))));
  auto* lay = new QVBoxLayout(&dlg);

  auto* ffBox = new QGroupBox(QStringLiteral("FFmpeg sidecar"), &dlg);
  auto* ffLay = new QVBoxLayout(ffBox);
  auto* ffLine = new QLabel(
      QStringLiteral("FFmpeg: %1")
          .arg(QString::fromStdString(gs::ffmpeg_location_summary(ffmpegLocation_))),
      ffBox);
  ffLine->setObjectName(QStringLiteral("formatDialogFfmpegLine"));
  ffLine->setWordWrap(true);
  ffLine->setTextInteractionFlags(Qt::TextSelectableByMouse);
  ffLay->addWidget(ffLine);

  auto* ffExplain = new QLabel(
      ffmpegLocation_.found()
          ? QStringLiteral("Lookup order: GS_FFMPEG, beside this program, PATH, then standard install folders.")
          : QStringLiteral("Without FFmpeg, GIF works via the bundled gifsicle engine; APNG, WebP, MP4 and WebM stay disabled."),
      ffBox);
  ffExplain->setWordWrap(true);
  ffLay->addWidget(ffExplain);
  lay->addWidget(ffBox);

  QSpinBox* qualitySpin = nullptr;
  QSpinBox* fpsSpin = nullptr;
  QCheckBox* loopCheck = nullptr;

  if (!gs::output_format_needs_ffmpeg(fmt)) {
    auto* gifBox = new QGroupBox(QStringLiteral("GIF encoder (bundled gifsicle)"), &dlg);
    auto* gifLay = new QVBoxLayout(gifBox);
    auto* gifNote = new QLabel(
        QStringLiteral("GIF optimization, lossy compression, palette colors, dithering, frame delay, "
                       "and loop count are configured on the Actions tab."),
        gifBox);
    gifNote->setWordWrap(true);
    gifLay->addWidget(gifNote);
    auto* goActions = new QPushButton(QStringLiteral("Open Actions tab"), gifBox);
    goActions->setObjectName(QStringLiteral("formatOpenActionsButton"));
    connect(goActions, &QPushButton::clicked, &dlg, [this, &dlg]() {
      if (tabs_ && actionsTabIndex_ >= 0) tabs_->setCurrentIndex(actionsTabIndex_);
      dlg.accept();
    });
    gifLay->addWidget(goActions);
    lay->addWidget(gifBox);
  } else {
    auto* encBox = new QGroupBox(
        QStringLiteral("%1 (%2)")
            .arg(QString::fromUtf8(gs::output_format_name(fmt)),
                 QString::fromStdString(gs::ffmpeg_encode_label(fmt))),
        &dlg);
    auto* form = new QFormLayout(encBox);

    if (fmt == gs::OutputFormat::Apng) {
      auto* losslessLbl = new QLabel(
          QStringLiteral("Lossless 32-bit RGBA PNG frames (no lossy quality slider)."), encBox);
      losslessLbl->setWordWrap(true);
      form->addRow(QStringLiteral("Quality"), losslessLbl);
    } else {
      qualitySpin = new QSpinBox(encBox);
      qualitySpin->setObjectName(QStringLiteral("formatQualitySpin"));
      qualitySpin->setRange(-1, 100);
      qualitySpin->setSpecialValueText(QStringLiteral("Codec default"));
      qualitySpin->setToolTip(
          QStringLiteral("-1 uses the tested codec default; 0..100 sets explicit quality (higher = better)."));
      int curQ = -1;
      if (fmt == gs::OutputFormat::Webp) curQ = ffmpegPrefs_.webpQuality;
      else if (fmt == gs::OutputFormat::Mp4H264) curQ = ffmpegPrefs_.h264Quality;
      else if (fmt == gs::OutputFormat::Mp4H265) curQ = ffmpegPrefs_.h265Quality;
      else if (fmt == gs::OutputFormat::WebmVp9) curQ = ffmpegPrefs_.vp9Quality;
      else if (fmt == gs::OutputFormat::WebmAv1) curQ = ffmpegPrefs_.av1Quality;
      qualitySpin->setValue(curQ);
      form->addRow(QStringLiteral("Quality (0–100, higher is better)"), qualitySpin);
    }

    fpsSpin = new QSpinBox(encBox);
    fpsSpin->setObjectName(QStringLiteral("formatFpsSpin"));
    fpsSpin->setRange(0, 120);
    fpsSpin->setSpecialValueText(QStringLiteral("Keep source timing"));
    fpsSpin->setSuffix(QStringLiteral(" fps"));
    fpsSpin->setValue(ffmpegPrefs_.fps);
    form->addRow(QStringLiteral("Frame rate"), fpsSpin);

    loopCheck = new QCheckBox(QStringLiteral("Loop animation forever"), encBox);
    loopCheck->setObjectName(QStringLiteral("formatLoopCheck"));
    loopCheck->setChecked(ffmpegPrefs_.loop);
    const bool supportsContainerLoop =
        (fmt == gs::OutputFormat::Webp || fmt == gs::OutputFormat::Apng);
    loopCheck->setEnabled(supportsContainerLoop);
    if (!supportsContainerLoop) {
      loopCheck->setToolTip(
          QStringLiteral("MP4 and WebM containers leave looping to the video player."));
    }
    form->addRow(QString(), loopCheck);

    auto* resetBtn = new QPushButton(QStringLiteral("Reset to recipe defaults"), encBox);
    resetBtn->setObjectName(QStringLiteral("formatResetDefaultsButton"));
    connect(resetBtn, &QPushButton::clicked, &dlg, [qualitySpin, fpsSpin, loopCheck]() {
      if (qualitySpin) qualitySpin->setValue(-1);
      if (fpsSpin) fpsSpin->setValue(0);
      if (loopCheck) loopCheck->setChecked(true);
    });
    form->addRow(QString(), resetBtn);
    lay->addWidget(encBox);
  }

  auto* buttons = new QDialogButtonBox(
      gs::output_format_needs_ffmpeg(fmt)
          ? (QDialogButtonBox::Ok | QDialogButtonBox::Cancel)
          : QDialogButtonBox::Close,
      &dlg);
  connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  lay->addWidget(buttons);

  if (dlg.exec() == QDialog::Accepted && gs::output_format_needs_ffmpeg(fmt)) {
    FfmpegFormatPrefs next = ffmpegPrefs_;
    if (qualitySpin) {
      if (fmt == gs::OutputFormat::Webp) next.webpQuality = qualitySpin->value();
      else if (fmt == gs::OutputFormat::Mp4H264) next.h264Quality = qualitySpin->value();
      else if (fmt == gs::OutputFormat::Mp4H265) next.h265Quality = qualitySpin->value();
      else if (fmt == gs::OutputFormat::WebmVp9) next.vp9Quality = qualitySpin->value();
      else if (fmt == gs::OutputFormat::WebmAv1) next.av1Quality = qualitySpin->value();
    }
    if (fpsSpin) next.fps = fpsSpin->value();
    if (loopCheck && loopCheck->isEnabled()) next.loop = loopCheck->isChecked();
    setFfmpegFormatPrefs(next);
  }
}

void MainWindow::refreshFfmpegStatus(bool relocate) {
  if (!formatCombo_) return;
  if (relocate || !ffmpegLocated_) {
    // applicationDirPath(), not applicationFilePath(): the locator appends the
    // executable's directory itself and the sidecar lives beside the binary.
    ffmpegLocation_ = gs::locate_ffmpeg(QCoreApplication::applicationDirPath().toStdString());
    ffmpegPath_ = QString::fromStdString(ffmpegLocation_.path);
    ffmpegLocated_ = true;
  }
  const bool have = ffmpegLocation_.found();

  auto* model = qobject_cast<QStandardItemModel*>(formatCombo_->model());
  for (int row = 0; row < formatCombo_->count(); ++row) {
    const gs::OutputFormat f = gs::output_format_from_token(
        formatCombo_->itemData(row, Qt::UserRole).toString().toStdString());
    if (!gs::output_format_needs_ffmpeg(f)) continue;
    if (QStandardItem* item = model ? model->item(row) : nullptr) {
      // Enable = the encoder is actually there. Nothing is enabled "in
      // principle": a greyed entry that fails after a two-minute batch is the
      // exact experience the owner asked us to delete.
      item->setFlags(have ? (item->flags() | Qt::ItemIsEnabled)
                          : (item->flags() & ~Qt::ItemIsEnabled));
    }
    formatCombo_->setItemData(
        row,
        have ? QStringLiteral("Encoded by FFmpeg: %1")
                   .arg(QString::fromStdString(gs::ffmpeg_encode_label(f)))
             : QStringLiteral("%1 needs FFmpeg, which was not found. Click Settings… to search "
                              "again after installing it.").arg(QString::fromLatin1(
                                   gs::output_format_name(f))),
        Qt::ToolTipRole);
  }
  // A selection whose encoder vanished must not stay selected: the run path
  // would have to refuse it, and refusing AFTER the user pressed Run is worse
  // than moving the selection now.
  if (!have && gs::output_format_needs_ffmpeg(selectedOutputFormat())) {
    const QSignalBlocker block(formatCombo_);  // no re-entry; the state is already applied
    formatCombo_->setCurrentIndex(1);          // plain GIF
  }

  if (ffmpegStatusLabel_) {
    if (have) {
      ffmpegStatusLabel_->setText(
          QStringLiteral("FFmpeg: %1")
              .arg(QString::fromStdString(gs::ffmpeg_location_summary(ffmpegLocation_))));
    } else {
      ffmpegStatusLabel_->setText(QStringLiteral(
          "FFmpeg not found — GIF works; APNG, WebP, MP4 and WebM stay disabled. "
          "Put an ffmpeg next to this program, or click Settings… after installing one."));
    }
  }
  if (formatNoteLabel_) {
    const gs::OutputFormat f = selectedOutputFormat();
    if (gs::output_format_needs_ffmpeg(f)) {
      int q = -1;
      if (f == gs::OutputFormat::Webp) q = ffmpegPrefs_.webpQuality;
      else if (f == gs::OutputFormat::Mp4H264) q = ffmpegPrefs_.h264Quality;
      else if (f == gs::OutputFormat::Mp4H265) q = ffmpegPrefs_.h265Quality;
      else if (f == gs::OutputFormat::WebmVp9) q = ffmpegPrefs_.vp9Quality;
      else if (f == gs::OutputFormat::WebmAv1) q = ffmpegPrefs_.av1Quality;

      QString paramSummary =
          (f == gs::OutputFormat::Apng)
              ? QStringLiteral("lossless")
              : (q < 0 ? QStringLiteral("default quality")
                       : QStringLiteral("quality %1").arg(q));
      paramSummary += (ffmpegPrefs_.fps > 0)
                          ? QStringLiteral(", %1 fps").arg(ffmpegPrefs_.fps)
                          : QStringLiteral(", source timing");
      if (f == gs::OutputFormat::Webp || f == gs::OutputFormat::Apng) {
        paramSummary += ffmpegPrefs_.loop ? QStringLiteral(", loop forever")
                                          : QStringLiteral(", play once");
      }
      formatNoteLabel_->setText(
          QStringLiteral("Results will be written as .%1 (%2 · %3). Actions-tab GIF settings do "
                         "not apply; configure encoder parameters via Settings….")
              .arg(QString::fromLatin1(gs::output_format_extension(f)),
                   QString::fromStdString(gs::ffmpeg_encode_label(f)),
                   paramSummary));
    } else {
      formatNoteLabel_->setText(QStringLiteral(
          "GIF is produced by the bundled gifsicle engine, with every Actions-tab setting "
          "applied. Converting a moving picture to a still one is exactly what we are not "
          "here to do."));
    }
  }
}

gs::FfmpegEncodeRequest MainWindow::buildFfmpegRequest(const QString& input,
                                                       const QString& partial,
                                                       gs::OutputFormat format,
                                                       const FfmpegFormatPrefs& prefs) const {
  gs::FfmpegEncodeRequest request;
  request.input = input.toStdString();
  request.partial = partial.toStdString();
  request.format = format;
  request.fps = prefs.fps > 0 ? prefs.fps : -1;
  request.loop = prefs.loop ? 0 : 1;
  switch (format) {
    case gs::OutputFormat::Webp:
      request.quality = prefs.webpQuality;
      break;
    case gs::OutputFormat::Mp4H264:
      request.quality = prefs.h264Quality;
      break;
    case gs::OutputFormat::Mp4H265:
      request.quality = prefs.h265Quality;
      break;
    case gs::OutputFormat::WebmVp9:
      request.quality = prefs.vp9Quality;
      break;
    case gs::OutputFormat::WebmAv1:
      request.quality = prefs.av1Quality;
      break;
    case gs::OutputFormat::Apng:
    case gs::OutputFormat::Gif:
      request.quality = -1;
      break;
  }
  return request;
}

bool MainWindow::configureEncoder(const gs::Settings& one, const QString& input,
                                  const QString& partial, gs::OutputFormat format,
                                  QString* error) {
  QStringList qargs;
  if (!gs::output_format_needs_ffmpeg(format)) {
    gs::GifsicleCommand cmd(one);
    for (const auto& a : cmd.args()) qargs << QString::fromStdString(a);
    process_->setProgram(enginePath_);
    process_->setArguments(qargs);
    return true;
  }
  if (ffmpegPath_.isEmpty() || !ffmpegLocation_.found()) {
    if (error) {
      *error = QStringLiteral("No FFmpeg was found, so %1 cannot be written.\n%2")
                   .arg(QString::fromLatin1(gs::output_format_name(format)),
                        QString::fromStdString(gs::ffmpeg_location_summary(ffmpegLocation_)));
    }
    return false;
  }
  const gs::FfmpegEncodeRequest request =
      buildFfmpegRequest(input, partial, format, activeFfmpegPrefs_);
  const std::string refusal = gs::ffmpeg_request_refusal(request);
  if (!refusal.empty()) {
    if (error) *error = QString::fromStdString(refusal);
    return false;
  }
  for (const auto& a : gs::ffmpeg_encode_args(request)) qargs << QString::fromStdString(a);
  process_->setProgram(ffmpegPath_);
  process_->setArguments(qargs);
  return true;
}

void MainWindow::appendInputs(const QStringList& files) {
  // GS-205 / P1-27 (S37): ONE admission rule for the picker, the drop list and
  // anything else that queues input — gs::admit_input() in core/InputAdmission.h.
  // It requires a readable REGULAR file whose bytes start with GIF87a/GIF89a,
  // so a dropped FOLDER (the old `exists()` check said yes to directories) and
  // a .jpg renamed to .gif are refused here, with a reason, instead of being
  // queued and failing later inside the engine.
  int added = 0;
  QStringList refused;
  for (const auto& f : files) {
    if (inputs_.contains(f)) continue;
    const gs::AdmitResult verdict = gs::admit_input(f.toStdString());
    if (!verdict.admitted) {
      refused << QStringLiteral("%1 — %2").arg(QFileInfo(f).fileName(),
                                               QString::fromLatin1(gs::admit_issue_name(verdict.issue)));
      continue;
    }
    inputs_.append(f);
    manualOrder_.append(f);
    auto* item = new QListWidgetItem();
    item->setData(Qt::UserRole, f);
    // Slice C: the cell is a thumbnail + the animation facts, and the tooltip
    // keeps the full path (the grid elides it, the tooltip must not).
    applyQueueItemFacts(item, f);
    inputList_->addItem(item);
    ++added;
  }
  if (added > 0) {
    lastOutputPaths_.clear();
    runButton_->setEnabled(!busy_ && ensureEngine() && !inputs_.isEmpty());
    // A new file lands in insertion order, then the active sort is re-applied
    // so an explicit "sort by frames" cannot quietly hold a hand-placed row at
    // the end; the filter is re-applied so a typed needle covers new arrivals.
    if (queueSortKey_ != SortQueueOrder) applyQueueSort();
    applyQueueFilter();
    refreshQueueLabel();
    refreshCommand();
    refreshOutputSummary();
    if (inputList_->currentRow() < 0) inputList_->setCurrentRow(0);
    updateQueueDetails();
    schedulePreview();
  }
  if (!refused.isEmpty()) {
    // Never a silent drop: the status line always names the count and the
    // first reason, and a refusal that leaves the queue EMPTY is also a dialog
    // (otherwise clicking "Add" on a bad file looks like the app did nothing).
    const QString detail = refused.join(QStringLiteral("; "));
    updateStatus(QStringLiteral("Refused %1 file(s): %2").arg(refused.size()).arg(detail));
    if (added == 0 && inputs_.isEmpty() && !refused.isEmpty()) {
      QMessageBox::warning(
          this, QStringLiteral("Gifscythe"),
          QStringLiteral("Not a GIF file (needs a readable file starting with "
                         "GIF87a/GIF89a):\n%1")
              .arg(detail));
    }
  } else if (added > 0) {
    updateStatus(QStringLiteral("%1 GIF file(s) in queue.").arg(inputs_.size()));
  }
}

void MainWindow::refreshQueueLabel() {
  qint64 total = 0;
  int existing = 0;
  // Slice C: the queue total is an ANIMATION total, not only a byte total —
  // "24 files" says nothing about whether this is a 2-second flipbook or four
  // minutes of video, which is the first thing a moving-picture tool must say.
  qint64 frames = 0;
  qint64 duration_ms = 0;
  for (const auto& f : inputs_) {
    const QFileInfo fi(f);
    if (fi.exists()) {
      total += fi.size();
      ++existing;
    }
    const gs::GifFacts facts = factsFor(f);
    if (facts.ok) {
      frames += facts.frames;
      duration_ms += facts.duration_ms;
    }
  }
  QString text = QStringLiteral("%1 file(s) · %2 frame(s) · %3 · %4 on disk")
                     .arg(inputs_.size())
                     .arg(frames)
                     .arg(formatDuration(duration_ms))
                     .arg(humanSize(total));
  if (existing != inputs_.size()) {
    text += QStringLiteral(" (%1 missing)").arg(inputs_.size() - existing);
  }
  // When a filter hides rows, say how many are actually showing: a grid that
  // silently drops six of ten files looks like data loss, not a filter.
  int hidden = 0;
  if (inputList_) {
    for (int i = 0; i < inputList_->count(); ++i) {
      if (inputList_->item(i)->isHidden()) ++hidden;
    }
  }
  if (hidden > 0) text += QStringLiteral(" · %1 hidden by filter").arg(hidden);
  if (queueCountLabel_) queueCountLabel_->setText(text);
  refreshTabTitles();
}

// ---------------------------------------------------------------------------
// Slice C — the Input grid's data helpers.
//
// Everything below is deliberately cheap and synchronous: the facts come from
// a header walk (no LZW), thumbnails are scaled by QImageReader BEFORE decode
// (so a 1200x1200 GIF costs a 96x96 allocation, not a 5 MB one), and both are
// cached per path. A queue of hundreds of files must still feel like a list.
// ---------------------------------------------------------------------------

gs::GifFacts MainWindow::factsFor(const QString& path) {
  auto it = factsCache_.find(path);
  if (it == factsCache_.end()) {
    it = factsCache_.insert(path, gs::read_gif_facts(path.toStdString()));
  }
  return it.value();
}

QPixmap MainWindow::thumbnailFor(const QString& path) {
  const QFileInfo fi(path);
  // Key on size+mtime as well as the path: a file re-encoded in place (the
  // common "fix it in another tool then re-drop" loop) must not keep showing
  // the old animation's first frame.
  const QString key = QStringLiteral("%1|%2|%3")
                          .arg(path)
                          .arg(fi.size())
                          .arg(fi.lastModified().toMSecsSinceEpoch());
  auto it = thumbCache_.constFind(key);
  if (it != thumbCache_.constEnd()) return it.value();

  QPixmap pixmap;
  QImageReader reader(path);
  reader.setAutoTransform(true);
  const QSize native = reader.size();
  if (native.isValid()) {
    const QSize target = native.scaled(QSize(96, 96), Qt::KeepAspectRatio);
    if (!target.isEmpty()) reader.setScaledSize(target);
  }
  const QImage image = reader.read();
  if (!image.isNull()) {
    pixmap = QPixmap::fromImage(image);
  } else {
    // No Qt image plugin, or a GIF whose first frame will not decode: a
    // labelled placeholder, never an empty cell that looks like a bug.
    pixmap = QPixmap(96, 96);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(palette().color(QPalette::Mid));
    painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QStringLiteral("GIF"));
    painter.end();
  }
  // Bounded cache: a session that drops thousands of files through the grid
  // must not grow a pixmap per file for the whole life of the window.
  if (thumbCache_.size() > 512) thumbCache_.clear();
  thumbCache_.insert(key, pixmap);
  return pixmap;
}

void MainWindow::applyQueueItemFacts(QListWidgetItem* item, const QString& path) {
  if (!item) return;
  const QFileInfo fi(path);
  const gs::GifFacts facts = factsFor(path);

  QStringList caption;
  QStringList detail;
  detail << path;
  if (facts.ok) {
    caption << (facts.frames == 1 ? QStringLiteral("1 frame")
                                  : QStringLiteral("%1 frames").arg(facts.frames));
    // A multi-frame GIF whose delays are all zero is a real and common file
    // (screen recordings, some exporters). It has frames but no duration, and
    // saying "0.00 s" would read as a broken file rather than a fast one.
    if (facts.duration_ms > 0) {
      caption << formatDuration(facts.duration_ms);
    } else if (facts.frames > 1) {
      caption << QStringLiteral("no delay");
    }
    detail << QStringLiteral("%1×%2").arg(facts.width).arg(facts.height);
    detail << QStringLiteral("%1 frame(s)").arg(facts.frames);
    detail << QStringLiteral("duration %1").arg(formatDuration(facts.duration_ms));
    if (facts.fps() > 0.0) {
      detail << QStringLiteral("%1 fps").arg(QString::number(facts.fps(), 'f', 2));
    }
    detail << (facts.loop_count < 0
                   ? QStringLiteral("plays once")
                   : (facts.loop_count == 0
                          ? QStringLiteral("loops forever")
                          : QStringLiteral("loops %1×").arg(facts.loop_count)));
  } else {
    caption << QStringLiteral("header unreadable");
    detail << QStringLiteral("could not read the GIF header (queued anyway: the engine decides)");
  }
  if (fi.exists()) detail << humanSize(fi.size());

  item->setText(QStringLiteral("%1\n%2").arg(fi.fileName(), caption.join(QStringLiteral(" · "))));
  item->setIcon(QIcon(thumbnailFor(path)));
  item->setToolTip(detail.join(QStringLiteral("\n")));
}

void MainWindow::updateQueueDetails() {
  if (!queueSelectionDetails_) return;
  const QString selected = selectedInput();
  if (selected.isEmpty()) {
    queueSelectionDetails_->setText(
        inputs_.isEmpty()
            ? QStringLiteral("No files queued — drop GIFs on the grid, or use Add files… / Add folder…")
            : QStringLiteral("Select a file to see its animation facts"));
    return;
  }
  const gs::GifFacts facts = factsFor(selected);
  const QFileInfo fi(selected);
  QStringList bits;
  bits << fi.fileName();
  if (facts.ok) {
    bits << QStringLiteral("%1×%2").arg(facts.width).arg(facts.height);
    bits << QStringLiteral("%1 frame(s)").arg(facts.frames);
    bits << formatDuration(facts.duration_ms);
    if (facts.fps() > 0.0) bits << QStringLiteral("%1 fps").arg(QString::number(facts.fps(), 'f', 2));
    bits << (facts.loop_count < 0
                 ? QStringLiteral("plays once")
                 : (facts.loop_count == 0 ? QStringLiteral("loops forever")
                                          : QStringLiteral("loops %1×").arg(facts.loop_count)));
  } else {
    bits << QStringLiteral("header unreadable");
  }
  if (fi.exists()) bits << humanSize(fi.size());
  queueSelectionDetails_->setText(bits.join(QStringLiteral("  ·  ")));
}

void MainWindow::updateSortButtonLabels() {
  // The active key gets an arrow so the grid always answers "sorted by what?"
  // without a second label. Queue order has one direction only.
  for (QToolButton* button : sortButtons_) {
    if (!button) continue;
    const int key = button->property("sortKey").toInt();
    button->setChecked(key == queueSortKey_);
    QString label;
    switch (key) {
      case SortQueueOrder: label = QStringLiteral("Queue order"); break;
      case SortName: label = QStringLiteral("Name"); break;
      case SortSize: label = QStringLiteral("Size"); break;
      case SortModified: label = QStringLiteral("Modified"); break;
      case SortFrames: label = QStringLiteral("Frames"); break;
      default: label = QStringLiteral("Duration"); break;
    }
    if (key == queueSortKey_ && key != SortQueueOrder) {
      label += queueSortAscending_ ? QStringLiteral(" ↑") : QStringLiteral(" ↓");
    }
    button->setText(label);
  }
}

void MainWindow::applyQueueSort() {
  if (inputs_.size() < 2 || !inputList_) {
    updateSortButtonLabels();
    return;
  }
  const int count = inputs_.size();
  QVector<int> order(count);
  std::iota(order.begin(), order.end(), 0);

  if (queueSortKey_ == SortQueueOrder) {
    // Back to the order the user built by hand. Every queued path still has a
    // rank there (appendInputs and removeSelected keep manualOrder_ in step),
    // and a path that somehow has none sorts last instead of vanishing.
    QHash<QString, int> rank;
    for (int i = 0; i < manualOrder_.size(); ++i) rank.insert(manualOrder_.at(i), i);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
      return rank.value(inputs_.at(a), count) < rank.value(inputs_.at(b), count);
    });
  } else {
    // Pre-compute every key once: the comparator runs O(n log n) times and a
    // QFileInfo or a GIF header walk inside it would make a 500-file sort slow.
    QVector<qint64> sizes(count, 0);
    QVector<qint64> modified(count, 0);
    QVector<int> frames(count, 0);
    QVector<qint64> durations(count, 0);
    QVector<QString> names(count);
    for (int i = 0; i < count; ++i) {
      const QFileInfo fi(inputs_.at(i));
      names[i] = fi.fileName();
      sizes[i] = fi.size();
      modified[i] = fi.lastModified().toSecsSinceEpoch();
      const gs::GifFacts facts = factsFor(inputs_.at(i));
      frames[i] = facts.ok ? facts.frames : 0;
      durations[i] = facts.ok ? facts.duration_ms : 0;
    }
    const bool ascending = queueSortAscending_;
    const int key = queueSortKey_;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
      int cmp = 0;
      switch (key) {
        case SortName:
          cmp = QString::compare(names[a], names[b], Qt::CaseInsensitive);
          break;
        case SortSize:
          cmp = sizes[a] < sizes[b] ? -1 : (sizes[a] > sizes[b] ? 1 : 0);
          break;
        case SortModified:
          cmp = modified[a] < modified[b] ? -1 : (modified[a] > modified[b] ? 1 : 0);
          break;
        case SortFrames:
          cmp = frames[a] < frames[b] ? -1 : (frames[a] > frames[b] ? 1 : 0);
          break;
        case SortDuration:
          cmp = durations[a] < durations[b] ? -1 : (durations[a] > durations[b] ? 1 : 0);
          break;
        default:
          break;
      }
      // Ties keep INSERTION order in both directions: a reversed tie-break
      // would silently reshuffle equal files when the user flips the arrow,
      // and Merge order is the output order.
      if (cmp == 0) return a < b;
      return ascending ? cmp < 0 : cmp > 0;
    });
  }

  // Rebuild BOTH halves of the queue in one move: inputs_ (the engine's order)
  // and the widget's items. Taking the items out first keeps them alive across
  // the re-add, so icons, facts and the selection's identity survive.
  const QString keepSelected = selectedInput();
  QVector<QListWidgetItem*> items;
  items.reserve(count);
  while (inputList_->count() > 0) items.append(inputList_->takeItem(0));

  QStringList reordered;
  reordered.reserve(count);
  for (int i : order) reordered << inputs_.at(i);
  inputs_ = reordered;
  for (int i : order) inputList_->addItem(items.at(i));

  const int restored = inputs_.indexOf(keepSelected);
  if (restored >= 0) inputList_->setCurrentRow(restored);
  lastOutputPaths_.clear();
  refreshQueueLabel();
  refreshCommand();
  refreshOutputSummary();
  updateQueueDetails();
}

void MainWindow::applyQueueFilter() {
  if (!inputList_) return;
  const QString needle =
      queueFilterEdit_ ? queueFilterEdit_->text().trimmed() : QString();
  int visible = 0;
  int firstVisible = -1;
  for (int i = 0; i < inputList_->count(); ++i) {
    QListWidgetItem* item = inputList_->item(i);
    if (!item) continue;
    const QString path = item->data(Qt::UserRole).toString();
    const bool match =
        needle.isEmpty() || item->text().contains(needle, Qt::CaseInsensitive) ||
        path.contains(needle, Qt::CaseInsensitive);
    item->setHidden(!match);
    if (match) {
      ++visible;
      if (firstVisible < 0) firstVisible = i;
    }
  }
  // Never leave the selection on a hidden row: the preview and the facts strip
  // would describe a file the user cannot see.
  QListWidgetItem* current = inputList_->currentItem();
  if (current && current->isHidden() && firstVisible >= 0) {
    inputList_->setCurrentRow(firstVisible);
  }
  if (visible == 0 && !needle.isEmpty()) {
    queueSelectionDetails_->setText(
        QStringLiteral("No queued file matches \"%1\"").arg(needle));
  } else {
    updateQueueDetails();
  }
  refreshQueueLabel();
}

void MainWindow::chooseInputs() {
  const auto files = QFileDialog::getOpenFileNames(
      this, QStringLiteral("Open GIF files"), QString(),
      QStringLiteral("GIF files (*.gif *.GIF);;All files (*)"));
  if (!files.isEmpty()) appendInputs(files);
}

void MainWindow::chooseInputFolder() {
  const QString dir = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Add every GIF in a folder"), QString());
  if (dir.isEmpty()) return;
  appendInputFolder(dir);
}

void MainWindow::appendInputFolder(const QString& rootDir) {
  const QString cleanRoot = cleanDirPath(rootDir);
  if (cleanRoot.isEmpty() || !QFileInfo(cleanRoot).isDir()) return;

  // N-63: walk sub-folders deterministically so Keep folder structure can
  // recreate in/a/b/c.gif -> <dest>/a/b/c_result.gif. Every entry still passes
  // through appendInputs() -> gs::admit_input() (bytes decide, never suffix).
  QStringList candidates;
  QDirIterator it(cleanRoot, QDir::Files | QDir::Readable,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString path = it.next();
    const QFileInfo entry(path);
    if (entry.suffix().compare(QStringLiteral("gif"), Qt::CaseInsensitive) == 0) {
      candidates << QDir::cleanPath(entry.absoluteFilePath());
    }
  }
  std::sort(candidates.begin(), candidates.end(), [](const QString& a, const QString& b) {
    return QString::compare(a, b, Qt::CaseInsensitive) < 0;
  });
  if (candidates.isEmpty()) {
    updateStatus(QStringLiteral("No .gif files in %1").arg(QDir::toNativeSeparators(cleanRoot)));
    return;
  }
  for (const QString& f : candidates) {
    inputRoots_.insert(cleanFilePath(f), cleanRoot);
  }
  appendInputs(candidates);
  // Keep inputRoots_ tidy: drop any candidate that gs::admit_input rejected.
  QSet<QString> queuedClean;
  for (const QString& f : inputs_) queuedClean.insert(cleanFilePath(f));
  for (const QString& f : candidates) {
    const QString cf = cleanFilePath(f);
    if (!queuedClean.contains(cf)) inputRoots_.remove(cf);
  }
  refreshCommand();
  refreshOutputSummary();
}

void MainWindow::onFilesDropped(const QStringList& files) {
  appendInputs(files);
}

void MainWindow::removeSelected() {
  const auto selected = inputList_->selectedItems();
  if (selected.isEmpty()) return;
  QList<int> rows;
  for (QListWidgetItem* item : selected) {
    const int row = inputList_->row(item);
    if (row >= 0) rows.append(row);
  }
  std::sort(rows.begin(), rows.end(), std::greater<int>());
  lastOutputPaths_.clear();
  for (int row : rows) {
    if (row >= 0 && row < inputs_.size()) {
      inputRoots_.remove(cleanFilePath(inputs_.at(row)));
      manualOrder_.removeAll(inputs_.at(row));
      inputs_.removeAt(row);
    }
    delete inputList_->takeItem(row);
  }
  runButton_->setEnabled(!busy_ && !inputs_.isEmpty() && ensureEngine());
  refreshQueueLabel();
  updateStatus(QStringLiteral("%1 GIF file(s) in queue.").arg(inputs_.size()));
  refreshTemplateNote();
  refreshCommand();
  refreshOutputSummary();
  onSelectionChanged();
}

void MainWindow::clearQueue() {
  lastOutputPaths_.clear();
  inputs_.clear();
  inputList_->clear();
  factsCache_.clear();
  thumbCache_.clear();
  inputRoots_.clear();
  manualOrder_.clear();
  if (queueFilterEdit_) {
    QSignalBlocker blocker(queueFilterEdit_);
    queueFilterEdit_->clear();
  }
  runButton_->setEnabled(false);
  refreshQueueLabel();
  updateStatus(QStringLiteral("Queue cleared."));
  refreshTemplateNote();
  refreshCommand();
  refreshOutputSummary();
  invalidatePreview();
  cleanupPreviewFiles();
  lastPreviewPath_.clear();
  previewPanel_->setBefore(QString());
  previewPanel_->clearAfter(QStringLiteral("queue empty"));
}

void MainWindow::chooseBatchDir() {
  if (busy_) return;
  const auto dir = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Choose output folder"), batchDirEdit_->text());
  if (!dir.isEmpty()) {
    batchDirEdit_->setText(dir);
    if (destFolderRadio_) destFolderRadio_->setChecked(true);
  }
}

QString MainWindow::batchDir() const {
  if (destFolderRadio_ && !destFolderRadio_->isChecked()) return {};
  return batchDirEdit_ ? batchDirEdit_->text().trimmed() : QString();
}

bool MainWindow::destinationIncomplete() const {
  return destFolderRadio_ && destFolderRadio_->isChecked() &&
         (!batchDirEdit_ || batchDirEdit_->text().trimmed().isEmpty());
}

QString MainWindow::targetDirForInput(const QString& input) const {
  const QFileInfo fi(input);
  if (!destFolderRadio_ || !destFolderRadio_->isChecked()) {
    return fi.absolutePath();
  }
  const QString base = batchDirEdit_ ? batchDirEdit_->text().trimmed() : QString();
  if (base.isEmpty()) return {};
  if (keepStructureCheck_ && keepStructureCheck_->isChecked()) {
    const QString cleanIn = cleanFilePath(input);
    const auto it = inputRoots_.constFind(cleanIn);
    if (it != inputRoots_.constEnd() && !it.value().isEmpty()) {
      const std::string rel =
          gs::relative_under_root(cleanIn.toStdString(), it.value().toStdString());
      const std::string relDir = gs::relative_dir_part(rel);
      if (!relDir.empty()) {
        return QDir::fromNativeSeparators(
            QString::fromStdString(gs::join_path(base.toStdString(), relDir)));
      }
    }
  }
  return base;
}

void MainWindow::openOutputFolder() {
  QString dir = batchDir();
  if (dir.isEmpty()) {
    if (!lastOutputPaths_.isEmpty()) {
      dir = QFileInfo(lastOutputPaths_.front()).absolutePath();
    } else if (!inputs_.isEmpty()) {
      dir = QFileInfo(inputs_.front()).absolutePath();
    }
  }
  if (dir.isEmpty()) return;
  QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void MainWindow::openSelectedOutput() {
  if (!outputFiles_) return;
  auto* item = outputFiles_->currentItem();
  if (!item) return;
  const QString path = item->data(Qt::UserRole).toString();
  if (path.isEmpty()) return;
  QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

gs::NameContext MainWindow::templateContextFor(const QString& input,
                                               int index,
                                               gs::OutputFormat fmt,
                                               std::time_t now) const {
  const QFileInfo fi(input);
  gs::NameContext ctx;
  ctx.filename = fi.completeBaseName().toStdString();
  ctx.extension = fi.suffix().toLower().toStdString();
  ctx.directory = QDir::toNativeSeparators(fi.absolutePath()).toStdString();
  ctx.folder_name = QFileInfo(fi.absolutePath()).fileName().toStdString();
  ctx.format = gs::output_format_name(fmt);
  ctx.index = index;
  ctx.start_index = startIndexSpin_ ? startIndexSpin_->value() : 1;
  if (const std::tm* lt = std::localtime(&now)) {
    ctx.now = *lt;
  }

  const auto it = factsCache_.constFind(input);
  const gs::GifFacts facts =
      (it != factsCache_.constEnd()) ? it.value() : gs::read_gif_facts(input.toStdString());
  if (facts.ok) {
    ctx.width = static_cast<unsigned>(std::max(0, facts.width));
    ctx.height = static_cast<unsigned>(std::max(0, facts.height));
    ctx.dst_width = ctx.width;
    ctx.dst_height = ctx.height;
    ctx.frames = facts.frames;
    ctx.duration_seconds = static_cast<double>(facts.duration_ms) / 1000.0;
  }
  if (settingsPanel_) {
    gs::Settings s;
    settingsPanel_->writeInto(s);
    if (s.resize_kind != gs::ResizeKind::None) {
      if (s.resize_kind == gs::ResizeKind::Scale && ctx.width > 0 && ctx.height > 0) {
        if (s.scale_x > 0.0)
          ctx.dst_width = std::max(1u, static_cast<unsigned>(ctx.width * s.scale_x + 0.5));
        if (s.scale_y > 0.0)
          ctx.dst_height = std::max(1u, static_cast<unsigned>(ctx.height * s.scale_y + 0.5));
      } else {
        if (s.resize_w > 0) ctx.dst_width = s.resize_w;
        if (s.resize_h > 0) ctx.dst_height = s.resize_h;
      }
    }
  }
  return ctx;
}

QString MainWindow::renderedOutputName(const QFileInfo& fi, int index) const {
  const QString rawTmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  const std::string tmpl = rawTmpl.isEmpty() ? std::string(kDefaultNameTemplate)
                                             : rawTmpl.toStdString();
  const gs::OutputFormat fmt = selectedOutputFormat();
  const gs::NameContext ctx =
      templateContextFor(fi.filePath(), index, fmt, std::time(nullptr));
  const gs::NameRender rendered = gs::render_name_template(tmpl, ctx);
  std::string safeStem = gs::sanitize_output_name(rendered.text, gs::NameRules::Host);
  if (safeStem.empty()) {
    const std::string fallbackBase = fi.completeBaseName().toStdString();
    safeStem = fallbackBase.empty() ? std::string("output_result")
                                    : (fallbackBase + "_result");
    safeStem = gs::sanitize_output_name(safeStem, gs::NameRules::Host);
    if (safeStem.empty()) safeStem = "output_result";
  }
  return QString::fromStdString(safeStem) + QLatin1Char('.') +
         QString::fromLatin1(gs::output_format_extension(fmt));
}

QString MainWindow::singleTargetOutput() const {
  if (inputs_.isEmpty() || destinationIncomplete()) return {};
  const QString dir = targetDirForInput(inputs_.front());
  if (dir.isEmpty()) return {};
  return dir + QLatin1Char('/') + renderedOutputName(QFileInfo(inputs_.front()), 0);
}

QString MainWindow::explodeTargetPrefix() const {
  if (inputs_.isEmpty() || destinationIncomplete()) return {};
  const QFileInfo fi(inputs_.front());
  const QString dir = targetDirForInput(inputs_.front());
  if (dir.isEmpty()) return {};
  const QString rawTmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  // Preserve the cross-surface <stem>_frame.NNN contract (U-76 / T7) when the
  // filename template is untouched; honour custom stems when edited.
  if (rawTmpl.isEmpty() || rawTmpl == QString::fromUtf8(kDefaultNameTemplate)) {
    const std::string base = fi.completeBaseName().toStdString();
    std::string safe = gs::sanitize_output_name(
        (base.empty() ? std::string("output") : base) + "_frame", gs::NameRules::Host);
    if (safe.empty()) safe = "output_frame";
    return dir + QLatin1Char('/') + QString::fromStdString(safe);
  }
  const gs::NameContext ctx =
      templateContextFor(inputs_.front(), 0, gs::OutputFormat::Gif, std::time(nullptr));
  const gs::NameRender rendered = gs::render_name_template(rawTmpl.toStdString(), ctx);
  std::string safe = gs::sanitize_output_name(rendered.text, gs::NameRules::Host);
  if (safe.empty()) safe = "output_frame";
  return dir + QLatin1Char('/') + QString::fromStdString(safe);
}

QStringList MainWindow::plannedOutputPaths() const {
  if (inputs_.isEmpty() || destinationIncomplete()) return {};
  const gs::Mode mode = settingsPanel_->mode();
  if (mode == gs::Mode::Batch) return plannedBatchTargets();
  if (mode == gs::Mode::Explode) {
    if (inputs_.size() > 1) return {};
    const QString pfx = explodeTargetPrefix();
    return pfx.isEmpty() ? QStringList{} : QStringList{pfx + QStringLiteral(".*")};
  }
  const QString single = singleTargetOutput();
  return single.isEmpty() ? QStringList{} : QStringList{single};
}

void MainWindow::refreshOutputFiles() {
  if (!outputFiles_) return;
  outputFiles_->clear();

  const QStringList paths = lastOutputPaths_.isEmpty() ? plannedOutputPaths() : lastOutputPaths_;
  if (paths.isEmpty()) {
    auto* item = new QListWidgetItem(QStringLiteral("No output planned yet."), outputFiles_);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    if (openSelectedOutputButton_) openSelectedOutputButton_->setEnabled(false);
    return;
  }

  bool anyExisting = false;
  for (const QString& path : paths) {
    const bool prefix = path.endsWith(QStringLiteral(".*"));
    const QString actualPath = prefix ? path.left(path.size() - 2) : path;
    const QFileInfo fi(actualPath);
    QString label;
    if (prefix) {
      label = QStringLiteral("Planned frame prefix: %1").arg(actualPath);
    } else if (fi.exists() && fi.isFile()) {
      label = QStringLiteral("✓ %1  ·  %2").arg(fi.fileName(), humanSize(fi.size()));
      anyExisting = true;
    } else {
      label = QStringLiteral("Planned: %1").arg(path);
    }
    auto* item = new QListWidgetItem(label, outputFiles_);
    item->setData(Qt::UserRole, actualPath);
    item->setToolTip(path);
    if (!prefix && fi.exists() && fi.isFile())
      item->setForeground(QBrush(QColor(80, 190, 150)));
  }
  if (openSelectedOutputButton_)
    openSelectedOutputButton_->setEnabled(anyExisting);
  if (outputFiles_->count() > 0 && anyExisting) outputFiles_->setCurrentRow(0);
}

QStringList MainWindow::plannedBatchTargets() const {
  if (inputs_.isEmpty() || destinationIncomplete()) return {};

  const QString rawTmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  const std::string tmpl = rawTmpl.isEmpty() ? std::string(kDefaultNameTemplate)
                                             : rawTmpl.toStdString();
  const gs::OutputFormat fmt = selectedOutputFormat();
  const std::string extWithDot = std::string(".") + gs::output_format_extension(fmt);
  const std::time_t now = std::time(nullptr);

  // Render stems per file and group by normalized destination directory so
  // intra-folder duplicates become _1, _2 case-insensitively (OD-23 / N-61).
  QVector<QString> dirs(inputs_.size());
  std::map<std::string, std::vector<int>> indicesByDir;
  std::map<std::string, std::vector<std::string>> namesByDir;

  for (int i = 0; i < inputs_.size(); ++i) {
    const QString input = inputs_.at(i);
    const QFileInfo fi(input);
    const QString dir = targetDirForInput(input);
    dirs[i] = dir;
    const gs::NameContext ctx = templateContextFor(input, i, fmt, now);
    const gs::NameRender rendered = gs::render_name_template(tmpl, ctx);
    std::string safeStem = gs::sanitize_output_name(rendered.text, gs::NameRules::Host);
    if (safeStem.empty()) {
      const std::string fallbackBase = fi.completeBaseName().toStdString();
      safeStem = fallbackBase.empty() ? std::string("output_result")
                                      : (fallbackBase + "_result");
      safeStem = gs::sanitize_output_name(safeStem, gs::NameRules::Host);
      if (safeStem.empty()) safeStem = "output_result";
    }
    const std::string dirKey = gs::path_key(dir.toStdString());
    indicesByDir[dirKey].push_back(i);
    namesByDir[dirKey].push_back(safeStem + extWithDot);
  }

  QVector<QString> resolved(inputs_.size());
  for (auto& kv : indicesByDir) {
    const std::string& dirKey = kv.first;
    const std::vector<int>& slotIndices = kv.second;
    const std::vector<std::string> numbered =
        gs::disambiguate_names(namesByDir[dirKey]);
    for (std::size_t k = 0; k < slotIndices.size() && k < numbered.size(); ++k) {
      const int idx = slotIndices[k];
      resolved[idx] = dirs[idx] + QLatin1Char('/') + QString::fromStdString(numbered[k]);
    }
  }

  QStringList targets;
  targets.reserve(inputs_.size());
  for (const QString& t : resolved) targets << t;
  return targets;
}

gs::OutputPlan MainWindow::planBatch() const {
  std::vector<std::string> in;
  std::vector<std::string> out;
  in.reserve(static_cast<size_t>(inputs_.size()));
  out.reserve(static_cast<size_t>(inputs_.size()));
  for (const QString& p : inputs_) in.push_back(p.toStdString());
  const QStringList targets = plannedBatchTargets();
  for (const QString& p : targets) out.push_back(p.toStdString());
  return gs::plan_outputs(in, out);
}

bool MainWindow::templateIsConstant() const {
  const QString tmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  if (tmpl.isEmpty()) return false;
  return gs::template_is_constant(tmpl.toStdString());
}

QString MainWindow::defaultOutputFor(const QString& input) const {
  const QFileInfo fi(input);
  const QString dir = targetDirForInput(input);
  return dir + QLatin1Char('/') + renderedOutputName(fi, 0);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched == nameTemplateEdit_ && event) {
    if (event->type() == QEvent::FocusOut) {
      if (tokenPopup_) tokenPopup_->hide();
    } else if (event->type() == QEvent::KeyPress && tokenPopup_ && tokenPopup_->isVisible()) {
      auto* ke = static_cast<QKeyEvent*>(event);
      const int count = tokenPopup_->count();
      if (ke->key() == Qt::Key_Down && count > 0) {
        const int next = (tokenPopup_->currentRow() + 1) % count;
        tokenPopup_->setCurrentRow(next);
        return true;
      }
      if (ke->key() == Qt::Key_Up && count > 0) {
        const int prev = (tokenPopup_->currentRow() - 1 + count) % count;
        tokenPopup_->setCurrentRow(prev);
        return true;
      }
      if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter ||
          ke->key() == Qt::Key_Tab) {
        acceptSelectedToken();
        return true;
      }
      if (ke->key() == Qt::Key_Escape) {
        tokenPopup_->hide();
        return true;
      }
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::updateTokenPopup() {
  if (!nameTemplateEdit_ || !tokenPopup_ || busy_) return;
  const QString text = nameTemplateEdit_->text();
  const int cursor = nameTemplateEdit_->cursorPosition();
  const QString before = text.left(cursor);
  const int openBrace = before.lastIndexOf(QLatin1Char('{'));
  const int closeBrace = before.lastIndexOf(QLatin1Char('}'));
  if (openBrace < 0 || closeBrace > openBrace) {
    tokenPopup_->hide();
    return;
  }
  const QString query = before.mid(openBrace + 1).trimmed();

  struct Ranked {
    int rank;
    QString token;
    QString label;
  };
  QVector<Ranked> matches;
  for (const gs::TemplateToken& info : gs::template_tokens()) {
    const QString inner = QString::fromUtf8(info.name);
    const QString tok = QStringLiteral("{%1}").arg(inner);
    const QString desc = QString::fromUtf8(info.summary);
    int rank = -1;
    if (query.isEmpty()) {
      rank = 0;
    } else if (inner.startsWith(query, Qt::CaseInsensitive)) {
      rank = 0;
    } else if (inner.contains(query, Qt::CaseInsensitive) ||
               desc.contains(query, Qt::CaseInsensitive)) {
      rank = 1;
    }
    if (rank >= 0) {
      matches.append({rank, tok, QStringLiteral("%1  —  %2").arg(tok, desc)});
    }
  }
  std::stable_sort(matches.begin(), matches.end(),
                   [](const Ranked& a, const Ranked& b) { return a.rank < b.rank; });

  if (matches.isEmpty()) {
    tokenPopup_->hide();
    return;
  }

  tokenPopup_->clear();
  for (const Ranked& m : matches) {
    auto* item = new QListWidgetItem(m.label, tokenPopup_);
    item->setData(Qt::UserRole, m.token);
  }
  tokenPopup_->setCurrentRow(0);
  const int width = std::max(340, nameTemplateEdit_->width());
  tokenPopup_->setFixedWidth(width);
  tokenPopup_->move(nameTemplateEdit_->mapToGlobal(QPoint(0, nameTemplateEdit_->height() + 2)));
  tokenPopup_->show();
}

void MainWindow::acceptSelectedToken() {
  if (!nameTemplateEdit_ || !tokenPopup_) return;
  QListWidgetItem* item = tokenPopup_->currentItem();
  if (!item && tokenPopup_->count() > 0) item = tokenPopup_->item(0);
  if (!item) {
    tokenPopup_->hide();
    return;
  }
  const QString chosen = item->data(Qt::UserRole).toString();
  const QString text = nameTemplateEdit_->text();
  const int cursor = nameTemplateEdit_->cursorPosition();
  const int openBrace = text.left(cursor).lastIndexOf(QLatin1Char('{'));
  if (openBrace < 0) {
    tokenPopup_->hide();
    return;
  }
  const QString updated = text.left(openBrace) + chosen + text.mid(cursor);
  const int newCursor = openBrace + chosen.size();
  nameTemplateEdit_->setText(updated);
  nameTemplateEdit_->setCursorPosition(newCursor);
  tokenPopup_->hide();
  nameTemplateEdit_->setFocus();
}

void MainWindow::refreshTemplateNote() {
  if (!templateNoteLabel_ || !nameTemplateEdit_) return;
  const QString raw = nameTemplateEdit_->text().trimmed();
  const std::string tmpl = raw.isEmpty() ? std::string(kDefaultNameTemplate)
                                         : raw.toStdString();
  const gs::OutputFormat fmt = selectedOutputFormat();
  const QString ext = QString::fromLatin1(gs::output_format_extension(fmt));

  gs::NameContext sampleCtx;
  if (!inputs_.isEmpty()) {
    sampleCtx = templateContextFor(inputs_.front(), 0, fmt, std::time(nullptr));
  } else {
    sampleCtx.filename = "clip";
    sampleCtx.extension = "gif";
    sampleCtx.directory = "/folder";
    sampleCtx.folder_name = "folder";
    sampleCtx.format = gs::output_format_name(fmt);
    sampleCtx.index = 0;
    sampleCtx.start_index = startIndexSpin_ ? startIndexSpin_->value() : 1;
    sampleCtx.width = 320;
    sampleCtx.height = 240;
    sampleCtx.dst_width = 320;
    sampleCtx.dst_height = 240;
    sampleCtx.frames = 12;
    sampleCtx.duration_seconds = 1.20;
    const std::time_t now = std::time(nullptr);
    if (const std::tm* lt = std::localtime(&now)) sampleCtx.now = *lt;
  }
  const gs::NameRender sample = gs::render_name_template(tmpl, sampleCtx);
  std::string safeStem = gs::sanitize_output_name(sample.text, gs::NameRules::Host);
  if (safeStem.empty()) safeStem = "clip_result";
  QStringList notes;
  notes << QStringLiteral("Preview: %1.%2").arg(QString::fromStdString(safeStem), ext);
  for (const std::string& w : sample.warnings) {
    notes << QString::fromStdString(w);
  }
  if (inputs_.size() > 1 && gs::template_is_constant(tmpl)) {
    notes << QStringLiteral("Output filename is constant — duplicates will be numbered (_1, _2, …).");
  }
  templateNoteLabel_->setText(notes.join(QStringLiteral("  ·  ")));
}

void MainWindow::setCollisionPolicy(gs::CollisionPolicy policy) {
  collisionPolicy_ = policy;
  refreshCollisionButton();
  refreshOutputSummary();
}

void MainWindow::refreshCollisionButton() {
  if (!collisionButton_) return;
  const QString label = QString::fromUtf8(gs::collision_policy_name(collisionPolicy_));
  collisionButton_->setText(label + QStringLiteral("  \u21c5"));
  if (collisionMenu_) {
    const QString token = QString::fromUtf8(gs::collision_policy_token(collisionPolicy_));
    for (QAction* act : collisionMenu_->actions()) {
      act->setChecked(act->data().toString() == token);
    }
  }
}

void MainWindow::refreshSizeGuardState() {
  if (!discardLargerCheck_) return;
  const bool sameFmt =
      gs::size_guard_applicable(gs::OutputFormat::Gif, selectedOutputFormat());
  discardLargerCheck_->setEnabled(!busy_ && sameFmt);
}

MainWindow::AskCollisionOutcome MainWindow::promptCollisionAsk(
    const QString& sourcePath, const QString& targetPath, bool allowApplyAll) {
  AskCollisionOutcome out;
  QDialog dlg(this);
  dlg.setObjectName(QStringLiteral("collisionAskDialog"));
  dlg.setWindowTitle(QStringLiteral("Output file already exists"));
  auto* lay = new QVBoxLayout(&dlg);

  auto* msg = new QLabel(
      QStringLiteral("Destination already has a file at:\n%1\n\n"
                     "Source file: %2\n\n"
                     "Choose how to handle this file:")
          .arg(QDir::toNativeSeparators(targetPath),
               QFileInfo(sourcePath).fileName()),
      &dlg);
  msg->setObjectName(QStringLiteral("collisionAskLabel"));
  msg->setWordWrap(true);
  msg->setTextInteractionFlags(Qt::TextSelectableByMouse);
  lay->addWidget(msg);

  auto* applyAll = new QCheckBox(
      QStringLiteral("Apply this choice to all remaining existing files in this run"),
      &dlg);
  applyAll->setObjectName(QStringLiteral("askApplyAllCheck"));
  applyAll->setVisible(allowApplyAll);
  lay->addWidget(applyAll);

  auto* btnRow = new QHBoxLayout();
  auto* renameBtn = new QPushButton(QStringLiteral("Rename (numbered)"), &dlg);
  renameBtn->setObjectName(QStringLiteral("askRenameButton"));
  auto* overwriteBtn = new QPushButton(QStringLiteral("Overwrite"), &dlg);
  overwriteBtn->setObjectName(QStringLiteral("askOverwriteButton"));
  auto* skipBtn = new QPushButton(QStringLiteral("Skip"), &dlg);
  skipBtn->setObjectName(QStringLiteral("askSkipButton"));
  auto* cancelBtn = new QPushButton(QStringLiteral("Cancel run"), &dlg);
  cancelBtn->setObjectName(QStringLiteral("askCancelButton"));

  btnRow->addWidget(renameBtn);
  btnRow->addWidget(overwriteBtn);
  btnRow->addWidget(skipBtn);
  btnRow->addStretch();
  btnRow->addWidget(cancelBtn);
  lay->addLayout(btnRow);

  skipBtn->setDefault(true);

  connect(renameBtn, &QPushButton::clicked, &dlg, [&]() {
    out.policy = gs::CollisionPolicy::Rename;
    out.applyToAll = applyAll->isChecked();
    dlg.accept();
  });
  connect(overwriteBtn, &QPushButton::clicked, &dlg, [&]() {
    out.policy = gs::CollisionPolicy::Overwrite;
    out.applyToAll = applyAll->isChecked();
    dlg.accept();
  });
  connect(skipBtn, &QPushButton::clicked, &dlg, [&]() {
    out.policy = gs::CollisionPolicy::Skip;
    out.applyToAll = applyAll->isChecked();
    dlg.accept();
  });
  connect(cancelBtn, &QPushButton::clicked, &dlg, [&]() {
    out.cancelled = true;
    dlg.reject();
  });

  if (dlg.exec() != QDialog::Accepted) {
    out.cancelled = true;
  }
  return out;
}

void MainWindow::moveCurrent(int delta) {
  if (busy_) return;
  const int row = inputList_->currentRow();
  if (row < 0 || row >= inputs_.size()) return;
  const int target = row + delta;
  if (target < 0 || target >= inputs_.size()) return;
  lastOutputPaths_.clear();
  QListWidgetItem* item = inputList_->takeItem(row);
  if (!item) return;
  const int manualRow = manualOrder_.indexOf(inputs_.at(row));
  const int manualTarget = manualOrder_.indexOf(inputs_.at(target));
  if (manualRow >= 0 && manualTarget >= 0 && manualRow != manualTarget) {
    manualOrder_.move(manualRow, manualTarget);
  }
  inputList_->insertItem(target, item);
  const QString path = inputs_.takeAt(row);
  inputs_.insert(target, path);
  inputList_->setCurrentRow(target);
  refreshTemplateNote();
  refreshCommand();
  refreshOutputSummary();
}

void MainWindow::refreshOutputSummary() {
  if (!outputSummaryLabel_) return;
  const gs::Mode mode = settingsPanel_->mode();
  if (destinationIncomplete()) {
    outputSummaryLabel_->setText(
        QStringLiteral("Destination → Folder REQUIRED: choose a destination folder "
                       "(or switch Output to Source folder)"));
    refreshOutputFiles();
    return;
  }
  QString text;
  switch (mode) {
    case gs::Mode::Batch: {
      const QString tmpl =
          nameTemplateEdit_ && !nameTemplateEdit_->text().trimmed().isEmpty()
              ? nameTemplateEdit_->text().trimmed()
              : QString::fromUtf8(kDefaultNameTemplate);
      if (inputs_.isEmpty()) {
        text = QStringLiteral("Batch → each input writes \"%1.%2\"%3")
                   .arg(tmpl,
                        QString::fromLatin1(gs::output_format_extension(selectedOutputFormat())),
                        batchDir().isEmpty() ? QStringLiteral(" in its source folder")
                                             : QStringLiteral(" into %1").arg(batchDir()));
      } else {
        const QStringList targets = plannedBatchTargets();
        const gs::OutputPlan plan = planBatch();
        if (!plan.ok) {
          text = QStringLiteral("Batch (%1 files) → the run will be refused: %2")
                     .arg(inputs_.size())
                     .arg(QString::fromStdString(plan.describe()));
        } else {
          const QString first = targets.value(0);
          const QString last = targets.value(targets.size() - 1);
          text = (inputs_.size() == 1 || first == last)
                     ? QStringLiteral("Batch (1 file) → %1").arg(first)
                     : QStringLiteral("Batch (%1 files) → %2 … %3")
                           .arg(inputs_.size())
                           .arg(first, last);
          if (!plan.preexisting.empty()) {
            text += QStringLiteral("  (%1 target(s) already exist — policy: %2)")
                        .arg(plan.preexisting.size())
                        .arg(QString::fromUtf8(gs::collision_policy_name(collisionPolicy_)));
          }
        }
      }
      break;
    }
    case gs::Mode::Merge: {
      if (inputs_.isEmpty()) {
        text = QStringLiteral("Merge → queue GIFs to weld into one animation");
      } else {
        const QString out = singleTargetOutput();
        std::vector<std::string> planIn;
        for (const QString& p : inputs_) planIn.push_back(p.toStdString());
        const gs::OutputPlan onePlan = gs::plan_outputs(planIn, {out.toStdString()});
        text = !onePlan.ok
                   ? QStringLiteral("Merge → the run will be refused: %1")
                         .arg(QString::fromStdString(onePlan.describe()))
                   : QStringLiteral("Merge (all inputs welded) → %1").arg(out);
      }
      break;
    }
    case gs::Mode::Explode:
      if (inputs_.size() > 1) {
        text = QStringLiteral("Explode → REFUSED for %1 queued files: one file per run "
                              "(the engine would scatter the others' frames into the CWD)")
                   .arg(inputs_.size());
        break;
      }
      text = inputs_.isEmpty()
                 ? QStringLiteral("Explode → frames write as <stem>_frame.000, .001, …")
                 : QStringLiteral("Explode → frames write as %1.000, .001, …")
                       .arg(explodeTargetPrefix());
      break;
    case gs::Mode::Auto: {
      if (inputs_.isEmpty()) {
        text = QStringLiteral("Auto → queue a GIF to generate an output path");
      } else {
        const QString out = singleTargetOutput();
        std::vector<std::string> planIn;
        for (const QString& p : inputs_) planIn.push_back(p.toStdString());
        const gs::OutputPlan onePlan = gs::plan_outputs(planIn, {out.toStdString()});
        text = !onePlan.ok
                   ? QStringLiteral("Auto → the run will be refused: %1")
                         .arg(QString::fromStdString(onePlan.describe()))
                   : QStringLiteral("Auto → %1").arg(out);
      }
      break;
    }
  }
  outputSummaryLabel_->setText(text);
  refreshOutputFiles();
}

gs::Settings MainWindow::currentSettings() const {
  gs::Settings s;
  settingsPanel_->writeInto(s);

  s.inputs.clear();
  s.inputs.reserve(static_cast<size_t>(inputs_.size()));
  for (const auto& in : inputs_) s.inputs.push_back(in.toStdString());

  if (!inputs_.isEmpty() && !destinationIncomplete()) {
    if (s.mode == gs::Mode::Explode) {
      if (inputs_.size() == 1) s.output = explodeTargetPrefix().toStdString();
    } else if (s.mode == gs::Mode::Merge || s.mode == gs::Mode::Auto) {
      s.output = singleTargetOutput().toStdString();
    }
  }
  return s;
}

void MainWindow::refreshCommand() {
  if (!commandPane_) return;
  const QString engine = QString::fromStdString(
      gs::shell_quote(enginePath_.toStdString()));

  auto settings = currentSettings();
  const gs::OutputFormat fmt = selectedOutputFormat();

  if (settings.mode == gs::Mode::Batch && !settings.inputs.empty()) {
    QStringList lines;
    const int n = static_cast<int>(settings.inputs.size());
    if (n > 1) {
      lines << QStringLiteral("# batch: one engine command per file (%1 files)")
                   .arg(n);
    }
    const QStringList planned = plannedBatchTargets();
    const int kMaxShown = 20;
    for (int i = 0; i < n; ++i) {
      if (i == kMaxShown && n > kMaxShown) {
        lines << QStringLiteral("# … and %1 more files (same command pattern)")
                     .arg(n - kMaxShown);
        break;
      }
      gs::Settings one = settings;
      one.mode = gs::Mode::Auto;
      const QString in = QString::fromStdString(settings.inputs[i]);
      one.inputs = {in.toStdString()};
      const QString out = planned.value(i, defaultOutputFor(in));
      one.output = out.toStdString();
      if (gs::output_format_needs_ffmpeg(fmt)) {
        const gs::FfmpegEncodeRequest req =
            buildFfmpegRequest(in, out, fmt, ffmpegPrefs_);
        lines << shellishJoin(ffmpegPath_, gs::ffmpeg_encode_args(req));
      } else {
        gs::GifsicleCommand cmd(one);
        lines << engine + QLatin1Char(' ') + QString::fromStdString(cmd.toString());
      }
    }
    commandPane_->setPlainText(lines.join(QLatin1Char('\n')));
    return;
  }

  if (gs::output_format_needs_ffmpeg(fmt)) {
    const QString staged = QString::fromStdString(
        gs::ffmpeg_staging_name(settings.output, fmt));
    const gs::FfmpegEncodeRequest req =
        buildFfmpegRequest(inputs_.value(0), staged, fmt, ffmpegPrefs_);
    commandPane_->setPlainText(shellishJoin(ffmpegPath_, gs::ffmpeg_encode_args(req)));
    return;
  }
  gs::GifsicleCommand cmd(settings);
  commandPane_->setPlainText(
      engine + QLatin1Char(' ')
      + QString::fromStdString(cmd.toString()));
}

// ---------------------------------------------------------------------------
// Status tab result rows (W-32 / slice A)
//
// Cardinality follows the contract: Batch installs one row per queued file,
// Merge/Auto one row whose Before is the SUM of its inputs, Explode one row
// whose After is the SUM of its frames. Byte figures appear only on a
// `succeeded` row - the contract's honesty rule.
// ---------------------------------------------------------------------------
void MainWindow::refreshTabTitles() {
  if (!tabs_) return;
  tabs_->setTabText(0, QStringLiteral("Input: %1 file(s)").arg(inputs_.size()));
  if (settingsPanel_ && actionsTabIndex_ >= 0) {
    tabs_->setTabText(actionsTabIndex_,
                      QStringLiteral("Actions [%1/%2]")
                          .arg(settingsPanel_->enabledActionCount())
                          .arg(settingsPanel_->actionCount()));
  }
}

void MainWindow::beginResultRows(const QStringList& files) {
  if (!resultTable_) return;
  resultTable_->setRowCount(0);
  resultBeforeBytes_.clear();
  resultActiveRow_ = -1;
  if (files.isEmpty()) {
    refreshResultSummary();
    return;
  }
  resultTable_->setRowCount(files.size());
  for (int i = 0; i < files.size(); ++i) {
    const QFileInfo fi(files.at(i));
    const qint64 bytes = fi.exists() ? fi.size() : -1;
    resultBeforeBytes_.append(bytes);
    auto* name = new QTableWidgetItem(fi.fileName());
    name->setToolTip(files.at(i));
    resultTable_->setItem(i, 0, name);
    resultTable_->setItem(i, 1, new QTableWidgetItem(bytes >= 0 ? humanSize(bytes)
                                                               : QStringLiteral("—")));
    for (int c = 2; c <= 5; ++c) resultTable_->setItem(i, c, new QTableWidgetItem(QString()));
    resultTable_->item(i, 4)->setText(QStringLiteral("pending"));
  }
  refreshResultSummary();
}

void MainWindow::beginSingleResultRow(const QString& label, qint64 beforeBytes) {
  if (!resultTable_) return;
  resultTable_->setRowCount(0);
  resultBeforeBytes_.clear();
  resultActiveRow_ = 0;
  resultTable_->setRowCount(1);
  resultTable_->setItem(0, 0, new QTableWidgetItem(label));
  resultTable_->setItem(0, 1, new QTableWidgetItem(beforeBytes >= 0 ? humanSize(beforeBytes)
                                                                    : QStringLiteral("—")));
  for (int c = 2; c <= 5; ++c) resultTable_->setItem(0, c, new QTableWidgetItem(QString()));
  resultBeforeBytes_.append(beforeBytes);
  resultTable_->item(0, 4)->setText(QStringLiteral("pending"));
  refreshResultSummary();
}

void MainWindow::setResultState(int row, const QString& state, const QString& detail,
                                qint64 afterBytes) {
  if (!resultTable_ || row < 0 || row >= resultTable_->rowCount()) return;
  auto* stateItem = resultTable_->item(row, 4);
  if (stateItem) stateItem->setText(state);
  auto* detailItem = resultTable_->item(row, 5);
  if (detailItem) detailItem->setText(detail);
  // The contract's rule: a row that did not succeed shows NO byte figures.
  if (state == QStringLiteral("succeeded") && afterBytes >= 0) {
    if (auto* after = resultTable_->item(row, 2)) after->setText(humanSize(afterBytes));
    const qint64 before = row < resultBeforeBytes_.size() ? resultBeforeBytes_.at(row) : -1;
    if (auto* change = resultTable_->item(row, 3)) {
      if (before > 0 && afterBytes >= 0) {
        const double pct = 100.0 * static_cast<double>(afterBytes - before) /
                           static_cast<double>(before);
        change->setText(QStringLiteral("%1%2%")
                            .arg(afterBytes == before ? QString() : (afterBytes > before ? QStringLiteral("+") : QStringLiteral("−")),
                                 QString::number(qAbs(pct), 'f', 1)));
      } else {
        change->setText(QStringLiteral("—"));
      }
    }
  }
  refreshResultSummary();
}

void MainWindow::markRemainingSkipped(int fromRow) {
  if (!resultTable_) return;
  for (int r = std::max(0, fromRow); r < resultTable_->rowCount(); ++r) {
    auto* st = resultTable_->item(r, 4);
    if (!st) continue;
    if (st->text() == QStringLiteral("pending") || st->text() == QStringLiteral("running")) {
      st->setText(QStringLiteral("skipped"));
      if (auto* d = resultTable_->item(r, 5))
        d->setText(QStringLiteral("not attempted - the run stopped before this file"));
    }
  }
  refreshResultSummary();
}

void MainWindow::refreshResultSummary() {
  if (!resultTable_) return;
  int succeeded = 0, failed = 0, skipped = 0, cancelled = 0, done = 0;
  const int total = resultTable_->rowCount();
  for (int r = 0; r < total; ++r) {
    auto* st = resultTable_->item(r, 4);
    if (!st) continue;
    const QString s = st->text();
    if (s == QStringLiteral("succeeded")) { ++succeeded; ++done; }
    else if (s == QStringLiteral("failed")) { ++failed; ++done; }
    else if (s == QStringLiteral("skipped")) { ++skipped; ++done; }
    else if (s == QStringLiteral("cancelled")) { ++cancelled; ++done; }
  }
  if (resultHeaderLabel_) {
    resultHeaderLabel_->setText(QStringLiteral("Processing list: %1 of %2 files")
                                    .arg(done).arg(total));
  }
  if (resultSummaryLabel_) {
    resultSummaryLabel_->setText(
        total == 0
            ? QStringLiteral("No run yet.")
            : QStringLiteral("%1 succeeded · %2 failed · %3 skipped · %4 cancelled")
                  .arg(succeeded).arg(failed).arg(skipped).arg(cancelled));
  }
}

void MainWindow::setBusy(bool busy) {
  busy_ = busy;
  // Audit U-35 / P1-22: leaving busy re-enables Run only after re-checking
  // the engine, exactly like appendInputs/removeSelected do — an engine that
  // vanished mid-run must not light Run back up. The && short-circuits, so
  // ensureEngine() never runs when ENTERING busy.
  runButton_->setEnabled(!busy && !inputs_.isEmpty() && ensureEngine());
  cancelButton_->setEnabled(busy);
  if (cancelButton_) cancelButton_->setText(busy ? QStringLiteral("Stop")
                                                 : QStringLiteral("Cancel"));
  removeButton_->setEnabled(!busy);
  clearButton_->setEnabled(!busy);
  moveUpButton_->setEnabled(!busy);
  moveDownButton_->setEnabled(!busy);
  if (addFolderButton_) addFolderButton_->setEnabled(!busy);
  if (queueFilterEdit_) queueFilterEdit_->setEnabled(!busy);
  for (QToolButton* button : sortButtons_) {
    if (button) button->setEnabled(!busy);
  }
  settingsPanel_->setEnabled(!busy);
  // Audit U-45 / P1-23: lock the WHOLE Output tab while a run is in flight.
  if (destSourceRadio_) destSourceRadio_->setEnabled(!busy);
  if (destFolderRadio_) destFolderRadio_->setEnabled(!busy);
  if (batchDirEdit_) batchDirEdit_->setEnabled(!busy);
  if (batchDirBrowse_) batchDirBrowse_->setEnabled(!busy);
  if (nameTemplateEdit_) nameTemplateEdit_->setEnabled(!busy);
  if (tokenMenuButton_) tokenMenuButton_->setEnabled(!busy);
  if (startIndexSpin_) startIndexSpin_->setEnabled(!busy);
  if (formatCombo_) formatCombo_->setEnabled(!busy);
  if (formatSettingsButton_) formatSettingsButton_->setEnabled(!busy);
  if (collisionButton_) collisionButton_->setEnabled(!busy);
  if (keepStructureCheck_) keepStructureCheck_->setEnabled(!busy);
  refreshSizeGuardState();
  if (busy && tokenPopup_) tokenPopup_->hide();

  progressBar_->setVisible(busy);
  if (busy) {
    if (batchMode_ == gs::Mode::Batch && !batchQueue_.isEmpty()) {
      progressBar_->setRange(0, batchQueue_.size());
      progressBar_->setValue(std::max(0, batchIndex_));
      progressBar_->setFormat(QStringLiteral("File %v of %m  ·  %p%"));
    } else {
      progressBar_->setRange(0, 0);
      progressBar_->setFormat(QStringLiteral("Working…"));
    }
    previewTimer_->stop();
    invalidatePreview();
    killPreview();
  }
}

void MainWindow::runCommand() {
  if (busy_) return;
  if (!ensureEngine()) {
    QMessageBox::critical(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Could not find the GIF engine (gifsicle).\n"
                       "Build it with ./scripts/build_engine.sh\n"
                       "or set the GS_ENGINE environment variable.\n"
                       "Looked for: %1").arg(enginePath_));
    return;
  }
  if (inputs_.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
                         QStringLiteral("Add at least one GIF file to the queue."));
    return;
  }
  if (destinationIncomplete()) {
    QMessageBox::warning(
        this, QStringLiteral("Gifscythe"),
        QStringLiteral("Choose a destination folder on the Output tab "
                       "(or switch Output back to Source folder) before running."));
    return;
  }

  auto settings = currentSettings();
  auto warnings = gs::validate(settings);
  warnings.erase(std::remove_if(warnings.begin(), warnings.end(),
      [](const gs::Warning& w) {
        return w.field == "input";
      }), warnings.end());
  if (!warnings.empty()) {
    QStringList msgs;
    for (const auto& w : warnings) {
      msgs << QStringLiteral("%1 (%2): %3")
                  .arg(QString::fromStdString(w.field),
                       QString::fromStdString(w.value),
                       QString::fromStdString(w.reason));
    }
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Some settings may not work as intended:\n%1")
            .arg(msgs.join(QLatin1Char('\n'))));
    return;
  }

  // Freeze output format, FFmpeg settings, size guard, and root folder for the
  // entire run (same discipline as U-58's batchSettings_).
  activeFormat_ = selectedOutputFormat();
  activeFfmpegPrefs_ = ffmpegPrefs_;
  activeDiscardLarger_ = discardLargerCheck_ && discardLargerCheck_->isChecked();
  activeDestRoot_ = (destFolderRadio_ && destFolderRadio_->isChecked() && batchDirEdit_)
                        ? batchDirEdit_->text().trimmed()
                        : QString();
  activeRootExists_ = !activeDestRoot_.isEmpty() && QFileInfo(activeDestRoot_).isDir();

  if (gs::output_format_needs_ffmpeg(activeFormat_)) {
    refreshFfmpegStatus(true);
    if (!ffmpegLocation_.found()) {
      QMessageBox::critical(
          this, QStringLiteral("Gifscythe"),
          QStringLiteral("%1 needs FFmpeg, and none was found.\n\n%2\n\n"
                         "GIF still works: pick GIF, or put an ffmpeg next to this program "
                         "and try again.")
              .arg(QString::fromLatin1(gs::output_format_name(activeFormat_)),
                   QString::fromStdString(gs::ffmpeg_location_summary(ffmpegLocation_))));
      return;
    }
    if (settings.mode != gs::Mode::Auto && settings.mode != gs::Mode::Batch) {
      QMessageBox::warning(
          this, QStringLiteral("Gifscythe"),
          QStringLiteral("%1 output is available for single-file and Batch runs only.\n\n"
                         "Merge and Explode work on GIF frames and stay GIF: switch the output "
                         "format back to GIF, or use a single file / a batch.")
              .arg(QString::fromLatin1(gs::output_format_name(activeFormat_))));
      return;
    }
  }

  batchMode_ = settings.mode;
  batchSkipMask_.clear();
  batchDiscardedMask_.clear();
  resetRunLog();
  lastOutputPaths_.clear();
  refreshOutputFiles();
  appendLog(QStringLiteral("Starting %1 run with %2 input file(s).")
                .arg(settings.mode == gs::Mode::Batch ? QStringLiteral("Batch")
                     : settings.mode == gs::Mode::Merge ? QStringLiteral("Merge")
                     : settings.mode == gs::Mode::Explode ? QStringLiteral("Explode")
                                                          : QStringLiteral("Auto"))
                .arg(inputs_.size()));

  if (settings.mode == gs::Mode::Batch) {
    // ---- Plan EVERY output before the first process starts (audit U-01) ----
    const gs::OutputPlan plan = planBatch();
    if (!plan.ok) {
      beginResultRows(inputs_);
      for (int r = 0; r < resultTable_->rowCount(); ++r)
        setResultState(r, QStringLiteral("refusedBeforeRun"),
                       QStringLiteral("the planned output would overwrite user data"));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Refusing to run — the planned output would overwrite user data:\n\n%1")
              .arg(QString::fromStdString(plan.describe())));
      return;
    }
    batchTargets_ = plannedBatchTargets();
    batchSkipMask_.fill(false, inputs_.size());
    batchDiscardedMask_.fill(false, inputs_.size());

    // N-63: resolve existing-file collisions up front according to the chosen
    // policy (Ask / Rename / Overwrite / Skip) before any process runs.
    auto existsOnDisk = [](const std::string& p) -> bool {
      std::error_code ec;
      return std::filesystem::symlink_status(gs::u8path_compat(p), ec).type() !=
                 std::filesystem::file_type::not_found &&
             !ec;
    };
    int existingCount = 0;
    for (const QString& t : batchTargets_) {
      if (existsOnDisk(t.toStdString())) ++existingCount;
    }
    std::set<std::string> baseInputsLower;
    for (const QString& p : inputs_) baseInputsLower.insert(gs::path_key(p.toStdString()));

    bool askAllActive = false;
    gs::CollisionPolicy askAllPolicy = gs::CollisionPolicy::Skip;
    std::set<std::string> finalizedLower = baseInputsLower;

    for (int i = 0; i < inputs_.size(); ++i) {
      QString cand = batchTargets_.at(i);
      if (existsOnDisk(cand.toStdString())) {
        gs::CollisionPolicy eff = collisionPolicy_;
        if (eff == gs::CollisionPolicy::Ask) {
          if (askAllActive) {
            eff = askAllPolicy;
          } else {
            const AskCollisionOutcome out =
                promptCollisionAsk(inputs_.at(i), cand, existingCount > 1);
            if (out.cancelled) {
              updateStatus(QStringLiteral("Cancelled before run — no files written."));
              return;
            }
            eff = out.policy;
            if (out.applyToAll) {
              askAllActive = true;
              askAllPolicy = eff;
            }
          }
        }
        if (eff == gs::CollisionPolicy::Skip) {
          batchSkipMask_[i] = true;
          continue;
        }
        if (eff == gs::CollisionPolicy::Rename) {
          std::set<std::string> reserved = finalizedLower;
          for (int j = i + 1; j < batchTargets_.size(); ++j) {
            reserved.insert(gs::path_key(batchTargets_.at(j).toStdString()));
          }
          const gs::CollisionDecision dec = gs::decide_collision(
              cand.toStdString(), gs::CollisionPolicy::Rename,
              [&](const std::string& probe) {
                return existsOnDisk(probe) ||
                       reserved.count(gs::path_key(probe)) > 0;
              });
          if (dec.action == gs::CollisionAction::Skip) {
            batchSkipMask_[i] = true;
            continue;
          }
          cand = QString::fromStdString(dec.final_path);
          batchTargets_[i] = cand;
        }
      }
      finalizedLower.insert(gs::path_key(cand.toStdString()));
    }

    beginResultRows(inputs_);
    int firstIdx = -1;
    for (int i = 0; i < inputs_.size(); ++i) {
      if (batchSkipMask_.at(i)) {
        const QString fullLoc = QDir::toNativeSeparators(batchTargets_.at(i));
        setResultState(i, QStringLiteral("skipped"),
                       QStringLiteral("skipped, file already exists — %1").arg(fullLoc));
        appendLog(QStringLiteral("Skipped %1 (skipped, file already exists: %2).")
                      .arg(QFileInfo(inputs_.at(i)).fileName(), fullLoc));
      } else if (firstIdx < 0) {
        firstIdx = i;
      }
    }

    if (firstIdx < 0) {
      batchQueue_.clear();
      batchIndex_ = -1;
      resultActiveRow_ = -1;
      refreshOutputFiles();
      updateStatus(QStringLiteral("Skipped %1 existing file(s) — nothing written.")
                       .arg(inputs_.size()));
      return;
    }

    batchQueue_ = inputs_;
    batchIndex_ = firstIdx;
    explodeSnapshot_.clear();
    setBusy(true);
    resultActiveRow_ = firstIdx;
    setResultState(firstIdx, QStringLiteral("running"), QStringLiteral("optimizing…"));
    updateStatus(QStringLiteral("Optimizing %1/%2…").arg(firstIdx + 1).arg(batchQueue_.size()));
    QString in = batchQueue_.at(firstIdx);
    pendingOutput_ = batchTargets_.value(firstIdx);
    if (activeRootExists_) {
      QDir().mkpath(QFileInfo(pendingOutput_).absolutePath());
    }
    pendingPartial_ = QString::fromStdString(
        gs::claim_partial_output_path(pendingOutput_.toStdString()));
    if (pendingPartial_.isEmpty()) {
      batchQueue_.clear();
      batchIndex_ = -1;
      setBusy(false);
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Batch stopped — no free staging path beside %1").arg(pendingOutput_));
      return;
    }
    partialSnapshot_ = gs::snapshot_output(pendingPartial_.toStdString());
    // U-58 / P1-38: freeze the settings for the WHOLE batch at start —
    // every continuation job (onProcessFinished) builds its argv from this
    // snapshot, never from a live currentSettings() re-read.
    batchSettings_ = settings;
    gs::Settings one = settings;
    one.mode = gs::Mode::Auto;
    one.inputs = {in.toStdString()};
    one.output = pendingPartial_.toStdString();
    QString encodeError;
    if (!configureEncoder(one, in, pendingPartial_, activeFormat_, &encodeError)) {
      discardPendingPartial();
      pendingPartial_.clear();
      batchQueue_.clear();
      batchIndex_ = -1;
      setBusy(false);
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("encoderUnavailable: %1").arg(encodeError.left(120)));
      resultActiveRow_ = -1;
      QMessageBox::critical(this, QStringLiteral("Gifscythe"), encodeError);
      return;
    }
    appendLog(QStringLiteral("Optimizing %1/%2: %3")
                  .arg(batchIndex_ + 1).arg(batchQueue_.size()).arg(in));
    process_->start();
    if (!process_->waitForStarted(5000)) {
      discardPendingPartial();
      pendingPartial_.clear();
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("engineNotStartable: %1").arg(process_->errorString()));
      markRemainingSkipped(resultActiveRow_ + 1);
      resultActiveRow_ = -1;
      setBusy(false);
      QMessageBox::critical(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Could not start the GIF engine: %1").arg(process_->errorString()));
      return;
    }
    return;
  }

  // Merge / Explode / Auto: single run.
  if (settings.mode == gs::Mode::Explode && inputs_.size() > 1) {
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Explode operates on one GIF at a time (%1 files queued).").arg(inputs_.size()));
    return;
  }
  if (settings.output.empty()) {
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Choose an output destination before running."));
    return;
  }

  // Plan the single target too (audit U-01).
  if (settings.mode != gs::Mode::Explode && !settings.output.empty()) {
    std::vector<std::string> plan_inputs;
    plan_inputs.reserve(static_cast<size_t>(inputs_.size()));
    for (const QString& p : inputs_) plan_inputs.push_back(p.toStdString());
    const gs::OutputPlan one_plan = gs::plan_outputs(plan_inputs, {settings.output});
    if (!one_plan.ok) {
      beginSingleResultRow(QFileInfo(QString::fromStdString(settings.output)).fileName(), -1);
      setResultState(0, QStringLiteral("refusedBeforeRun"),
                     QStringLiteral("the planned output would overwrite user data"));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Refusing to run — the planned output would overwrite user data:\n\n%1")
              .arg(QString::fromStdString(one_plan.describe())));
      return;
    }

    // N-63 collision policy for single-output modes (Merge / Auto).
    auto existsOnDiskSingle = [](const std::string& p) -> bool {
      std::error_code ec;
      return std::filesystem::symlink_status(gs::u8path_compat(p), ec).type() !=
                 std::filesystem::file_type::not_found &&
             !ec;
    };
    if (existsOnDiskSingle(settings.output)) {
      gs::CollisionPolicy eff = collisionPolicy_;
      const QString outQ = QString::fromStdString(settings.output);
      if (eff == gs::CollisionPolicy::Ask) {
        const AskCollisionOutcome out = promptCollisionAsk(inputs_.front(), outQ, false);
        if (out.cancelled) {
          updateStatus(QStringLiteral("Cancelled before run — no files written."));
          return;
        }
        eff = out.policy;
      }
      if (eff == gs::CollisionPolicy::Skip) {
        qint64 sum = 0;
        bool any = false;
        for (const QString& f : inputs_) {
          const QFileInfo fi(f);
          if (fi.exists()) { sum += fi.size(); any = true; }
        }
        beginSingleResultRow(QFileInfo(outQ).fileName(), any ? sum : -1);
        const QString fullLoc = QDir::toNativeSeparators(outQ);
        setResultState(0, QStringLiteral("skipped"),
                       QStringLiteral("skipped, file already exists — %1").arg(fullLoc));
        resultActiveRow_ = -1;
        updateStatus(QStringLiteral("Skipped existing file — nothing written."));
        return;
      }
      if (eff == gs::CollisionPolicy::Rename) {
        std::set<std::string> reserved;
        for (const QString& p : inputs_) reserved.insert(gs::path_key(p.toStdString()));
        const gs::CollisionDecision dec = gs::decide_collision(
            settings.output, gs::CollisionPolicy::Rename,
            [&](const std::string& probe) {
              return existsOnDiskSingle(probe) ||
                     reserved.count(gs::path_key(probe)) > 0;
            });
        settings.output = dec.final_path;
      }
    }
  }

  pendingOutput_ = QString::fromStdString(settings.output);
  if (activeRootExists_) {
    QDir().mkpath(QFileInfo(pendingOutput_).absolutePath());
  }
  if (settings.mode != gs::Mode::Explode) {
    pendingPartial_ = QString::fromStdString(
        gs::claim_partial_output_path(pendingOutput_.toStdString()));
    if (pendingPartial_.isEmpty()) {
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Refusing to run — no free staging path beside %1").arg(pendingOutput_));
      return;
    }
    partialSnapshot_ = gs::snapshot_output(pendingPartial_.toStdString());
    settings.output = pendingPartial_.toStdString();
  } else {
    pendingPartial_ = QString::fromStdString(
        gs::claim_partial_explode_prefix(pendingOutput_.toStdString()));
    if (pendingPartial_.isEmpty()) {
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Refusing to run — no free staging prefix beside %1").arg(pendingOutput_));
      return;
    }
    settings.output = pendingPartial_.toStdString();
  }
  batchIndex_ = -1;
  batchQueue_.clear();
  if (settings.mode == gs::Mode::Explode) {
    explodeSnapshot_ = gs::snapshot_explode_candidates(pendingPartial_.toStdString());
  } else {
    explodeSnapshot_.clear();
  }

  {
    qint64 beforeBytes = -1;
    if (batchMode_ == gs::Mode::Explode) {
      const QFileInfo in(inputs_.value(0));
      beforeBytes = in.exists() ? in.size() : -1;
      beginSingleResultRow(in.fileName() + QStringLiteral(" (all frames)"), beforeBytes);
    } else {
      qint64 sum = 0;
      bool any = false;
      for (const QString& f : inputs_) {
        const QFileInfo fi(f);
        if (fi.exists()) { sum += fi.size(); any = true; }
      }
      beforeBytes = any ? sum : -1;
      const QString label = QFileInfo(pendingOutput_).fileName() +
                            (inputs_.size() > 1
                                 ? QStringLiteral(" (merged from %1 files)").arg(inputs_.size())
                                 : QString());
      beginSingleResultRow(label, beforeBytes);
    }
    resultActiveRow_ = 0;
    setResultState(0, QStringLiteral("running"), QStringLiteral("optimizing…"));
  }
  {
    QString encodeError;
    if (!configureEncoder(settings, inputs_.value(0), pendingPartial_, activeFormat_,
                          &encodeError)) {
      discardPendingPartial();
      pendingPartial_.clear();
      setBusy(false);
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("encoderUnavailable: %1").arg(encodeError.left(120)));
      resultActiveRow_ = -1;
      QMessageBox::critical(this, QStringLiteral("Gifscythe"), encodeError);
      return;
    }
  }
  setBusy(true);
  updateStatus(QStringLiteral("Running…"));
  appendLog(QStringLiteral("Optimizing %1 → %2")
                .arg(QString::fromStdString(settings.inputs.empty() ? std::string() : settings.inputs.front()),
                     QString::fromStdString(settings.output)));
  process_->start();
  if (!process_->waitForStarted(5000)) {
    discardPendingPartial();
    pendingPartial_.clear();
    setResultState(resultActiveRow_, QStringLiteral("failed"),
                   QStringLiteral("engineNotStartable: %1").arg(process_->errorString()));
    resultActiveRow_ = -1;
    setBusy(false);
    QMessageBox::critical(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Could not start the GIF engine: %1").arg(process_->errorString()));
    return;
  }
}

void MainWindow::discardPendingPartial() {
  if (pendingPartial_.isEmpty()) return;
  if (batchMode_ == gs::Mode::Explode)
    gs::discard_explode_frames(pendingPartial_.toStdString());
  else
    gs::discard_partial(pendingPartial_.toStdString());
}

void MainWindow::cancelRun() {
  appendLog(QStringLiteral("Cancellation requested."));
  const bool engineWasRunning =
      (process_ && process_->state() != QProcess::NotRunning);
  if (engineWasRunning) {
    cancelling_ = true;
    process_->kill();
    process_->waitForFinished(3000);
  }

  if (process_ && process_->state() == QProcess::NotRunning &&
      process_->exitStatus() == QProcess::NormalExit &&
      process_->exitCode() == 0 && !pendingPartial_.isEmpty()) {
    setBusy(false);
    updateStatus(QStringLiteral("Run had already finished - result kept."));
    return;
  }

  discardPendingPartial();
  pendingPartial_.clear();
  batchQueue_.clear();
  batchIndex_ = -1;
  invalidatePreview();
  setBusy(false);
  if (resultActiveRow_ >= 0) {
    setResultState(resultActiveRow_, QStringLiteral("cancelled"), QStringLiteral("stopped by the user"));
    markRemainingSkipped(resultActiveRow_ + 1);
    resultActiveRow_ = -1;
  }
  updateStatus(QStringLiteral("Cancelled."));
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
  captureProcessOutput();
  const QString err = processStderr_;

  const bool wasCancelling = cancelling_;
  cancelling_ = false;

  if (status != QProcess::NormalExit || exitCode != 0) {
    if (wasCancelling) {
      return;
    }
    discardPendingPartial();
    pendingPartial_.clear();
    setBusy(false);
    batchQueue_.clear();
    batchIndex_ = -1;
    appendLog(QStringLiteral("Engine failed with exit code %1.").arg(exitCode));
    if (resultActiveRow_ >= 0) {
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("engineFailed: exit %1").arg(exitCode));
      markRemainingSkipped(resultActiveRow_ + 1);
      resultActiveRow_ = -1;
    }
    updateStatus(QStringLiteral("Optimization failed (exit %1).").arg(exitCode));
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        err.isEmpty() ? QStringLiteral("The GIF engine returned an error (exit %1).").arg(exitCode)
                      : err);
    return;
  }

  if (batchMode_ == gs::Mode::Explode) {
    const gs::ExplodeResult vr =
        gs::verify_explode_frames(pendingPartial_.toStdString(), explodeSnapshot_);
    explodeSnapshot_.clear();
    if (!vr.ok) {
      discardPendingPartial();
      pendingPartial_.clear();
      setBusy(false);
      batchQueue_.clear();
      batchIndex_ = -1;
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("outputMissing: the engine exited 0 but wrote no frame this run"));
      resultActiveRow_ = -1;
      updateStatus(QStringLiteral("No frames produced — engine exited 0 but wrote none."));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QString::fromStdString(vr.describe()));
      return;
    }
    const std::string perr = gs::promote_explode_frames(
        pendingPartial_.toStdString(), pendingOutput_.toStdString());
    if (!perr.empty()) {
      discardPendingPartial();
      pendingPartial_.clear();
      setBusy(false);
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("outputUnverifiable: the verified frames could not be promoted"));
      resultActiveRow_ = -1;
      updateStatus(QStringLiteral("Explode failed while replacing frames."));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Could not promote the exploded frames: %1\n\n"
                         "Some frames under %2 may have been replaced.")
              .arg(QString::fromStdString(perr), pendingOutput_));
      return;
    }
    const QString partialBase = QFileInfo(pendingPartial_).fileName();
    const QString outBase = QFileInfo(pendingOutput_).fileName();
    pendingPartial_.clear();
    lastOutputPaths_.clear();
    const QString frameDir = QFileInfo(pendingOutput_).absolutePath();
    for (const std::string& frame : vr.frames) {
      QString name = QString::fromStdString(frame);
      if (name.startsWith(partialBase + QLatin1Char('.')))
        name = outBase + name.mid(partialBase.size());
      lastOutputPaths_ << QDir(frameDir).filePath(name);
    }
    refreshOutputFiles();
    setBusy(false);
    appendLog(QStringLiteral("Explode verified %1 frame(s) on disk.").arg(vr.frames.size()));
    {
      qint64 frameBytes = 0;
      for (const QString& p : lastOutputPaths_) frameBytes += QFileInfo(p).size();
      setResultState(resultActiveRow_, QStringLiteral("succeeded"),
                     QStringLiteral("%1 frame(s) verified on disk").arg(lastOutputPaths_.size()),
                     frameBytes);
      resultActiveRow_ = -1;
    }
    updateStatus(QStringLiteral("Explode complete — %1 frame(s) written.")
                     .arg(vr.frames.size()));
    previewPanel_->clearAfter(QStringLiteral("Explode wrote frame files; see Output for the folder and names"));
    return;
  }

  // Verify the isolated write before replacing the user's previous output.
  bool discardedLarger = false;
  if (!pendingOutput_.isEmpty()) {
    const std::string verificationError =
        gs::verify_output_as(pendingPartial_.toStdString(), partialSnapshot_, activeFormat_);
    if (!verificationError.empty()) {
      discardPendingPartial();
      pendingPartial_.clear();
      setBusy(false);
      batchQueue_.clear();
      batchIndex_ = -1;
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("outputInvalid: %1")
                         .arg(QString::fromStdString(verificationError).left(160)));
      resultActiveRow_ = -1;
      updateStatus(QStringLiteral("Output verification failed."));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("The encoder exited 0, but its output was rejected:\n%1\n\n%2")
              .arg(pendingOutput_, QString::fromStdString(verificationError)));
      return;
    }

    // N-64: same-format size guard. Only applies when source format == output
    // format (GIF -> GIF), after output verification and before promotion.
    const qint64 beforeB = (resultActiveRow_ >= 0 && resultActiveRow_ < resultBeforeBytes_.size())
                               ? resultBeforeBytes_.at(resultActiveRow_)
                               : -1;
    const qint64 partialB = QFileInfo(pendingPartial_).size();
    const gs::SizeGuardDecision sg = gs::evaluate_size_guard(
        activeDiscardLarger_,
        gs::size_guard_applicable(gs::OutputFormat::Gif, activeFormat_),
        beforeB > 0 ? static_cast<std::uintmax_t>(beforeB) : 0u,
        partialB > 0 ? static_cast<std::uintmax_t>(partialB) : 0u);
    if (sg.discard) {
      discardPendingPartial();
      pendingPartial_.clear();
      discardedLarger = true;
      if (batchIndex_ >= 0 && batchIndex_ < batchDiscardedMask_.size()) {
        batchDiscardedMask_[batchIndex_] = true;
      }
      if (resultActiveRow_ >= 0) {
        setResultState(
            resultActiveRow_, QStringLiteral("skipped"),
            QStringLiteral("discarded: result (%1) larger than original (%2)")
                .arg(humanSize(partialB), humanSize(beforeB)));
      }
      appendLog(QStringLiteral("Discarded larger result for %1 (%2 > %3).")
                    .arg(pendingOutput_, humanSize(partialB), humanSize(beforeB)));
    } else {
      const std::string promotionError =
          gs::promote_partial(pendingPartial_.toStdString(), pendingOutput_.toStdString());
      if (!promotionError.empty()) {
        const QString keptPartial = pendingPartial_;
        pendingPartial_.clear();
        setBusy(false);
        batchQueue_.clear();
        batchIndex_ = -1;
        setResultState(resultActiveRow_, QStringLiteral("failed"),
                       QStringLiteral("outputUnverifiable: the verified file could not replace the destination"));
        resultActiveRow_ = -1;
        updateStatus(QStringLiteral("Could not promote output - result kept."));
        QMessageBox::warning(this, QStringLiteral("Gifscythe"),
            QStringLiteral("The verified output could not replace the destination:\n%1\n\n%2\n\n"
                           "Your work was NOT discarded. It is saved here:\n%3")
                .arg(pendingOutput_, QString::fromStdString(promotionError), keptPartial));
        return;
      }
      pendingPartial_.clear();
    }
  }

  if (!discardedLarger && resultActiveRow_ >= 0) {
    const qint64 afterBytes = pendingOutput_.isEmpty() ? -1 : QFileInfo(pendingOutput_).size();
    setResultState(resultActiveRow_, QStringLiteral("succeeded"), QString(), afterBytes);
  }

  // Continue batch?
  if (batchIndex_ >= 0 && batchMode_ == gs::Mode::Batch) {
    ++batchIndex_;
    while (batchIndex_ < batchQueue_.size() &&
           batchIndex_ < batchSkipMask_.size() &&
           batchSkipMask_.at(batchIndex_)) {
      ++batchIndex_;
    }
    if (batchIndex_ < batchQueue_.size()) {
      if (progressBar_) progressBar_->setValue(batchIndex_);
      resultActiveRow_ = batchIndex_;
      setResultState(resultActiveRow_, QStringLiteral("running"), QStringLiteral("optimizing…"));
      appendLog(QStringLiteral("Continuing with file %1/%2: %3.")
                    .arg(batchIndex_ + 1).arg(batchQueue_.size())
                    .arg(batchQueue_.at(batchIndex_)));
      updateStatus(QStringLiteral("Optimizing %1/%2…")
                       .arg(batchIndex_ + 1)
                       .arg(batchQueue_.size()));
      QString in = batchQueue_.at(batchIndex_);
      pendingOutput_ = batchTargets_.value(batchIndex_);
      if (activeRootExists_) {
        QDir().mkpath(QFileInfo(pendingOutput_).absolutePath());
      }
      pendingPartial_ = QString::fromStdString(
          gs::claim_partial_output_path(pendingOutput_.toStdString()));
      if (pendingPartial_.isEmpty()) {
        batchQueue_.clear();
        batchIndex_ = -1;
        setResultState(resultActiveRow_, QStringLiteral("refusedBeforeRun"),
                       QStringLiteral("no free staging path beside the planned output"));
        markRemainingSkipped(resultActiveRow_ + 1);
        resultActiveRow_ = -1;
        setBusy(false);
        QMessageBox::warning(this, QStringLiteral("Gifscythe"),
            QStringLiteral("Batch stopped — no free staging path beside %1").arg(pendingOutput_));
        return;
      }
      partialSnapshot_ = gs::snapshot_output(pendingPartial_.toStdString());
      // U-58 / P1-38: continuation jobs MUST come from the batch-start
      // snapshot frozen into batchSettings_ (runCommand), never from a
      // live currentSettings().
      gs::Settings one = batchSettings_;
      one.mode = gs::Mode::Auto;
      one.inputs = {in.toStdString()};
      one.output = pendingPartial_.toStdString();
      QString encodeError;
      if (!configureEncoder(one, in, pendingPartial_, activeFormat_, &encodeError)) {
        discardPendingPartial();
        pendingPartial_.clear();
        batchQueue_.clear();
        batchIndex_ = -1;
        setBusy(false);
        setResultState(resultActiveRow_, QStringLiteral("failed"),
                       QStringLiteral("encoderUnavailable: %1").arg(encodeError.left(120)));
        resultActiveRow_ = -1;
        QMessageBox::critical(this, QStringLiteral("Gifscythe"), encodeError);
        return;
      }
      appendLog(QStringLiteral("Optimizing %1/%2: %3")
                    .arg(batchIndex_ + 1).arg(batchQueue_.size()).arg(in));
      process_->start();
      return;
    }
    // Batch complete.
    const int n = batchQueue_.size();
    QStringList written;
    for (int i = 0; i < batchTargets_.size(); ++i) {
      const bool skippedExisting = (i < batchSkipMask_.size() && batchSkipMask_.at(i));
      const bool discardedBig = (i < batchDiscardedMask_.size() && batchDiscardedMask_.at(i));
      if (!skippedExisting && !discardedBig) written << batchTargets_.at(i);
    }
    lastOutputPaths_ = written;
    resultActiveRow_ = -1;
    batchQueue_.clear();
    batchIndex_ = -1;
    if (progressBar_) progressBar_->setValue(n);
    refreshOutputFiles();
    setBusy(false);
    appendLog(QStringLiteral("Batch finished: %1 of %2 output file(s) written on disk.")
                  .arg(written.size()).arg(n));
    updateStatus(QStringLiteral("Optimization complete — %1 file(s). See Output for the verified paths.")
                     .arg(written.size()));
    showWrittenOutputPreview();
    return;
  }

  resultActiveRow_ = -1;
  if (discardedLarger) {
    lastOutputPaths_.clear();
    refreshOutputFiles();
    setBusy(false);
    updateStatus(QStringLiteral("Result discarded — output was larger than original."));
    previewPanel_->clearAfter(
        QStringLiteral("Result discarded because it was larger than the original GIF."));
    return;
  }

  lastOutputPaths_ = {pendingOutput_};
  refreshOutputFiles();
  setBusy(false);
  appendLog(QStringLiteral("Output verified and written to disk: %1").arg(pendingOutput_));
  updateStatus(QStringLiteral("Optimization complete. See Output for the verified path."));
  showWrittenOutputPreview();
}

void MainWindow::onProcessError(QProcess::ProcessError error) {
  if (error == QProcess::FailedToStart) {
    discardPendingPartial();
    pendingPartial_.clear();
    setBusy(false);
    batchQueue_.clear();
    batchIndex_ = -1;
    if (resultActiveRow_ >= 0) {
      setResultState(resultActiveRow_, QStringLiteral("failed"),
                     QStringLiteral("engineNotStartable: %1").arg(process_->errorString()));
      markRemainingSkipped(resultActiveRow_ + 1);
      resultActiveRow_ = -1;
    }
    appendLog(QStringLiteral("Could not start the GIF engine: %1").arg(process_->errorString()));
    updateStatus(QStringLiteral("Failed to start the GIF engine."));
  }
  // Crashes are also reported via finished().
}

// ===================== Preview pipeline =====================

QString MainWindow::selectedInput() const {
  const int row = inputList_->currentRow();
  if (row < 0 || row >= inputs_.size()) return {};
  return inputs_.at(row);
}

void MainWindow::onSelectionChanged() {
  forcePreview_ = false;
  const QString sel = selectedInput();
  updateQueueDetails();
  previewPanel_->setBefore(sel);
  previewPanel_->clearAfter(sel.isEmpty() ? QStringLiteral("select a file to preview")
                                          : QStringLiteral("waiting for changes…"));
  schedulePreview();
}

void MainWindow::schedulePreview() {
  // P1-10 / U-47: a selection/settings/queue change invalidates any in-flight
  // preview IMMEDIATELY — the old code advanced previewSeq_ only when a new
  // eligible preview started, so a stale completion that arrived during the
  // debounce window (or after an Explode/clear early-return) still passed the
  // seq guard and displayed a result for settings that no longer exist.
  invalidatePreview();
  // Never leave an old animation under a new setting while the debounce timer
  // is waiting. The status text has its own caption area; it cannot overlay the
  // GIF canvas.
  previewPanel_->clearAfter(QStringLiteral("waiting for changes…"));
  if (busy_) return;  // main run active; preview is replaced by the real output
  previewTimer_->start();  // restart debounce
}

void MainWindow::invalidatePreview() {
  ++previewSeq_;  // every captured-seq completion older than this is now stale
}

void MainWindow::cleanupPreviewFiles(const QString& keep) {
  // U-34 / P1-10: sweep EVERY superseded preview file, not just "seq-1 on the
  // non-stale success path" — the old rule leaked the output of every killed,
  // failed or superseded run for the whole session. `keep` is the file the
  // After pane is currently playing (QMovie reads lazily; deleting the
  // displayed file would break the animation).
  QDir d(previewDir_);
  const auto entries = d.entryList({QStringLiteral("preview_*.gif")}, QDir::Files);
  for (const auto& name : entries) {
    const QString abs = d.absoluteFilePath(name);
    if (abs != keep) QFile::remove(abs);
  }
}

void MainWindow::killPreview() {
  previewTimer_->stop();
  if (previewProcess_ && previewProcess_->state() != QProcess::NotRunning) {
    previewProcess_->kill();
    previewProcess_->waitForFinished(1000);
  }
}

void MainWindow::startPreview() {
  if (busy_) return;
  const QString input = selectedInput();
  if (input.isEmpty() || !QFileInfo::exists(input)) {
    previewPanel_->clearAfter(QStringLiteral("select an existing file to preview"));
    return;
  }
  // Re-encoding a 64 MB animation just because a spin box changed makes the
  // UI feel stuck and duplicates the real run. Large files therefore opt out
  // of automatic settings previews; the explicit Preview changes button is
  // still available when the user really wants one.
  constexpr qint64 kLargePreviewBytes = 32LL * 1024LL * 1024LL;
  if (QFileInfo(input).size() > kLargePreviewBytes && !forcePreview_) {
    previewPanel_->clearAfter(QStringLiteral(
        "large GIF: automatic preview is paused to keep the UI responsive — click Preview changes or run"));
    return;
  }
  forcePreview_ = false;
  // N-62: the After pane plays a GIF (QMovie). When the chosen output is APNG,
  // WebP or a video, any GIF shown here would be a preview of a file that will
  // never be written - so name what will happen instead of showing a decoy.
  if (gs::output_format_needs_ffmpeg(selectedOutputFormat())) {
    const gs::OutputFormat f = selectedOutputFormat();
    previewPanel_->clearAfter(
        QStringLiteral("no in-app preview for %1 — the result is written as .%2 and opens in "
                       "your own viewer")
            .arg(QString::fromLatin1(gs::output_format_name(f)),
                 QString::fromLatin1(gs::output_format_extension(f))));
    return;
  }
  // U-70 / P1-42 (F:NF-13): the preview path re-probed the engine with a
  // bare toStdString(), bypassing the u8path_compat boundary that
  // ensureEngine() wraps — the one std::string -> fs::path conversion this
  // codebase allows at API edges (it exists for legacy-ACP Windows hosts,
  // where the bare form throws or splits). One rule everywhere: every
  // fs::path boundary goes through u8path_compat.
  if (!gs::path_is_executable(gs::u8path_compat(enginePath_.toStdString()))) {
    previewPanel_->clearAfter(QStringLiteral("engine not found — preview unavailable"));
    return;
  }
  if (settingsPanel_->mode() == gs::Mode::Explode) {
    previewPanel_->clearAfter(
        QStringLiteral("preview not available in Explode mode (writes many frame files)"));
    return;
  }

  auto settings = currentSettings();
  settings.mode = gs::Mode::Auto;  // single-file re-encode, like batch does
  settings.inputs = {input.toStdString()};
  const int seq = ++previewSeq_;
  const QString outPath = previewDir_ + QStringLiteral("/preview_%1.gif").arg(seq);
  settings.output = outPath.toStdString();

  gs::GifsicleCommand cmd(settings);
  QStringList qargs;
  for (const auto& a : cmd.args()) qargs << QString::fromStdString(a);

  killPreview();  // any previous run becomes stale (its captured seq < previewSeq_)

  // Fresh QProcess per run: the finished-lambda captures THIS run's seq, so a
  // killed stale process can never be misread as the newest run's completion.
  auto* p = new QProcess(this);
  p->setObjectName(QStringLiteral("previewProcess"));
  configureHeadless(p);
  previewProcess_ = p;
  connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this, p, seq, input, outPath](int exitCode, QProcess::ExitStatus status) {
            p->readAllStandardOutput();
            p->readAllStandardError();
            p->deleteLater();
            if (previewProcess_ == p) previewProcess_ = nullptr;  // no dangling member
            if (seq != previewSeq_) {
              // Stale — superseded before completion. U-34: delete the file it
              // wrote; the old code returned here WITHOUT cleanup, so every
              // superseded preview leaked into the temp dir for the session.
              QFile::remove(outPath);
              return;
            }
            const QFileInfo outFi(outPath);
            if (status != QProcess::NormalExit || exitCode != 0 || !outFi.exists() ||
                outFi.size() == 0) {
              QFile::remove(outPath);  // U-34: never leave a failed/partial preview behind
              previewPanel_->clearAfter(
                  QStringLiteral("preview failed (engine exit %1) — check the command pane")
                      .arg(exitCode));
              return;
            }
            previewPanel_->setAfter(outPath, QFileInfo(input).size(), outFi.size());
            // U-34: sweep every older preview file (not just "seq-1"), keeping
            // only the one now on display.
            lastPreviewPath_ = outPath;
            cleanupPreviewFiles(outPath);
          });
  connect(p, &QProcess::errorOccurred, this, [this, p, seq](QProcess::ProcessError err) {
    if (err == QProcess::FailedToStart) {
      p->deleteLater();
      if (previewProcess_ == p) previewProcess_ = nullptr;
      if (seq == previewSeq_) {
        previewPanel_->clearAfter(QStringLiteral("preview could not start the engine"));
      }
    }
  });

  previewPanel_->setGenerating(true);
  p->start(enginePath_, qargs);
}

void MainWindow::appendLog(const QString& message) {
  if (!logPane_ || message.isEmpty()) return;
  QString text = message;
  text.replace(QChar('\r'), QChar());
  const QStringList lines = text.split(QChar('\n'), Qt::KeepEmptyParts);
  const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
  for (const QString& line : lines) {
    if (!line.isEmpty()) logPane_->appendPlainText(QStringLiteral("[%1] %2").arg(stamp, line));
  }
  auto* bar = logPane_->verticalScrollBar();
  if (bar) bar->setValue(bar->maximum());
}

void MainWindow::captureProcessOutput() {
  if (!process_) return;
  const QByteArray err = process_->readAllStandardError();
  const QByteArray out = process_->readAllStandardOutput();
  if (!err.isEmpty()) {
    const QString text = QString::fromLocal8Bit(err);
    processStderr_ += text;
    // Keep diagnostics bounded even if a malformed input makes the engine very
    // chatty. The final dialog still names that the log was truncated.
    constexpr int kMaxCaptured = 64 * 1024;
    if (processStderr_.size() > kMaxCaptured) {
      processStderr_.truncate(kMaxCaptured);
      if (!processStderr_.endsWith(QStringLiteral("[stderr truncated]")))
        processStderr_ += QStringLiteral("\n[stderr truncated]");
    }
    appendLog(QStringLiteral("engine: ") + text);
  }
  if (!out.isEmpty()) {
    const QString text = QString::fromLocal8Bit(out);
    processStdout_ += text;
    constexpr int kMaxCaptured = 16 * 1024;
    if (processStdout_.size() > kMaxCaptured) {
      processStdout_.truncate(kMaxCaptured);
      if (!processStdout_.endsWith(QStringLiteral("[stdout truncated]")))
        processStdout_ += QStringLiteral("\n[stdout truncated]");
    }
    appendLog(QStringLiteral("engine output: ") + text);
  }
}

void MainWindow::resetRunLog() {
  processStderr_.clear();
  processStdout_.clear();
  if (logPane_) logPane_->clear();
}

void MainWindow::showWrittenOutputPreview() {
  if (batchMode_ == gs::Mode::Explode) {
    previewPanel_->clearAfter(QStringLiteral("Explode wrote frame files; see Output for the folder and names"));
    return;
  }
  QString output;
  if (batchMode_ == gs::Mode::Batch && !batchTargets_.isEmpty()) {
    const int row = inputList_ ? inputList_->currentRow() : 0;
    const int idx = row >= 0 ? row : 0;
    const bool skipped =
        (idx < batchSkipMask_.size() && batchSkipMask_.at(idx)) ||
        (idx < batchDiscardedMask_.size() && batchDiscardedMask_.at(idx));
    if (!skipped) {
      output = batchTargets_.value(idx);
    } else if (!lastOutputPaths_.isEmpty()) {
      output = lastOutputPaths_.front();
    }
  } else {
    output = pendingOutput_;
  }
  const QFileInfo outInfo(output);
  if (!outInfo.exists() || !outInfo.isFile()) {
    previewPanel_->clearAfter(QStringLiteral("run finished, but no output file is available to preview"));
    return;
  }
  // N-62: the After pane is a GIF movie. Handing it an .mp4 would show a black
  // rectangle and look like a failed run, so it states the outcome instead.
  if (gs::output_format_needs_ffmpeg(activeFormat_)) {
    previewPanel_->clearAfter(
        QStringLiteral("%1 written (%2). No in-app preview for this format — open the file in "
                       "your own viewer.")
            .arg(QString::fromLatin1(gs::output_format_name(activeFormat_)),
                 humanSize(outInfo.size())));
    return;
  }
  const QString input = selectedInput();
  const qint64 before = input.isEmpty() ? 0 : QFileInfo(input).size();
  previewPanel_->setAfter(
      output, before, outInfo.size(),
      QStringLiteral("After = the verified file written to disk. Play/Stop only control playback."));
}

void MainWindow::updateStatus(const QString& message) {
  if (statusLabel_) statusLabel_->setText(message);
  if (previewPanel_ && !busy_) previewPanel_->setStatus(message);
}

// ===================== Session persistence (S7) =====================

QString MainWindow::sessionFilePath() const {
  const QByteArray env = qgetenv("GS_SETTINGS_PATH");
  if (!env.isEmpty()) return QString::fromLocal8Bit(env);
  const QString base =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  if (base.isEmpty()) return {};
  return QDir(base).filePath(QStringLiteral("gifscythe.conf"));
}

void MainWindow::loadSessionState() {
  const QString path = sessionFilePath();
  if (path.isEmpty()) {
    updateStatus(QStringLiteral("Settings persistence is unavailable on this system — "
                                  "changes will not be remembered between sessions."));
    return;
  }
  if (!QFileInfo::exists(path)) return;

  std::vector<gs::LoadWarning> warnings;
  std::map<std::string, std::string> guiKeys;
  auto loaded = gs::load_settings_file(path.toStdString(), &warnings, &guiKeys);
  if (!loaded) {
    updateStatus(QStringLiteral("Could not read the settings file — using defaults (%1)")
                     .arg(path));
    return;
  }

  applyGuiState(*loaded, guiKeys);

  if (!warnings.empty()) {
    updateStatus(QStringLiteral("Settings loaded with %1 warning(s) — %2")
                     .arg(warnings.size())
                     .arg(path));
  }
}

bool MainWindow::saveSessionState() {
  const QString path = sessionFilePath();
  if (path.isEmpty()) return true;

  const QString dir = QFileInfo(path).absolutePath();
  if (!QDir().mkpath(dir)) {
    qWarning("Gifscythe: cannot create settings folder %s", qPrintable(dir));
    return false;
  }

  std::ostringstream oss;
  serializeGuiState(oss, "# GUI state (Gifscythe-specific keys; SettingsIO/CLI ignore them)");

  const QByteArray data = QByteArray::fromStdString(oss.str());
  QSaveFile f(path);
  if (!f.open(QIODevice::WriteOnly)) {
    qWarning("Gifscythe: cannot open settings file for writing: %s (%s)",
             qPrintable(path), qPrintable(f.errorString()));
    return false;
  }
  if (f.write(data) != data.size()) {
    qWarning("Gifscythe: short write to settings file: %s", qPrintable(path));
    f.cancelWriting();
    return false;
  }
  if (!f.commit()) {
    qWarning("Gifscythe: cannot commit settings file: %s (%s)",
             qPrintable(path), qPrintable(f.errorString()));
    return false;
  }
  return true;
}
