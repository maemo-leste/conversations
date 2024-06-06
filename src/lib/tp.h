#pragma once

#include <QtCore>
#include <QLocalServer>
#include <QtDBus/QtDBus>

// TODO: clean these up!
#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ContactMessenger>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingSendMessage>
#include <TelepathyQt/SimpleObserver>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/SimpleTextObserver>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Types>
#include <TelepathyQt/ReceivedMessage>
#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/Account>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ContactMessenger>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingSendMessage>
#include <TelepathyQt/SimpleTextObserver>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/Types>

#include "lib/utils.h"
#include "lib/config.h"
#include "models/ChatMessage.h"

#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>

#ifdef OSSO_ABOOK
#include <libosso-abook/osso-abook.h>
#endif

class Telepathy;
class TelepathyAccount;
class TelepathyChannel;
class TelepathyHandler;

class AccountChannel
{
public:
    explicit AccountChannel();
    QString name;
    bool auto_join = false;
    qint64 date_created;
    qint64 date_last_message = 0;
    TelepathyChannel* tpChannel = nullptr;
public:
    bool hasActiveChannel() { return tpChannel != nullptr; }
};

class TelepathyAccount : public QObject {
Q_OBJECT

public:
    explicit TelepathyAccount(Tp::AccountPtr macc);
    ~TelepathyAccount() override;

    Tp::AccountPtr acc;
    QMap<QString, AccountChannel*> channels;

    QString nickname() const { return m_nickname; }
    QString protocolName() const { return m_protocol_name; }
    QString getServiceName();
    QString name;

    void setAutoJoin(const QString &remote_id, bool autoJoin);
    void ensureChannel(const QString &remote_id);

signals:
    void databaseAddition(const QSharedPointer<ChatMessage> &msg);
    void openChannelWindow(const QString& local_uid, const QString &remote_uid, const QString &group_uid, const QString& service, const QString& channel);
    void removed(TelepathyAccount* account);
    void channelJoined(QString local_uid, QString channel);
    void channelLeft(QString local_uid, QString channel);

public slots:
    void sendMessage(const QString &remote_id, const QString &message);
    void joinChannel(const QString &remote_id, bool persistent);
    void leaveChannel(const QString &remote_id);
    void setChatState(const QString &remote_id, Tp::ChannelChatState state);

    /* TODO: make these private again and use connect/emit */
    void onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);
    void onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel);

    void TpOpenChannelWindow(Tp::TextChannelPtr channel);

    Tp::TextChannel* hasChannel(const QString& remote_id);
    void _removeChannel(TelepathyChannel* chanptr);

    bool log_event(time_t epoch, const QString &text, bool outgoing, const Tp::TextChannelPtr &channel, const QString &remote_uid, const QString &remote_alias);

    static QString getRemoteUid(Tp::TextChannelPtr channel);
    QString getGroupUid(Tp::TextChannelPtr channel);
    QString getLocalUid();

private slots:
    void onOnline(bool online);
    void onAccReady(Tp::PendingOperation *op);
    void onChannelJoined(const Tp::ChannelRequestPtr &channelRequest, QString channel);
    void onChannelLeft(QString channel);
    void onRemoved(void);
    void joinSavedGroupChats();

    // TODO return value
    // sendChannelMessage (if we cannot just use sendMessage)

private:
    QString m_nickname;
    QString m_protocol_name;

    Tp::SimpleTextObserverPtr textobserver;
    Tp::SimpleObserverPtr channelobserver;
    Tp::ContactMessengerPtr messenger;
    Tp::TextChannel *hgbchan;
    Tp::AccountManagerPtr m_accountmanager;

    void readGroupchatChannels();
    void writeGroupchatChannels();
    void _joinChannel(const QString &channel);
};

class TelepathyChannel : public QObject {
Q_OBJECT

public:
    explicit TelepathyChannel(const Tp::ChannelPtr& mchannel, TelepathyAccount* macc);
    ~TelepathyChannel() override;

    bool auto_join = false;  // auto-join on program startup

public slots:
    void sendMessage(const QString &message) const;

private slots:
    void onGroupAddContacts(Tp::PendingOperation *op);
    void onChannelReady(Tp::PendingOperation *op);
    void onInvalidated(Tp::DBusProxy* proxy, const QString& errorName, const QString& errorMessage);

    void onChanMessageReceived(const Tp::ReceivedMessage &message);
    void onChanPendingMessageRemoved(const Tp::ReceivedMessage &message);
    void onChanMessageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &) const;
    // sendChannelMessage (if we cannot just use sendMessage)

public:
    TelepathyAccount* m_account;

    Tp::ChannelPtr m_channel;

private:
};


class Telepathy : public QObject {
Q_OBJECT

public:
    explicit Telepathy(QObject *parent = nullptr);
    ~Telepathy() override;

    QList<TelepathyAccount*> accounts;

    TelepathyAccount* accountByName(const QString &backend_name);
    AccountChannel* channelByName(const QString &backend_name, const QString &remote_id);
    void joinChannel(const QString &backend_name, const QString &remote_id, bool persistent);
    void leaveChannel(const QString &backend_name, const QString &remote_id);
    bool participantOfChannel(const QString &backend_name, const QString &remote_id);
    TelepathyAccount* rtcomLocalUidToAccount(const QString &backend_name);

signals:
    void databaseAddition(const QSharedPointer<ChatMessage> &msg);
    void openChannelWindow(const QString& local_uid, const QString &remote_uid, const QString &group_uid, const QString& service, const QString& channel);
    void accountManagerReady();
    void accountAdded(TelepathyAccount* account);
    void channelJoined(QString backend_name, QString channel);
    void channelLeft(QString backend_name, QString channel);

public slots:
    void sendMessage(const QString &backend_name, const QString &remote_id, const QString &message);
    void setChatState(const QString &backend_name, const QString &remote_id, Tp::ChannelChatState state);

    void onDatabaseAddition(const QSharedPointer<ChatMessage> &msg);
    void onOpenChannelWindow(const QString& local_uid, const QString &remote_uid, const QString &group_uid, const QString& service, const QString& channel);

    void onNewAccount(const Tp::AccountPtr &account);
    void onAccountRemoved(TelepathyAccount* account);


private slots:
    void onAccountManagerReady(Tp::PendingOperation *op);

private:
    Tp::ClientRegistrarPtr registrar;
    Tp::AccountManagerPtr m_accountmanager;
    Tp::AbstractClientPtr clienthandler;
};


class TelepathyHandler : public Tp::AbstractClientHandler {
public:
    TelepathyHandler(const Tp::ChannelClassSpecList &channelFilter);
    ~TelepathyHandler() { }
    bool bypassApproval() const;
    void handleChannels(const Tp::MethodInvocationContextPtr<> &context,
        const Tp::AccountPtr &account,
        const Tp::ConnectionPtr &connection,
        const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const Tp::AbstractClientHandler::HandlerInfo &handlerInfo);

    void setTelepathyParent(Telepathy* parent);
public:
    Telepathy* m_telepathy_parent;

};
