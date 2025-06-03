#pragma once
#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QDesktopServices>
#include <QObject>
#include <QtCore>
#include <QtGui>
#include <QFileInfo>
#include <QLineEdit>
#include <QTextEdit>

#ifdef QUICK
#include <QQuickImageProvider>
#endif

#include <iostream>
#include <hildon-uri.h>

#include "conversations.h"
#include "lib/config.h"
#include "lib/state.h"
#include "lib/abook/abook_public.h"
#include "lib/abook/abook_roster.h"
#include "lib/rtcom/rtcom_public.h"
#include "lib/mainwindow.h"
#include "searchwindow.h"
#include "models/ChatModel.h"
#include "models/ChatMessage.h"

namespace Ui {
    class ChatWindow;
}

class ChatWindow : public QConversationsMainWindow {
Q_OBJECT
Q_PROPERTY(QString local_uid MEMBER local_uid);
Q_PROPERTY(QString remote_uid MEMBER remote_uid);
Q_PROPERTY(bool groupchat MEMBER groupchat NOTIFY groupchatChanged);

public:
    Ui::ChatWindow *ui;
    explicit ChatWindow(Conversations *ctx, const QString &local_uid, const QString &remote_uid, const QString &group_uid, const QString &channel, const QString &service_uid, QWidget *parent = nullptr);
    static Conversations *getContext();
    ~ChatWindow() override;
public:
    QString local_uid;
    QString group_uid;
    QString remote_uid;
    QString channel;
    QString service_uid;
    bool groupchat;
public:
#ifndef QUICK
    void setupChatWidget();
    static QString generateChatHTML(const QSharedPointer<ChatMessage> &chats);
#endif
    void setHighlight(const unsigned int event_id);
    void fillBufferUntil(const unsigned int event_id) const;
    Q_INVOKABLE void showMessageContextMenu(unsigned int event_id, QPoint point);

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
    void onIgnoreNotificationsToggled();
    void onSetupGroupchat();
    void onAutoJoinToggled();
    void onSetWindowTitle();
    void onChatRequestClear();
    void onChatRequestDelete();
    void onShowMessageContextMenu(int event_id, QVariant test);
    void onSetupAuthorizeActions();
    void onContactsChanged(std::map<std::string, std::shared_ptr<AbookContact>> contacts);
    void onAvatarChanged(std::string local_uid_str, std::string remote_uid_str);
    void onAddFriend();
    void onRemoveFriend();
    void onAcceptFriend();
    void onRejectFriend();
    void onDisplayChatBox();

signals:
    void closed(const QString &remote_uid);
    void sendMessage(const QString &_local_uid, const QString &_remote_uid, const QString &message);
    void jumpToMessage(int event_id);
    void scrollDown();
    void chatPostReady();
    void chatPreReady();
    void chatCleared();
    void avatarChanged();
    void groupchatChanged();

private:
    Conversations *m_ctx;
    ChatModel *chatModel = nullptr;
    static ChatWindow *pChatWindow;
    SearchWindow *m_searchWindow = nullptr;
    bool m_auto_join = false;
    bool m_ignore_notifications = false;
    QSharedPointer<ContactItem> m_abook_contact;

private:
    QTimer *m_windowFocusTimer;
    bool m_enterKeySendsChat = false;
    AvatarImageProvider* avatarProvider = nullptr;
    unsigned int m_windowFocus = 0; // seconds
    bool m_active = false;  // do we have an active Tp connection?
    bool m_windowActive = false;
    QWidget* m_chatBox = nullptr;
private:
    QString remoteId() const;
    void detectActiveChannel();
    void setChatState(Tp::ChannelChatState state) const;
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event);
};

