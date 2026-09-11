#include "qtui/PreviewPanel.h"

#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QMovie>
#include <QVBoxLayout>

namespace {
const QSize kPreviewSize(240, 180);
}

PreviewPanel::PreviewPanel(QWidget* parent) : QWidget(parent) {
  auto* box = new QGroupBox(QStringLiteral("Preview"), this);
  box->setObjectName(QStringLiteral("previewGroup"));
  auto* lay = new QVBoxLayout(box);

  auto* beforeCap = new QLabel(QStringLiteral("Before (original, selected file)"), box);
  beforeCap->setObjectName(QStringLiteral("previewBeforeCaption"));
  beforeLabel_ = new QLabel(box);
  beforeLabel_->setObjectName(QStringLiteral("previewBefore"));
  beforeLabel_->setMinimumSize(kPreviewSize);
  beforeLabel_->setAlignment(Qt::AlignCenter);
  beforeLabel_->setStyleSheet(QStringLiteral("background:#202020;color:#909090;"));
  beforeLabel_->setText(QStringLiteral("no file selected"));

  auto* afterCap = new QLabel(QStringLiteral("After (preview of current actions)"), box);
  afterCap->setObjectName(QStringLiteral("previewAfterCaption"));
  afterLabel_ = new QLabel(box);
  afterLabel_->setObjectName(QStringLiteral("previewAfter"));
  afterLabel_->setMinimumSize(kPreviewSize);
  afterLabel_->setAlignment(Qt::AlignCenter);
  afterLabel_->setStyleSheet(QStringLiteral("background:#202020;color:#909090;"));
  afterLabel_->setText(QStringLiteral("—"));

  savingsLabel_ = new QLabel(box);
  savingsLabel_->setObjectName(QStringLiteral("previewSavings"));
  savingsLabel_->setAlignment(Qt::AlignCenter);

  captionLabel_ = new QLabel(box);
  captionLabel_->setObjectName(QStringLiteral("previewCaption"));
  captionLabel_->setWordWrap(true);

  lay->addWidget(beforeCap);
  lay->addWidget(beforeLabel_);
  lay->addWidget(afterCap);
  lay->addWidget(afterLabel_);
  lay->addWidget(savingsLabel_);
  lay->addWidget(captionLabel_);
  lay->addStretch();

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->addWidget(box);
  updateCaption();
}

void PreviewPanel::stopMovie(QMovie** movie) {
  if (*movie) {
    (*movie)->stop();
    // Deleted SYNCHRONOUSLY, not via deleteLater (S10): on Windows an open
    // QMovie keeps a delete-lock on its GIF file until the object is really
    // gone, so a deleteLater left the superseded preview file undeletable in
    // the same event-loop tick (CI-windows T20: sweep and teardown both hit
    // the locked file). Every call site here is outside the movie's own
    // signals, so synchronous deletion is safe.
    delete *movie;
    *movie = nullptr;
  }
}

void PreviewPanel::releaseMovies() {
  // Drop both movie handles NOW (S10): MainWindow's destructor sweeps the
  // preview temp dir, and on Windows that sweep fails while any QMovie still
  // holds a file open.
  stopMovie(&afterMovie_);
  afterLabel_->setMovie(nullptr);
  stopMovie(&beforeMovie_);
  beforeLabel_->setMovie(nullptr);
}

void PreviewPanel::setBefore(const QString& gifPath) {
  stopMovie(&beforeMovie_);
  if (gifPath.isEmpty() || !QFileInfo::exists(gifPath)) {
    beforeLabel_->setText(QStringLiteral("no file selected"));
    return;
  }
  beforeMovie_ = new QMovie(gifPath, QByteArray(), this);
  if (!beforeMovie_->isValid()) {
    stopMovie(&beforeMovie_);
    beforeLabel_->setText(QStringLiteral("cannot play file"));
    return;
  }
  beforeMovie_->setScaledSize(kPreviewSize);
  beforeLabel_->setMovie(beforeMovie_);
  beforeMovie_->start();
}

void PreviewPanel::setAfter(const QString& gifPath, qint64 beforeBytes, qint64 afterBytes) {
  stopMovie(&afterMovie_);
  if (gifPath.isEmpty() || !QFileInfo::exists(gifPath)) {
    clearAfter(QStringLiteral("preview produced no file"));
    return;
  }
  afterMovie_ = new QMovie(gifPath, QByteArray(), this);
  if (!afterMovie_->isValid()) {
    stopMovie(&afterMovie_);
    clearAfter(QStringLiteral("preview file is not a playable GIF"));
    return;
  }
  afterMovie_->setScaledSize(kPreviewSize);
  afterLabel_->setMovie(afterMovie_);
  afterMovie_->start();

  if (beforeBytes > 0 && afterBytes > 0) {
    const double pct = 100.0 * static_cast<double>(afterBytes - beforeBytes) /
                               static_cast<double>(beforeBytes);
    savingsLabel_->setText(QStringLiteral("%1 → %2  (%3%)")
                               .arg(humanSize(beforeBytes), humanSize(afterBytes))
                               .arg(pct >= 0 ? QStringLiteral("+") + QString::number(pct, 'f', 1)
                                             : QString::number(pct, 'f', 1)));
  } else {
    savingsLabel_->clear();
  }
  generating_ = false;
  status_.clear();
  updateCaption();
}

void PreviewPanel::clearAfter(const QString& reason) {
  stopMovie(&afterMovie_);
  afterLabel_->setMovie(nullptr);
  afterLabel_->setText(QStringLiteral("—"));
  savingsLabel_->clear();
  generating_ = false;
  status_ = reason;
  updateCaption();
}

void PreviewPanel::setGenerating(bool generating) {
  generating_ = generating;
  if (generating) status_.clear();
  updateCaption();
}

void PreviewPanel::setStatus(const QString& status) {
  status_ = status;
  updateCaption();
}

void PreviewPanel::updateCaption() {
  // Base caption stays honest (one-way preview of a single file).
  QString text = QStringLiteral(
      "Preview = the SELECTED file re-encoded with current actions "
      "(single-file run; Batch applies the same settings to every file).");
  if (generating_) {
    text += QStringLiteral("  Generating…");
  } else if (!status_.isEmpty()) {
    text += QStringLiteral("  ") + status_;
  }
  captionLabel_->setText(text);
}

QString PreviewPanel::humanSize(qint64 bytes) {
  if (bytes >= 1024 * 1024)
    return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0), 0, 'f', 2);
  if (bytes >= 1024)
    return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / 1024.0, 0, 'f', 1);
  return QStringLiteral("%1 B").arg(bytes);
}
