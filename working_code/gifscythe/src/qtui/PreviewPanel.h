// PreviewPanel.h - Before/after animation preview (eZgif feel).
//
// "Before" plays the selected original file. "After" plays the result of a
// DEBOUNCED, ASYNC single-file engine run (MainWindow owns the QProcess;
// this panel only displays). Never blocks the UI thread (audit S3-7 rule).
// Captions stay honest: the After pane is a preview of the SELECTED file
// with current actions — not proof of what a Batch/Merge run will write.

#ifndef GIFSCYTHE_PREVIEWPANEL_H
#define GIFSCYTHE_PREVIEWPANEL_H

#include <QWidget>

class QLabel;
class QMovie;

class PreviewPanel : public QWidget {
  Q_OBJECT
 public:
  explicit PreviewPanel(QWidget* parent = nullptr);

  void setBefore(const QString& gifPath);
  void setAfter(const QString& gifPath, qint64 beforeBytes, qint64 afterBytes);
  void clearAfter(const QString& reason);
  void setGenerating(bool generating);
  void setStatus(const QString& status);  // appended to the base caption
  // Stop and delete both movies immediately, releasing their file handles
  // (Windows delete-locks the file of a live QMovie; the owner's temp-dir
  // sweep needs the handles gone first).
  void releaseMovies();

 private:
  void stopMovie(QMovie** movie);
  void updateCaption();
  static QString humanSize(qint64 bytes);

  QLabel* beforeLabel_ = nullptr;
  QLabel* afterLabel_ = nullptr;
  QLabel* savingsLabel_ = nullptr;
  QLabel* captionLabel_ = nullptr;
  QMovie* beforeMovie_ = nullptr;
  QMovie* afterMovie_ = nullptr;
  bool generating_ = false;
  QString status_;
};

#endif  // GIFSCYTHE_PREVIEWPANEL_H
