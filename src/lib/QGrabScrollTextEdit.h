#include <QTextEdit>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTouchEvent>

#include <QTextEdit>
#include <QScroller>
#include <QScrollerProperties>

class QGrabScrollTextEdit : public QTextEdit {
public:
  explicit QGrabScrollTextEdit(QWidget *parent = nullptr)
      : QTextEdit(parent) {
    // setTextInteractionFlags(Qt::NoTextInteraction);
    setAttribute(Qt::WA_AcceptTouchEvents);
    QScroller *scroller = QScroller::scroller(this->viewport());
    QScrollerProperties props = scroller->scrollerProperties();

    // no workie
    // QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(
    //     QScrollerProperties::OvershootAlwaysOff
    // );

    props.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
    props.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
    props.setScrollMetric(QScrollerProperties::MaximumVelocity, 1.5);
    props.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.15);
    scroller->setScrollerProperties(props);

    QScroller::grabGesture(this->viewport(), QScroller::TouchGesture);
    QScroller::grabGesture(this->viewport(), QScroller::LeftMouseButtonGesture);
  }
};
