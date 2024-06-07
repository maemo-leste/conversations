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
    explicit ChatWindow(Conversations *ctx, const QString &local_uid, const QString &remote_uid, const QString &group_uid, const QString &channel, const QString &service_uid, QWidget *parent = nullptr);
    static Conversations *getContext();
    ~ChatWindow() override;
public:
    const QString local_uid;
    const QString group_uid;
    const QString remote_uid;
    const QString channel;
    const QString service_uid;
    const bool groupchat;
public:
    void setHighlight(const unsigned int event_id);
    void fillBufferUntil(const unsigned int event_id) const;

public slots:
    void onChatClear();
    void onChatDelete();
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
    void onChannelJoinedOrLeft(const QString &_local_uid, const QString &_channel);
    void onEnterKeySendsChatToggled(bool enabled);
    void onSetupGroupchat();
    void onAutoJoinToggled();
    void onSetWindowTitle();
    void onChatRequestClear();
    void onChatRequestDelete();

signals:
    void closed(const QString &remote_uid);
    void sendMessage(const QString &_local_uid, const QString &_remote_uid, const QString &message);
    void jumpToMessage(int event_id);
    void scrollDown();
    void chatPostReady();
    void chatPreReady();
    void chatCleared();

private:
    Conversations *m_ctx;
    ChatModel *chatModel;
    static ChatWindow *pChatWindow;
    SearchWindow *m_searchWindow = nullptr;
private:
    QTimer *m_windowFocusTimer;
    bool m_enterKeySendsChat = false;
    unsigned int m_windowFocus = 0; // seconds
    bool m_active = false;  // do we have an active Tp connection?
private:
    QString remoteId() const;
    void detectActiveChannel();
    void setChatState(Tp::ChannelChatState state) const;
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
};
