#pragma once
#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QComboBox>
#include <QObject>
#include <QtCore>
#include <QtGui>
#include <QFileInfo>
#include <QScrollArea>
#include <QComboBox>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/Connection>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"

namespace Ui {
  class JoinChannelWindow;
}

class JoinChannel : public QMainWindow {
Q_OBJECT

public:
  explicit JoinChannel(Conversations *ctx, QWidget *parent = nullptr);
  static Conversations *getContext();
  ~JoinChannel() override;
  Ui::JoinChannelWindow *ui;

  void show_account_dialog();
  void selectItem(const QString &item);
  void selectItem(TelepathyAccountPtr account_ptr);
  void submit();

signals:
  void autoCloseChatWindowsChanged(bool enabled);
  void joinChannel(QString account, QString channel);
private slots:
  void onAccountsChanged();

private:
  Conversations *m_ctx;
  TelepathyAccountPtr m_acc;
  static JoinChannel *pJoinChannel;
  void closeEvent(QCloseEvent *event) override;
  void messageBox(QString msg);

  // filter accounts that support groupchat
  // @TODO: should filter on Tp capabilities somehow, but this doesnt work:
  //   Tp::ConnectionCapabilities caps = acc->acc->capabilities();
  //   qDebug() << acc->local_uid << "chatrooms" << caps.textChatrooms();
  [[nodiscard]] QList<TelepathyAccountPtr> groupchat_accounts() const {
    QList<TelepathyAccountPtr> rtn;
    for (const TelepathyAccountPtr &acc : m_ctx->telepathy->accounts) {
      if (acc->protocolName() != "tel")
        rtn.append(acc);
    }
    return rtn;
  }
};
