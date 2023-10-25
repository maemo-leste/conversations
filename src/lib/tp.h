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
#include <TelepathyQt/SimpleTextObserver>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingHandles>
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
#include <TelepathyQt/TextChannel>
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
};


class TelepathyAccount : public QObject {
Q_OBJECT

public:
    explicit TelepathyAccount(Tp::AccountPtr macc);
    ~TelepathyAccount() override;

    QString nickname() const { return m_nickname; }
    QString localUid() const { return m_local_uid; }
    QString protocolName() const { return m_protocol_name; }

signals:
    void databaseAddition(const QSharedPointer<ChatMessage> &msg);

public slots:
    void sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message);

private slots:
    void onOnline(bool online);
    void onAccReady(Tp::PendingOperation *op);

    void onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);
    void onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel);

public:
    Tp::AccountPtr acc;

private:
    QString m_nickname;
    QString m_local_uid;
    QString m_protocol_name;

    Tp::SimpleTextObserverPtr observer;
    Tp::ContactMessengerPtr messenger;
    Tp::TextChannel *hgbchan;
    Tp::AccountManagerPtr m_accountmanager;

    Tp::AbstractClientPtr clienthandler;
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


#endif // CONVTP_H
