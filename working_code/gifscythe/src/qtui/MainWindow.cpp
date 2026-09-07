#include "MainWindow.h"
#include "DropListWidget.h"

#include "core/EngineLocator.h"
#include "core/Validate.h"
#include "core/version.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
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
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("Gifscythe %1").arg(QStringLiteral(GS_VERSION)));
  resize(980, 640);

  enginePath_ = QString::fromStdString(
      gs::locate_engine(QCoreApplication::applicationFilePath().toStdString()));

  process_ = new QProcess(this);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &MainWindow::onProcessFinished);
  connect(process_, &QProcess::errorOccurred, this, &MainWindow::onProcessError);

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

  auto* central = new QWidget(this);
  auto* root = new QVBoxLayout(central);
  auto* content = new QHBoxLayout();

  // ---- Left: animation queue ----
  auto* left = new QVBoxLayout();
  left->addWidget(new QLabel(QStringLiteral("Animation queue"), central));
  inputList_ = new DropListWidget(central);
  inputList_->setMinimumWidth(300);
  inputList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(inputList_, &DropListWidget::filesDropped, this, &MainWindow::onFilesDropped);
  left->addWidget(inputList_, 1);

  auto* queueBtns = new QHBoxLayout();
  auto* addButton = new QPushButton(QStringLiteral("Add GIF files…"), central);
  connect(addButton, &QPushButton::clicked, this, &MainWindow::chooseInputs);
  removeButton_ = new QPushButton(QStringLiteral("Remove"), central);
  connect(removeButton_, &QPushButton::clicked, this, &MainWindow::removeSelected);
  clearButton_ = new QPushButton(QStringLiteral("Clear"), central);
  connect(clearButton_, &QPushButton::clicked, this, &MainWindow::clearQueue);
  queueBtns->addWidget(addButton);
  queueBtns->addWidget(removeButton_);
  queueBtns->addWidget(clearButton_);
  left->addLayout(queueBtns);
  content->addLayout(left, 1);

  // ---- Right: settings ----
  auto* settingsBox = new QGroupBox(QStringLiteral("Optimize"), central);
  auto* form = new QFormLayout(settingsBox);

  modeCombo_ = new QComboBox(settingsBox);
  modeCombo_->addItem(QStringLiteral("Batch (each file)"), static_cast<int>(gs::Mode::Batch));
  modeCombo_->addItem(QStringLiteral("Merge into one"), static_cast<int>(gs::Mode::Merge));
  modeCombo_->addItem(QStringLiteral("Explode frames"), static_cast<int>(gs::Mode::Explode));
  modeCombo_->addItem(QStringLiteral("Auto"), static_cast<int>(gs::Mode::Auto));
  modeCombo_->setCurrentIndex(0);  // Batch default
  modeCombo_->setToolTip(QStringLiteral(
      "Batch optimizes each GIF separately. Merge concatenates all into one animation."));
  form->addRow(QStringLiteral("Mode"), modeCombo_);

  optimizeSpin_ = new QSpinBox(settingsBox);
  optimizeSpin_->setRange(0, 3);
  optimizeSpin_->setValue(3);
  optimizeSpin_->setSpecialValueText(QStringLiteral("Off"));
  optimizeSpin_->setToolTip(QStringLiteral(
      "0 = off, 1–3 = higher levels usually produce smaller GIFs."));
  form->addRow(QStringLiteral("Optimization level"), optimizeSpin_);

  lossySpin_ = new QSpinBox(settingsBox);
  lossySpin_->setRange(0, 200);
  lossySpin_->setSpecialValueText(QStringLiteral("Off"));
  lossySpin_->setValue(0);
  form->addRow(QStringLiteral("Lossy compression"), lossySpin_);

  auto* outputRow = new QHBoxLayout();
  outputEdit_ = new QLineEdit(settingsBox);
  outputEdit_->setPlaceholderText(
      QStringLiteral("Output file (required for Merge; auto <name>_opt.gif in Batch)"));
  auto* browse = new QPushButton(QStringLiteral("Browse…"), settingsBox);
  connect(browse, &QPushButton::clicked, this, &MainWindow::chooseOutput);
  outputRow->addWidget(outputEdit_);
  outputRow->addWidget(browse);
  form->addRow(QStringLiteral("Save as"), outputRow);

  auto* hint = new QLabel(
      QStringLiteral("Command preview stays in sync with the controls above."),
      settingsBox);
  hint->setWordWrap(true);
  form->addRow(hint);
  content->addWidget(settingsBox, 1);
  root->addLayout(content, 2);

  root->addWidget(new QLabel(QStringLiteral("Generated gifsicle command"), central));
  commandPane_ = new QPlainTextEdit(central);
  commandPane_->setReadOnly(true);
  commandPane_->setPlaceholderText(QStringLiteral("Add a GIF to generate a command…"));
  root->addWidget(commandPane_, 1);

  progressBar_ = new QProgressBar(central);
  progressBar_->setRange(0, 0);  // indeterminate while running
  progressBar_->setVisible(false);
  root->addWidget(progressBar_);

  auto* actions = new QHBoxLayout();
  runButton_ = new QPushButton(QStringLiteral("Optimize GIF"), central);
  runButton_->setEnabled(false);
  connect(runButton_, &QPushButton::clicked, this, &MainWindow::runCommand);
  cancelButton_ = new QPushButton(QStringLiteral("Cancel"), central);
  cancelButton_->setEnabled(false);
  connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancelRun);
  actions->addWidget(runButton_);
  actions->addWidget(cancelButton_);
  actions->addStretch();
  statusLabel_ = new QLabel(central);
  actions->addWidget(statusLabel_);
  root->addLayout(actions);

  // Keep the live pane honest: every control that affects settings refreshes it.
  connect(optimizeSpin_, qOverload<int>(&QSpinBox::valueChanged),
          this, &MainWindow::refreshCommand);
  connect(lossySpin_, qOverload<int>(&QSpinBox::valueChanged),
          this, &MainWindow::refreshCommand);
  connect(outputEdit_, &QLineEdit::textChanged, this, &MainWindow::refreshCommand);
  connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &MainWindow::refreshCommand);
  connect(inputList_->model(), &QAbstractItemModel::rowsInserted,
          this, &MainWindow::refreshCommand);
  connect(inputList_->model(), &QAbstractItemModel::rowsRemoved,
          this, &MainWindow::refreshCommand);

  setCentralWidget(central);
  ensureEngine();
  refreshCommand();
}

MainWindow::~MainWindow() {
  if (process_ && process_->state() != QProcess::NotRunning) {
    process_->kill();
    process_->waitForFinished(2000);
  }
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (busy_) {
    cancelRun();
  }
  QMainWindow::closeEvent(event);
}

bool MainWindow::ensureEngine() {
  if (gs::path_is_executable(enginePath_.toStdString())) {
    updateStatus(QStringLiteral("Ready — engine: %1").arg(enginePath_));
    return true;
  }
  // Re-probe in case the user built the engine after launch.
  enginePath_ = QString::fromStdString(
      gs::locate_engine(QCoreApplication::applicationFilePath().toStdString()));
  if (gs::path_is_executable(enginePath_.toStdString())) {
    updateStatus(QStringLiteral("Ready — engine: %1").arg(enginePath_));
    return true;
  }
  updateStatus(QStringLiteral("Engine not found — build with ./scripts/build_gifsicle.sh"));
  runButton_->setEnabled(false);
  return false;
}

void MainWindow::appendInputs(const QStringList& files) {
  int added = 0;
  for (const auto& f : files) {
    if (inputs_.contains(f)) continue;
    inputs_.append(f);
    inputList_->addItem(QFileInfo(f).fileName());
    ++added;
  }
  if (added > 0) {
    runButton_->setEnabled(!busy_ && ensureEngine() && !inputs_.isEmpty());
    updateStatus(QStringLiteral("%1 GIF file(s) in queue.").arg(inputs_.size()));
    refreshCommand();
  }
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
    if (f.endsWith(QStringLiteral(".gif"), Qt::CaseInsensitive) || QFileInfo::exists(f))
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
  updateStatus(QStringLiteral("%1 GIF file(s) in queue.").arg(inputs_.size()));
  refreshCommand();
}

void MainWindow::clearQueue() {
  inputs_.clear();
  inputList_->clear();
  runButton_->setEnabled(false);
  updateStatus(QStringLiteral("Queue cleared."));
  refreshCommand();
}

void MainWindow::chooseOutput() {
  const auto file = QFileDialog::getSaveFileName(
      this, QStringLiteral("Save optimized GIF"), QString(),
      QStringLiteral("GIF files (*.gif)"));
  if (!file.isEmpty()) {
    outputEdit_->setText(file);
    refreshCommand();
  }
}

QString MainWindow::defaultOutputFor(const QString& input) const {
  QFileInfo fi(input);
  return fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
       + QStringLiteral("_opt.gif");
}

gs::Settings MainWindow::currentSettings() const {
  gs::Settings s;
  const int modeData = modeCombo_ ? modeCombo_->currentData().toInt()
                                  : static_cast<int>(gs::Mode::Batch);
  s.mode = static_cast<gs::Mode>(modeData);

  // Build inputs as a std::vector without deprecated toStdVector().
  s.inputs.clear();
  s.inputs.reserve(static_cast<size_t>(inputs_.size()));
  for (const auto& in : inputs_) s.inputs.push_back(in.toStdString());

  s.optimize_level = optimizeSpin_->value();
  if (lossySpin_->value() > 0) s.lossy = lossySpin_->value();
  s.output = outputEdit_->text().trimmed().toStdString();
  return s;
}

void MainWindow::refreshCommand() {
  if (!commandPane_) return;
  auto settings = currentSettings();

  // For display in batch mode with empty output, show the auto-derived name.
  if (settings.mode == gs::Mode::Batch && settings.output.empty() && !settings.inputs.empty()) {
    settings.output = defaultOutputFor(QString::fromStdString(settings.inputs.front()))
                          .toStdString();
  }

  gs::GifsicleCommand cmd(settings);
  commandPane_->setPlainText(
      QString::fromStdString(gs::shell_quote(enginePath_.toStdString()))
      + QLatin1Char(' ')
      + QString::fromStdString(cmd.toString()));
}

void MainWindow::setBusy(bool busy) {
  busy_ = busy;
  runButton_->setEnabled(!busy && !inputs_.isEmpty());
  cancelButton_->setEnabled(busy);
  removeButton_->setEnabled(!busy);
  clearButton_->setEnabled(!busy);
  modeCombo_->setEnabled(!busy);
  optimizeSpin_->setEnabled(!busy);
  lossySpin_->setEnabled(!busy);
  outputEdit_->setEnabled(!busy);
  progressBar_->setVisible(busy);
}

void MainWindow::runCommand() {
  if (busy_) return;
  if (!ensureEngine()) {
    QMessageBox::critical(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Could not find the gifsicle engine.\n"
                       "Build it with ./scripts/build_gifsicle.sh\n"
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
  // Allow empty output in batch — we auto-derive per file.
  warnings.erase(std::remove_if(warnings.begin(), warnings.end(),
      [](const gs::Warning& w) {
        return w.field == "input";  // already checked
      }), warnings.end());

  batchMode_ = settings.mode;

  if (settings.mode == gs::Mode::Batch) {
    // Process one file at a time with auto output names.
    batchQueue_ = inputs_;
    batchIndex_ = 0;
    setBusy(true);
    updateStatus(QStringLiteral("Optimizing 1/%1…").arg(batchQueue_.size()));
    // Kick off first file via a synthetic "finished" that starts the next.
    // Directly start:
    QString in = batchQueue_.at(0);
    QString out = outputEdit_->text().trimmed();
    if (batchQueue_.size() == 1 && !out.isEmpty()) {
      pendingOutput_ = out;
    } else {
      pendingOutput_ = defaultOutputFor(in);
    }
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
          QStringLiteral("Could not start gifsicle: %1").arg(process_->errorString()));
      return;
    }
    return;
  }

  // Merge / Explode / Auto: single run, require output (except explode may use pattern).
  if (settings.output.empty() && settings.mode != gs::Mode::Explode) {
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        QStringLiteral("Choose an output file before running (or switch to Batch mode "
                       "for automatic <name>_opt.gif names)."));
    return;
  }
  // Explode with empty output: default to first input's stem.
  if (settings.output.empty() && settings.mode == gs::Mode::Explode) {
    QFileInfo fi(inputs_.front());
    settings.output = (fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
                       + QStringLiteral("_frame")).toStdString();
  }

  pendingOutput_ = QString::fromStdString(settings.output);
  batchIndex_ = -1;
  batchQueue_.clear();

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
        QStringLiteral("Could not start gifsicle: %1").arg(process_->errorString()));
  }
}

void MainWindow::cancelRun() {
  if (process_ && process_->state() != QProcess::NotRunning) {
    process_->kill();
    process_->waitForFinished(3000);
  }
  batchQueue_.clear();
  batchIndex_ = -1;
  setBusy(false);
  updateStatus(QStringLiteral("Cancelled."));
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
  const QString err = QString::fromLocal8Bit(process_->readAllStandardError());
  // Drain stdout so it doesn't fill the pipe (we don't use it when -o is set).
  process_->readAllStandardOutput();

  if (status != QProcess::NormalExit || exitCode != 0) {
    setBusy(false);
    batchQueue_.clear();
    batchIndex_ = -1;
    updateStatus(QStringLiteral("Optimization failed (exit %1).").arg(exitCode));
    QMessageBox::warning(this, QStringLiteral("Gifscythe"),
        err.isEmpty() ? QStringLiteral("gifsicle returned an error (exit %1).").arg(exitCode)
                      : err);
    return;
  }

  // Verify output for non-explode modes.
  if (batchMode_ != gs::Mode::Explode && !pendingOutput_.isEmpty()) {
    QFileInfo fi(pendingOutput_);
    if (!fi.exists() || fi.size() == 0) {
      setBusy(false);
      batchQueue_.clear();
      batchIndex_ = -1;
      updateStatus(QStringLiteral("No output produced — check the command below."));
      QMessageBox::warning(this, QStringLiteral("Gifscythe"),
          QStringLiteral("gifsicle exited 0 but no output file was written:\n%1")
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
      pendingOutput_ = defaultOutputFor(in);
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
    return;
  }

  setBusy(false);
  updateStatus(QStringLiteral("Optimization complete."));
}

void MainWindow::onProcessError(QProcess::ProcessError error) {
  if (error == QProcess::FailedToStart) {
    setBusy(false);
    batchQueue_.clear();
    batchIndex_ = -1;
    updateStatus(QStringLiteral("Failed to start gifsicle."));
  }
  // Crashes are also reported via finished().
}

void MainWindow::updateStatus(const QString& message) {
  if (statusLabel_) statusLabel_->setText(message);
}
