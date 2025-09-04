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
#include <QTableView>
#include <QHeaderView>
#include <QWidget>
#include <QMenu>

#include "conversations.h"
#include "lib/utils.h"
#include "lib/QCenteredIconDelegate.h"
#include "contacts/TpContactsModel.h"

namespace Ui {
  class TpContactsWidget;
}

class TpContactsWidget : public QWidget {
Q_OBJECT

public:
  explicit TpContactsWidget(TpContactsProxyModel *proxyModel, QWidget *parent = nullptr);
  int proxyColumn(QSortFilterProxyModel *proxy, int sourceColumn);
  ~TpContactsWidget() override;

signals:
  void contactClicked(int idx);

public slots:
  void onSetColumnStyleDelegate();
  void onSetTableHeight();

private:
  void setupUITable();
  RichItemDelegate *m_richContentDelegate = nullptr;
  CenteredIconDelegate *m_centeredIconDelegate = nullptr;
  Ui::TpContactsWidget *ui;
  TpContactsProxyModel *m_proxyModel = nullptr;
};
