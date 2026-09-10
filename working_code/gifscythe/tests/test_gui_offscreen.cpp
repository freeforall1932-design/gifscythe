// test_gui_offscreen.cpp - Automated GUI verification for COMPILED_AUDIT §6.B
// + the 2026-09-07 UI retrofit (tabs / full controls / async preview).
//
// Runs the real MainWindow under QT_QPA_PLATFORM=offscreen:
//   T1  defaults: Batch default (E4), engine status (B14), tabs exist
//   T2  live command pane sync (B13)
//   T3  queue ops: drop-append+dedupe (B6/B7), multi-select remove (B8),
//       clear (B9), count/size label
//   T4  Batch run: N inputs -> N outputs, frame counts preserved (B10);
//       explicit Save-as honored (E1)
//   T5  Merge run: 2 inputs -> 1 output, frames = sum (B11)
//   T6  Merge with empty output -> refuses, no silent stdout loss (B12)
//   T7  Explode with empty output -> auto prefix, frames written (E2)
//   T8  Failed engine run -> honest "failed" status + dialog (B4)
//   T9  async start (B1 proxy), busy indicators (B3), cancel mid-run (B2)
//   T10 close window while running kills the engine process (B15)
//   T11 Actions-tab controls map to the right gifsicle flags (S3-4 /
//       U-MISS-13) incl. VP-1 loopcount=0, VP-2 -O0, VP-5 crop plus-form,
//       E7 delay unit label (1/100 s, never ms)
//   T12 before/after preview pipeline: debounced, async, honest captions,
//       savings display, Explode-mode refusal (S3-7)
//   T13 Output tab: batch folder honored; summary honest
//   T14 settings persistence (S7): close saves, relaunch restores (Actions
//       state + batch dir + name template); queue/Save-as NOT restored;
//       corrupt file -> defaults + honest warning status
//   T15 queue reorder (S3-9): move up/down keeps list+model in sync,
//       selection follows, bounds are no-ops, merge order = queue order
//   T17 batch output planning (audit U-01): duplicate targets and
//       target-equals-source are planned up front and REFUSED
//   T16 naming templates (S3-25): default renders <name>_opt.gif (E4),
//       custom template in pane + E2E, separator escape stripped,
//       constant-template multi-file collision REFUSED with dialog
//
// Modal dialogs are recorded and auto-closed by a DialogKiller timer.
// Widget lookup is by objectName (stable against layout changes).

#include "qtui/MainWindow.h"
#include "qtui/DropListWidget.h"
#include "qtui/PreviewPanel.h"
#include "qtui/SettingsPanel.h"

#include "core/EngineLocator.h"
#include "core/ProcessRunner.h"
#include "core/version.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QMimeData>
#include <QMovie>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool cond, const char* what, int line) {
  ++g_checks;
  if (!cond) {
    ++g_failures;
    std::printf("  FAIL (line %d): %s\n", line, what);
  }
}
#define CHECK(cond) check((cond), #cond, __LINE__)
#define CHECK_MSG(cond, msg) check((cond), msg, __LINE__)

// ---- DialogKiller: record + close any modal dialog so tests never block ----
std::vector<QString> g_dialogs;

class DialogKiller : public QObject {
 public:
  explicit DialogKiller(QObject* parent = nullptr) : QObject(parent) {
    connect(&timer_, &QTimer::timeout, this, &DialogKiller::sweep);
    timer_.start(25);
  }
  void sweep() {
    QWidget* m = qApp->activeModalWidget();
    if (!m) return;
    if (auto* mb = qobject_cast<QMessageBox*>(m)) {
      g_dialogs.push_back(mb->text() + QStringLiteral(" | ") + mb->informativeText());
    } else {
      g_dialogs.push_back(m->windowTitle());
    }
    m->close();
  }
  QTimer timer_;
};

bool dialogsContain(const QString& needle) {
  for (const auto& d : g_dialogs)
    if (d.contains(needle, Qt::CaseInsensitive)) return true;
  return false;
}

// ---- objectName-based widget lookup ----
template <class T>
T* byName(QWidget* w, const char* name) {
  return w->findChild<T*>(QString::fromLatin1(name));
}

struct Widgets {
  DropListWidget* list = nullptr;
  QPlainTextEdit* pane = nullptr;
  QComboBox* mode = nullptr;
  QSpinBox* optimize = nullptr;
  QSpinBox* lossy = nullptr;
  QLineEdit* output = nullptr;
  QLineEdit* batchDir = nullptr;
  QPushButton* run = nullptr;
  QPushButton* cancel = nullptr;
  QPushButton* remove = nullptr;
  QPushButton* clear = nullptr;
  QLabel* status = nullptr;
  QProgressBar* progress = nullptr;
  QProcess* process = nullptr;         // main engine run
  QTabWidget* tabs = nullptr;
  QLabel* countLabel = nullptr;
  QLabel* outputSummary = nullptr;
  QLabel* previewCaption = nullptr;
  QLabel* previewSavings = nullptr;
  QLabel* previewBefore = nullptr;
  QLabel* previewAfter = nullptr;
};

Widgets findWidgets(QWidget* w) {
  Widgets x;
  x.list = byName<DropListWidget>(w, "queueList");
  x.pane = byName<QPlainTextEdit>(w, "commandPane");
  x.mode = byName<QComboBox>(w, "modeCombo");
  x.optimize = byName<QSpinBox>(w, "optimizeSpin");
  x.lossy = byName<QSpinBox>(w, "lossySpin");
  x.output = byName<QLineEdit>(w, "outputEdit");
  x.batchDir = byName<QLineEdit>(w, "batchDirEdit");
  x.run = byName<QPushButton>(w, "runButton");
  x.cancel = byName<QPushButton>(w, "cancelButton");
  x.remove = byName<QPushButton>(w, "removeButton");
  x.clear = byName<QPushButton>(w, "clearButton");
  x.status = byName<QLabel>(w, "statusLabel");
  x.progress = byName<QProgressBar>(w, "progressBar");
  x.process = byName<QProcess>(w, "engineProcess");
  x.tabs = byName<QTabWidget>(w, "mainTabs");
  x.countLabel = byName<QLabel>(w, "queueCountLabel");
  x.outputSummary = byName<QLabel>(w, "outputSummary");
  x.previewCaption = byName<QLabel>(w, "previewCaption");
  x.previewSavings = byName<QLabel>(w, "previewSavings");
  x.previewBefore = byName<QLabel>(w, "previewBefore");
  x.previewAfter = byName<QLabel>(w, "previewAfter");
  return x;
}

bool waitForStatus(QWidget* w, const QString& needle, int timeoutMs = 45000) {
  QElapsedTimer el;
  el.start();
  while (el.elapsed() < timeoutMs) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(2);
    auto* st = byName<QLabel>(w, "statusLabel");
    if (st && st->text().contains(needle, Qt::CaseInsensitive)) return true;
  }
  return false;
}

// Wait until a label's text contains needle (or timeout).
bool waitForLabel(QLabel* label, const QString& needle, int timeoutMs = 25000) {
  if (!label) return false;
  QElapsedTimer el;
  el.start();
  while (el.elapsed() < timeoutMs) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(2);
    if (label->text().contains(needle, Qt::CaseInsensitive)) return true;
  }
  return false;
}

void spinEvents(int ms) {
  QElapsedTimer el;
  el.start();
  while (el.elapsed() < ms) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(2);
  }
}

// ---- File / engine helpers ----
QString g_engine;
QString g_refDir;

QString findRefDir() {
  const fs::path exe = QCoreApplication::applicationFilePath().toStdString();
  const fs::path dir = exe.parent_path();
  std::vector<fs::path> cands;
  if (const char* env = std::getenv("GS_TEST_REF_DIR")) cands.emplace_back(env);
  cands.push_back(dir / ".." / ".." / "reference_code" / "gifsicle");
  cands.push_back(dir / ".." / ".." / ".." / "reference_code" / "gifsicle");
  cands.push_back(dir / ".." / "reference_code" / "gifsicle");
  for (const auto& c : cands) {
    std::error_code ec;
    if (fs::exists(c / "logo.gif", ec)) return QString::fromStdString(fs::weakly_canonical(c, ec).string());
  }
  return {};
}

bool copyFile(const QString& src, const QString& dst) {
  QFile::remove(dst);
  return QFile::copy(src, dst);
}

int frameCount(const QString& file) {
  QProcess p;
  p.start(g_engine, {QStringLiteral("--info"), file});
  if (!p.waitForStarted(15000)) return -1;
  if (!p.waitForFinished(60000)) { p.kill(); return -1; }
  const QString out = QString::fromUtf8(p.readAllStandardOutput())
                    + QString::fromUtf8(p.readAllStandardError());
  static const QRegularExpression re(QStringLiteral("(\\d+) images?"));
  const auto m = re.match(out);
  return m.hasMatch() ? m.captured(1).toInt() : -1;
}

bool makeBigGif(const QString& src, const QString& dest, int copies) {
  QStringList args;
  for (int i = 0; i < copies; ++i) args << src;
  args << QStringLiteral("-o") << dest;
  QProcess p;
  p.start(g_engine, args);
  if (!p.waitForStarted(15000)) return false;
  if (!p.waitForFinished(180000)) { p.kill(); return false; }
  return p.exitCode() == 0 && QFileInfo::exists(dest);
}

// Deliver files to the queue the way a drop would.
//
// Note: Qt only dispatches QDropEvents to widgets while a real platform
// drag session is active, so the harness emits DropListWidget::filesDropped —
// the exact signal the drop handler emits (see audit §6.B B6 note).
void dropFiles(QWidget* w, const QStringList& files) {
  auto* list = byName<DropListWidget>(w, "queueList");
  list->filesDropped(files);  // signals are public in Qt5+
  QCoreApplication::processEvents();
}

MainWindow* makeWindow() {
  auto* w = new MainWindow();
  w->show();  // offscreen: makes isVisible() meaningful for progress bar etc.
  QCoreApplication::processEvents();
  return w;
}

// Path stored in a queue row (display text now carries size info).
QString rowPath(DropListWidget* list, int row) {
  auto* item = list->item(row);
  return item ? item->data(Qt::UserRole).toString() : QString();
}

}  // namespace

// Stage tracker + watchdog: on Windows CI the harness once hung with ZERO
// output before its first printf (run #20). The watchdog thread starts
// before QApplication exists, so even a hang inside platform-plugin init
// reports where we got stuck, then exits 124 instead of burning CI hours.
static std::string g_stage = "process start";

int main(int argc, char** argv) {
  setbuf(stdout, nullptr);  // unbuffered: crash diagnostics keep our trace
  setvbuf(stderr, nullptr, _IONBF, 0);
  std::thread watchdog([] {
    std::this_thread::sleep_for(std::chrono::seconds(480));
    std::fprintf(stderr, "[harness] WATCHDOG (8 min): stuck at stage: %s\n",
                 g_stage.c_str());
    std::fflush(stderr);
    _exit(124);
  });
  watchdog.detach();

  g_stage = "constructing QApplication (platform plugin init)";
  qputenv("QT_QPA_PLATFORM", "offscreen");  // force offscreen even if unset
  QApplication app(argc, argv);
  // S7: isolate session persistence from the real user config dir — every
  // window in this run shares one scratch settings file (T14 manages its own
  // GS_SETTINGS_PATH and restores this one). Intentional leak: lives for the
  // whole run, process exit cleans up.
  static QTemporaryDir* g_cfgHome = new QTemporaryDir();
  if (g_cfgHome->isValid()) {
    qputenv("GS_SETTINGS_PATH",
            QFile::encodeName(g_cfgHome->path() + QStringLiteral("/gifscythe.conf")));
  }
  g_stage = "DialogKiller + header print";
  DialogKiller killer;

  std::printf("==> GUI offscreen tests (COMPILED_AUDIT 6.B + retrofit harness)\n");
  g_stage = "engine/ref discovery";

  g_engine = QString::fromStdString(
      gs::locate_engine(QCoreApplication::applicationFilePath().toStdString()));
  if (!gs::path_is_executable(g_engine.toStdString())) {
    std::printf("FATAL: gifsicle engine not found (tried from %s). "
                "Run ./build.sh first or set GS_ENGINE.\n",
                qPrintable(QCoreApplication::applicationFilePath()));
    return 1;
  }
  g_refDir = findRefDir();
  if (g_refDir.isEmpty()) {
    std::printf("FATAL: reference_code/gifsicle not found (set GS_TEST_REF_DIR).\n");
    return 1;
  }
  std::printf("  engine: %s\n  refs:   %s\n", qPrintable(g_engine), qPrintable(g_refDir));

  const QString logo = g_refDir + QStringLiteral("/logo.gif");    // 12 frames
  const QString logo1 = g_refDir + QStringLiteral("/logo1.gif");  // 1 frame

  // ================= T1: defaults (E4, B14, tabs) ========================
  {
    g_stage = "T1"; std::printf("== T1 defaults + tabs ==\n");
    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    CHECK(x.list && x.pane && x.mode && x.optimize && x.lossy && x.output &&
          x.run && x.cancel && x.remove && x.clear && x.progress && x.process &&
          x.tabs && x.countLabel && x.outputSummary && x.previewCaption);
    // S7 additions: reorder buttons + naming template (default = historical
    // <name>_opt.gif behavior, audit E4).
    CHECK(byName<QPushButton>(w, "moveUpButton") != nullptr);
    CHECK(byName<QPushButton>(w, "moveDownButton") != nullptr);
    auto* tmplEdit = byName<QLineEdit>(w, "nameTemplateEdit");
    CHECK(tmplEdit && tmplEdit->text() == QStringLiteral("{name}_opt.gif"));
    if (x.tabs) {
      CHECK(x.tabs->count() == 3);
      CHECK(x.tabs->tabText(0) == QStringLiteral("Input"));
      CHECK(x.tabs->tabText(1) == QStringLiteral("Actions"));
      CHECK(x.tabs->tabText(2) == QStringLiteral("Output"));
    }
    if (x.mode) {
      CHECK(x.mode->currentIndex() == 0);
      CHECK(x.mode->currentData().toInt() == static_cast<int>(gs::Mode::Batch));  // E4
    }
    if (x.status) {
      CHECK(x.status->text().contains(QStringLiteral("engine"), Qt::CaseInsensitive));  // B14
      CHECK(x.status->text().contains(g_engine));
    }
    if (x.run) CHECK(!x.run->isEnabled());  // empty queue -> Run disabled (B14)
    CHECK(w->windowTitle().contains(QStringLiteral(GS_VERSION)));  // A5 in-GUI
    delete w;
  }

  // ================= T2: live command pane sync (B13) ====================
  {
    g_stage = "T2"; std::printf("== T2 live pane ==\n");
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    CHECK(copyFile(logo, a));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a});
    spinEvents(50);
    QString pane = x.pane->toPlainText();
    CHECK(pane.contains(QStringLiteral("-O3")));            // optimize default
    CHECK(pane.contains(a));                                 // input present
    CHECK(pane.contains(gs::shell_quote(g_engine.toStdString()).c_str()));
    // Batch runs one per-file Auto command — never a single "-b …" line.
    CHECK(!pane.contains(QStringLiteral(" -b ")));
    CHECK(pane.contains(QStringLiteral("a_opt.gif")));       // real derived output

    x.optimize->setValue(2);
    spinEvents(20);
    CHECK(x.pane->toPlainText().contains(QStringLiteral("-O2")));

    x.lossy->setValue(40);
    spinEvents(20);
    CHECK(x.pane->toPlainText().contains(QStringLiteral("--lossy=40")));

    x.mode->setCurrentIndex(1);  // Merge
    spinEvents(20);
    CHECK(x.pane->toPlainText().contains(QStringLiteral("-m")));

    const QString spaced = tmp.path() + QStringLiteral("/dir with space/out.gif");
    x.output->setText(spaced);
    spinEvents(20);
    pane = x.pane->toPlainText();
    CHECK_MSG(pane.contains(QLatin1Char('\'')), "space path is shell-quoted in pane");
    CHECK(pane.contains(QStringLiteral("dir with space")));

    x.mode->setCurrentIndex(0);  // back to Batch
    delete w;
  }

  // ================= T3: queue ops (B6/B7/B8/B9) =========================
  {
    g_stage = "T3"; std::printf("== T3 queue ops ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    const QString b = tmp.path() + QStringLiteral("/b.gif");
    const QString c = tmp.path() + QStringLiteral("/c.gif");
    CHECK(copyFile(logo, a));
    CHECK(copyFile(logo1, b));
    CHECK(copyFile(logo, c));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);

    dropFiles(w, {a, b});  // B6: drop appends
    CHECK(x.list->count() == 2);
    dropFiles(w, {a, c});  // B7: duplicate a ignored, c appended
    CHECK(x.list->count() == 3);
    CHECK(rowPath(x.list, 2) == c);
    if (auto* it = x.list->item(2)) {
      CHECK(it->text().startsWith(QStringLiteral("c.gif")));  // "name — size" format
    } else { CHECK_MSG(false, "item(2) exists after dedupe drop"); }
    CHECK(x.run->isEnabled());
    CHECK_MSG(x.countLabel->text().contains(QStringLiteral("3 file")),
              "count label shows 3 files");

    // B8: multi-select remove keeps indices consistent
    if (x.list->count() == 3) {
      x.list->item(0)->setSelected(true);
      x.list->item(2)->setSelected(true);
      x.remove->click();
      spinEvents(20);
      CHECK(x.list->count() == 1);
      CHECK(rowPath(x.list, 0) == b);
    } else {
      CHECK_MSG(false, "queue has 3 items before multi-remove");
    }

    // B9: clear empties queue + disables Run
    x.clear->click();
    spinEvents(20);
    CHECK(x.list->count() == 0);
    CHECK(!x.run->isEnabled());
    CHECK(x.countLabel->text().contains(QStringLiteral("0 file")));
    delete w;
  }

  // ================= T4: Batch E2E (B10) + explicit output (E1) ==========
  {
    g_stage = "T4"; std::printf("== T4 batch E2E ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");   // 12 frames
    const QString b = tmp.path() + QStringLiteral("/b.gif");   // 1 frame
    CHECK(copyFile(logo, a));
    CHECK(copyFile(logo1, b));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a, b});
    CHECK(x.mode->currentData().toInt() == static_cast<int>(gs::Mode::Batch));

    QElapsedTimer clickTime;
    clickTime.start();
    x.run->click();  // async: must NOT block for the whole run
    const qint64 clickMs = clickTime.elapsed();
    CHECK_MSG(clickMs < 3000, "Run click returns fast (no waitForFinished block)");

    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "batch reports complete");
    const QString aOut = tmp.path() + QStringLiteral("/a_opt.gif");
    const QString bOut = tmp.path() + QStringLiteral("/b_opt.gif");
    CHECK(QFileInfo::exists(aOut));
    CHECK(QFileInfo::exists(bOut));
    CHECK(QFileInfo(aOut).size() > 0);
    CHECK(QFileInfo(bOut).size() > 0);
    CHECK_MSG(frameCount(aOut) == 12, "batch preserves frame count (a: 12)");
    CHECK_MSG(frameCount(bOut) == 1, "batch preserves frame count (b: 1)");
    delete w;

    // E1: single file + explicit Save-as path is honored (not only _opt.gif)
    QTemporaryDir tmp2;
    const QString a2 = tmp2.path() + QStringLiteral("/a.gif");
    CHECK(copyFile(logo, a2));
    MainWindow* w2 = makeWindow();
    auto x2 = findWidgets(w2);
    dropFiles(w2, {a2});
    const QString explicitOut = tmp2.path() + QStringLiteral("/my explicit result.gif");
    x2.output->setText(explicitOut);
    x2.run->click();
    CHECK_MSG(waitForStatus(w2, QStringLiteral("complete")), "explicit-output batch completes");
    CHECK(QFileInfo::exists(explicitOut));
    CHECK(!QFileInfo::exists(tmp2.path() + QStringLiteral("/a_opt.gif")));
    delete w2;
  }

  // ================= T5: Merge E2E (B11) =================================
  {
    g_stage = "T5"; std::printf("== T5 merge E2E ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    const QString b = tmp.path() + QStringLiteral("/b.gif");
    CHECK(copyFile(logo, a));
    CHECK(copyFile(logo1, b));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a, b});
    x.mode->setCurrentIndex(1);  // Merge (explicit choice)
    const QString merged = tmp.path() + QStringLiteral("/merged.gif");
    x.output->setText(merged);
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "merge reports complete");
    CHECK(QFileInfo::exists(merged));
    CHECK_MSG(frameCount(merged) == 13, "merge frame count = 12 + 1");
    CHECK(!QFileInfo::exists(tmp.path() + QStringLiteral("/a_opt.gif")));  // not batch
    delete w;
  }

  // ================= T6: Merge empty output refuses (B12) ================
  {
    g_stage = "T6"; std::printf("== T6 merge refuse ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    const QString b = tmp.path() + QStringLiteral("/b.gif");
    CHECK(copyFile(logo, a));
    CHECK(copyFile(logo1, b));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a, b});
    x.mode->setCurrentIndex(1);  // Merge
    x.output->clear();
    g_dialogs.clear();
    x.run->click();
    spinEvents(120);
    CHECK_MSG(dialogsContain(QStringLiteral("output")),
              "merge w/o output warns via dialog");
    CHECK(x.process->state() == QProcess::NotRunning);  // never started
    CHECK(!QFileInfo::exists(tmp.path() + QStringLiteral("/a_opt.gif")));
    CHECK(x.outputSummary->text().contains(QStringLiteral("REQUIRED")));
    delete w;
  }

  // ================= T7: Explode auto-prefix (E2) ========================
  {
    g_stage = "T7"; std::printf("== T7 explode ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    CHECK(copyFile(logo, a));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a});
    x.mode->setCurrentIndex(2);  // Explode
    x.output->clear();
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "explode reports complete");
    // gifsicle -e -o <stem>_frame writes <stem>_frame.NNN
    const QString prefix = tmp.path() + QStringLiteral("/a_frame");
    CHECK(QFileInfo::exists(prefix + QStringLiteral(".000")));
    CHECK(QFileInfo::exists(prefix + QStringLiteral(".011")));
    delete w;
  }

  // ================= T8: failed run is honest (B4) =======================
  {
    g_stage = "T8"; std::printf("== T8 failure honesty ==\n");
    QTemporaryDir tmp;
    // The engine must fail, so the file has to exist and be readable — the
    // drop filter (audit U-13) is now ".gif" AND exists, so a merely
    // nonexistent path can no longer be used to reach the failure path.
    const QString ghost = tmp.path() + QStringLiteral("/ghost.gif");
    {
      QFile f(ghost);
      CHECK(f.open(QIODevice::WriteOnly));
      f.write("this is not a GIF file at all\n");
      f.close();
    }

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {ghost});  // exists + .gif suffix passes the drop filter
    CHECK(x.list->count() == 1);

    // U-13 regression: the filter used to be `endsWith(".gif") || exists(f)`,
    // which queued ANY existing file (.exe, .jpg, .txt) and let it fail only
    // at run time. Both halves must hold now.
    const QString notGif = tmp.path() + QStringLiteral("/notes.txt");
    {
      QFile f(notGif);
      CHECK(f.open(QIODevice::WriteOnly));
      f.write("hello");
      f.close();
    }
    dropFiles(w, {notGif});                                    // exists, not .gif
    CHECK_MSG(x.list->count() == 1, "existing non-.gif file is rejected by the drop filter");
    dropFiles(w, {tmp.path() + QStringLiteral("/missing.gif")});  // .gif, absent
    CHECK_MSG(x.list->count() == 1, "nonexistent .gif is rejected by the drop filter");
    g_dialogs.clear();
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("failed")), "failed run says failed");
    CHECK(x.process->state() == QProcess::NotRunning);
    CHECK(x.run->isEnabled());       // busy cleared honestly, Run re-enabled
    CHECK(!x.cancel->isEnabled());
    CHECK(!g_dialogs.empty());       // failure surfaced as dialog too
    delete w;
  }

  // ================= T9: busy UI + cancel mid-run (B1/B2/B3) =============
  {
    g_stage = "T9"; std::printf("== T9 cancel mid-run ==\n");
    QTemporaryDir tmp;
    const QString big = tmp.path() + QStringLiteral("/big.gif");
    std::printf("  (building 4800-frame GIF for a multi-second run...)\n");
    CHECK_MSG(makeBigGif(logo, big, 400), "big.gif generated");

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {big});
    x.optimize->setValue(3);
    x.lossy->setValue(100);  // slow path

    QElapsedTimer clickTime;
    clickTime.start();
    x.run->click();
    CHECK_MSG(clickTime.elapsed() < 3000, "start does not block UI thread");

    // Busy indicators (B3)
    CHECK(x.progress->isVisible());
    CHECK(x.cancel->isEnabled());
    CHECK(!x.run->isEnabled());

    // UI thread alive while engine runs (B1 proxy): count timer ticks
    int ticks = 0;
    QTimer ticker;
    ticker.setInterval(10);
    QObject::connect(&ticker, &QTimer::timeout, [&ticks]() { ++ticks; });
    ticker.start();
    spinEvents(600);
    ticker.stop();
    CHECK_MSG(ticks >= 20, "event loop keeps ticking during run (UI not blocked)");
    CHECK_MSG(x.process->state() == QProcess::Running, "engine still running at ~0.6s");

    // Cancel mid-run (B2)
    g_dialogs.clear();  // isolate the cancel window: no dialog may appear here
    x.cancel->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("Cancelled"), 10000), "status shows Cancelled");
    CHECK(x.process->state() == QProcess::NotRunning);  // no zombie
    CHECK(!x.cancel->isEnabled());
    CHECK(x.run->isEnabled());           // controls re-enabled
    CHECK(!x.progress->isVisible());     // busy indicator cleared
    // A cancel is expected, not an error: no "optimization failed" dialog.
    CHECK_MSG(g_dialogs.empty(), "cancel does not pop a spurious error dialog");
    delete w;
  }

  // ================= T10: close while running (B15) ======================
  {
    g_stage = "T10"; std::printf("== T10 close while running ==\n");
    QTemporaryDir tmp;
    const QString big = tmp.path() + QStringLiteral("/big.gif");
    CHECK_MSG(makeBigGif(logo, big, 400), "big.gif generated");

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {big});
    x.optimize->setValue(3);
    x.lossy->setValue(100);
    x.run->click();
    spinEvents(400);
    CHECK(x.process->state() == QProcess::Running);
    w->close();  // closeEvent -> cancelRun -> kill
    spinEvents(200);
    CHECK_MSG(x.process->state() == QProcess::NotRunning, "engine killed on close");
    delete w;
  }

  // ================= T11: Actions controls -> gifsicle flags =============
  {
    g_stage = "T11"; std::printf("== T11 control mapping ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    CHECK(copyFile(logo, a));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a});
    const auto paneText = [&x]() { return x.pane->toPlainText(); };
    const auto setAndWait = [&]() { spinEvents(15); };

    // E7 guard: the delay label must state 1/100 s — never "ms".
    auto* delayLabel = byName<QLabel>(w, "delayLabel");
    CHECK(delayLabel && delayLabel->text().contains(QStringLiteral("1/100")));
    CHECK(delayLabel && !delayLabel->text().contains(QStringLiteral("ms"), Qt::CaseInsensitive));

    // Animation: delay (-d), loop (VP-1), disposal, threads, unoptimize
    auto* delayCheck = byName<QCheckBox>(w, "delayCheck");
    auto* delaySpin = byName<QSpinBox>(w, "delaySpin");
    delayCheck->setChecked(true);
    delaySpin->setValue(7);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("-d 7")));

    auto* loopCombo = byName<QComboBox>(w, "loopCombo");
    loopCombo->setCurrentIndex(1);  // Loop forever
    setAndWait();
    CHECK_MSG(paneText().contains(QStringLiteral("--loopcount=0")), "forever = --loopcount=0 (VP-1)");
    loopCombo->setCurrentIndex(2);  // Loop N times
    auto* loopSpin = byName<QSpinBox>(w, "loopSpin");
    loopSpin->setValue(5);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--loopcount=5")));

    auto* disposalCombo = byName<QComboBox>(w, "disposalCombo");
    disposalCombo->setCurrentIndex(3);  // background (2)
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--disposal 2")));

    auto* threadsSpin = byName<QSpinBox>(w, "threadsSpin");
    threadsSpin->setValue(2);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("-j2")));

    auto* unoptCheck = byName<QCheckBox>(w, "unoptimizeCheck");
    unoptCheck->setChecked(true);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("-U")));

    // Optimize: -O0 is real (VP-2); colors; dither names; color method; careful
    x.optimize->setValue(0);
    setAndWait();
    CHECK_MSG(paneText().contains(QStringLiteral("-O0")), "-O0 emitted for level 0 (VP-2)");
    x.optimize->setValue(3);
    setAndWait();

    auto* colorsCheck = byName<QCheckBox>(w, "colorsCheck");
    auto* colorsSpin = byName<QSpinBox>(w, "colorsSpin");
    colorsCheck->setChecked(true);
    colorsSpin->setValue(64);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("-k 64")));

    auto* ditherCombo = byName<QComboBox>(w, "ditherCombo");
    ditherCombo->setCurrentIndex(1);  // Default -> bare -f
    setAndWait();
    {
      const QString pane = paneText();
      CHECK_MSG(pane.contains(QStringLiteral(" -f ")) || pane.endsWith(QStringLiteral(" -f")),
                "dither Default emits bare -f");
    }
    int ro64Idx = ditherCombo->findData(QStringLiteral("ro64"));
    CHECK(ro64Idx >= 0);
    ditherCombo->setCurrentIndex(ro64Idx);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--dither=ro64")));

    auto* colorMethodCombo = byName<QComboBox>(w, "colorMethodCombo");
    int medIdx = colorMethodCombo->findData(QStringLiteral("median-cut"));
    CHECK(medIdx >= 0);
    colorMethodCombo->setCurrentIndex(medIdx);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--color-method median-cut")));

    auto* carefulCheck = byName<QCheckBox>(w, "carefulCheck");
    carefulCheck->setChecked(true);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--careful")));

    // Resize: fit + method; scale percent
    auto* resizeKind = byName<QComboBox>(w, "resizeKindCombo");
    resizeKind->setCurrentIndex(1);  // Fit inside
    auto* resizeW = byName<QSpinBox>(w, "resizeWSpin");
    auto* resizeH = byName<QSpinBox>(w, "resizeHSpin");
    resizeW->setValue(320);
    resizeH->setValue(200);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--resize-fit 320x200")));

    auto* resizeMethod = byName<QComboBox>(w, "resizeMethodCombo");
    int lanczosIdx = resizeMethod->findData(QStringLiteral("lanczos3"));
    CHECK(lanczosIdx >= 0);
    resizeMethod->setCurrentIndex(lanczosIdx);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--resize-method lanczos3")));

    resizeKind->setCurrentIndex(4);  // Scale by %
    auto* scaleX = byName<QDoubleSpinBox>(w, "scaleXSpin");
    auto* scaleY = byName<QDoubleSpinBox>(w, "scaleYSpin");
    scaleX->setValue(50.0);
    scaleY->setValue(50.0);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--scale 0.5x0.5")));
    resizeKind->setCurrentIndex(0);  // No resize (clean pane for later checks)
    setAndWait();

    // Geometry: rotate/flip/position/interlace
    auto* rotateCombo = byName<QComboBox>(w, "rotateCombo");
    rotateCombo->setCurrentIndex(1);  // 90
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--rotate-90")));
    auto* flipH = byName<QCheckBox>(w, "flipHCheck");
    flipH->setChecked(true);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--flip-horizontal")));
    auto* posCheck = byName<QCheckBox>(w, "positionCheck");
    auto* posX = byName<QSpinBox>(w, "posXSpin");
    auto* posY = byName<QSpinBox>(w, "posYSpin");
    posCheck->setChecked(true);
    posX->setValue(5);
    posY->setValue(6);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("-p 5,6")));
    auto* interlace = byName<QCheckBox>(w, "interlaceCheck");
    interlace->setChecked(true);
    setAndWait();
    {
      const QString pane = paneText();
      CHECK_MSG(pane.contains(QStringLiteral(" -i ")) || pane.endsWith(QStringLiteral(" -i")),
                "interlace emits -i");
    }

    // Crop: plus-form X,Y+WxH (VP-5) + crop-transparency
    auto* cropCheck = byName<QCheckBox>(w, "cropCheck");
    auto* cropX = byName<QSpinBox>(w, "cropXSpin");
    auto* cropY = byName<QSpinBox>(w, "cropYSpin");
    auto* cropW = byName<QSpinBox>(w, "cropWSpin");
    auto* cropH = byName<QSpinBox>(w, "cropHSpin");
    cropCheck->setChecked(true);
    cropX->setValue(1);
    cropY->setValue(2);
    cropW->setValue(30);
    cropH->setValue(40);
    setAndWait();
    CHECK_MSG(paneText().contains(QStringLiteral("--crop 1,2+30x40")),
              "crop uses plus-form X,Y+WxH (VP-5)");
    auto* cropT = byName<QCheckBox>(w, "cropTransparencyCheck");
    cropT->setChecked(true);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--crop-transparency")));

    // Colors: gamma (VP-3 — only emitted when chosen), background, transparent
    auto* gammaCombo = byName<QComboBox>(w, "gammaCombo");
    CHECK_MSG(!paneText().contains(QStringLiteral("--gamma")), "no --gamma unless chosen (VP-3)");
    gammaCombo->setCurrentIndex(1);  // sRGB
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--gamma=srgb")));
    gammaCombo->setCurrentIndex(2);  // Oklab
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--gamma=oklab")));

    auto* bgEdit = byName<QLineEdit>(w, "backgroundEdit");
    bgEdit->setText(QStringLiteral("#ffffff"));
    setAndWait();
    // '#' is not shell-safe, so the display pane quotes it (shell_quote).
    CHECK(paneText().contains(QStringLiteral("--background '#ffffff'")));
    auto* trEdit = byName<QLineEdit>(w, "transparentEdit");
    trEdit->setText(QStringLiteral("#ff0000"));
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--transparent '#ff0000'")));

    // Metadata: removals + comments (quoted in pane)
    auto* rmComments = byName<QCheckBox>(w, "removeCommentsCheck");
    rmComments->setChecked(true);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--no-comments")));
    auto* rmNames = byName<QCheckBox>(w, "removeNamesCheck");
    rmNames->setChecked(true);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--no-names")));
    auto* rmExt = byName<QCheckBox>(w, "removeExtensionsCheck");
    rmExt->setChecked(true);
    setAndWait();
    CHECK(paneText().contains(QStringLiteral("--no-extensions")));

    auto* commentEdit = byName<QLineEdit>(w, "commentEdit");
    auto* addComment = byName<QPushButton>(w, "addCommentButton");
    commentEdit->setText(QStringLiteral("hello world"));
    addComment->click();
    setAndWait();
    CHECK_MSG(paneText().contains(QStringLiteral("'hello world'")),
              "comment with space is shell-quoted in pane");

    // Explode by name (-E) — mode-dependent control
    x.mode->setCurrentIndex(2);  // Explode
    setAndWait();
    auto* explodeByName = byName<QCheckBox>(w, "explodeByNameCheck");
    CHECK(explodeByName->isEnabled());
    explodeByName->setChecked(true);
    setAndWait();
    CHECK_MSG(paneText().contains(QStringLiteral("-E")), "explode-by-name emits -E");

    delete w;
  }

  // ================= T12: preview pipeline (S3-7) ========================
  {
    g_stage = "T12"; std::printf("== T12 preview ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    CHECK(copyFile(logo, a));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a});
    // appendInputs auto-selects row 0 -> before movie should load
    spinEvents(150);
    CHECK(x.previewBefore && x.previewAfter && x.previewSavings && x.previewCaption);
    if (!x.previewBefore) { delete w; return 1; }
    CHECK_MSG(x.previewBefore->movie() != nullptr, "before pane plays the selected original");

    // Debounced async run: after ~1.2s + engine time the savings appear.
    CHECK_MSG(waitForLabel(x.previewSavings, QStringLiteral("→"), 25000),
              "preview produced after-image with size comparison");
    CHECK(x.previewAfter->movie() != nullptr);
    // Caption stays honest about single-file semantics
    CHECK(x.previewCaption->text().contains(QStringLiteral("SELECTED")));

    // Changing a control re-triggers the debounced preview (savings updates)
    x.optimize->setValue(1);
    CHECK_MSG(waitForLabel(x.previewSavings, QStringLiteral("→"), 25000),
              "preview regenerates after control change");

    // Explode mode: preview honestly refuses (multi-file output)
    x.mode->setCurrentIndex(2);
    CHECK_MSG(waitForLabel(x.previewCaption, QStringLiteral("Explode"), 25000),
              "preview honestly unavailable in Explode mode");
    x.mode->setCurrentIndex(0);
    delete w;
  }

  // ================= T13: Output tab (batch folder + summary) ============
  {
    g_stage = "T13"; std::printf("== T13 output tab ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    const QString b = tmp.path() + QStringLiteral("/b.gif");
    CHECK(copyFile(logo, a));
    CHECK(copyFile(logo1, b));
    const QString outDir = tmp.path() + QStringLiteral("/outdir");
    CHECK(QDir().mkpath(outDir));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {a, b});
    CHECK(x.outputSummary->text().contains(QStringLiteral("_opt.gif")));
    x.batchDir->setText(outDir);
    spinEvents(30);
    CHECK(x.outputSummary->text().contains(outDir));
    // The live pane must describe the real per-file runs, not a single -b.
    {
      const QString pane = x.pane->toPlainText();
      CHECK(pane.contains(QStringLiteral("# batch")));
      CHECK(pane.contains(QStringLiteral("a_opt.gif")));
      CHECK(pane.contains(QStringLiteral("b_opt.gif")));
      CHECK(!pane.contains(QStringLiteral(" -b ")));
    }

    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "batch with custom folder completes");
    CHECK(QFileInfo::exists(outDir + QStringLiteral("/a_opt.gif")));
    CHECK(QFileInfo::exists(outDir + QStringLiteral("/b_opt.gif")));
    CHECK(!QFileInfo::exists(tmp.path() + QStringLiteral("/a_opt.gif")));  // not beside inputs
    CHECK(byName<QPushButton>(w, "openDirButton") != nullptr);
    delete w;
  }

  // ================= T14: settings persistence (S7) ======================
  {
    g_stage = "T14"; std::printf("== T14 settings persistence ==\n");
    const QByteArray prevEnv = qgetenv("GS_SETTINGS_PATH");
    QTemporaryDir cfg;
    CHECK(cfg.isValid());
    const QString cfgPath = cfg.path() + QStringLiteral("/gifscythe.conf");
    qputenv("GS_SETTINGS_PATH", QFile::encodeName(cfgPath));

    // First launch: no file -> defaults, and nothing is written until close.
    {
      MainWindow* w = makeWindow();
      auto x = findWidgets(w);
      CHECK(x.mode->currentData().toInt() == static_cast<int>(gs::Mode::Batch));  // E4
      CHECK(!QFileInfo::exists(cfgPath));
      delete w;
    }

    // Session A: change controls across every group + batch dir + template,
    // then close — closeEvent must save the session.
    {
      MainWindow* w = makeWindow();
      byName<QComboBox>(w, "modeCombo")->setCurrentIndex(1);  // Merge
      byName<QSpinBox>(w, "optimizeSpin")->setValue(1);
      byName<QSpinBox>(w, "lossySpin")->setValue(42);
      byName<QCheckBox>(w, "colorsCheck")->setChecked(true);
      byName<QSpinBox>(w, "colorsSpin")->setValue(64);
      auto* dither = byName<QComboBox>(w, "ditherCombo");
      dither->setCurrentIndex(dither->findData(QStringLiteral("ro64")));
      byName<QComboBox>(w, "resizeKindCombo")->setCurrentIndex(1);  // Fit
      byName<QSpinBox>(w, "resizeWSpin")->setValue(222);
      byName<QCheckBox>(w, "delayCheck")->setChecked(true);
      byName<QSpinBox>(w, "delaySpin")->setValue(7);
      byName<QComboBox>(w, "loopCombo")->setCurrentIndex(2);  // Loop N
      byName<QSpinBox>(w, "loopSpin")->setValue(5);
      byName<QComboBox>(w, "gammaCombo")->setCurrentIndex(3);  // Custom
      byName<QLineEdit>(w, "gammaEdit")->setText(QStringLiteral("2.35"));
      byName<QLineEdit>(w, "backgroundEdit")->setText(QStringLiteral("#abcdef"));
      byName<QCheckBox>(w, "carefulCheck")->setChecked(true);
      byName<QCheckBox>(w, "flipHCheck")->setChecked(true);
      byName<QLineEdit>(w, "commentEdit")->setText(QStringLiteral("persist me"));
      byName<QPushButton>(w, "addCommentButton")->click();
      const QString outDir = cfg.path() + QStringLiteral("/outdir");
      byName<QLineEdit>(w, "batchDirEdit")->setText(outDir);
      byName<QLineEdit>(w, "nameTemplateEdit")->setText(QStringLiteral("{name}_small"));
      spinEvents(30);
      w->close();  // closeEvent -> saveSessionState
      spinEvents(30);
      CHECK_MSG(QFileInfo::exists(cfgPath), "close wrote the settings file");
      delete w;
    }

    // File content: core SettingsIO keys + GUI-only keys coexist.
    {
      QFile f(cfgPath);
      CHECK(f.open(QIODevice::ReadOnly | QIODevice::Text));
      const QString body = QString::fromUtf8(f.readAll());
      CHECK(body.contains(QStringLiteral("mode = merge")));
      CHECK(body.contains(QStringLiteral("optimize = 1")));
      CHECK(body.contains(QStringLiteral("lossy = 42")));
      CHECK(body.contains(QStringLiteral("dither = ro64")));
      CHECK(body.contains(QStringLiteral("gamma = 2.35")));
      CHECK(body.contains(QStringLiteral("comment = persist me")));
      CHECK(body.contains(QStringLiteral("batch_dir = ") + cfg.path() + QStringLiteral("/outdir")));
      CHECK(body.contains(QStringLiteral("name_template = {name}_small")));
      // Queue/Save-as are deliberately NOT persisted.
      CHECK(!body.contains(QStringLiteral("input =")));
      CHECK(!body.contains(QStringLiteral("output =")));
    }

    // Session B: a brand-new window restores everything.
    {
      MainWindow* w = makeWindow();
      auto x = findWidgets(w);
      CHECK(x.mode->currentData().toInt() == static_cast<int>(gs::Mode::Merge));
      CHECK(byName<QSpinBox>(w, "optimizeSpin")->value() == 1);
      CHECK(byName<QSpinBox>(w, "lossySpin")->value() == 42);
      CHECK(byName<QCheckBox>(w, "colorsCheck")->isChecked());
      CHECK(byName<QSpinBox>(w, "colorsSpin")->value() == 64);
      CHECK(byName<QSpinBox>(w, "colorsSpin")->isEnabled());  // dependent state synced
      CHECK(byName<QComboBox>(w, "ditherCombo")->currentData().toString() ==
            QStringLiteral("ro64"));
      CHECK(byName<QComboBox>(w, "resizeKindCombo")->currentIndex() == 1);
      CHECK(byName<QSpinBox>(w, "resizeWSpin")->value() == 222);
      CHECK(byName<QCheckBox>(w, "delayCheck")->isChecked());
      CHECK(byName<QSpinBox>(w, "delaySpin")->value() == 7);
      CHECK(byName<QSpinBox>(w, "delaySpin")->isEnabled());
      CHECK(byName<QComboBox>(w, "loopCombo")->currentData().toInt() == 2);
      CHECK(byName<QSpinBox>(w, "loopSpin")->value() == 5);
      CHECK(byName<QSpinBox>(w, "loopSpin")->isEnabled());
      CHECK(byName<QComboBox>(w, "gammaCombo")->currentData().toInt() == 3);
      CHECK(byName<QLineEdit>(w, "gammaEdit")->text() == QStringLiteral("2.35"));
      CHECK(byName<QLineEdit>(w, "gammaEdit")->isEnabled());
      CHECK(byName<QLineEdit>(w, "backgroundEdit")->text() == QStringLiteral("#abcdef"));
      CHECK(byName<QCheckBox>(w, "carefulCheck")->isChecked());
      CHECK(byName<QCheckBox>(w, "flipHCheck")->isChecked());
      auto* comments = byName<QListWidget>(w, "commentList");
      CHECK(comments && comments->count() == 1);
      CHECK(comments && comments->item(0)->text() == QStringLiteral("persist me"));
      CHECK(byName<QLineEdit>(w, "batchDirEdit")->text() ==
            cfg.path() + QStringLiteral("/outdir"));
      CHECK(byName<QLineEdit>(w, "nameTemplateEdit")->text() ==
            QStringLiteral("{name}_small"));
      // NOT restored (documented decision): queue and Save-as.
      CHECK(x.list->count() == 0);
      CHECK(x.output->text().isEmpty());
      // The live pane reflects the restored state (constructor refresh).
      const QString pane = x.pane->toPlainText();
      CHECK(pane.contains(QStringLiteral("--lossy=42")));
      CHECK(pane.contains(QStringLiteral("--gamma=2.35")));
      CHECK(pane.contains(QStringLiteral("-d 7")));
      CHECK(pane.contains(QStringLiteral("--loopcount=5")));
      CHECK(pane.contains(QStringLiteral("--dither=ro64")));
      CHECK(pane.contains(QStringLiteral("--resize-fit 222x")));
      CHECK(pane.contains(QStringLiteral("--background '#abcdef'")));
      CHECK(pane.contains(QStringLiteral("--comment 'persist me'")));
      delete w;
    }

    // Corrupt file: valid keys apply, invalid ones warn — never a crash,
    // never a silent pretend-first-launch.
    {
      QFile f(cfgPath);
      CHECK(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
      f.write("mode = wat\noptimize = xyz\nlossy = 42\n");
      f.close();
      MainWindow* w = makeWindow();
      auto x = findWidgets(w);
      CHECK(x.status->text().contains(QStringLiteral("warning")));  // surfaced (S5 rule)
      CHECK(x.mode->currentData().toInt() == static_cast<int>(gs::Mode::Auto));  // parse fallback
      CHECK(byName<QSpinBox>(w, "optimizeSpin")->value() == 3);  // rejected -> default kept
      CHECK(byName<QSpinBox>(w, "lossySpin")->value() == 42);    // valid key still applied
      delete w;
    }

    qputenv("GS_SETTINGS_PATH", prevEnv);  // restore the scratch path
  }

  // ================= T15: queue reorder (S3-9) ===========================
  {
    g_stage = "T15"; std::printf("== T15 queue reorder ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    const QString b = tmp.path() + QStringLiteral("/b.gif");
    const QString c = tmp.path() + QStringLiteral("/c.gif");
    CHECK(copyFile(logo, a));
    CHECK(copyFile(logo1, b));
    CHECK(copyFile(logo, c));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    auto* up = byName<QPushButton>(w, "moveUpButton");
    auto* down = byName<QPushButton>(w, "moveDownButton");
    CHECK(up && down);
    dropFiles(w, {a, b, c});
    CHECK(x.list->count() == 3);

    // Move the LAST row up: a,b,c -> a,c,b; selection follows the item.
    x.list->setCurrentRow(2);
    up->click();
    spinEvents(20);
    CHECK(rowPath(x.list, 0) == a);
    CHECK(rowPath(x.list, 1) == c);
    CHECK(rowPath(x.list, 2) == b);
    CHECK(x.list->currentRow() == 1);

    // Bounds are honest no-ops.
    x.list->setCurrentRow(0);
    up->click();
    spinEvents(10);
    CHECK(rowPath(x.list, 0) == a);
    x.list->setCurrentRow(2);
    down->click();
    spinEvents(10);
    CHECK(rowPath(x.list, 2) == b);
    CHECK(x.list->count() == 3);

    // Move the FIRST row down: a,c,b -> c,a,b.
    x.list->setCurrentRow(0);
    down->click();
    spinEvents(10);
    CHECK(rowPath(x.list, 0) == c);
    CHECK(rowPath(x.list, 1) == a);
    CHECK(rowPath(x.list, 2) == b);

    // Merge consumes the queue in order — the live pane must show c,a,b.
    x.mode->setCurrentIndex(1);  // Merge
    const QString merged = tmp.path() + QStringLiteral("/m.gif");
    x.output->setText(merged);
    spinEvents(20);
    {
      const QString pane = x.pane->toPlainText();
      const int posC = pane.indexOf(c);
      const int posA = pane.indexOf(a);
      const int posB = pane.indexOf(b);
      CHECK(posC >= 0 && posA > posC && posB > posA);  // pane order = queue order
    }
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "merge after reorder completes");
    CHECK(QFileInfo::exists(merged));
    delete w;
  }

  // ================= T16: naming templates (S3-25) =======================
  {
    g_stage = "T16"; std::printf("== T16 naming templates ==\n");
    QTemporaryDir tmp;
    const QString a = tmp.path() + QStringLiteral("/a.gif");
    const QString b = tmp.path() + QStringLiteral("/b.gif");
    CHECK(copyFile(logo, a));
    CHECK(copyFile(logo1, b));

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    auto* tmpl = byName<QLineEdit>(w, "nameTemplateEdit");
    CHECK(tmpl != nullptr);
    dropFiles(w, {a});
    // Default template renders the historical <name>_opt.gif (audit E4).
    CHECK(x.pane->toPlainText().contains(QStringLiteral("a_opt.gif")));

    // Custom template (no extension) -> ".gif" appended, pane + summary follow.
    tmpl->setText(QStringLiteral("{name}_small"));
    spinEvents(20);
    CHECK(x.pane->toPlainText().contains(QStringLiteral("a_small.gif")));
    CHECK(x.outputSummary->text().contains(QStringLiteral("a_small.gif")));
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "templated single-file batch completes");
    CHECK(QFileInfo::exists(tmp.path() + QStringLiteral("/a_small.gif")));

    // A template cannot escape the output folder: separators are stripped.
    tmpl->setText(QStringLiteral("../../evil"));
    spinEvents(20);
    {
      const QString pane = x.pane->toPlainText();
      CHECK(pane.contains(QStringLiteral("evil.gif")));
      CHECK(!pane.contains(QStringLiteral("../../")));
    }

    // Constant template + 2 files = collision -> summary warns and the run
    // is REFUSED with an explanatory dialog (no silent overwrite).
    dropFiles(w, {b});
    tmpl->setText(QStringLiteral("same_name.gif"));
    spinEvents(20);
    CHECK(x.outputSummary->text().contains(QStringLiteral("refused")));
    g_dialogs.clear();
    x.run->click();
    spinEvents(120);
    CHECK_MSG(dialogsContain(QStringLiteral("{name}")),
              "collision refusal dialog explains the {name} requirement");
    CHECK(x.process->state() == QProcess::NotRunning);  // never started
    CHECK(!QFileInfo::exists(tmp.path() + QStringLiteral("/same_name.gif")));

    // With {name} back, both files run to their own outputs.
    tmpl->setText(QStringLiteral("{name}_x.gif"));
    spinEvents(20);
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "templated 2-file batch completes");
    CHECK(QFileInfo::exists(tmp.path() + QStringLiteral("/a_x.gif")));
    CHECK(QFileInfo::exists(tmp.path() + QStringLiteral("/b_x.gif")));
    delete w;
  }

  // ============ T17: batch output planning (audit U-01) ==================
  // The old code derived each batch output inside the run loop and never
  // compared targets, so (a) a template like "{name}.gif" with an empty batch
  // folder wrote the output ON TOP OF THE SOURCE, and (b) two queued files
  // sharing a base name both landed on <batchdir>/<stem>_opt.gif and the
  // second silently deleted the first. Both are now planned and refused.
  {
    g_stage = "T17"; std::printf("== T17 batch output planning ==\n");
    QTemporaryDir tmp;
    CHECK(QDir(tmp.path()).mkpath(QStringLiteral("d1")));
    CHECK(QDir(tmp.path()).mkpath(QStringLiteral("d2")));
    CHECK(QDir(tmp.path()).mkpath(QStringLiteral("out")));
    const QString h1 = tmp.path() + QStringLiteral("/d1/hero.gif");
    const QString h2 = tmp.path() + QStringLiteral("/d2/hero.gif");
    CHECK(copyFile(logo, h1));
    CHECK(copyFile(logo1, h2));
    const qint64 h1_before = QFileInfo(h1).size();
    const qint64 h2_before = QFileInfo(h2).size();

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    auto* tmpl = byName<QLineEdit>(w, "nameTemplateEdit");
    CHECK(tmpl != nullptr);
    dropFiles(w, {h1, h2});
    CHECK(x.list->count() == 2);

    // (a) Same base name + one batch folder + the DEFAULT template -> both
    //     render out/hero_opt.gif. The label must warn and the run refuse.
    x.batchDir->setText(tmp.path() + QStringLiteral("/out"));
    spinEvents(20);
    CHECK_MSG(x.outputSummary->text().contains(QStringLiteral("refused")),
              "summary warns about the duplicate target before Run");
    g_dialogs.clear();
    x.run->click();
    spinEvents(120);
    CHECK_MSG(!g_dialogs.empty(), "duplicate-target run is refused with a dialog");
    CHECK(x.process->state() == QProcess::NotRunning);   // never started
    CHECK(!QFileInfo::exists(tmp.path() + QStringLiteral("/out/hero_opt.gif")));
    CHECK(QFileInfo(h1).size() == h1_before);            // sources untouched
    CHECK(QFileInfo(h2).size() == h2_before);

    // (b) The self-overwrite case: template "{name}.gif" with NO batch folder
    //     resolves each output to the input itself.
    x.batchDir->clear();
    tmpl->setText(QStringLiteral("{name}.gif"));
    spinEvents(20);
    CHECK_MSG(x.outputSummary->text().contains(QStringLiteral("refused")),
              "summary warns that the target is a source file");
    g_dialogs.clear();
    x.run->click();
    spinEvents(120);
    CHECK_MSG(dialogsContain(QStringLiteral("source file")),
              "self-overwrite refusal dialog names the source");
    CHECK(x.process->state() == QProcess::NotRunning);
    CHECK(QFileInfo(h1).size() == h1_before);            // source NOT replaced
    CHECK(QFileInfo(h2).size() == h2_before);

    // (c) Positive control. Both queued files are called hero.gif, so the only
    //     way to get distinct targets from one template is to let each write
    //     next to its own input (no batch folder): d1/hero_x.gif and
    //     d2/hero_x.gif. That must run, and must not touch the sources.
    x.batchDir->clear();
    tmpl->setText(QStringLiteral("{name}_x.gif"));
    spinEvents(20);
    CHECK_MSG(!x.outputSummary->text().contains(QStringLiteral("refused")),
              "distinct targets are not refused");
    g_dialogs.clear();
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("complete")), "planned 2-file batch completes");
    CHECK(QFileInfo::exists(tmp.path() + QStringLiteral("/d1/hero_x.gif")));
    CHECK(QFileInfo::exists(tmp.path() + QStringLiteral("/d2/hero_x.gif")));
    CHECK(QFileInfo(h1).size() == h1_before);
    CHECK(QFileInfo(h2).size() == h2_before);

    // (d) And the same stems ARE refused as soon as a shared batch folder
    //     makes the two targets coincide again.
    x.batchDir->setText(tmp.path() + QStringLiteral("/out"));
    spinEvents(20);
    CHECK_MSG(x.outputSummary->text().contains(QStringLiteral("refused")),
              "a shared batch folder turns distinct inputs into one target");
    delete w;
  }

  std::printf("==> %d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) {
    std::printf("ALL GUI TESTS PASSED\n");
    return 0;
  }
  std::printf("GUI TESTS FAILED\n");
  return 1;
}
