#pragma once
#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"
#include "searchwindow.h"
#include "models/ChatModel.h"
#include "models/ChatMessage.h"

namespace Ui {
    class ChatWindow;
}

class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    Ui::ChatWindow *ui;
    explicit ChatWindow(Conversations *ctx, QSharedPointer<ChatMessage> msg, QWidget *parent = nullptr);
    static Conversations *getContext();
    ~ChatWindow() override;

    ChatModel *chatModel;
    bool groupchat;

public slots:
  void onDatabaseAddition(const QSharedPointer<ChatMessage> &msg);

private slots:
    void onChatPreReady();
    void onGatherMessage();
    void onOpenSearchWindow();
    void onExportToCsv();
    void onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg);
    void onAutoCloseChatWindowsChanged(bool enabled);
    void onSearchResultClicked(const QSharedPointer<ChatMessage> &msg);
    void onGroupchatJoinLeaveRequested();
    void onChannelJoinedOrLeft(const QString &local_uid, const QString &channel);
    void onSetupGroupchat();
    void onAutoJoinToggled();
    void onSetWindowTitle();

signals:
    void closed(const QString &remote_uid);
    void sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message);
    void jumpToMessage(int event_id);
    void scrollDown();
    void chatPostReady();
    void chatPreReady();

private:
    Conversations *m_ctx;
    static ChatWindow *pChatWindow;
    QSharedPointer<ChatMessage> m_chatMessage;
    QString m_channel;
    QString m_local_uid;
    bool m_enterKeySendsChat = false;
    SearchWindow *m_searchWindow = nullptr;

    QTimer *m_windowFocusTimer;
    unsigned int m_windowFocus = 0; // seconds
    bool m_active = false;  // do we have an active Tp connection?

    void fillBufferUntil(const QSharedPointer<ChatMessage> &msg) const;
    QString remoteId() const;
    void detectActiveChannel();
    void setChatState(Tp::ChannelChatState state) const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
};
