#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("Gifscythe 0.1.0"));
  resize(980, 620);
  enginePath_ = QCoreApplication::applicationDirPath() + QStringLiteral("/gifsicle");

  auto* openAct = new QAction(QStringLiteral("&Open GIF files..."), this);
  openAct->setShortcut(QKeySequence::Open);
  connect(openAct, &QAction::triggered, this, &MainWindow::chooseInputs);
  auto* quitAct = new QAction(QStringLiteral("&Quit"), this);
  quitAct->setShortcut(QKeySequence::Quit);
  connect(quitAct, &QAction::triggered, this, &QWidget::close);
  auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
  fileMenu->addAction(openAct); fileMenu->addSeparator(); fileMenu->addAction(quitAct);

  auto* central = new QWidget(this);
  auto* root = new QVBoxLayout(central);
  auto* content = new QHBoxLayout();

  auto* left = new QVBoxLayout();
  left->addWidget(new QLabel(QStringLiteral("Animation queue"), central));
  inputList_ = new QListWidget(central);
  inputList_->setAcceptDrops(true);
  inputList_->setMinimumWidth(300);
  left->addWidget(inputList_, 1);
  auto* addButton = new QPushButton(QStringLiteral("Add GIF files…"), central);
  connect(addButton, &QPushButton::clicked, this, &MainWindow::chooseInputs);
  left->addWidget(addButton);
  content->addLayout(left, 1);

  auto* settingsBox = new QGroupBox(QStringLiteral("Optimize"), central);
  auto* form = new QFormLayout(settingsBox);
  optimizeSpin_ = new QSpinBox(settingsBox);
  optimizeSpin_->setRange(0, 3); optimizeSpin_->setValue(3);
  optimizeSpin_->setToolTip(QStringLiteral("Higher levels usually produce smaller GIFs."));
  form->addRow(QStringLiteral("Optimization level"), optimizeSpin_);
  lossySpin_ = new QSpinBox(settingsBox);
  lossySpin_->setRange(0, 200); lossySpin_->setSpecialValueText(QStringLiteral("Off"));
  lossySpin_->setValue(0);
  form->addRow(QStringLiteral("Lossy compression"), lossySpin_);
  auto* outputRow = new QHBoxLayout();
  outputEdit_ = new QLineEdit(settingsBox);
  outputEdit_->setPlaceholderText(QStringLiteral("Output file (optional)"));
  auto* browse = new QPushButton(QStringLiteral("Browse…"), settingsBox);
  connect(browse, &QPushButton::clicked, this, &MainWindow::chooseOutput);
  outputRow->addWidget(outputEdit_); outputRow->addWidget(browse);
  form->addRow(QStringLiteral("Save as"), outputRow);
  auto* hint = new QLabel(QStringLiteral("The command preview is always kept in sync."), settingsBox);
  hint->setWordWrap(true); form->addRow(hint);
  content->addWidget(settingsBox, 1);
  root->addLayout(content, 2);

  root->addWidget(new QLabel(QStringLiteral("Generated gifsicle command"), central));
  commandPane_ = new QPlainTextEdit(central);
  commandPane_->setReadOnly(true);
  commandPane_->setPlaceholderText(QStringLiteral("Add a GIF to generate a command…"));
  root->addWidget(commandPane_, 1);

  auto* actions = new QHBoxLayout();
  runButton_ = new QPushButton(QStringLiteral("Optimize GIF"), central);
  runButton_->setEnabled(false);
  connect(runButton_, &QPushButton::clicked, this, &MainWindow::runCommand);
  actions->addWidget(runButton_);
  actions->addStretch();
  statusLabel_ = new QLabel(QStringLiteral("Ready — add an animated GIF to begin."), central);
  actions->addWidget(statusLabel_);
  root->addLayout(actions);
  connect(optimizeSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::refreshCommand);
  connect(lossySpin_, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::refreshCommand);
  setCentralWidget(central);
}

void MainWindow::chooseInputs() {
  const auto files = QFileDialog::getOpenFileNames(this, QStringLiteral("Open GIF files"), QString(), QStringLiteral("GIF files (*.gif);;All files (*)"));
  if (files.isEmpty()) return;
  inputs_ = files; inputList_->clear();
  for (const auto& file : files) inputList_->addItem(QFileInfo(file).fileName());
  runButton_->setEnabled(true); updateStatus(QStringLiteral("%1 GIF file(s) ready.").arg(files.size()));
  refreshCommand();
}

void MainWindow::chooseOutput() {
  const auto file = QFileDialog::getSaveFileName(this, QStringLiteral("Save optimized GIF"), QString(), QStringLiteral("GIF files (*.gif)"));
  if (!file.isEmpty()) { outputEdit_->setText(file); refreshCommand(); }
}

gs::Settings MainWindow::currentSettings() const {
  gs::Settings s; s.mode = gs::Mode::Merge; s.inputs = inputs_.toStdVector();
  s.optimize_level = optimizeSpin_->value();
  if (lossySpin_->value() > 0) s.lossy = lossySpin_->value();
  s.output = outputEdit_->text().toStdString();
  return s;
}

void MainWindow::refreshCommand() {
  const auto settings = currentSettings();
  gs::GifsicleCommand cmd(settings);
  commandPane_->setPlainText(enginePath_ + QStringLiteral(" ") + QString::fromStdString(cmd.toString()));
}

void MainWindow::runCommand() {
  const auto settings = currentSettings();
  gs::GifsicleCommand cmd(settings);
  QProcess proc(this); proc.setProgram(enginePath_); proc.setArguments(QStringList::fromStdVector(cmd.args()));
  proc.start();
  if (!proc.waitForStarted(3000)) {
    QMessageBox::critical(this, QStringLiteral("Gifscythe"), QStringLiteral("Could not start gifsicle: %1").arg(proc.errorString())); return;
  }
  proc.waitForFinished(60000);
  const auto err = QString::fromLocal8Bit(proc.readAllStandardError());
  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
    updateStatus(QStringLiteral("Optimization failed."));
    QMessageBox::warning(this, QStringLiteral("Gifscythe"), err.isEmpty() ? QStringLiteral("gifsicle returned an error.") : err);
  } else {
    updateStatus(QStringLiteral("Optimization complete."));
  }
}

void MainWindow::updateStatus(const QString& message) { statusLabel_->setText(message); }
