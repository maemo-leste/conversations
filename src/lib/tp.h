#ifndef CONVTP_H
#define CONVTP_H

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
#include "models/ChatMessage.h"
#ifdef RTCOM
#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>
#endif
#ifdef OSSO_ABOOK
#include <libosso-abook/osso-abook.h>
#endif

class Telepathy;
class TelepathyAccount;
class TelepathyChannel;
class TelepathyHandler;


class TelepathyAccount : public QObject {
Q_OBJECT

public:
    explicit TelepathyAccount(Tp::AccountPtr macc);
    ~TelepathyAccount() override;

    QString nickname() const { return m_nickname; }
    QString protocolName() const { return m_protocol_name; }

signals:
    void databaseAddition(const QSharedPointer<ChatMessage> &msg);

public slots:
    void sendMessage(const QString &remote_uid, const QString &message);

    /* TODO: make these private again and use connect/emit */
    void onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);
    void onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel);

    bool log_event(time_t epoch, const QString &text, bool outgoing, const Tp::TextChannelPtr &channel, const QString &remote_uid, const QString &remote_alias);

private slots:
    void onOnline(bool online);
    void onAccReady(Tp::PendingOperation *op);

    // TODO return value
    void joinChannel(const QString &channel);
    // sendChannelMessage (if we cannot just use sendMessage)

public:
    Tp::AccountPtr acc;

    QList<TelepathyChannel*> channels;
private:
    QString m_nickname;
    QString m_backend_name;
    QString m_protocol_name;

    Tp::SimpleTextObserverPtr textobserver;
    Tp::SimpleObserverPtr channelobserver;
    Tp::ContactMessengerPtr messenger;
    Tp::TextChannel *hgbchan;
    Tp::AccountManagerPtr m_accountmanager;
};

class TelepathyChannel : public QObject {
Q_OBJECT

public:
    explicit TelepathyChannel(Tp::ChannelPtr mchannel, TelepathyAccount* macc);
    ~TelepathyChannel() override;

public slots:
    void sendMessage(const QString &message);

private slots:
    void onGroupAddContacts(Tp::PendingOperation *op);
    void onChannelReady(Tp::PendingOperation *op);

    void onChanMessageReceived(const Tp::ReceivedMessage &message);
    void onChanPendingMessageRemoved(const Tp::ReceivedMessage &message);
    void onChanMessageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &);
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

signals:
    void databaseAddition(const QSharedPointer<ChatMessage> &msg);
    void accountManagerReady();

public slots:
    void sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message);
    void onDatabaseAddition(const QSharedPointer<ChatMessage> &msg);

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



#endif // CONVTP_H
