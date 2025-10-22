#pragma once

#include <QImage>
#include <QLoggingCategory>
#include <QWidget>
#include <QFileInfo>

#include "models/PreviewItem.h"

QT_BEGIN_NAMESPACE
class QGestureEvent;
class QPanGesture;
class QPinchGesture;
class QSwipeGesture;
QT_END_NAMESPACE


class ImageWidget : public QWidget {
Q_OBJECT

public:
  ImageWidget(QWidget *parent = nullptr);
  void grabGestures(const QVector<Qt::GestureType> &gestures);

  void appendImage(QSharedPointer<PreviewItem> item, bool goto_image = true);
  void goNextImage();
  void goPrevImage();
  void goToImage(int index);

  int pos = 0;
  QList<QSharedPointer<PreviewItem>> items;

protected:
  bool event(QEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
private:
  void fitToWindow();
  bool gestureEvent(QGestureEvent *event);
  void panTriggered(QPanGesture*);
  void pinchTriggered(QPinchGesture*);
  void swipeTriggered(QSwipeGesture*);

  void loadImage();
  QImage loadImage(const QSharedPointer<PreviewItem> &item);

  QString path;

  QImage prevImage, nextImage;
  QImage currentImage;

  qreal horizontalOffset;
  qreal verticalOffset;
  qreal rotationAngle;
  qreal scaleFactor;
  qreal currentStepScaleFactor;

  // single-finger panning
  QPoint lastMousePos;
  bool dragging;
};

