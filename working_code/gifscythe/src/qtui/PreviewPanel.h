// PreviewPanel.h - single-viewport Before/After animation preview.
//
// Before is the selected source; After is an explicitly requested settings
// preview or a verified output. MainWindow owns the encoder process; this
// widget only displays movies and playback/status controls.

#ifndef GIFSCYTHE_PREVIEWPANEL_H
#define GIFSCYTHE_PREVIEWPANEL_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QMovie;
class QPushButton;
class QStackedWidget;
class QTabBar;

class PreviewPanel : public QWidget {
  Q_OBJECT
 public:
  explicit PreviewPanel(QWidget* parent = nullptr);

  void setBefore(const QString& gifPath);
  bool setAfter(const QString& gifPath, qint64 beforeBytes, qint64 afterBytes,
                const QString& context = QString());
  void clearAfter(const QString& reason);
  void setGenerating(bool generating);
  void setStatus(const QString& status);
  void setPreviewAvailable(bool available);
  void playMovies();
  void stopMovies();
  void setAutoPlay(bool enabled);
  bool autoPlay() const;
  // Release file handles synchronously before sweeping preview files (Windows).
  void releaseMovies();

 signals:
  void previewRequested();

 private:
  void stopMovie(QMovie** movie);
  void updatePlaybackControls();
  void updateCaption();
  static QString humanSize(qint64 bytes);

  QLabel* beforeLabel_ = nullptr;
  QLabel* afterLabel_ = nullptr;
  QLabel* savingsLabel_ = nullptr;
  QLabel* captionLabel_ = nullptr;
  QTabBar* viewTabs_ = nullptr;
  QStackedWidget* viewportPages_ = nullptr;
  QMovie* beforeMovie_ = nullptr;
  QMovie* afterMovie_ = nullptr;
  QPushButton* previewButton_ = nullptr;
  QPushButton* playButton_ = nullptr;
  QPushButton* stopButton_ = nullptr;
  QCheckBox* autoPlayCheck_ = nullptr;
  bool generating_ = false;
  QString status_;
  QString captionBase_;
};

#endif  // GIFSCYTHE_PREVIEWPANEL_H
