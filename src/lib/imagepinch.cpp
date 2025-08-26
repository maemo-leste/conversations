#include <QDir>
#include <QImageReader>
#include <QGestureEvent>
#include <QScreen>
#include <QPainter>

#include "imagepinch.h"

ImageWidget::ImageWidget(QWidget *parent) :
  QWidget(parent), pos(0), horizontalOffset(0), verticalOffset(0),
  rotationAngle(0), scaleFactor(1), currentStepScaleFactor(1) {
  setMinimumSize(QSize(100, 100));
}

void ImageWidget::grabGestures(const QVector<Qt::GestureType> &gestures) {
  for (const Qt::GestureType gesture : gestures)
    grabGesture(gesture);
}

bool ImageWidget::event(QEvent *event) {
  if (event->type() == QEvent::Gesture)
    return gestureEvent(static_cast<QGestureEvent*>(event));
  return QWidget::event(event);
}

void ImageWidget::paintEvent(QPaintEvent*) {
  QPainter p(this);

  const qreal iw = currentImage.width();
  const qreal ih = currentImage.height();
  const qreal wh = height();
  const qreal ww = width();

  p.translate(ww / 2, wh / 2);
  p.translate(horizontalOffset, verticalOffset);
  p.rotate(rotationAngle);
  p.scale(currentStepScaleFactor * scaleFactor, currentStepScaleFactor * scaleFactor);
  p.translate(-iw / 2, -ih / 2);
  p.drawImage(0, 0, currentImage);
}


void ImageWidget::mouseDoubleClickEvent(QMouseEvent *) {
  rotationAngle = 0;
  scaleFactor = 1;
  currentStepScaleFactor = 1;
  verticalOffset = 0;
  horizontalOffset = 0;
  update();
}

bool ImageWidget::gestureEvent(QGestureEvent *event) {
  if (QGesture *swipe = event->gesture(Qt::SwipeGesture))
    swipeTriggered(static_cast<QSwipeGesture *>(swipe));
  else if (QGesture *pan = event->gesture(Qt::PanGesture))
    panTriggered(static_cast<QPanGesture *>(pan));
  if (QGesture *pinch = event->gesture(Qt::PinchGesture))
    pinchTriggered(static_cast<QPinchGesture *>(pinch));
  return true;
}

void ImageWidget::panTriggered(QPanGesture *gesture) {
#ifndef QT_NO_CURSOR
  switch (gesture->state()) {
    case Qt::GestureStarted:
    case Qt::GestureUpdated:
      setCursor(Qt::SizeAllCursor);
      break;
    default:
      setCursor(Qt::ArrowCursor);
  }
#endif
  const QPointF delta = gesture->delta();
  horizontalOffset += delta.x();
  verticalOffset += delta.y();
  update();
}

void ImageWidget::pinchTriggered(QPinchGesture *gesture)
{
  QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
  if (changeFlags & QPinchGesture::ScaleFactorChanged) {
    currentStepScaleFactor = gesture->totalScaleFactor();
  }

  if (gesture->state() == Qt::GestureFinished) {
    scaleFactor *= currentStepScaleFactor;
    currentStepScaleFactor = 1;
  }
  update();
}

void ImageWidget::swipeTriggered(QSwipeGesture *gesture) {
  if (gesture->state() == Qt::GestureFinished) {
    if (gesture->horizontalDirection() == QSwipeGesture::Left
      || gesture->verticalDirection() == QSwipeGesture::Up) {
      goPrevImage();
    } else {
      goNextImage();
    }
    update();
  }
}

void ImageWidget::resizeEvent(QResizeEvent*) {
  if (scaleFactor == 1.0 && currentStepScaleFactor == 1.0 && !currentImage.isNull()) {
    fitToWindow();
  } else {
    update();
  }
}

void ImageWidget::fitToWindow() {
  // @TODO: does not fit quite, disable for now
  // if (currentImage.isNull())
  //   return;
  //
  // const qreal iw = currentImage.width();
  // const qreal ih = currentImage.height();
  // const qreal ww = width();
  // const qreal wh = height();
  //
  // if (iw <= 0 || ih <= 0 || ww <= 0 || wh <= 0)
  //   return;
  //
  // qreal scaleX = ww / iw;
  // qreal scaleY = wh / ih;
  // qreal fitScale = qMin(scaleX, scaleY);
  //
  // scaleFactor = fitScale;
  // currentStepScaleFactor = 1.0;
  // horizontalOffset = 0;
  // verticalOffset = 0;
  // rotationAngle = 0;
  //
  // update();
}

void ImageWidget::appendImage(QSharedPointer<PreviewItem> item, bool goto_image) {
  items << item;

  if (goto_image)
    goToImage(items.size() - 1);

  update();
}

QImage ImageWidget::loadImage(QSharedPointer<PreviewItem> item) const {
  const auto info = QFileInfo(item->filePath);
  const QString filename = info.fileName();

  QImageReader reader(item->filePath);
  reader.setAutoTransform(true);
  if (!reader.canRead()) {
    qWarning() << QDir::toNativeSeparators(filename) << ": can't load image";
    return QImage();
  }

  QImage image;
  if (!reader.read(&image)) {
    qWarning() << QDir::toNativeSeparators(filename) << ": corrupted image: " << reader.errorString();
    return QImage();
  }
  const QSize maximumSize(2000, 2000);
  if (image.size().width() > maximumSize.width() || image.height() > maximumSize.height())
      image = image.scaled(maximumSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  return image;
}

void ImageWidget::goNextImage() {
  if (items.isEmpty())
    return;

  if (pos < items.size()-1) {
    ++pos;
    prevImage = currentImage;
    currentImage = nextImage;
    if (pos+1 < items.size())
      nextImage = loadImage(items.at(pos+1));
    else
      nextImage = QImage();
  }

  fitToWindow();
  update();
}

void ImageWidget::goPrevImage() {
  if (items.isEmpty())
    return;

  if (pos > 0) {
    --pos;
    nextImage = currentImage;
    currentImage = prevImage;
    if (pos > 0)
      prevImage = loadImage(items.at(pos-1));
    else
      prevImage = QImage();
  }

  fitToWindow();
  update();
}

void ImageWidget::goToImage(int index) {
  if (items.isEmpty())
    return;

  if (index < 0 || index >= items.size()) {
    qWarning() << "goToImage: invalid index: " << index;
    return;
  }

  if (index == pos+1) {
    goNextImage();
    return;
  }

  if (pos > 0 && index == pos-1) {
    goPrevImage();
    return;
  }

  pos = index;

  if (index > 0)
    prevImage = loadImage(items.at(pos-1));
  else
    prevImage = QImage();

  currentImage = loadImage(items.at(pos));
  if (pos+1 < items.size())
    nextImage = loadImage(items.at(pos+1));
  else
    nextImage = QImage();

  if (width() > 100 && height() > 100)
    fitToWindow();

  update();
}

void ImageWidget::mousePressEvent(QMouseEvent *event) {
  dragging = true;
  lastMousePos = event->pos();
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event) {
  if (dragging) {
    const QPoint delta = event->pos() - lastMousePos;
    horizontalOffset += delta.x();
    verticalOffset += delta.y();
    lastMousePos = event->pos();
    update();
  }
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event) {
  dragging = false;
}
