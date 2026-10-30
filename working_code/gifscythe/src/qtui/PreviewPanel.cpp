#include "qtui/PreviewPanel.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

#include <algorithm>

namespace {
const QSize kMinimumPreviewSize(240, 180);
const char* kPreviewCaption =
    "Before is the selected source. Click Apply setting preview to encode "
    "one temporary After using the current Actions.";

// QMovie::setScaledSize only affects *future* frames. If a file was selected
// while Actions was hidden, its already-decoded first frame could remain tiny
// (or clipped) after the viewport was laid out, especially with autoplay off.
// Paint the current full canvas into the available rectangle instead. Resizing
// is immediate and never re-reads or re-encodes the file.
class AspectMovieLabel : public QLabel {
 public:
  explicit AspectMovieLabel(QWidget* parent) : QLabel(parent) {}

 protected:
  void paintEvent(QPaintEvent* event) override {
    QMovie* current = movie();
    const QPixmap frame = current ? current->currentPixmap() : QPixmap();
    if (frame.isNull()) {
      QLabel::paintEvent(event);
      return;
    }
    const QRect available = contentsRect();
    if (available.isEmpty()) return;
    QSize size = frame.size().scaled(available.size(), Qt::KeepAspectRatio);
    // QSize::scaled can round one side to zero for extreme aspect ratios.
    size.setWidth(std::max(1, size.width()));
    size.setHeight(std::max(1, size.height()));
    QRect target(QPoint(0, 0), size);
    target.moveCenter(available.center());
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(target, frame);
  }
};

// A media surface needs contrast in both light and dark themes. Derive the
// canvas from the palette rather than hard-coding a window background colour.
QPalette mediaCanvasPalette(const QPalette& src) {
  const QColor text = src.color(QPalette::WindowText);
  const QColor base = src.color(QPalette::Base);
  const bool textIsDarker = text.lightness() < base.lightness();
  const QColor dark = textIsDarker ? text : base;
  const QColor other = textIsDarker ? base : text;
  constexpr qreal kLift = 0.13;
  const QColor fill(
      qRound(dark.red() + (other.red() - dark.red()) * kLift),
      qRound(dark.green() + (other.green() - dark.green()) * kLift),
      qRound(dark.blue() + (other.blue() - dark.blue()) * kLift));
  QPalette out = src;
  out.setColor(QPalette::Window, fill);
  out.setColor(QPalette::Base, fill);
  return out;
}
}  // namespace

PreviewPanel::PreviewPanel(QWidget* parent) : QWidget(parent) {
  auto* box = new QGroupBox(QStringLiteral("Preview"), this);
  box->setObjectName(QStringLiteral("previewGroup"));
  auto* lay = new QVBoxLayout(box);

  // One shared frame, with two pages occupying exactly the same viewport.
  // Status text remains outside the media surface and cannot cover a frame.
  viewTabs_ = new QTabBar(box);
  viewTabs_->setObjectName(QStringLiteral("previewViewTabs"));
  viewTabs_->setExpanding(false);
  viewTabs_->addTab(QStringLiteral("Before"));
  viewTabs_->addTab(QStringLiteral("After"));
  lay->addWidget(viewTabs_);

  const QPalette canvasPal = mediaCanvasPalette(palette());
  auto* frame = new QFrame(box);
  frame->setObjectName(QStringLiteral("previewCanvas"));
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setMinimumSize(kMinimumPreviewSize);
  frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  frame->setPalette(canvasPal);
  frame->setAutoFillBackground(true);
  frame->setBackgroundRole(QPalette::Base);
  auto* frameLayout = new QVBoxLayout(frame);
  frameLayout->setContentsMargins(2, 2, 2, 2);
  viewportPages_ = new QStackedWidget(frame);
  viewportPages_->setObjectName(QStringLiteral("previewViewport"));
  viewportPages_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto makeLabel = [this, &canvasPal](const QString& name, const QString& placeholder) {
    auto* label = new AspectMovieLabel(viewportPages_);
    label->setObjectName(name);
    // Ignore the native movie dimensions for layout: a large GIF must not make
    // the window grow or clip, and small GIFs may use the available viewport.
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    label->setMinimumSize(1, 1);
    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::PlainText);
    label->setPalette(canvasPal);
    label->setForegroundRole(QPalette::Mid);
    label->setText(placeholder);
    viewportPages_->addWidget(label);
    return label;
  };
  beforeLabel_ = makeLabel(QStringLiteral("previewBefore"), QStringLiteral("no file selected"));
  afterLabel_ = makeLabel(QStringLiteral("previewAfter"), QStringLiteral("Apply settings to preview"));
  frameLayout->addWidget(viewportPages_);
  lay->addWidget(frame, 1);
  connect(viewTabs_, &QTabBar::currentChanged, this, [this](int index) {
    viewportPages_->setCurrentIndex(index);
  });

  savingsLabel_ = new QLabel(box);
  savingsLabel_->setObjectName(QStringLiteral("previewSavings"));
  savingsLabel_->setAlignment(Qt::AlignCenter);
  lay->addWidget(savingsLabel_);

  auto* playbackRow = new QHBoxLayout();
  previewButton_ = new QPushButton(QStringLiteral("Apply setting preview"), box);
  previewButton_->setObjectName(QStringLiteral("previewGenerateButton"));
  previewButton_->setToolTip(QStringLiteral("Encode one temporary preview with the current Actions"));
  previewButton_->setEnabled(false);
  playButton_ = new QPushButton(QStringLiteral("Play"), box);
  playButton_->setObjectName(QStringLiteral("previewPlayButton"));
  playButton_->setToolTip(QStringLiteral("Play the Before and After animations"));
  stopButton_ = new QPushButton(QStringLiteral("Stop"), box);
  stopButton_->setObjectName(QStringLiteral("previewStopButton"));
  stopButton_->setToolTip(QStringLiteral("Stop preview playback"));
  autoPlayCheck_ = new QCheckBox(QStringLiteral("Auto-play previews"), box);
  autoPlayCheck_->setObjectName(QStringLiteral("previewAutoPlay"));
  autoPlayCheck_->setToolTip(QStringLiteral(
      "When off, use Play to watch the preview; playback never encodes a file."));
  autoPlayCheck_->setChecked(true);
  playbackRow->addWidget(previewButton_);
  playbackRow->addWidget(playButton_);
  playbackRow->addWidget(stopButton_);
  playbackRow->addWidget(autoPlayCheck_);
  playbackRow->addStretch();
  lay->addLayout(playbackRow);

  captionLabel_ = new QLabel(box);
  captionLabel_->setObjectName(QStringLiteral("previewCaption"));
  captionLabel_->setWordWrap(true);
  captionLabel_->setTextFormat(Qt::PlainText);
  lay->addWidget(captionLabel_);

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
    delete *movie;  // synchronous: QMovie otherwise keeps Windows files locked
    *movie = nullptr;
  }
}

void PreviewPanel::releaseMovies() {
  afterLabel_->setMovie(nullptr);
  stopMovie(&afterMovie_);
  afterLabel_->setText(QStringLiteral("Apply settings to preview"));
  beforeLabel_->setMovie(nullptr);
  stopMovie(&beforeMovie_);
  beforeLabel_->setText(QStringLiteral("no file selected"));
  savingsLabel_->clear();
  updatePlaybackControls();
}

void PreviewPanel::setBefore(const QString& gifPath) {
  beforeLabel_->setMovie(nullptr);
  stopMovie(&beforeMovie_);
  beforeLabel_->setText(QStringLiteral("no file selected"));
  viewTabs_->setCurrentIndex(0);
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
  beforeLabel_->setText(QString());
  beforeLabel_->setMovie(beforeMovie_);
  connect(beforeMovie_, &QMovie::updated, beforeLabel_, [this](const QRect&) { beforeLabel_->update(); });
  if (autoPlay()) beforeMovie_->start();
  else beforeMovie_->jumpToFrame(0);
  updatePlaybackControls();
}

bool PreviewPanel::setAfter(const QString& gifPath, qint64 beforeBytes,
                            qint64 afterBytes, const QString& context) {
  afterLabel_->setMovie(nullptr);
  stopMovie(&afterMovie_);
  afterLabel_->setText(QStringLiteral("Apply settings to preview"));
  if (gifPath.isEmpty() || !QFileInfo::exists(gifPath)) {
    clearAfter(QStringLiteral("preview produced no file"));
    return false;
  }
  afterMovie_ = new QMovie(gifPath, QByteArray(), this);
  if (!afterMovie_->isValid()) {
    stopMovie(&afterMovie_);
    clearAfter(QStringLiteral("preview file is not a playable GIF"));
    return false;
  }
  afterLabel_->setText(QString());
  afterLabel_->setMovie(afterMovie_);
  connect(afterMovie_, &QMovie::updated, afterLabel_, [this](const QRect&) { afterLabel_->update(); });
  if (autoPlay()) afterMovie_->start();
  else afterMovie_->jumpToFrame(0);
  viewTabs_->setCurrentIndex(1);

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
  captionBase_ = context.isEmpty() ? QString::fromLatin1(kPreviewCaption) : context;
  generating_ = false;
  status_.clear();
  updatePlaybackControls();
  updateCaption();
  return true;
}

void PreviewPanel::clearAfter(const QString& reason) {
  afterLabel_->setMovie(nullptr);
  stopMovie(&afterMovie_);
  afterLabel_->setText(QStringLiteral("Apply settings to preview"));
  savingsLabel_->clear();
  generating_ = false;
  status_ = reason;
  captionBase_ = QString::fromLatin1(kPreviewCaption);
  updatePlaybackControls();
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

void PreviewPanel::setPreviewAvailable(bool available) {
  previewButton_->setEnabled(available);
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
  if (autoPlayCheck_ && autoPlayCheck_->isChecked() != enabled) autoPlayCheck_->setChecked(enabled);
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
  if (generating_) text += QStringLiteral("  Generating preview…");
  else if (!status_.isEmpty()) text += QStringLiteral("  ") + status_;
  captionLabel_->setText(text);
}

QString PreviewPanel::humanSize(qint64 bytes) {
  if (bytes >= 1024 * 1024)
    return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0), 0, 'f', 2);
  if (bytes >= 1024)
    return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / 1024.0, 0, 'f', 1);
  return QStringLiteral("%1 B").arg(bytes);
}
