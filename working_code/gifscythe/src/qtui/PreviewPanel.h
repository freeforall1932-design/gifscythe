// PreviewPanel.h - Before/after animation preview (eZgif feel).
//
// "Before" plays the selected original file. "After" plays either the result
// of a DEBOUNCED, ASYNC single-file engine run or, after a real run, the exact
// file that was written to disk. MainWindow owns the QProcess; this panel only
// displays. Captions and playback controls live outside the GIF canvas so text
// can never be painted over an animation.

#ifndef GIFSCYTHE_PREVIEWPANEL_H
#define GIFSCYTHE_PREVIEWPANEL_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QMovie;
class QPushButton;

class PreviewPanel : public QWidget {
  Q_OBJECT
 public:
  explicit PreviewPanel(QWidget* parent = nullptr);

  void setBefore(const QString& gifPath);
  // context is optional. The default describes a temporary settings preview;
  // callers can say "Showing the file written to disk" after a real run.
  void setAfter(const QString& gifPath, qint64 beforeBytes, qint64 afterBytes,
                const QString& context = QString());
  void clearAfter(const QString& reason);
  void setGenerating(bool generating);
  void setStatus(const QString& status);  // appended to the base caption
  // Explicit playback controls. Stop pauses at the beginning without deleting
  // the movie object, so Play can resume/restart without another encode.
  void playMovies();
  void stopMovies();
  void setAutoPlay(bool enabled);
  bool autoPlay() const;
  // Stop and delete both movies immediately, releasing their file handles
  // (Windows delete-locks the file of a live QMovie; the owner's temp-dir
  // sweep needs the handles gone first).
  void releaseMovies();

 signals:
  void previewRequested();  // user explicitly asks for a fresh settings preview

 private:
  void stopMovie(QMovie** movie);
  void updatePlaybackControls();
  void updateCaption();
  static QString humanSize(qint64 bytes);

  QLabel* beforeLabel_ = nullptr;
  QLabel* afterLabel_ = nullptr;
  QLabel* savingsLabel_ = nullptr;
  QLabel* captionLabel_ = nullptr;
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
