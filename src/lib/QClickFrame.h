#pragma once
#include <QFrame>
#include <QMouseEvent>

class ClickFrame : public QFrame {
  Q_OBJECT
public:
  explicit ClickFrame(QWidget *parent = nullptr) : QFrame(parent) {}
protected:
  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton)
      emit clicked();
    QFrame::mousePressEvent(event);
  }
signals:
  void clicked();
};
