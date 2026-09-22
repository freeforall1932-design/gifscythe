// DropListWidget.h - QListWidget that accepts file drag-and-drop.
#ifndef GIFSCYTHE_DROPLISTWIDGET_H
#define GIFSCYTHE_DROPLISTWIDGET_H

#include <QListWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

class DropListWidget : public QListWidget {
  Q_OBJECT
 public:
  explicit DropListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
  }

 signals:
  void filesDropped(const QStringList& files);

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
    else event->ignore();
  }
  void dragMoveEvent(QDragMoveEvent* event) override {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
    else event->ignore();
  }
  void dropEvent(QDropEvent* event) override {
    QStringList files;
    for (const QUrl& url : event->mimeData()->urls()) {
      if (url.isLocalFile()) files << url.toLocalFile();
    }
    if (!files.isEmpty()) {
      emit filesDropped(files);
      event->acceptProposedAction();
    } else {
      event->ignore();
    }
  }
};

#endif  // GIFSCYTHE_DROPLISTWIDGET_H
