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
  explicit OverviewWidget(Conversations *ctx, OverviewProxyModel *proxyModel, QWidget *parent = nullptr);
  ~OverviewWidget() override;

signals:
  void overviewRowClicked(int idx);

public slots:
  void onSetColumnStyleDelegate();
  void onSetTableHeight();
  void onAvatarDisplayChanged();

private:
  void setupUITable();
  RichItemDelegate *m_richItemDelegate = nullptr;
  Ui::OverviewWidget *ui;
  OverviewProxyModel *m_proxyModel = nullptr;
  Conversations *m_ctx;
};
