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
#include "requests/RequestModel.h"
#include "lib/QRichItemDelegate.h"

namespace Ui {
  class RequestWidget;
}

class RequestWidget : public QWidget
{
Q_OBJECT

public:
  explicit RequestWidget(Conversations *ctx, QWidget *parent = nullptr);
  ~RequestWidget() override;

  unsigned int itemHeight() { return m_itemHeight; }

signals:
  void overviewRowClicked(int idx);
  void openDialog(QSharedPointer<ContactItem> item);

public slots:
  void onSetContentDelegate();
  void onSetTableHeight();

private:
  void setupUITable();
  unsigned int m_itemHeight = 66;
  RichItemDelegate *m_richItemDelegate = nullptr;
  Ui::RequestWidget *ui;
  Conversations *m_ctx;
};
