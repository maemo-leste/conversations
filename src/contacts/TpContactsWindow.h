#pragma once

#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#ifdef QUICK
#include <QQuickItem>
#endif
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"
#include "models/ChatModel.h"
#include "models/ChatMessage.h"
#include "contacts/TpContactsWidget.h"
#include "models/PreviewItem.h"
#include "lib/imagepinch.h"

namespace Ui {
  class TpContactsWindow;
}

class TpContactsWindow : public QMainWindow {
  Q_OBJECT

public:
  Ui::TpContactsWindow *ui;
  explicit TpContactsWindow(const TelepathyChannelPtr &channel, QWidget *parent = nullptr);
  ~TpContactsWindow() override;

signals:
  void closed();
  void contactClicked(Tp::ContactPtr ptr);

private slots:
  void onSetWindowTitle();

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  TelepathyChannelPtr channel;
  TpContactsProxyModel *m_proxyModel = nullptr;
  TpContactsModel *m_model = nullptr;
  TpContactsWidget *m_widgetContacts = nullptr;
  static TpContactsWindow *pTpContactsWindow;
};
