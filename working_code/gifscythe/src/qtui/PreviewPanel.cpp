#include "qtui/PreviewPanel.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <utility>

namespace {
const QSize kPreviewSize(240, 180);
const char* kPreviewCaption =
    "Preview = the selected file re-encoded with current actions "
    "(single-file run; Batch applies the same settings to every file).";
}

PreviewPanel::PreviewPanel(QWidget* parent) : QWidget(parent) {
  auto* box = new QGroupBox(QStringLiteral("Preview"), this);
  box->setObjectName(QStringLiteral("previewGroup"));
  auto* lay = new QVBoxLayout(box);

  // Captions are siblings of the canvas, never children of it. This is
  // intentional: a QLabel showing a QMovie must not also carry status text,
  // otherwise a late setText() can paint over the last animation frame.
  auto makeCanvas = [box](const QString& caption, const QString& captionName,
                          const QString& labelName, QLabel** labelOut) {
    auto* cap = new QLabel(caption, box);
    cap->setObjectName(captionName);
    cap->setWordWrap(true);

    auto* frame = new QFrame(box);
    frame->setObjectName(labelName + QStringLiteral("Canvas"));
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setMinimumSize(kPreviewSize);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    frame->setStyleSheet(QStringLiteral("QFrame { background: #202020; }"));
    auto* frameLay = new QVBoxLayout(frame);
    frameLay->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel(frame);
    label->setObjectName(labelName);
    label->setMinimumSize(kPreviewSize);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::PlainText);
    label->setAttribute(Qt::WA_OpaquePaintEvent, true);
    label->setStyleSheet(QStringLiteral("background:#202020;color:#909090;"));
    frameLay->addWidget(label);
    *labelOut = label;
    return std::pair<QLabel*, QFrame*>(cap, frame);
  };

  const auto before = makeCanvas(QStringLiteral("Before (original, selected file)"),
                                 QStringLiteral("previewBeforeCaption"),
                                 QStringLiteral("previewBefore"), &beforeLabel_);
  const auto after = makeCanvas(QStringLiteral("After (what will be written)"),
                                QStringLiteral("previewAfterCaption"),
                                QStringLiteral("previewAfter"), &afterLabel_);

  beforeLabel_->setText(QStringLiteral("no file selected"));
  afterLabel_->setText(QStringLiteral("—"));
  savingsLabel_ = new QLabel(box);
  savingsLabel_->setObjectName(QStringLiteral("previewSavings"));
  savingsLabel_->setAlignment(Qt::AlignCenter);

  auto* playbackRow = new QHBoxLayout();
  previewButton_ = new QPushButton(QStringLiteral("Preview changes"), box);
  previewButton_->setObjectName(QStringLiteral("previewGenerateButton"));
  previewButton_->setToolTip(QStringLiteral("Encode a fresh temporary preview with the current Actions"));
  playButton_ = new QPushButton(QStringLiteral("Play"), box);
  playButton_->setObjectName(QStringLiteral("previewPlayButton"));
  playButton_->setToolTip(QStringLiteral("Play both GIF previews"));
  stopButton_ = new QPushButton(QStringLiteral("Stop"), box);
  stopButton_->setObjectName(QStringLiteral("previewStopButton"));
  stopButton_->setToolTip(QStringLiteral("Stop both previews at their first frame"));
  autoPlayCheck_ = new QCheckBox(QStringLiteral("Auto-play previews"), box);
  autoPlayCheck_->setObjectName(QStringLiteral("previewAutoPlay"));
  autoPlayCheck_->setToolTip(QStringLiteral(
      "When off, generating a preview does not start playback. Use Play when you want to watch it."));
  autoPlayCheck_->setChecked(true);
  playbackRow->addWidget(previewButton_);
  playbackRow->addWidget(playButton_);
  playbackRow->addWidget(stopButton_);
  playbackRow->addWidget(autoPlayCheck_);
  playbackRow->addStretch();

  captionLabel_ = new QLabel(box);
  captionLabel_->setObjectName(QStringLiteral("previewCaption"));
  captionLabel_->setWordWrap(true);
  captionLabel_->setTextFormat(Qt::PlainText);
  captionLabel_->setMinimumHeight(42);  // status text gets its own space, not the GIF canvas

  lay->addWidget(before.first);
  lay->addWidget(before.second);
  lay->addWidget(after.first);
  lay->addWidget(after.second);
  lay->addWidget(savingsLabel_);
  lay->addLayout(playbackRow);
  lay->addWidget(captionLabel_);
  lay->addStretch();

  connect(previewButton_, &QPushButton::clicked, this, [this]() { emit previewRequested(); });
  connect(playButton_, &QPushButton::clicked, this, &PreviewPanel::playMovies);
  connect(stopButton_, &QPushButton::clicked, this, &PreviewPanel::stopMovies);
  connect(autoPlayCheck_, &QCheckBox::toggled, this, &PreviewPanel::setAutoPlay);

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->addWidget(box);
  captionBase_ = QString::fromLatin1(kPreviewCaption);
  updatePlaybackControls();
  updateCaption();
}

void PreviewPanel::stopMovie(QMovie** movie) {
  if (*movie) {
    (*movie)->stop();
    // Deleted SYNCHRONOUSLY, not via deleteLater (S10): on Windows an open
    // QMovie keeps a delete-lock on its GIF file until the object is really
    // gone, so a temp-dir sweep can remove it in the same event-loop tick.
    delete *movie;
    *movie = nullptr;
  }
}

void PreviewPanel::releaseMovies() {
  stopMovie(&afterMovie_);
  afterLabel_->setMovie(nullptr);
  afterLabel_->setText(QStringLiteral("—"));
  stopMovie(&beforeMovie_);
  beforeLabel_->setMovie(nullptr);
  beforeLabel_->setText(QStringLiteral("no file selected"));
  updatePlaybackControls();
}

void PreviewPanel::setBefore(const QString& gifPath) {
  stopMovie(&beforeMovie_);
  beforeLabel_->setMovie(nullptr);
  beforeLabel_->setText(QStringLiteral("no file selected"));
  if (gifPath.isEmpty() || !QFileInfo::exists(gifPath)) {
    updatePlaybackControls();
    return;
  }
  beforeMovie_ = new QMovie(gifPath, QByteArray(), this);
  if (!beforeMovie_->isValid()) {
    stopMovie(&beforeMovie_);
    beforeLabel_->setText(QStringLiteral("cannot play file"));
    updatePlaybackControls();
    return;
  }
  beforeMovie_->setScaledSize(kPreviewSize);
  // Clear the label's text before attaching the movie. The text is a fallback
  // only; it is never allowed to coexist with a live animation label.
  beforeLabel_->setText(QString());
  beforeLabel_->setMovie(beforeMovie_);
  if (autoPlay()) beforeMovie_->start();
  updatePlaybackControls();
}

void PreviewPanel::setAfter(const QString& gifPath, qint64 beforeBytes,
                            qint64 afterBytes, const QString& context) {
  stopMovie(&afterMovie_);
  afterLabel_->setMovie(nullptr);
  afterLabel_->setText(QStringLiteral("—"));
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
  afterLabel_->setText(QString());
  afterLabel_->setMovie(afterMovie_);
  if (autoPlay()) afterMovie_->start();

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
  if (!context.isEmpty()) captionBase_ = context;
  else captionBase_ = QString::fromLatin1(kPreviewCaption);
  generating_ = false;
  status_.clear();
  updatePlaybackControls();
  updateCaption();
}

void PreviewPanel::clearAfter(const QString& reason) {
  stopMovie(&afterMovie_);
  afterLabel_->setMovie(nullptr);
  afterLabel_->setText(QStringLiteral("—"));
  savingsLabel_->clear();
  generating_ = false;
  status_ = reason;
  captionBase_ = QString::fromLatin1(kPreviewCaption);
  updatePlaybackControls();
  updateCaption();
}

void PreviewPanel::setGenerating(bool generating) {
  generating_ = generating;
  if (generating) {
    status_.clear();
    captionBase_ = QString::fromLatin1(kPreviewCaption);
  }
  updateCaption();
}

void PreviewPanel::setStatus(const QString& status) {
  status_ = status;
  updateCaption();
}

void PreviewPanel::playMovies() {
  if (beforeMovie_) beforeMovie_->start();
  if (afterMovie_) afterMovie_->start();
  updatePlaybackControls();
}

void PreviewPanel::stopMovies() {
  if (beforeMovie_) beforeMovie_->stop();
  if (afterMovie_) afterMovie_->stop();
  updatePlaybackControls();
}

void PreviewPanel::setAutoPlay(bool enabled) {
  if (autoPlayCheck_ && autoPlayCheck_->isChecked() != enabled) {
    autoPlayCheck_->setChecked(enabled);
  }
  if (enabled) playMovies();
  else stopMovies();
}

bool PreviewPanel::autoPlay() const {
  return autoPlayCheck_ && autoPlayCheck_->isChecked();
}

void PreviewPanel::updatePlaybackControls() {
  const bool anyMovie = beforeMovie_ || afterMovie_;
  if (playButton_) playButton_->setEnabled(anyMovie);
  if (stopButton_) stopButton_->setEnabled(anyMovie);
}

void PreviewPanel::updateCaption() {
  QString text = captionBase_.isEmpty() ? QString::fromLatin1(kPreviewCaption) : captionBase_;
  if (generating_) {
    text += QStringLiteral("  Generating preview…");
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
