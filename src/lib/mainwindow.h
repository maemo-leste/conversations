#pragma once

#include <QMainWindow>
#include <QKeyEvent>
#include <algorithm>
#include "config.h"

#include "conversations.h"

class QConversationsMainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit QConversationsMainWindow(Conversations *ctx, QWidget *parent = nullptr) : QMainWindow(parent), m_ctx(ctx) {}
  ~QConversationsMainWindow() override = default;

protected:
  void keyPressEvent(QKeyEvent *event) override {
    const int key = event->key();
    auto scaling = config()->get(ConfigKeys::TextScaling).toFloat();

    // @TODO: enable minus/plus, but only in VM
    // how to detect if we are in the VM?
    //
    // 114/115 are volume keys, on the pinephone at least
    if (key == 114 /*|| key == Qt::Key_Minus*/) {
      if (scaling > 1.0f) {
        scaling = std::max(1.0f, scaling - 0.25f);
        config()->set(ConfigKeys::TextScaling, scaling);
        emit m_ctx->onTextScalingChanged();
      }
    } else if (key == 115 /*|| key == Qt::Key_Equal*/) {
      if (scaling < 2.0f) {
        scaling = std::min(2.0f, scaling + 0.25f);
        config()->set(ConfigKeys::TextScaling, scaling);
        emit m_ctx->onTextScalingChanged();
      }
    } else {
      QMainWindow::keyPressEvent(event);
    }
  }

private:
  Conversations *m_ctx;
};