#pragma once
#include <QCompleter>
#include <QCheckBox>
#include <QPushButton>
#include <QStringList>
#include <QClipboard>
#include <QScroller>
#include <QStringListModel>
#include <QTimer>
#include <QEasingCurve>
#include <QMessageBox>
#include <QWidget>
#include <QMenu>

#include "conversations.h"
#include "lib/utils.h"

namespace Ui {
  class AboutWidget;
}

class AboutWidget : public QWidget
{
Q_OBJECT

public:
  explicit AboutWidget(Conversations *ctx, QWidget *parent = nullptr);
  ~AboutWidget() override;
private:
  Ui::AboutWidget *ui;
  Conversations *m_ctx;
};
