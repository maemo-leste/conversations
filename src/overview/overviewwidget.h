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

private slots:
  void onAccountButtonClicked(const QString service);

signals:
  void overviewRowClicked(int idx);

private:
  QMap<QString, OverviewBtnWidget*> m_accountButtons;
  void setupUIAccounts();
  void setupUITable();
  void addAccountButton(const QString title, const QString service);

  Ui::OverviewWidget *ui;
  Conversations *m_ctx;
};
