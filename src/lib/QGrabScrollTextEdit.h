#include <QTextEdit>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTouchEvent>

class QGrabScrollTextEdit : public QTextEdit {
public:
  explicit QGrabScrollTextEdit(QWidget *parent = nullptr) :
    QTextEdit(parent), isDragging(false) {
    setTextInteractionFlags(Qt::NoTextInteraction);
    setAttribute(Qt::WA_AcceptTouchEvents);
  }

protected:
  bool event(QEvent *event) override {
    if (event->type() == QEvent::TouchBegin ||
        event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd) {
      return handleTouchEvent(static_cast<QTouchEvent *>(event));
    }
    return QTextEdit::event(event);
  }

  bool handleTouchEvent(const QTouchEvent *event) {
    if (event->touchPoints().count() == 1) {
      const QTouchEvent::TouchPoint &point = event->touchPoints().first();

      if (event->type() == QEvent::TouchBegin) {
        isDragging = true;
        dragStartPos = point.pos().toPoint();
        return true;
      } else if (event->type() == QEvent::TouchUpdate && isDragging) {
        const QPoint delta = point.pos().toPoint() - dragStartPos;
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        dragStartPos = point.pos().toPoint();
        return true;
      } else if (event->type() == QEvent::TouchEnd) {
        isDragging = false;
        return true;
      }
    }
    return false;
  }

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton) {
      isDragging = true;
      dragStartPos = event->pos();
      setCursor(Qt::ClosedHandCursor);
      event->accept();
    } else {
      QTextEdit::mousePressEvent(event);
    }
  }

  void mouseMoveEvent(QMouseEvent *event) override {
    if (isDragging) {
      QPoint delta = event->pos() - dragStartPos;
      verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
      horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
      dragStartPos = event->pos();
      event->accept();
    } else {
      QTextEdit::mouseMoveEvent(event);
    }
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton && isDragging) {
      isDragging = false;
      setCursor(Qt::ArrowCursor);
      event->accept();
    } else {
      QTextEdit::mouseReleaseEvent(event);
    }
  }

private:
  bool isDragging;
  QPoint dragStartPos;
};
