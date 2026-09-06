// MainWindow.cpp - implementation of the Gifscythe GUI shell.

#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("Gifscythe 0.1.0"));
  resize(720, 480);

  // Actions
  auto* openAct = new QAction(QStringLiteral("&Open GIF files..."), this);
  openAct->setShortcut(QKeySequence::Open);
  connect(openAct, &QAction::triggered, this, &MainWindow::chooseInputs);
  auto* quitAct = new QAction(QStringLiteral("&Quit"), this);
  quitAct->setShortcut(QKeySequence::Quit);
  connect(quitAct, &QAction::triggered, this, &QWidget::close);
  QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
  fileMenu->addAction(openAct);
  fileMenu->addSeparator();
  fileMenu->addAction(quitAct);

  // Central widget: inputs on the left, live command pane + actions on the right.
  auto* central = new QWidget(this);
  auto* root = new QVBoxLayout(central);

  auto* topRow = new QHBoxLayout();
  inputList_ = new QListWidget(central);
  inputList_->setMinimumWidth(220);
  topRow->addWidget(inputList_, 1);
  root->addLayout(topRow);

  commandPane_ = new QPlainTextEdit(central);
  commandPane_->setReadOnly(true);
  commandPane_->setPlainText(QStringLiteral("# choose GIF files to begin"));
  root->addWidget(commandPane_);

  auto* btnRow = new QHBoxLayout();
  runButton_ = new QPushButton(QStringLiteral("Run command"), central);
  connect(runButton_, &QPushButton::clicked, this, &MainWindow::runCommand);
  auto* refreshBtn = new QPushButton(QStringLiteral("Refresh command"), central);
  connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshCommand);
  btnRow->addWidget(runButton_);
  btnRow->addWidget(refreshBtn);
  root->addLayout(btnRow);

  setCentralWidget(central);
}

void MainWindow::chooseInputs() {
  QStringList files = QFileDialog::getOpenFileNames(
      this, QStringLiteral("Open GIF files"), QString(),
      QStringLiteral("GIF files (*.gif);;All files (*)"));
  if (files.isEmpty()) return;
  inputs_ = files;
  inputList_->clear();
  for (const auto& f : files) inputList_->addItem(f);
  refreshCommand();
}

gs::Settings MainWindow::currentSettings() const {
  gs::Settings s;
  s.mode = gs::Mode::Merge;
  s.inputs = inputs_.toStdVector();
  // Default friendly settings (these would be bound to sliders/checkboxes):
  s.optimize_level = 3;
  s.loopcount = 0;
  s.delay_cs = 5;
  return s;
}

void MainWindow::refreshCommand() {
  gs::Settings s = currentSettings();
  gs::GifsicleCommand cmd(s);
  // Show the engine + the generated command (the "show me the command" pane).
  commandPane_->setPlainText(
      QStringLiteral("%1 %2").arg(enginePath_,
                                   QString::fromStdString(cmd.toString())));
}

void MainWindow::runCommand() {
  gs::Settings s = currentSettings();
  gs::GifsicleCommand cmd(s);
  QString program = enginePath_;
  QStringList args = QStringList::fromStdVector(cmd.args());

  QProcess proc(this);
  proc.setProgram(program);
  proc.setArguments(args);
  proc.start();
  if (!proc.waitForStarted(3000)) {
    QMessageBox::critical(this, QStringLiteral("Gifscythe"),
                          QStringLiteral("Could not start gifsicle engine.\n%1")
                              .arg(proc.errorString()));
    return;
  }
  proc.waitForFinished(60000);
  QString text = QString::fromLatin1(proc.readAllStandardOutput());
  QString err = QString::fromLatin1(proc.readAllStandardError());
  commandPane_->setPlainText(text + ((err.isEmpty()) ? QString() : (QStringLiteral("\n") + err)));
}
