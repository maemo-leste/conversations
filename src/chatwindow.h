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
#include <QQuickImageProvider>

#include <iostream>
#include <hildon-uri.h>

#include "conversations.h"
#include "lib/config.h"
#include "lib/state.h"
#include "searchwindow.h"
#include "models/ChatModel.h"
#include "models/ChatMessage.h"

namespace Ui {
    class ChatWindow;
}

class ChatWindow : public QMainWindow {
Q_OBJECT
Q_PROPERTY(QString local_uid MEMBER local_uid);
Q_PROPERTY(QString remote_uid MEMBER remote_uid);
Q_PROPERTY(bool groupchat MEMBER groupchat NOTIFY groupchatChanged);
Q_PROPERTY(bool displayChatGradient MEMBER displayChatGradient NOTIFY displayChatGradientChanged);

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
    bool displayChatGradient;
public:
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
    void onSetupGroupchat();
    void onAutoJoinToggled();
    void onSetWindowTitle();
    void onChatRequestClear();
    void onChatRequestDelete();
    void onShowMessageContextMenu(int event_id, QVariant test);
    void onSetupAuthorizeActions();
    void onTrySubscribeAvatarChanged();
    void onAddFriend();
    void onRemoveFriend();
    void onAcceptFriend();
    void onRejectFriend();

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
    void displayChatGradientChanged();

private:
    Conversations *m_ctx;
    ChatModel *chatModel;
    static ChatWindow *pChatWindow;
    SearchWindow *m_searchWindow = nullptr;
    bool m_auto_join = false;
    QSharedPointer<ContactItem> m_abook_contact;

private:
    QTimer *m_windowFocusTimer;
    bool m_enterKeySendsChat = false;
    unsigned int m_windowFocus = 0; // seconds
    bool m_active = false;  // do we have an active Tp connection?
    bool m_windowActive = false;
private:
    QString remoteId() const;
    void detectActiveChannel();
    void setChatState(Tp::ChannelChatState state) const;
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event);
};

class AvatarImageProvider : public QQuickImageProvider {
public:
    AvatarImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override {
        int last_slash = id.lastIndexOf("?token=");
        QString hex_str = id.mid(last_slash + 7);
        QString _uid = id.left(last_slash);

        if(abook_roster_cache.contains(_uid))
            return abook_roster_cache[_uid]->avatar();
        return {};
    }
};
