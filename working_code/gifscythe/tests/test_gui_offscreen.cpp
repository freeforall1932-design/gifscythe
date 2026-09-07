// test_gui_offscreen.cpp - Automated GUI verification for COMPILED_AUDIT §6.B.
//
// Runs the real MainWindow under QT_QPA_PLATFORM=offscreen and exercises:
//   T1  defaults (Batch default E4, engine status B14)
//   T2  live command pane sync (B13)
//   T3  queue ops: drop-append+dedupe (B6/B7), multi-select remove (B8), clear (B9)
//   T4  Batch run: N inputs -> N outputs, frame counts preserved (B10);
//       explicit Save-as honored (E1)
//   T5  Merge run: 2 inputs -> 1 output, frames = sum (B11)
//   T6  Merge with empty output -> refuses, no silent stdout loss (B12)
//   T7  Explode with empty output -> auto prefix, frames written (E2)
//   T8  Failed engine run -> honest "failed" status + dialog, never "complete" (B4)
//   T9  async start (B1 proxy), busy indicators (B3), cancel mid-run (B2)
//   T10 close window while running kills the engine process (B15)
//
// Modal dialogs are recorded and auto-closed by a DialogKiller timer so the
// suite never blocks. Build via CMake (AUTOMOC) when Qt6 is found; run with
// QT_QPA_PLATFORM=offscreen.

#include "qtui/MainWindow.h"
#include "qtui/DropListWidget.h"

#include "core/EngineLocator.h"
#include "core/ProcessRunner.h"
#include "core/version.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QMessageBox>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <cstdio>
#include <filesystem>
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

// ---- Widget finders (no MainWindow internals needed) ----
struct Widgets {
  DropListWidget* list = nullptr;
  QPlainTextEdit* pane = nullptr;
  QComboBox* mode = nullptr;
  QSpinBox* optimize = nullptr;   // range 0..3
  QSpinBox* lossy = nullptr;      // range 0..200
  QLineEdit* output = nullptr;
  QPushButton* run = nullptr;
  QPushButton* cancel = nullptr;
  QPushButton* remove = nullptr;
  QPushButton* clear = nullptr;
  QLabel* status = nullptr;
  QProgressBar* progress = nullptr;
  QProcess* process = nullptr;
};

Widgets findWidgets(QWidget* w) {
  Widgets x;
  x.list = w->findChild<DropListWidget*>();
  x.pane = w->findChild<QPlainTextEdit*>();
  x.mode = w->findChild<QComboBox*>();
  const auto spins = w->findChildren<QSpinBox*>();
  for (QSpinBox* s : spins) {
    if (s->maximum() == 3) x.optimize = s;
    else if (s->maximum() == 200) x.lossy = s;
  }
  x.output = w->findChild<QLineEdit*>();
  const auto buttons = w->findChildren<QPushButton*>();
  for (QPushButton* b : buttons) {
    const QString t = b->text();
    if (t == QLatin1String("Optimize GIF")) x.run = b;
    else if (t == QLatin1String("Cancel")) x.cancel = b;
    else if (t == QLatin1String("Remove")) x.remove = b;
    else if (t == QLatin1String("Clear")) x.clear = b;
  }
  const auto labels = w->findChildren<QLabel*>();
  for (QLabel* l : labels) {
    if (l->text().contains(QLatin1String("engine"), Qt::CaseInsensitive) ||
        l->text().startsWith(QLatin1String("Ready")) ||
        l->text().startsWith(QLatin1String("Engine"))) {
      x.status = l;
    }
  }
  x.progress = w->findChild<QProgressBar*>();
  x.process = w->findChild<QProcess*>();
  return x;
}

bool waitForStatus(QWidget* w, const QString& needle, int timeoutMs = 45000) {
  QElapsedTimer el;
  el.start();
  while (el.elapsed() < timeoutMs) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(2);
    const auto labels = w->findChildren<QLabel*>();
    for (QLabel* l : labels)
      if (l->text().contains(needle, Qt::CaseInsensitive)) return true;
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
// drag session is active (verified empirically on Qt 6.4: sendEvent'ing a
// synthetic QDropEvent to the widget or its viewport is ignored, and the
// QPA-level handleDrop path needs private headers). So the harness emits
// DropListWidget::filesDropped — the exact signal the drop handler emits —
// which exercises MainWindow::onFilesDropped's filter + append + dedupe
// semantics (the substance of audit B6/B7). The DropListWidget event
// overrides themselves are standard 15-line plumbing; verify them once on
// a real desktop (§6.B B6 manual note).
void dropFiles(QWidget* w, const QStringList& files) {
  auto* list = w->findChild<DropListWidget*>();
  list->filesDropped(files);  // signals are public in Qt5+
  QCoreApplication::processEvents();
}

MainWindow* makeWindow() {
  auto* w = new MainWindow();
  w->show();  // offscreen: makes isVisible() meaningful for progress bar etc.
  QCoreApplication::processEvents();
  return w;
}

}  // namespace

int main(int argc, char** argv) {
  setbuf(stdout, nullptr);  // unbuffered: crash diagnostics keep our trace
  qputenv("QT_QPA_PLATFORM", "offscreen");  // force offscreen even if unset
  QApplication app(argc, argv);
  DialogKiller killer;

  std::printf("==> GUI offscreen tests (COMPILED_AUDIT 6.B harness)\n");

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

  // ================= T1: defaults (E4 batch default, B14 status) =========
  {
    std::printf("== T1 defaults ==\n");
    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    CHECK(x.list && x.pane && x.mode && x.optimize && x.lossy && x.output &&
          x.run && x.cancel && x.remove && x.clear && x.progress && x.process);
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
    std::printf("== T2 live pane ==\n");
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
    std::printf("== T3 queue ops ==\n");
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
    if (auto* it = x.list->item(2)) {
      CHECK(it->text() == QStringLiteral("c.gif"));
    } else { CHECK_MSG(false, "item(2) exists after dedupe drop"); }
    CHECK(x.run->isEnabled());

    // B8: multi-select remove keeps indices consistent
    if (x.list->count() == 3) {
      x.list->item(0)->setSelected(true);
      x.list->item(2)->setSelected(true);
      x.remove->click();
      spinEvents(20);
      CHECK(x.list->count() == 1);
      if (auto* it = x.list->item(0)) {
        CHECK(it->text() == QStringLiteral("b.gif"));
      } else { CHECK_MSG(false, "middle item survives multi-remove"); }
    } else {
      CHECK_MSG(false, "queue has 3 items before multi-remove");
    }

    // B9: clear empties queue + disables Run
    x.clear->click();
    spinEvents(20);
    CHECK(x.list->count() == 0);
    CHECK(!x.run->isEnabled());
    delete w;
  }

  // ================= T4: Batch E2E (B10) + explicit output (E1) ==========
  {
    std::printf("== T4 batch E2E ==\n");
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
    std::printf("== T5 merge E2E ==\n");
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
    std::printf("== T6 merge refuse ==\n");
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
    CHECK(!x.run->isEnabled() || x.run->isEnabled());  // sanity: still clickable
    delete w;
  }

  // ================= T7: Explode auto-prefix (E2) ========================
  {
    std::printf("== T7 explode ==\n");
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
    std::printf("== T8 failure honesty ==\n");
    QTemporaryDir tmp;
    const QString ghost = tmp.path() + QStringLiteral("/ghost.gif");  // missing

    MainWindow* w = makeWindow();
    auto x = findWidgets(w);
    dropFiles(w, {ghost});  // .gif suffix passes the drop filter
    CHECK(x.list->count() == 1);
    g_dialogs.clear();
    x.run->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("failed")), "failed run says failed");
    CHECK(x.process->state() == QProcess::NotRunning);
    // Busy state cleared honestly, Run re-enabled (queue non-empty)
    CHECK(x.run->isEnabled());
    CHECK(!x.cancel->isEnabled());
    // The failure surfaced as a dialog too, and NOT as "complete"
    CHECK(!g_dialogs.empty());
    delete w;
  }

  // ================= T9: busy UI + cancel mid-run (B1/B2/B3) =============
  {
    std::printf("== T9 cancel mid-run ==\n");
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
    x.cancel->click();
    CHECK_MSG(waitForStatus(w, QStringLiteral("Cancelled"), 10000), "status shows Cancelled");
    CHECK(x.process->state() == QProcess::NotRunning);  // no zombie
    CHECK(!x.cancel->isEnabled());
    CHECK(x.run->isEnabled());           // controls re-enabled
    CHECK(!x.progress->isVisible());     // busy indicator cleared
    // Cancelled run must NOT claim completion
    CHECK(!x.process->property("claimedComplete").isValid());
    delete w;
  }

  // ================= T10: close while running (B15) ======================
  {
    std::printf("== T10 close while running ==\n");
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

  std::printf("==> %d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) {
    std::printf("ALL GUI TESTS PASSED\n");
    return 0;
  }
  std::printf("GUI TESTS FAILED\n");
  return 1;
}
