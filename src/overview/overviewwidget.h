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
#include "overview/OverviewModel.h"
#include "overviewbtnwidget.h"

namespace Ui {
  class OverviewWidget;
}

class OverviewWidget : public QWidget
{
Q_OBJECT

public:
  explicit OverviewWidget(Conversations *ctx, QWidget *parent = nullptr);
  ~OverviewWidget() override;

signals:
  void overviewRowClicked(int idx);

private:
  void setupUITable();

  Ui::OverviewWidget *ui;
  Conversations *m_ctx;
};
