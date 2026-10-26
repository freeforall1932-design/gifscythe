#include "MainWindow.h"
#include "DropListWidget.h"
#include "PreviewPanel.h"
#include "SettingsPanel.h"

#include "core/EngineLocator.h"
#include "core/OutputName.h"
#include "core/InputAdmission.h"
#include "core/OutputPlan.h"
#include "core/FfmpegCommand.h"
#include "core/FfmpegLocator.h"
#include "core/OutputVerify.h"
#include "core/SettingsIO.h"
#include "core/Validate.h"
#include "core/version.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QCloseEvent>
#include <QComboBox>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QImageReader>
#include <QKeySequence>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QSizePolicy>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QHeaderView>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <numeric>
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

// Default batch auto-name template (S3-25). {name} = the input's base name.
// The historical fixed suffix was "<name>_opt.gif" (audit E4) — the default
// template renders exactly that, so verified behavior is unchanged.
const char* kDefaultNameTemplate = "{name}_opt.gif";

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
  connect(outputEdit_, &QLineEdit::textChanged, this, [this]() {
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
  });
  connect(batchDirEdit_, &QLineEdit::textChanged, this, [this]() {
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
  });
  connect(nameTemplateEdit_, &QLineEdit::textChanged, this, [this]() {
    lastOutputPaths_.clear();
    refreshCommand();
    refreshOutputSummary();
  });
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
      QStringLiteral("Queue every GIF in a folder (one level; sub-folders are not walked).\n"
                     "Each file still passes the same admission rule as a drop."));
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

  // XNConvert-style workflow: the input tab owns the decision that determines
  // the files' names, and it owns the action that starts processing. Output is
  // reserved for destinations and verified files that already exist.
  auto* namingBox = new QGroupBox(QStringLiteral("Output naming"), page);
  namingBox->setObjectName(QStringLiteral("inputNamingGroup"));
  auto* namingForm = new QFormLayout(namingBox);
  nameTemplateEdit_ = new QLineEdit(namingBox);
  nameTemplateEdit_->setObjectName(QStringLiteral("nameTemplateEdit"));
  nameTemplateEdit_->setText(QString::fromLatin1(kDefaultNameTemplate));
  nameTemplateEdit_->setToolTip(QStringLiteral(
      "Batch auto-naming template.\n"
      "{name} = the input file's base name (required when batching >1 file).\n"
      "Path separators are stripped; \".gif\" is appended if missing."));
  namingForm->addRow(QStringLiteral("File name pattern"), nameTemplateEdit_);
  auto* tmplHint = new QLabel(QStringLiteral(
      "{name} becomes the source name. Example: {name}_optimized.gif → clip_optimized.gif. "
      "A multi-file Batch without {name} is refused to prevent overwriting results."), namingBox);
  tmplHint->setObjectName(QStringLiteral("nameTemplateHint"));
  tmplHint->setWordWrap(true);
  namingForm->addRow(QString(), tmplHint);
  left->addWidget(namingBox);

  // The primary action is NOT here: the reference puts Convert in the footer,
  // beside Close, where it is reachable from every tab (owner, 2026-10-24 -
  // the Input tab used to own it, which is not the reference's layout).
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

  auto* box = new QGroupBox(QStringLiteral("Where files are saved"), page);
  box->setObjectName(QStringLiteral("outputGroup"));
  auto* form = new QFormLayout(box);

  auto* outputRow = new QHBoxLayout();
  outputEdit_ = new QLineEdit(box);
  outputEdit_->setObjectName(QStringLiteral("outputEdit"));
  outputEdit_->setPlaceholderText(
      QStringLiteral("Save as… (required for Merge; single-file Batch honors it)"));
  outputBrowse_ = new QPushButton(QStringLiteral("Browse…"), box);
  outputBrowse_->setObjectName(QStringLiteral("browseOutputButton"));
  connect(outputBrowse_, &QPushButton::clicked, this, &MainWindow::chooseOutput);
  outputRow->addWidget(outputEdit_);
  outputRow->addWidget(outputBrowse_);
  form->addRow(QStringLiteral("Save as"), outputRow);
  auto* outputHint = new QLabel(QStringLiteral(
      "Use this for one final GIF. Merge always needs a destination; Batch uses it only "
      "when exactly one file is queued."), box);
  outputHint->setWordWrap(true);
  outputHint->setObjectName(QStringLiteral("outputSaveHint"));
  form->addRow(QString(), outputHint);

  auto* batchRow = new QHBoxLayout();
  batchDirEdit_ = new QLineEdit(box);
  batchDirEdit_->setObjectName(QStringLiteral("batchDirEdit"));
  batchDirEdit_->setPlaceholderText(
      QStringLiteral("Batch folder — default: next to each input"));
  batchDirBrowse_ = new QPushButton(QStringLiteral("Browse…"), box);
  batchDirBrowse_->setObjectName(QStringLiteral("browseBatchDirButton"));
  connect(batchDirBrowse_, &QPushButton::clicked, this, &MainWindow::chooseBatchDir);
  batchRow->addWidget(batchDirEdit_);
  batchRow->addWidget(batchDirBrowse_);
  form->addRow(QStringLiteral("Batch folder"), batchRow);

  // ---- Output form (owner, 2026-10-26) -------------------------------------
  // "Keep folder structure" is a checkbox and it starts ON: a folder added with
  // Add folder… is a tree, and flattening it silently loses the shape the user
  // already had. "If an output file already exists" is its own clickable block
  // with the four answers, ask by default.
  keepStructureCheck_ = new QCheckBox(
      QStringLiteral("Keep folder structure (sub-folders are recreated in the batch folder)"), box);
  keepStructureCheck_->setObjectName(QStringLiteral("keepStructureCheck"));
  keepStructureCheck_->setChecked(true);
  keepStructureCheck_->setToolTip(QStringLiteral(
      "On by default. Files added with Add folder… keep their sub-folder under the batch "
      "folder; a file you picked on its own is written flat, because there is no tree to keep."));
  form->addRow(QString(), keepStructureCheck_);
  auto* batchHint = new QLabel(QStringLiteral(
      "Optional folder for Batch results. Leave empty to save beside each source GIF."), box);
  batchHint->setWordWrap(true);
  batchHint->setObjectName(QStringLiteral("batchFolderHint"));
  form->addRow(QString(), batchHint);
  lay->addWidget(box);

  auto* collideBox = new QGroupBox(QStringLiteral("If an output file already exists"), page);
  collideBox->setObjectName(QStringLiteral("collisionGroup"));
  auto* collideLay = new QVBoxLayout(collideBox);
  collisionGroup_ = new QButtonGroup(collideBox);
  struct PolicySpec { const char* label; const char* why; int value; };
  const PolicySpec policies[] = {
      {"Ask each time (default)",
       "Never guess: a collision asks, and the dialog offers rename, overwrite, skip — plus "
       "\"all\" so a 200-file batch is one decision.", 0},
      {"Rename (numbered)",
       "Nothing is ever destroyed: go_result.webp, go_result_1.webp, go_result_2.webp.", 1},
      {"Overwrite",
       "Replace the old file. The replacement is written aside and verified first, so a failed "
       "run cannot leave a half-written file over the last good one.", 2},
      {"Skip",
       "Leave the existing file alone and record \"skipped, file already exists\" with its full "
       "path in the log and the tally.", 3},
  };
  for (const PolicySpec& spec : policies) {
    auto* radio = new QRadioButton(QString::fromLatin1(spec.label), collideBox);
    radio->setObjectName(QStringLiteral("collisionRadio%1").arg(spec.value));
    radio->setToolTip(QString::fromLatin1(spec.why));
    collisionGroup_->addButton(radio, spec.value);
    collideLay->addWidget(radio);
    if (spec.value == 0) radio->setChecked(true);
  }
  lay->addWidget(collideBox);

  // ---- Format (the reference's Output > Format + Settings...) --------------
  // The owner asked for this by name on 2026-10-24: "I can't change it to
  // whatever format like webp or apng or gif or video like mp4 (h265/h264/
  // vp9/av1)". The reference has exactly this control - a format combo plus a
  // per-format "Settings..." button - so the shape is settled.
  //
  // HONESTY RULE (the repo's own, OD-21): the combo must never promise an
  // encode this build cannot perform. gifsicle writes GIF and nothing else, and
  // APNG/WebP/MP4 need an FFmpeg sidecar that is NOT shipped yet, so those
  // entries are present (the user can see the plan) but DISABLED with the
  // reason in the tooltip and in the line underneath. Enabling one before the
  // encoder exists would mean a run that silently produces nothing.
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
  // The token is what the run path reads (selectedOutputFormat()), so the order
  // of this list is a presentation detail and can never change the meaning of
  // a saved session or an enabled entry.
  const char* const formatTokens[] = {"gif", "gif", "apng", "webp",
                                      "mp4-h264", "mp4-h265", "webm-vp9", "webm-av1"};
  static_assert(sizeof(formats) / sizeof(formats[0]) == sizeof(formatTokens) / sizeof(formatTokens[0]),
                "every format entry needs a token");
  // Disable an unavailable ITEM (not the whole combo) by clearing its
  // ItemIsEnabled flag in the model: the list stays readable, the entry cannot
  // be chosen, and the reason is one hover away. Qt 5/6 draw a disabled item as
  // greyed and refuse the click; Qt 4's "Qt::UserRole - 1" data trick is gone.
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
  formatForm->addRow(QStringLiteral("Output format"), formatCombo_);
  formatSettingsButton_ = new QPushButton(QStringLiteral("Settings…"), formatBox);
  formatSettingsButton_->setObjectName(QStringLiteral("formatSettingsButton"));
  formatSettingsButton_->setToolTip(
      QStringLiteral("Per-format encoder settings (the reference's Format > Settings…)"));
  connect(formatSettingsButton_, &QPushButton::clicked, this, [this]() {
    // "Where did it find FFmpeg?" is the first question anyone asks when a
    // format is greyed out, and the honest answer is a path - so this button
    // re-runs the search (people install FFmpeg while the app is open) and then
    // says exactly what it found and in which of the four places.
    refreshFfmpegStatus(true);
    const gs::OutputFormat fmt = selectedOutputFormat();
    QString body = QStringLiteral("FFmpeg: %1\n\n")
                       .arg(QString::fromStdString(gs::ffmpeg_location_summary(ffmpegLocation_)));
    if (ffmpegLocation_.found()) {
      body += QStringLiteral("Places it looked, in order: GS_FFMPEG, the copy beside this "
                             "program, PATH, then the usual install folders. The bundled copy "
                             "always wins, so a package behaves the same on every machine.\n\n");
    } else {
      body += QStringLiteral("Without FFmpeg, GIF still works (the bundled gifsicle engine) but "
                             "APNG, WebP and the video formats cannot be produced, so they stay "
                             "disabled instead of failing at the end of a run.\n\n");
    }
    if (gs::output_format_needs_ffmpeg(fmt) && ffmpegLocation_.found()) {
      body += QStringLiteral("Selected: %1 (%2). Encoder: %3.\n")
                  .arg(QString::fromLatin1(gs::output_format_name(fmt)),
                       QString::fromLatin1(gs::output_format_token(fmt)),
                       QString::fromStdString(gs::ffmpeg_encode_label(fmt)));
    } else {
      body += QStringLiteral("GIF settings live on the Actions tab, where every gifsicle option "
                             "already is (colours, lossy, dither, optimise level).");
    }
    QMessageBox::information(this, QStringLiteral("Format settings"), body);
  });
  formatForm->addRow(QString(), formatSettingsButton_);
  formatNoteLabel_ = new QLabel(formatBox);
  formatNoteLabel_->setObjectName(QStringLiteral("formatNote"));
  formatNoteLabel_->setWordWrap(true);
  formatForm->addRow(QString(), formatNoteLabel_);
  ffmpegStatusLabel_ = new QLabel(formatBox);
  ffmpegStatusLabel_->setObjectName(QStringLiteral("ffmpegStatus"));
  ffmpegStatusLabel_->setWordWrap(true);
  ffmpegStatusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  formatForm->addRow(QString(), ffmpegStatusLabel_);
  lay->addWidget(formatBox);

  // Locate the encoder NOW, before the user can pick a format, and keep the
  // combo in step whenever the selection changes.
  connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { refreshFfmpegStatus(false); });
  refreshFfmpegStatus(true);  // the first search of the session

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

void MainWindow::savePresetAs() {
  const QString dir = presetDir();
  if (dir.isEmpty()) {
    updateStatus(QStringLiteral("No writable settings folder — presets are unavailable here."));
    return;
  }
  bool ok = false;
  const QString name = QInputDialog::getText(
      this, QStringLiteral("Save preset"), QStringLiteral("Preset name:"),
      QLineEdit::Normal, presetCombo_ ? presetCombo_->currentText() : QString(), &ok);
  if (!ok || name.trimmed().isEmpty()) return;
  // Strip anything a filename cannot carry: a preset name must not become a
  // path (the same rule OutputName applies to the batch template).
  QString safe = name.trimmed();
  safe.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
  if (!QDir().mkpath(dir)) {
    updateStatus(QStringLiteral("Could not create the preset folder: %1").arg(dir));
    return;
  }
  gs::Settings s;
  // writeSessionInto: a preset records the WHOLE panel, including a card that is
  // currently switched off - otherwise loading it back would silently lose the
  // values that were sitting behind the Enabled box.
  settingsPanel_->writeSessionInto(s);
  s.inputs.clear();
  s.output.clear();
  std::ostringstream oss;
  gs::save_settings(oss, s);
  oss << "# Gifscythe preset (GUI keys are ignored by the CLI)\n";
  const QString bd = batchDir();
  if (!bd.isEmpty()) oss << "batch_dir = " << bd.toStdString() << "\n";
  const QString tmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  if (!tmpl.isEmpty() && tmpl != QString::fromLatin1(kDefaultNameTemplate))
    oss << "name_template = " << tmpl.toStdString() << "\n";
  // The output form belongs to the same state: a preset that forgets "keep
  // folder structure" or the collision answer rebuilds a different job.
  oss << "keep_structure = " << (keepStructureCheck_ && keepStructureCheck_->isChecked() ? 1 : 0) << "\n";
  oss << "collision = " << gs::collision_policy_token(collisionPolicy()) << "\n";
  // The output format is part of the naming/action state (owner: a preset must
  // remember everything that changes the result). Written only when it is not
  // the default, so old presets stay byte-identical and readable.
  const gs::OutputFormat presetFormat = selectedOutputFormat();
  if (presetFormat != gs::OutputFormat::Gif)
    oss << "output_format = " << gs::output_format_token(presetFormat) << "\n";
  // Slice D: the stack is part of the preset (presence / enabled / collapsed),
  // the way an XnConvert preset is an action list and not just a bag of numbers.
  // A flat list of ids, so a future action can be appended without a format bump.
  if (settingsPanel_) {
    const QString cards = settingsPanel_->cardState();
    if (!cards.isEmpty()) oss << "card_state = " << cards.toStdString() << "\n";
  }

  const QString path = QDir(dir).filePath(safe + QStringLiteral(".conf"));
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    updateStatus(QStringLiteral("Could not write the preset: %1").arg(path));
    return;
  }
  const QByteArray data = QByteArray::fromStdString(oss.str());
  file.write(data);
  if (!file.commit()) {
    updateStatus(QStringLiteral("Could not save the preset: %1").arg(path));
    return;
  }
  refreshPresetList();
  if (presetCombo_) {
    const int row = presetCombo_->findText(safe);
    if (row > 0) presetCombo_->setCurrentIndex(row);
  }
  updateStatus(QStringLiteral("Preset saved: %1").arg(safe));
}

void MainWindow::loadSelectedPreset() {
  if (!presetCombo_ || presetCombo_->currentIndex() <= 0) return;
  const QString name = presetCombo_->currentText();
  const QString path = QDir(presetDir()).filePath(name + QStringLiteral(".conf"));
  // load_settings_file returns nullopt for an unreadable file and collects
  // unknown keys + per-key warnings by pointer; the GUI-only batch_dir /
  // name_template keys arrive in that same map (U-36 / P3-7).
  std::vector<gs::LoadWarning> warnings;
  std::map<std::string, std::string> guiKeys;
  auto loaded = gs::load_settings_file(path.toStdString(), &warnings, &guiKeys);
  if (!loaded) {
    updateStatus(QStringLiteral("Could not read the preset %1 — it may have been removed.")
                     .arg(name));
    refreshPresetList();
    return;
  }
  settingsPanel_->readFrom(*loaded);
  auto guiKey = [&guiKeys](const char* k) -> QString {
    const auto it = guiKeys.find(k);
    return it == guiKeys.end() ? QString() : QString::fromStdString(it->second);
  };
  // Slice D: which actions are in the stack, which are on, which are folded.
  // Applied AFTER readFrom(), so the values land in the controls first and the
  // stack then decides which of them the user gets to see.
  const QString cards = guiKey("card_state");
  if (!cards.isEmpty()) settingsPanel_->setCardState(cards);

  const QString batch = guiKey("batch_dir");
  batchDirEdit_->setText(batch);
  const QString tmpl = guiKey("name_template");
  nameTemplateEdit_->setText(tmpl.isEmpty() ? QString::fromLatin1(kDefaultNameTemplate) : tmpl);
  applyOutputFormatToken(guiKey("output_format"));
  if (keepStructureCheck_) {
    const QString keep = guiKey("keep_structure");
    // Default stays ON for a file that predates the key.
    keepStructureCheck_->setChecked(keep.isEmpty() ? true : keep != QStringLiteral("0"));
  }
  if (collisionGroup_) {
    const gs::CollisionPolicy policy = gs::collision_policy_from_token(guiKey("collision").toStdString());
    const int id = policy == gs::CollisionPolicy::Rename ? 1
                 : policy == gs::CollisionPolicy::Overwrite ? 2
                 : policy == gs::CollisionPolicy::Skip ? 3 : 0;
    if (QAbstractButton* b = collisionGroup_->button(id)) b->setChecked(true);
  }

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

gs::CollisionPolicy MainWindow::collisionPolicy() const {
  if (!collisionGroup_) return gs::CollisionPolicy::Ask;
  const int id = collisionGroup_->checkedId();
  switch (id) {
    case 1: return gs::CollisionPolicy::Rename;
    case 2: return gs::CollisionPolicy::Overwrite;
    case 3: return gs::CollisionPolicy::Skip;
    default: return gs::CollisionPolicy::Ask;
  }
}

QStringList MainWindow::resolveBatchTargets(const QStringList& targets, int* skipped) {
  QStringList resolved;
  resolved.reserve(targets.size());
  if (skipped) *skipped = 0;
  const gs::CollisionPolicy policy = collisionPolicy();
  // "All" answers for the rest of THIS run, never remembered in the session: a
  // decision made while looking at one collision must not silently become the
  // user's permanent policy.
  gs::CollisionPolicy remembered = policy;
  bool have_remembered = (policy != gs::CollisionPolicy::Ask);

  const auto file_exists = [](const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(gs::u8path_compat(p), ec) &&
           !std::filesystem::is_directory(gs::u8path_compat(p), ec);
  };

  for (const QString& target : targets) {
    const std::string t = target.toStdString();
    gs::CollisionPolicy use = have_remembered ? remembered : policy;
    if (use == gs::CollisionPolicy::Ask && file_exists(t)) {
      QMessageBox box(this);
      box.setWindowTitle(QStringLiteral("File already exists"));
      box.setIcon(QMessageBox::Question);
      box.setText(QStringLiteral("%1\n\nWhat should happen?").arg(QDir::toNativeSeparators(target)));
      box.setInformativeText(QStringLiteral(
          "The path above is the exact destination. Rename never destroys anything; Overwrite "
          "replaces it only after the new file verifies; Skip leaves it exactly as it is."));
      QPushButton* renameOne = box.addButton(QStringLiteral("Rename"), QMessageBox::AcceptRole);
      QPushButton* overwriteOne = box.addButton(QStringLiteral("Overwrite"), QMessageBox::DestructiveRole);
      QPushButton* skipOne = box.addButton(QStringLiteral("Skip"), QMessageBox::RejectRole);
      box.addButton(QStringLiteral("Rename all"), QMessageBox::ActionRole);
      box.addButton(QStringLiteral("Overwrite all"), QMessageBox::ActionRole);
      box.addButton(QStringLiteral("Skip all"), QMessageBox::ActionRole);
      box.setDefaultButton(renameOne);
      box.exec();
      QAbstractButton* clicked = box.clickedButton();
      const QString label = clicked ? clicked->text() : QStringLiteral("Skip");
      if (label == QStringLiteral("Rename all")) { remembered = gs::CollisionPolicy::Rename; have_remembered = true; }
      else if (label == QStringLiteral("Overwrite all")) { remembered = gs::CollisionPolicy::Overwrite; have_remembered = true; }
      else if (label == QStringLiteral("Skip all")) { remembered = gs::CollisionPolicy::Skip; have_remembered = true; }
      else if (label == QStringLiteral("Rename")) use = gs::CollisionPolicy::Rename;
      else if (label == QStringLiteral("Overwrite")) use = gs::CollisionPolicy::Overwrite;
      else use = gs::CollisionPolicy::Skip;
      (void)renameOne; (void)overwriteOne; (void)skipOne;
    }
    const gs::CollisionDecision d = gs::decide_collision(t, use, file_exists);
    if (d.action == gs::CollisionAction::Skip) {
      // The owner's wording: the tally carries the file name AND its location.
      appendLog(QStringLiteral("%1: %2").arg(QString::fromStdString(gs::collision_action_name(d.action)),
                                             QDir::toNativeSeparators(target)));
      if (skipped) ++(*skipped);
      resolved << QString();
      continue;
    }
    const QString chosen = QString::fromStdString(d.final_path);
    if (d.action == gs::CollisionAction::Rename)
      appendLog(QStringLiteral("already exists; writing %1 instead").arg(QDir::toNativeSeparators(chosen)));
    resolved << chosen;
  }
  return resolved;
}

void MainWindow::applyOutputFormatToken(const QString& token) {
  if (!formatCombo_) return;
  if (token.isEmpty()) return;
  for (int row = 0; row < formatCombo_->count(); ++row) {
    if (formatCombo_->itemData(row, Qt::UserRole).toString() != token) continue;
    const QSignalBlocker block(formatCombo_);
    formatCombo_->setCurrentIndex(row);
    refreshFfmpegStatus(false);  // apply tooltips/notes for the restored choice
    return;
  }
  // Unknown token: leave the current selection alone rather than guessing.
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
      formatNoteLabel_->setText(
          QStringLiteral("Results will be written as .%1 (%2). The Actions-tab GIF settings do "
                         "not apply to this format; FFmpeg encodes it directly.")
              .arg(QString::fromLatin1(gs::output_format_extension(f)),
                   QString::fromStdString(gs::ffmpeg_encode_label(f))));
    } else {
      formatNoteLabel_->setText(QStringLiteral(
          "GIF is produced by the bundled gifsicle engine, with every Actions-tab setting "
          "applied. Converting a moving picture to a still one is exactly what we are not "
          "here to do."));
    }
  }
}

bool MainWindow::configureEncoder(const gs::Settings& one, const QString& input,
                                  const QString& partial, gs::OutputFormat format,
                                  QString* error) {
  QStringList qargs;
  if (!gs::output_format_needs_ffmpeg(format)) {
    // args() is non-const by design (it materialises the argv once), so this
    // command object must not be const - the compiler caught it on CI, not here.
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
  gs::FfmpegEncodeRequest request;
  request.input = input.toStdString();
  request.partial = partial.toStdString();
  request.format = format;
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
  // Slice C. The reference's "Add folder…" is a convenience for selecting a
  // batch of files, so it must not become a second, looser input path: every
  // entry goes through appendInputs(), i.e. through gs::admit_input(). One
  // level only — walking sub-folders silently would queue files the user did
  // not see, and this queue's ORDER is meaningful (Merge concatenates in it).
  const QString dir = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Add every GIF in a folder"), QString());
  if (dir.isEmpty()) return;
  const QDir folder(dir);
  QStringList files;
  const auto entries = folder.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
  for (const QFileInfo& entry : entries) {
    // The SUFFIX is only a pre-filter to keep the walk cheap; the bytes still
    // decide (a .gif that is really a JPEG is refused downstream, and a GIF
    // named oddly inside the folder is simply not picked up by this shortcut).
    if (entry.suffix().compare(QStringLiteral("gif"), Qt::CaseInsensitive) == 0) {
      files << entry.absoluteFilePath();
    }
  }
  if (files.isEmpty()) {
    updateStatus(QStringLiteral("No .gif files in %1").arg(QDir::toNativeSeparators(dir)));
    return;
  }
  if (!inputRoots_.contains(dir)) inputRoots_ << dir;
  appendInputs(files);
}

void MainWindow::onFilesDropped(const QStringList& files) {
  // Audit U-13 fixed the `|| exists(f)` half here (ANY existing file was
  // queued); GS-205 / P1-27 (S37) removes the other half's guesswork: the drop
  // path no longer filters on the SUFFIX, it hands every dropped path to the
  // same gs::admit_input() rule the picker uses. A directory, a missing file, a
  // non-GIF and a .gif that is not really a GIF all end up in the same
  // "Refused ..." feedback, and a GIF with an unusual extension is admitted —
  // the bytes decide, not the name.
  appendInputs(files);
}

void MainWindow::removeSelected() {
  const auto selected = inputList_->selectedItems();
  if (selected.isEmpty()) return;
  // Collect rows and delete highest-first so indices stay valid.
  QList<int> rows;
  for (QListWidgetItem* item : selected) {
    const int row = inputList_->row(item);
    if (row >= 0) rows.append(row);
  }
  std::sort(rows.begin(), rows.end(), std::greater<int>());
  lastOutputPaths_.clear();
  for (int row : rows) {
    if (row >= 0 && row < inputs_.size()) {
      manualOrder_.removeAll(inputs_.at(row));
      inputs_.removeAt(row);
    }
    delete inputList_->takeItem(row);
  }
  runButton_->setEnabled(!busy_ && !inputs_.isEmpty() && ensureEngine());
  refreshQueueLabel();
  updateStatus(QStringLiteral("%1 GIF file(s) in queue.").arg(inputs_.size()));
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
  manualOrder_.clear();
  if (queueFilterEdit_) {
    QSignalBlocker blocker(queueFilterEdit_);  // no filter pass over an empty list
    queueFilterEdit_->clear();
  }
  runButton_->setEnabled(false);
  refreshQueueLabel();
  updateStatus(QStringLiteral("Queue cleared."));
  refreshCommand();
  refreshOutputSummary();
  // P1-10: clearing the queue invalidates in-flight previews (their stale
  // completions must not repopulate the panes) and sweeps their temp files —
  // with nothing selected there is no displayed After image left to keep.
  invalidatePreview();
  cleanupPreviewFiles();
  lastPreviewPath_.clear();
  previewPanel_->setBefore(QString());
  previewPanel_->clearAfter(QStringLiteral("queue empty"));
}

void MainWindow::chooseOutput() {
  // Locked while a run is in flight (audit U-45 / P1-23): the planned output
  // of a running process must not be one dialog away from changing under it.
  if (busy_) return;
  const auto file = QFileDialog::getSaveFileName(
      this, QStringLiteral("Save optimized GIF"), QString(),
      QStringLiteral("GIF files (*.gif)"));
  if (!file.isEmpty()) {
    outputEdit_->setText(file);
    refreshCommand();
  }
}

void MainWindow::chooseBatchDir() {
  // Locked while a run is in flight (audit U-45 / P1-23). A picker could call
  // setText() on the DISABLED batchDirEdit_ — the running batch would keep
  // writing to its planned folder while every readout claimed the new one.
  if (busy_) return;
  const auto dir = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Choose batch output folder"), batchDirEdit_->text());
  if (!dir.isEmpty()) batchDirEdit_->setText(dir);
}

QString MainWindow::batchDir() const {
  return batchDirEdit_->text().trimmed();
}

void MainWindow::openOutputFolder() {
  QString dir = batchDir();
  if (dir.isEmpty()) {
    const QString out = outputEdit_->text().trimmed();
    if (!out.isEmpty()) {
      dir = QFileInfo(out).absolutePath();
    } else if (!lastOutputPaths_.isEmpty()) {
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

QStringList MainWindow::plannedOutputPaths() const {
  if (!inputs_.isEmpty() && settingsPanel_->mode() == gs::Mode::Batch)
    return plannedBatchTargets();

  const QString explicitOut = outputEdit_ ? outputEdit_->text().trimmed() : QString();
  if (!explicitOut.isEmpty()) return {explicitOut};
  if (!inputs_.isEmpty() && settingsPanel_->mode() == gs::Mode::Explode) {
    const QFileInfo fi(inputs_.front());
    return {fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
            + QStringLiteral("_frame.*")};
  }
  return {};
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

QString MainWindow::renderedOutputName(const QFileInfo& fi) const {
  QString tmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  if (tmpl.isEmpty()) tmpl = QString::fromLatin1(kDefaultNameTemplate);
  QString name = tmpl;
  name.replace(QStringLiteral("{name}"), fi.completeBaseName(), Qt::CaseInsensitive);
  // Honest guard, now in the core layer so it is unit-tested (audit U-21).
  // Strips any directory part on every platform (a template must never escape
  // the output folder) and, on Windows, also removes <>:"|?* and the ASCII
  // control characters, trims trailing dots/spaces that Win32 would strip
  // anyway, and defuses the reserved device names (CON, PRN, AUX, NUL,
  // COM1..9, LPT1..9). Returns empty when nothing usable is left.
  const QString safe = QString::fromStdString(
      gs::sanitize_output_name(name.toStdString(), gs::NameRules::Host));
  // Empty render (e.g. the template was only separators) falls back to the
  // historical default instead of writing a nameless/"." file.
  name = safe.isEmpty() ? fi.completeBaseName() + QStringLiteral("_opt") : safe;
  // The FORMAT owns the extension (owner's naming decision, 2026-10-26). A
  // template never carries one, and a name typed with the wrong one must not
  // produce "go_1.gif.mp4" or, worse, an .mp4 that is really a GIF: any known
  // media extension is stripped and the encoder's own extension is appended.
  const gs::OutputFormat fmt = selectedOutputFormat();
  static const char* kKnownExtensions[] = {".gif", ".png", ".apng", ".webp", ".mp4", ".webm"};
  for (const char* ext : kKnownExtensions) {
    const QString suffix = QString::fromLatin1(ext);
    if (name.endsWith(suffix, Qt::CaseInsensitive)) {
      name.chop(suffix.size());
      break;
    }
  }
  name += QStringLiteral(".") + QString::fromLatin1(gs::output_format_extension(fmt));
  return name;
}

QStringList MainWindow::plannedBatchTargets() const {
  // Every batch target, computed up front. E1: an explicit Save-as applies
  // only when exactly one file is queued.
  const QString explicitOut = outputEdit_->text().trimmed();
  const bool singleExplicit = (inputs_.size() == 1 && !explicitOut.isEmpty());
  QStringList targets;
  targets.reserve(inputs_.size());
  for (const QString& input : inputs_) {
    targets << (singleExplicit ? explicitOut : defaultOutputFor(input));
  }
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
  if (tmpl.isEmpty()) return false;  // empty -> default template (has {name})
  return !tmpl.contains(QStringLiteral("{name}"), Qt::CaseInsensitive);
}

QString MainWindow::defaultOutputFor(const QString& input) const {
  const QFileInfo fi(input);
  QString dir = batchDir().isEmpty() ? fi.absolutePath() : batchDir();
  // "Keep folder structure" (owner, 2026-10-26; a checkbox that starts ON).
  // It only means something relative to a folder the user ADDED: a file picked
  // on its own has no tree to keep, and inventing one would scatter results
  // into folders nobody asked for.
  if (keepStructureCheck_ && keepStructureCheck_->isChecked()) {
    for (const QString& root : inputRoots_) {
      const std::string rel =
          gs::relative_under_root(input.toStdString(), root.toStdString());
      if (rel.empty()) continue;
      const std::string sub = gs::relative_dir_part(rel);
      if (!sub.empty()) dir = QString::fromStdString(gs::join_path(dir.toStdString(), sub));
      break;
    }
  }
  return dir + QLatin1Char('/') + renderedOutputName(fi);
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
  // Keep the manual order in step even while a sort is active: a move the user
  // performed is intent, and "Queue order" must not undo it later.
  const int manualRow = manualOrder_.indexOf(inputs_.at(row));
  const int manualTarget = manualOrder_.indexOf(inputs_.at(target));
  if (manualRow >= 0 && manualTarget >= 0 && manualRow != manualTarget) {
    manualOrder_.move(manualRow, manualTarget);
  }
  inputList_->insertItem(target, item);
  // Keep inputs_ index-aligned with the list rows.
  const QString path = inputs_.takeAt(row);
  inputs_.insert(target, path);
  inputList_->setCurrentRow(target);  // fires selectionChanged -> preview refresh
  refreshCommand();
  refreshOutputSummary();
}

void MainWindow::refreshOutputSummary() {
  if (!outputSummaryLabel_) return;
  const gs::Mode mode = settingsPanel_->mode();
  const QString explicitOut = outputEdit_->text().trimmed();
  QString text;
  switch (mode) {
    case gs::Mode::Batch: {
      const QString tmpl =
          nameTemplateEdit_ && !nameTemplateEdit_->text().trimmed().isEmpty()
              ? nameTemplateEdit_->text().trimmed()
              : QString::fromLatin1(kDefaultNameTemplate);
      if (inputs_.size() == 1 && !explicitOut.isEmpty()) {
        text = QStringLiteral("Batch (1 file) → %1").arg(explicitOut);
      } else if (inputs_.isEmpty()) {
        text = QStringLiteral("Batch → each input writes \"%1\"%2")
                   .arg(tmpl,
                        batchDir().isEmpty() ? QStringLiteral(" next to it")
                                             : QStringLiteral(" into %1").arg(batchDir()));
      } else if (inputs_.size() > 1 && templateIsConstant()) {
        // Honest collision warning: every output would land on one file.
        text = QStringLiteral("Batch (%1 files) → template \"%2\" has no {name}: every "
                              "output would overwrite %3 — the run will be refused")
                   .arg(inputs_.size())
                   .arg(tmpl, defaultOutputFor(inputs_.front()));
      } else {
        // Plan the whole batch up front so the label can tell the truth
        // BEFORE the user clicks Run (audit U-01).
        const gs::OutputPlan plan = planBatch();
        if (!plan.ok) {
          text = QStringLiteral("Batch (%1 files) → the run will be refused: %2")
                     .arg(inputs_.size())
                     .arg(QString::fromStdString(plan.describe()));
        } else {
          const QString first = defaultOutputFor(inputs_.front());
          const QString last = defaultOutputFor(inputs_.back());
          // U-43: one file says "1 file" and does not print the same path twice.
          text = (inputs_.size() == 1 || first == last)
                     ? QStringLiteral("Batch (1 file) → %1").arg(first)
                     : QStringLiteral("Batch (%1 files) → %2 … %3")
                           .arg(inputs_.size())
                           .arg(first, last);
          if (!plan.preexisting.empty()) {
            text += QStringLiteral("  (%1 target(s) already exist and will be replaced)")
                        .arg(plan.preexisting.size());
          }
        }
      }
      break;
    }
    case gs::Mode::Merge:
      text = explicitOut.isEmpty()
                 ? QStringLiteral("Merge → output file REQUIRED (run is refused without one)")
                 : QStringLiteral("Merge (all inputs welded) → %1").arg(explicitOut);
      break;
    case gs::Mode::Explode:
      if (inputs_.size() > 1) {
        // N-05: the engine would explode every input but the last into the
        // CWD and exit 0 — the run is refused; say so before the click.
        text = QStringLiteral("Explode → REFUSED for %1 queued files: one file per run "
                              "(the engine would scatter the others' frames into the CWD)")
                   .arg(inputs_.size());
        break;
      }
      text = explicitOut.isEmpty()
                 ? (inputs_.isEmpty()
                        ? QStringLiteral("Explode → frames write as <stem>_frame.000, .001, … next to the input")
                        : QStringLiteral("Explode → frames write as %1_frame.000, .001, …")
                              .arg(QFileInfo(inputs_.front()).absolutePath() + QLatin1Char('/') +
                                   QFileInfo(inputs_.front()).completeBaseName()))
                 : QStringLiteral("Explode → frames write as %1.000, .001, …").arg(explicitOut);
      break;
    case gs::Mode::Auto:
      text = explicitOut.isEmpty()
                 ? QStringLiteral("Auto → the engine merges inputs to stdout UNLESS an output is set; "
                                   "set Save-as (auto-naming does not apply in Auto mode)")
                 : QStringLiteral("Auto → %1").arg(explicitOut);
      break;
  }
  outputSummaryLabel_->setText(text);
  refreshOutputFiles();
}

gs::Settings MainWindow::currentSettings() const {
  gs::Settings s;
  settingsPanel_->writeInto(s);

  // Build inputs as a std::vector without deprecated toStdVector().
  s.inputs.clear();
  s.inputs.reserve(static_cast<size_t>(inputs_.size()));
  for (const auto& in : inputs_) s.inputs.push_back(in.toStdString());

  s.output = outputEdit_->text().trimmed().toStdString();
  return s;
}

void MainWindow::refreshCommand() {
  if (!commandPane_) return;
  const QString engine = QString::fromStdString(
      gs::shell_quote(enginePath_.toStdString()));

  auto settings = currentSettings();

  // Batch mode runs ONE gifsicle process per file in Auto mode (see
  // runCommand) — it never passes -b. Reflect that honestly in the live pane
  // instead of printing a single "-b <all inputs> -o <first name>" line the
  // app would never execute.
  if (settings.mode == gs::Mode::Batch && !settings.inputs.empty()) {
    QStringList lines;
    const int n = static_cast<int>(settings.inputs.size());
    if (n > 1) {
      lines << QStringLiteral("# batch: one engine command per file (%1 files)")
                   .arg(n);
    }
    const QString explicitOut = QString::fromStdString(settings.output);
    const int kMaxShown = 20;  // avoid O(n) pane rebuilds for huge queues
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
      // Explicit Save-as is honored only for a single file (audit E1);
      // otherwise the per-file template name (default <name>_opt.gif) applies.
      QString out = explicitOut;
      if (n > 1 || out.isEmpty()) out = defaultOutputFor(in);
      one.output = out.toStdString();
      if (gs::output_format_needs_ffmpeg(selectedOutputFormat())) {
        gs::FfmpegEncodeRequest req;
        req.input = in.toStdString();
        req.partial = out.toStdString();
        req.format = selectedOutputFormat();
        lines << shellishJoin(ffmpegPath_, gs::ffmpeg_encode_args(req));
      } else {
        gs::GifsicleCommand cmd(one);
        lines << engine + QLatin1Char(' ') + QString::fromStdString(cmd.toString());
      }
    }
    commandPane_->setPlainText(lines.join(QLatin1Char('\n')));
    return;
  }

  if (gs::output_format_needs_ffmpeg(selectedOutputFormat())) {
    gs::FfmpegEncodeRequest req;
    req.input = inputs_.value(0).toStdString();
    req.partial = gs::ffmpeg_staging_name(settings.output, selectedOutputFormat());
    req.format = selectedOutputFormat();
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
  // ensureEngine() never runs when ENTERING busy (its "Ready —" status line
  // must not stomp the caller's "Optimizing…" / it would re-probe per file).
  runButton_->setEnabled(!busy && !inputs_.isEmpty() && ensureEngine());
  cancelButton_->setEnabled(busy);
  // Slice F: the reference labels the in-flight control "Stop"; idle it is a
  // plain "Cancel". Same slot (U-72's cancel latch), honest label.
  if (cancelButton_) cancelButton_->setText(busy ? QStringLiteral("Stop")
                                                 : QStringLiteral("Cancel"));
  removeButton_->setEnabled(!busy);
  clearButton_->setEnabled(!busy);
  moveUpButton_->setEnabled(!busy);
  moveDownButton_->setEnabled(!busy);
  // Slice C: the queue's shape is part of the RUN's plan, so the controls that
  // change it (add from a folder, filter, sort) are locked with Remove/Clear.
  // Filtering is a view, but a filter that hides a file mid-run and reorders
  // nothing still invites the reading that the run lost it.
  if (addFolderButton_) addFolderButton_->setEnabled(!busy);
  if (queueFilterEdit_) queueFilterEdit_->setEnabled(!busy);
  for (QToolButton* button : sortButtons_) {
    if (button) button->setEnabled(!busy);
  }
  settingsPanel_->setEnabled(!busy);
  // Audit U-45 / P1-23: lock the WHOLE output group, Browse buttons included.
  // Disabling only the text fields left a hole: a picker dialog can still call
  // setText() on a disabled QLineEdit, so the batch destination could change
  // mid-run while every readout (summary, live pane) drifted from the plan
  // the running processes were actually writing to.
  outputEdit_->setEnabled(!busy);
  outputBrowse_->setEnabled(!busy);
  batchDirEdit_->setEnabled(!busy);
  batchDirBrowse_->setEnabled(!busy);
  nameTemplateEdit_->setEnabled(!busy);
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
    // P1-10: invalidate BEFORE killing so the kill's finished() signal is
    // recognized as stale — otherwise it passes the seq guard and paints
    // "preview failed" over a run that has only just started.
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

  auto settings = currentSettings();
  auto warnings = gs::validate(settings);
  // "input" is already enforced above (empty-queue refusal), and an empty
  // output is fine in Batch/Explode (auto-derived per file). Surface any
  // remaining out-of-range settings instead of silently running with them.
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

  // ---- N-62: which encoder runs, and may it? -------------------------------
  // The format is frozen for the whole run here, exactly like batchSettings_
  // (U-58): the plan below is computed against one format, so a batch must not
  // change encoder halfway because the combo moved.
  activeFormat_ = selectedOutputFormat();
  if (gs::output_format_needs_ffmpeg(activeFormat_)) {
    // People install FFmpeg while the app is open; re-look before refusing.
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
    // Merge and Explode are GIF operations on gifsicle's own frame model - they
    // combine or split GIF frames, not encode a container. Running one as MP4
    // would produce something that is not what the label says, so it is refused
    // BEFORE anything is written.
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
    // A constant template (no {name}) with multiple inputs would write every
    // result to the SAME file, silently destroying N-1 of them. Refuse.
    if (inputs_.size() > 1 && templateIsConstant()) {
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("The name template \"%1\" contains no {name} placeholder, so all "
                         "%2 batch outputs would overwrite the same file.\n"
                         "Add {name} to the template, or run the files one at a time.")
              .arg(nameTemplateEdit_->text().trimmed())
              .arg(inputs_.size()));
      return;
    }
    // Ensure the chosen batch folder exists (honest failure, no silent skip).
    const QString dir = batchDir();
    if (!dir.isEmpty() && !QDir().mkpath(dir)) {
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Cannot create the batch output folder:\n%1").arg(dir));
      return;
    }
    // ---- Plan EVERY output before the first process starts (audit U-01) ----
    // Targets used to be derived inside the batch loop, one at a time, after
    // the previous engine process had already finished, and were never
    // compared to each other or to the inputs. Two verified consequences:
    // a template like "{name}.gif" with an empty batch folder resolves the
    // output to the INPUT ITSELF (gifsicle then replaced the source in place
    // and returned 0), and two queued files sharing a base name both render
    // <batchdir>/<stem>_opt.gif, so the second run silently deleted the first
    // result while the UI reported "Optimization complete — 2 file(s).".
    const gs::OutputPlan plan = planBatch();
    if (!plan.ok) {
      // W-32 R5: the run was refused BEFORE any process started, so every
      // planned row is `refusedBeforeRun` - not `failed`, and with no figures.
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
    // "If an output file already exists" is answered HERE, before a single
    // process starts: rename/overwrite adjust the target, skip drops that input
    // with its full path in the log - never a run that discovers the collision
    // after it has already written to it.
    {
      int skippedCount = 0;
      const QStringList resolved = resolveBatchTargets(batchTargets_, &skippedCount);
      if (skippedCount > 0) {
        QStringList keptInputs;
        QStringList keptTargets;
        for (int i = 0; i < batchTargets_.size(); ++i) {
          if (resolved.value(i).isEmpty()) continue;
          keptInputs << inputs_.value(i);
          keptTargets << resolved.value(i);
        }
        if (keptInputs.isEmpty()) {
          updateStatus(QStringLiteral("Nothing to do — every result already exists (%1 skipped).")
                           .arg(skippedCount));
          QMessageBox::information(this, QStringLiteral("Gifscythe"),
              QStringLiteral("Every planned result already exists and the choice was Skip, so "
                             "nothing was written.\n\nPick Rename or Overwrite to replace them, "
                             "or move the sources somewhere else."));
          return;
        }
        inputs_ = keptInputs;
        batchTargets_ = keptTargets;
        settings.inputs.clear();
        for (const QString& p : inputs_) settings.inputs.push_back(p.toStdString());
        updateStatus(QStringLiteral("%1 file(s) skipped: already exists").arg(skippedCount));
      } else {
        batchTargets_ = resolved;
      }
    }
    // W-32: install one row per planned job NOW, from the plan - not from the
    // live queue, which the user can still change while the run is in flight.
    beginResultRows(inputs_);

    // Process one file at a time, using the PLANNED target for each file.
    batchQueue_ = inputs_;
    batchIndex_ = 0;
    explodeSnapshot_.clear();  // batch never explodes; keep the snapshot honest
    setBusy(true);
    resultActiveRow_ = 0;
    setResultState(0, QStringLiteral("running"), QStringLiteral("optimizing…"));
    updateStatus(QStringLiteral("Optimizing 1/%1…").arg(batchQueue_.size()));
    QString in = batchQueue_.at(0);
    pendingOutput_ = batchTargets_.value(0);  // from the plan, not re-derived
    pendingPartial_ = QString::fromStdString(
        gs::claim_partial_output_path(pendingOutput_.toStdString()));  // AUD-01: never delete a file we do not own
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
    // snapshot, never from a live currentSettings() re-read. The output
    // plan above was computed and collision-checked against `settings`;
    // the jobs must run with exactly those values.
    batchSettings_ = settings;
    gs::Settings one = settings;
    one.mode = gs::Mode::Auto;  // single-file, no -b needed
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

  // Merge / Explode / Auto: single run, require output (except explode may use pattern).
  if (settings.output.empty() && settings.mode != gs::Mode::Explode) {
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Choose an output file before running (or switch to Batch mode "
                       "for automatic name-template outputs, default <name>_opt.gif)."));
    return;
  }
  // Explode with empty output: default to first input's stem.
  if (settings.output.empty() && settings.mode == gs::Mode::Explode) {
    QFileInfo fi(inputs_.front());
    settings.output = (fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
                       + QStringLiteral("_frame")).toStdString();
  }

  // Plan the single target too (audit U-01): merging a+b over a, or an Auto
  // run whose Save-as points at a queued file, destroys a source exactly like
  // the batch case does. Explode is exempt — its output is a PREFIX and the
  // engine appends .000/.001, so it cannot land on an input.
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
  }

  pendingOutput_ = QString::fromStdString(settings.output);
  if (settings.mode != gs::Mode::Explode) {
    pendingPartial_ = QString::fromStdString(
        gs::claim_partial_output_path(pendingOutput_.toStdString()));  // AUD-01: never delete a file we do not own
    if (pendingPartial_.isEmpty()) {
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Refusing to run — no free staging path beside %1").arg(pendingOutput_));
      return;
    }
    partialSnapshot_ = gs::snapshot_output(pendingPartial_.toStdString());
    settings.output = pendingPartial_.toStdString();
  } else {
    // AUD-02 / N-10 parity with the CLI: the engine opens every <prefix>.NNN
    // with truncating semantics, so writing the user's prefix directly let a
    // cancel or failure leave the PREVIOUS frame set short or half-written.
    // Write under a claimed partial prefix (nothing is deleted to get it) and
    // promote only after the frames verify.
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
  // Explode frame verification (audit U-17 / P1-19): snapshot the files under
  // the frame prefix NOW, before the engine runs, so completion can require at
  // least one NEW or CHANGED real GIF frame instead of trusting rc=0 (and so
  // stale frames from an earlier run cannot fake a success).
  if (settings.mode == gs::Mode::Explode) {
    explodeSnapshot_ = gs::snapshot_explode_candidates(pendingPartial_.toStdString());
  } else {
    explodeSnapshot_.clear();
  }


  // W-32 cardinality: Merge/Auto are N->1 and Explode is 1->N, so each is ONE
  // row. Before is the sum of the inputs for a merge, the single input for an
  // explode; the After figure is filled in only after verification.
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
  // One place decides engine vs FFmpeg for this job. It runs AFTER the result
  // row exists, so a refusal lands in that row instead of in an empty status
  // table (the row is the thing the user is looking at).
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
    return;  // U-28: the batch path returns here; this one has to as well
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
  // kill() makes gifsicle exit with a non-zero/crash status, which normally
  // routes through the "optimization failed" branch. Mark the cancellation so
  // onProcessFinished doesn't surface a spurious error dialog mid-cancel.
  // U-72 / P1-42: the flag is LATCHED — arming and clearing live in two
  // different places now. The old code cleared it right after
  // waitForFinished(3000), but that wait can time out, and even when it
  // succeeds the finished() signal can be queued-but-undelivered; a
  // finished() that arrived after the clear took the failure branch and
  // popped a spurious "Optimization failed" QMessageBox immediately after
  // "Cancelled.". The latch is armed only when a process was ACTUALLY
  // running (it can never outlive the killed run's own notification) and is
  // consumed once by onProcessFinished — see there.
  const bool engineWasRunning =
      (process_ && process_->state() != QProcess::NotRunning);
  if (engineWasRunning) {
    cancelling_ = true;
    process_->kill();
    process_->waitForFinished(3000);
  }

  // N-11: the engine may have already exited successfully in the gap between
  // the last event-loop turn and this call. pendingPartial_ then holds VERIFIED
  // output that onProcessFinished has simply not promoted yet. Discarding it
  // destroyed work the user cannot recover without re-running, and then
  // reported "Cancelled." as though nothing had happened. Leave it in place and
  // let the normal completion path promote it.
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
  invalidatePreview();  // P1-10: a cancelled run invalidates any preview state
  setBusy(false);
  // AUD-02: Explode now writes a partial frame set too (discarded above), so
  // the previous frames are untouched and the flat message is accurate for
  // every mode. (N-10's "may be incomplete" warning described the old,
  // unguarded write.)
  // W-32: completed rows keep their figures; the row the engine was working on
  // is `cancelled`; everything after it was never attempted.
  if (resultActiveRow_ >= 0) {
    setResultState(resultActiveRow_, QStringLiteral("cancelled"), QStringLiteral("stopped by the user"));
    markRemainingSkipped(resultActiveRow_ + 1);
    resultActiveRow_ = -1;
  }
  updateStatus(QStringLiteral("Cancelled."));
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
  // readyRead signals normally drained these streams already; drain once more
  // here for the final bytes and use the captured text in the failure dialog.
  captureProcessOutput();
  const QString err = processStderr_;

  // U-72 / P1-42: read-and-consume the cancel latch for THIS completion,
  // whatever the verdict turns out to be. Simply KEEPING the flag (the naive
  // fix for the early clear) would let it outlive one finished() and
  // silently swallow the NEXT run's genuine failure — a new pit of exactly
  // the class this finding belongs to. One completion, one consumption.
  const bool wasCancelling = cancelling_;
  cancelling_ = false;

  if (status != QProcess::NormalExit || exitCode != 0) {
    if (wasCancelling) {
      // A user-initiated cancel (or window close) kills the engine, which then
      // reports a non-zero exit. That's expected, not a failure to alarm about;
      // cancelRun() finishes the cleanup and sets the "Cancelled." status.
      return;
    }
    discardPendingPartial();
    pendingPartial_.clear();
    setBusy(false);
    batchQueue_.clear();
    batchIndex_ = -1;
    appendLog(QStringLiteral("Engine failed with exit code %1.").arg(exitCode));
    // W-32: named reason, and every row after this one is `skipped` - it never
    // ran, so it gets no byte figures and is not blamed as a failure.
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

  // Explode frame verification (audit U-17 / P1-19): exit 0 alone used to
  // report "Optimization complete." even with zero frames on disk — the
  // output check below explicitly skipped Explode. Require at least one NEW
  // or CHANGED real GIF under the prefix, and name the prefix on failure.
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
    // Only now do the user's frames change (AUD-02 / N-10).
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
    // vr.frames are named under the partial prefix; report the promoted names.
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
  if (!pendingOutput_.isEmpty()) {
    // The container that was promised, not "whatever GIF looks like": an APNG
    // that came out as a still PNG, or a WebP with one frame, is a failure.
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
    const std::string promotionError =
        gs::promote_partial(pendingPartial_.toStdString(), pendingOutput_.toStdString());
    if (!promotionError.empty()) {
      // N-11: this partial is VERIFIED good output — the engine produced it and
      // the verifier accepted it; only the final rename failed. On Windows that
      // rename fails for ordinary, recoverable reasons (an antivirus still
      // holding the handle, the target open in a viewer). Deleting it threw
      // away work the user cannot get back without re-running the whole job, so
      // keep it and tell them where it is.
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

  // W-32: reaching this point means the current job verified AND promoted, so
  // its row becomes `succeeded` with the on-disk size.
  if (resultActiveRow_ >= 0) {
    const qint64 afterBytes = pendingOutput_.isEmpty() ? -1 : QFileInfo(pendingOutput_).size();
    setResultState(resultActiveRow_, QStringLiteral("succeeded"), QString(), afterBytes);
  }

  // Continue batch?
  if (batchIndex_ >= 0 && batchMode_ == gs::Mode::Batch) {
    ++batchIndex_;
    if (batchIndex_ < batchQueue_.size()) {
      if (progressBar_) progressBar_->setValue(batchIndex_);
      resultActiveRow_ = batchIndex_;
      setResultState(resultActiveRow_, QStringLiteral("running"), QStringLiteral("optimizing…"));
      appendLog(QStringLiteral("Finished file %1/%2; continuing with %3.")
                    .arg(batchIndex_).arg(batchQueue_.size())
                    .arg(batchQueue_.at(batchIndex_)));
      updateStatus(QStringLiteral("Optimizing %1/%2…")
                       .arg(batchIndex_ + 1)
                       .arg(batchQueue_.size()));
      QString in = batchQueue_.at(batchIndex_);
      pendingOutput_ = batchTargets_.value(batchIndex_);  // from the plan
      pendingPartial_ = QString::fromStdString(
          gs::claim_partial_output_path(pendingOutput_.toStdString()));  // AUD-01: never delete a file we do not own
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
      // live currentSettings() — the plan was computed and collision-
      // checked against THOSE settings; re-reading here rebuilds argv
      // from state the plan never saw.
      gs::Settings one = batchSettings_;
      one.mode = gs::Mode::Auto;
      one.inputs = {in.toStdString()};
      one.output = pendingPartial_.toStdString();
      // Batch continuation keeps the format the batch STARTED with (declared
      // above, frozen at runCommand time), never the live combo.
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
    lastOutputPaths_ = batchTargets_;
    resultActiveRow_ = -1;
    batchQueue_.clear();
    batchIndex_ = -1;
    if (progressBar_) progressBar_->setValue(n);
    refreshOutputFiles();
    setBusy(false);
    appendLog(QStringLiteral("Batch verified %1 output file(s) on disk.").arg(n));
    updateStatus(QStringLiteral("Optimization complete — %1 file(s). See Output for the verified paths.").arg(n));
    // Do not re-encode a large GIF just to make the After pane. Show the exact
    // verified output produced by this run instead; settings previews remain
    // available when the user changes an Action.
    showWrittenOutputPreview();
    return;
  }

  lastOutputPaths_ = {pendingOutput_};
  resultActiveRow_ = -1;  // the single row was already marked succeeded above
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
    output = batchTargets_.value(row >= 0 ? row : 0);
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
  // GS_SETTINGS_PATH overrides everything (portable use + the offscreen
  // harness keeps CI machines clean). Mirrors the GS_ENGINE pattern.
  const QByteArray env = qgetenv("GS_SETTINGS_PATH");
  if (!env.isEmpty()) return QString::fromLocal8Bit(env);
  const QString base =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  if (base.isEmpty()) return {};  // no writable config location on this platform
  return QDir(base).filePath(QStringLiteral("gifscythe.conf"));
}

void MainWindow::loadSessionState() {
  const QString path = sessionFilePath();
  if (path.isEmpty()) {
    // U-37 / P3-8: "persistence unavailable" used to be completely silent —
    // the user had no way to know settings would not be remembered. One
    // honest status-bar note (a startup dialog would be noise for something
    // that changes nothing about this session).
    updateStatus(QStringLiteral("Settings persistence is unavailable on this system — "
                                  "changes will not be remembered between sessions."));
    return;
  }
  if (!QFileInfo::exists(path)) return;   // first launch -> defaults (normal)

  std::vector<gs::LoadWarning> warnings;
  std::map<std::string, std::string> guiKeys;
  auto loaded = gs::load_settings_file(path.toStdString(), &warnings, &guiKeys);
  if (!loaded) {
    // The file exists but is unreadable — say so instead of silently
    // pretending this is a first launch.
    updateStatus(QStringLiteral("Could not read the settings file — using defaults (%1)")
                     .arg(path));
    return;
  }

  settingsPanel_->readFrom(*loaded);

  // Slice D: which actions are in the stack, which are on, which are folded.
  // Applied AFTER the values, so a card that comes back keeps what it had.
  // GUI-only keys now arrive from the SAME parse as the core settings (audit
  // U-36 / P3-7): load_settings collects every key it does not recognize into
  // `guiKeys`. The former third parser (guiStateKey) re-opened and re-parsed
  // the file once per key; it is gone.
  auto guiKey = [&guiKeys](const char* k) -> QString {
    const auto it = guiKeys.find(k);
    return it == guiKeys.end() ? QString() : QString::fromStdString(it->second);
  };
  const QString cards = guiKey("card_state");
  if (!cards.isEmpty()) settingsPanel_->setCardState(cards);
  const QString batch = guiKey("batch_dir");
  if (!batch.isEmpty()) batchDirEdit_->setText(batch);
  const QString tmpl = guiKey("name_template");
  if (!tmpl.isEmpty()) nameTemplateEdit_->setText(tmpl);
  // Same round-trip for the session file: reopening the app must not quietly
  // reset "MP4 (H.264)" to GIF and then produce a GIF the user did not ask for.
  applyOutputFormatToken(guiKey("output_format"));

  // Surfaced, not discarded (S5 rule): a corrupt/partial file still applies
  // its valid keys, but the user sees that something was off.
  if (!warnings.empty()) {
    updateStatus(QStringLiteral("Settings loaded with %1 warning(s) — %2")
                     .arg(warnings.size())
                     .arg(path));
  }
}

bool MainWindow::saveSessionState() {
  const QString path = sessionFilePath();
  if (path.isEmpty()) return true;  // persistence unavailable — nothing to save

  const QString dir = QFileInfo(path).absolutePath();
  if (!QDir().mkpath(dir)) {
    qWarning("Gifscythe: cannot create settings folder %s", qPrintable(dir));
    return false;
  }

  gs::Settings s;
  // writeSessionInto, not writeInto: a card that is removed or switched off
  // must still come back with the values the user typed into it. The card stack
  // itself travels in the GUI key below.
  settingsPanel_->writeSessionInto(s);
  // Persist ONLY the Actions state + GUI keys below. The queue is session-
  // scoped (files move between sessions) and Save-as is a per-run choice —
  // restoring it could silently overwrite a stale path next launch.
  s.inputs.clear();
  s.output.clear();

  std::ostringstream oss;
  gs::save_settings(oss, s);
  oss << "# GUI state (Gifscythe-specific keys; SettingsIO/CLI ignore them)\n";
  const QString bd = batchDir();
  if (!bd.isEmpty()) oss << "batch_dir = " << bd.toStdString() << "\n";
  const QString tmpl = nameTemplateEdit_ ? nameTemplateEdit_->text().trimmed() : QString();
  if (!tmpl.isEmpty() && tmpl != QString::fromLatin1(kDefaultNameTemplate))
    oss << "name_template = " << tmpl.toStdString() << "\n";
  // N-62: the chosen container survives a restart too. Default stays omitted, so
  // the file is unchanged for GIF-only users.
  const gs::OutputFormat sessionFormat = selectedOutputFormat();
  if (sessionFormat != gs::OutputFormat::Gif)
    oss << "output_format = " << gs::output_format_token(sessionFormat) << "\n";
  oss << "keep_structure = " << (keepStructureCheck_ && keepStructureCheck_->isChecked() ? 1 : 0) << "\n";
  oss << "collision = " << gs::collision_policy_token(collisionPolicy()) << "\n";
  // Slice D: the card stack survives a restart (presence / enabled / collapsed).
  // A flat list of ids, so a future action can be appended without a format bump.
  if (settingsPanel_) {
    const QString cards = settingsPanel_->cardState();
    if (!cards.isEmpty()) oss << "card_state = " << cards.toStdString() << "\n";
  }

  const QByteArray data = QByteArray::fromStdString(oss.str());
  // Atomic save (audit U-16 / P1-18): QSaveFile writes a sibling temp file and
  // renames it over the target on commit(), so a crash or a full disk mid-write
  // can no longer truncate the previous session file — a reader sees either the
  // old conf or the complete new one. (The core-SettingsIO equivalent,
  // save_settings_file, got the same tmp+fsync+rename treatment.)
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
