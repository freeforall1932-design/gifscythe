#include "MainWindow.h"
#include "DropListWidget.h"
#include "PreviewPanel.h"
#include "SettingsPanel.h"

#include "core/EngineLocator.h"
#include "core/OutputName.h"
#include "core/OutputPlan.h"
#include "core/SettingsIO.h"
#include "core/Validate.h"
#include "core/version.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QSplitter>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

QString humanSize(qint64 bytes) {
  if (bytes >= 1024 * 1024)
    return QStringLiteral("%1 MB").arg(double(bytes) / (1024.0 * 1024.0), 0, 'f', 2);
  if (bytes >= 1024)
    return QStringLiteral("%1 KB").arg(double(bytes) / 1024.0, 0, 'f', 1);
  return QStringLiteral("%1 B").arg(bytes);
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
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &MainWindow::onProcessFinished);
  connect(process_, &QProcess::errorOccurred, this, &MainWindow::onProcessError);

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

  auto* splitter = new QSplitter(Qt::Horizontal, central);
  splitter->setObjectName(QStringLiteral("mainSplitter"));

  tabs_ = new QTabWidget(splitter);
  tabs_->setObjectName(QStringLiteral("mainTabs"));
  tabs_->addTab(buildInputTab(), QStringLiteral("Input"));
  settingsPanel_ = new SettingsPanel(tabs_);
  settingsPanel_->setObjectName(QStringLiteral("settingsPanel"));
  tabs_->addTab(settingsPanel_, QStringLiteral("Actions"));
  tabs_->addTab(buildOutputTab(), QStringLiteral("Output"));
  splitter->addWidget(tabs_);

  previewPanel_ = new PreviewPanel(splitter);
  previewPanel_->setObjectName(QStringLiteral("previewPanel"));
  splitter->addWidget(previewPanel_);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);
  root->addWidget(splitter, 1);

  buildBottomBar(central, root);
  setCentralWidget(central);

  // ---- Wiring: everything that changes settings refreshes the live pane,
  //      the output summary, and (debounced) the preview. ----
  connect(settingsPanel_, &SettingsPanel::changed, this, [this]() {
    refreshCommand();
    refreshOutputSummary();
    schedulePreview();
  });
  connect(outputEdit_, &QLineEdit::textChanged, this, [this]() {
    refreshCommand();
    refreshOutputSummary();
  });
  connect(batchDirEdit_, &QLineEdit::textChanged, this, [this]() {
    refreshCommand();
    refreshOutputSummary();
  });
  connect(nameTemplateEdit_, &QLineEdit::textChanged, this, [this]() {
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
  auto* page = new QWidget(tabs_);
  auto* left = new QVBoxLayout(page);
  left->addWidget(new QLabel(QStringLiteral("Animation queue — drop GIFs here"), page));

  inputList_ = new DropListWidget(page);
  inputList_->setObjectName(QStringLiteral("queueList"));
  inputList_->setMinimumWidth(300);
  inputList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  left->addWidget(inputList_, 1);

  queueCountLabel_ = new QLabel(QStringLiteral("0 files"), page);
  queueCountLabel_->setObjectName(QStringLiteral("queueCountLabel"));
  left->addWidget(queueCountLabel_);

  auto* queueBtns = new QHBoxLayout();
  auto* addButton = new QPushButton(QStringLiteral("Add GIF files…"), page);
  addButton->setObjectName(QStringLiteral("addButton"));
  connect(addButton, &QPushButton::clicked, this, &MainWindow::chooseInputs);
  removeButton_ = new QPushButton(QStringLiteral("Remove"), page);
  removeButton_->setObjectName(QStringLiteral("removeButton"));
  connect(removeButton_, &QPushButton::clicked, this, &MainWindow::removeSelected);
  clearButton_ = new QPushButton(QStringLiteral("Clear"), page);
  clearButton_->setObjectName(QStringLiteral("clearButton"));
  connect(clearButton_, &QPushButton::clicked, this, &MainWindow::clearQueue);
  queueBtns->addWidget(addButton);
  queueBtns->addWidget(removeButton_);
  queueBtns->addWidget(clearButton_);
  queueBtns->addStretch();
  left->addLayout(queueBtns);

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
  return page;
}

QWidget* MainWindow::buildOutputTab() {
  auto* page = new QWidget(tabs_);
  auto* lay = new QVBoxLayout(page);

  auto* box = new QGroupBox(QStringLiteral("Output"), page);
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
  form->addRow(QStringLiteral("Batch outputs"), batchRow);

  // Naming template (S3-25). Default renders the historical <name>_opt.gif
  // (audit E4); {name} is replaced with the input's base name.
  nameTemplateEdit_ = new QLineEdit(box);
  nameTemplateEdit_->setObjectName(QStringLiteral("nameTemplateEdit"));
  nameTemplateEdit_->setText(QString::fromLatin1(kDefaultNameTemplate));
  nameTemplateEdit_->setToolTip(QStringLiteral(
      "Batch auto-naming template.\n"
      "{name} = the input file's base name (required when batching >1 file).\n"
      "Path separators are stripped; \".gif\" is appended if missing."));
  form->addRow(QStringLiteral("Name template"), nameTemplateEdit_);
  auto* tmplHint = new QLabel(QStringLiteral(
      "{name} = input base name · used for batch auto-naming · runs that would "
      "overwrite one file are refused"), box);
  tmplHint->setObjectName(QStringLiteral("nameTemplateHint"));
  tmplHint->setWordWrap(true);
  form->addRow(QString(), tmplHint);

  openDirButton_ = new QPushButton(QStringLiteral("Open output folder"), box);
  openDirButton_->setObjectName(QStringLiteral("openDirButton"));
  connect(openDirButton_, &QPushButton::clicked, this, &MainWindow::openOutputFolder);
  form->addRow(QString(), openDirButton_);

  lay->addWidget(box);

  outputSummaryLabel_ = new QLabel(page);
  outputSummaryLabel_->setObjectName(QStringLiteral("outputSummary"));
  outputSummaryLabel_->setWordWrap(true);
  lay->addWidget(outputSummaryLabel_);
  lay->addStretch();
  return page;
}

void MainWindow::buildBottomBar(QWidget* central, QVBoxLayout* root) {
  root->addWidget(new QLabel(QStringLiteral("Generated engine command (one-way: controls → command)"),
                             central));
  commandPane_ = new QPlainTextEdit(central);
  commandPane_->setObjectName(QStringLiteral("commandPane"));
  commandPane_->setReadOnly(true);
  commandPane_->setPlaceholderText(QStringLiteral("Add a GIF to generate a command…"));
  root->addWidget(commandPane_, 1);

  progressBar_ = new QProgressBar(central);
  progressBar_->setObjectName(QStringLiteral("progressBar"));
  progressBar_->setRange(0, 0);  // indeterminate while running
  progressBar_->setVisible(false);
  root->addWidget(progressBar_);

  auto* actions = new QHBoxLayout();
  runButton_ = new QPushButton(QStringLiteral("Optimize GIF"), central);
  runButton_->setObjectName(QStringLiteral("runButton"));
  runButton_->setEnabled(false);
  connect(runButton_, &QPushButton::clicked, this, &MainWindow::runCommand);
  cancelButton_ = new QPushButton(QStringLiteral("Cancel"), central);
  cancelButton_->setObjectName(QStringLiteral("cancelButton"));
  cancelButton_->setEnabled(false);
  connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancelRun);
  actions->addWidget(runButton_);
  actions->addWidget(cancelButton_);
  actions->addStretch();
  statusLabel_ = new QLabel(central);
  statusLabel_->setObjectName(QStringLiteral("statusLabel"));
  actions->addWidget(statusLabel_);
  root->addLayout(actions);
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

void MainWindow::appendInputs(const QStringList& files) {
  int added = 0;
  for (const auto& f : files) {
    if (inputs_.contains(f)) continue;
    inputs_.append(f);
    const QFileInfo fi(f);
    auto* item = new QListWidgetItem(
        fi.exists()
            ? QStringLiteral("%1 — %2").arg(fi.fileName(), humanSize(fi.size()))
            : QStringLiteral("%1 — (missing)").arg(fi.fileName()));
    item->setData(Qt::UserRole, f);
    item->setToolTip(f);
    inputList_->addItem(item);
    ++added;
  }
  if (added > 0) {
    runButton_->setEnabled(!busy_ && ensureEngine() && !inputs_.isEmpty());
    refreshQueueLabel();
    updateStatus(QStringLiteral("%1 GIF file(s) in queue.").arg(inputs_.size()));
    refreshCommand();
    refreshOutputSummary();
    if (inputList_->currentRow() < 0) inputList_->setCurrentRow(0);
    schedulePreview();
  }
}

void MainWindow::refreshQueueLabel() {
  qint64 total = 0;
  int existing = 0;
  for (const auto& f : inputs_) {
    const QFileInfo fi(f);
    if (fi.exists()) {
      total += fi.size();
      ++existing;
    }
  }
  queueCountLabel_->setText(QStringLiteral("%1 file(s) · %2 on disk%3")
                                .arg(inputs_.size())
                                .arg(humanSize(total),
                                     existing == inputs_.size()
                                         ? QString()
                                         : QStringLiteral(" (%1 missing)").arg(inputs_.size() - existing)));
}

void MainWindow::chooseInputs() {
  const auto files = QFileDialog::getOpenFileNames(
      this, QStringLiteral("Open GIF files"), QString(),
      QStringLiteral("GIF files (*.gif);;All files (*)"));
  if (!files.isEmpty()) appendInputs(files);
}

void MainWindow::onFilesDropped(const QStringList& files) {
  QStringList gifs;
  for (const auto& f : files) {
    // Audit U-13: this was `|| exists(f)`, so ANY existing file (.exe, .jpg,
    // .txt) was queued and only failed later, at run time. Both conditions
    // were clearly meant to hold.
    if (f.endsWith(QStringLiteral(".gif"), Qt::CaseInsensitive) && QFileInfo::exists(f))
      gifs << f;
  }
  appendInputs(gifs);
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
  for (int row : rows) {
    if (row >= 0 && row < inputs_.size()) inputs_.removeAt(row);
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
  inputs_.clear();
  inputList_->clear();
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
    } else if (!inputs_.isEmpty()) {
      dir = QFileInfo(inputs_.front()).absolutePath();
    }
  }
  if (dir.isEmpty()) return;
  QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
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
  // The engine always writes GIF; keep the extension honest.
  if (!name.endsWith(QStringLiteral(".gif"), Qt::CaseInsensitive))
    name += QStringLiteral(".gif");
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
  const QString dir = batchDir().isEmpty() ? fi.absolutePath() : batchDir();
  return dir + QLatin1Char('/') + renderedOutputName(fi);
}

void MainWindow::moveCurrent(int delta) {
  if (busy_) return;
  const int row = inputList_->currentRow();
  if (row < 0 || row >= inputs_.size()) return;
  const int target = row + delta;
  if (target < 0 || target >= inputs_.size()) return;
  QListWidgetItem* item = inputList_->takeItem(row);
  if (!item) return;
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
      gs::GifsicleCommand cmd(one);
      lines << engine + QLatin1Char(' ') + QString::fromStdString(cmd.toString());
    }
    commandPane_->setPlainText(lines.join(QLatin1Char('\n')));
    return;
  }

  gs::GifsicleCommand cmd(settings);
  commandPane_->setPlainText(
      engine + QLatin1Char(' ')
      + QString::fromStdString(cmd.toString()));
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
  removeButton_->setEnabled(!busy);
  clearButton_->setEnabled(!busy);
  moveUpButton_->setEnabled(!busy);
  moveDownButton_->setEnabled(!busy);
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

  batchMode_ = settings.mode;

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
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Refusing to run — the planned output would overwrite user data:\n\n%1")
              .arg(QString::fromStdString(plan.describe())));
      return;
    }
    batchTargets_ = plannedBatchTargets();

    // Process one file at a time, using the PLANNED target for each file.
    batchQueue_ = inputs_;
    batchIndex_ = 0;
    explodeSnapshot_.clear();  // batch never explodes; keep the snapshot honest
    setBusy(true);
    updateStatus(QStringLiteral("Optimizing 1/%1…").arg(batchQueue_.size()));
    QString in = batchQueue_.at(0);
    pendingOutput_ = batchTargets_.value(0);  // from the plan, not re-derived
    gs::Settings one = settings;
    one.mode = gs::Mode::Auto;  // single-file, no -b needed
    one.inputs = {in.toStdString()};
    one.output = pendingOutput_.toStdString();
    gs::GifsicleCommand cmd(one);
    QStringList qargs;
    for (const auto& a : cmd.args()) qargs << QString::fromStdString(a);
    process_->setProgram(enginePath_);
    process_->setArguments(qargs);
    process_->start();
    if (!process_->waitForStarted(5000)) {
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
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("Refusing to run — the planned output would overwrite user data:\n\n%1")
              .arg(QString::fromStdString(one_plan.describe())));
      return;
    }
  }

  pendingOutput_ = QString::fromStdString(settings.output);
  batchIndex_ = -1;
  batchQueue_.clear();
  // Explode frame verification (audit U-17 / P1-19): snapshot the files under
  // the frame prefix NOW, before the engine runs, so completion can require at
  // least one NEW or CHANGED real GIF frame instead of trusting rc=0 (and so
  // stale frames from an earlier run cannot fake a success).
  if (settings.mode == gs::Mode::Explode) {
    explodeSnapshot_ = gs::snapshot_explode_candidates(pendingOutput_.toStdString());
  } else {
    explodeSnapshot_.clear();
  }

  gs::GifsicleCommand cmd(settings);
  QStringList qargs;
  for (const auto& a : cmd.args()) qargs << QString::fromStdString(a);

  setBusy(true);
  updateStatus(QStringLiteral("Running…"));
  process_->setProgram(enginePath_);
  process_->setArguments(qargs);
  process_->start();
  if (!process_->waitForStarted(5000)) {
    setBusy(false);
    QMessageBox::critical(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Could not start the GIF engine: %1").arg(process_->errorString()));
    return;  // U-28: the batch path returns here; this one has to as well
  }
}

void MainWindow::cancelRun() {
  // kill() makes gifsicle exit with a non-zero/crash status, which normally
  // routes through the "optimization failed" branch. Mark the cancellation so
  // onProcessFinished doesn't surface a spurious error dialog mid-cancel.
  cancelling_ = true;
  if (process_ && process_->state() != QProcess::NotRunning) {
    process_->kill();
    process_->waitForFinished(3000);
  }
  cancelling_ = false;
  batchQueue_.clear();
  batchIndex_ = -1;
  invalidatePreview();  // P1-10: a cancelled run invalidates any preview state
  setBusy(false);
  updateStatus(QStringLiteral("Cancelled."));
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
  const QString err = QString::fromLocal8Bit(process_->readAllStandardError());
  // Drain stdout so it doesn't fill the pipe (we don't use it when -o is set).
  process_->readAllStandardOutput();

  if (status != QProcess::NormalExit || exitCode != 0) {
    if (cancelling_) {
      // A user-initiated cancel (or window close) kills the engine, which then
      // reports a non-zero exit. That's expected, not a failure to alarm about;
      // cancelRun() finishes the cleanup and sets the "Cancelled." status.
      return;
    }
    setBusy(false);
    batchQueue_.clear();
    batchIndex_ = -1;
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
        gs::verify_explode_frames(pendingOutput_.toStdString(), explodeSnapshot_);
    explodeSnapshot_.clear();
    if (!vr.ok) {
      setBusy(false);
      batchQueue_.clear();
      batchIndex_ = -1;
      updateStatus(QStringLiteral("No frames produced — engine exited 0 but wrote none."));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QString::fromStdString(vr.describe()));
      return;
    }
    setBusy(false);
    updateStatus(QStringLiteral("Explode complete — %1 frame(s) written.")
                     .arg(vr.frames.size()));
    schedulePreview();
    return;
  }

  // Verify output for non-explode modes.
  if (!pendingOutput_.isEmpty()) {
    QFileInfo fi(pendingOutput_);
    if (!fi.exists() || fi.size() == 0) {
      setBusy(false);
      batchQueue_.clear();
      batchIndex_ = -1;
      updateStatus(QStringLiteral("No output produced — check the command below."));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("The GIF engine exited 0 but no output file was written:\n%1")
              .arg(pendingOutput_));
      return;
    }
  }

  // Continue batch?
  if (batchIndex_ >= 0 && batchMode_ == gs::Mode::Batch) {
    ++batchIndex_;
    if (batchIndex_ < batchQueue_.size()) {
      updateStatus(QStringLiteral("Optimizing %1/%2…")
                       .arg(batchIndex_ + 1)
                       .arg(batchQueue_.size()));
      QString in = batchQueue_.at(batchIndex_);
      pendingOutput_ = batchTargets_.value(batchIndex_);  // from the plan
      auto settings = currentSettings();
      gs::Settings one = settings;
      one.mode = gs::Mode::Auto;
      one.inputs = {in.toStdString()};
      one.output = pendingOutput_.toStdString();
      gs::GifsicleCommand cmd(one);
      QStringList qargs;
      for (const auto& a : cmd.args()) qargs << QString::fromStdString(a);
      process_->setProgram(enginePath_);
      process_->setArguments(qargs);
      process_->start();
      return;
    }
    // Batch complete.
    const int n = batchQueue_.size();
    batchQueue_.clear();
    batchIndex_ = -1;
    setBusy(false);
    updateStatus(QStringLiteral("Optimization complete — %1 file(s).").arg(n));
    schedulePreview();  // refresh preview with final settings
    return;
  }

  setBusy(false);
  updateStatus(QStringLiteral("Optimization complete."));
  schedulePreview();
}

void MainWindow::onProcessError(QProcess::ProcessError error) {
  if (error == QProcess::FailedToStart) {
    setBusy(false);
    batchQueue_.clear();
    batchIndex_ = -1;
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
  const QString sel = selectedInput();
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
  if (busy_) return;  // main run active; preview resumes after it finishes
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
  if (!gs::path_is_executable(enginePath_.toStdString())) {
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

void MainWindow::updateStatus(const QString& message) {
  if (statusLabel_) statusLabel_->setText(message);
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

  // GUI-only keys now arrive from the SAME parse as the core settings (audit
  // U-36 / P3-7): load_settings collects every key it does not recognize into
  // `guiKeys`. The former third parser (guiStateKey) re-opened and re-parsed
  // the file once per key; it is gone.
  auto guiKey = [&guiKeys](const char* k) -> QString {
    const auto it = guiKeys.find(k);
    return it == guiKeys.end() ? QString() : QString::fromStdString(it->second);
  };
  const QString batch = guiKey("batch_dir");
  if (!batch.isEmpty()) batchDirEdit_->setText(batch);
  const QString tmpl = guiKey("name_template");
  if (!tmpl.isEmpty()) nameTemplateEdit_->setText(tmpl);

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
  settingsPanel_->writeInto(s);
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
