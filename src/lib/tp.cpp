#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/tp.h"
#include "lib/utils.h"

/*
 * TODO:
 * Priority:
 * - On incoming messages, let's make sure we match them to an account using
 *   abook, see
 *   https://github.com/maemo-leste/osso-abook/blob/master/lib/osso-abook-aggregator.h#L140
 *   https://github.com/maemo-leste/osso-abook/blob/master/lib/osso-abook-aggregator.h#L145
 * - Clean up class signatures (public/private), slots/signal usage, etc
 *
 * Soon(tm):
 * - In the account manager, keep track of when accounts are *added* or
 *   *deleted*
 * - Deal with message history, if we can fetch it (say XMPP)
 * - Deal with self-messages sent by us from another client (see if we can get
 *   them for say XMPP)
 * - Deal with message read delivery reports
 * - Investigate if we have to acknowledge() or forget() incoming messages
 *
 *
 * Research:
 *
 * - https://telepathy.freedesktop.org/spec/Channel_Interface_SMS.html
 * - https://telepathy.freedesktop.org/spec/Channel_Type_Contact_Search.html
 */

Telepathy::Telepathy(QObject *parent) : QObject(parent) {
    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(
                QDBusConnection::sessionBus(),
                Tp::Features()
                    << Tp::Account::FeatureCore
                );

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
                QDBusConnection::sessionBus(),
                Tp::Features()
                    << Tp::Connection::FeatureCore
                    << Tp::Connection::FeatureSelfContact
                );

    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());

    channelFactory->addCommonFeatures(Tp::Channel::FeatureCore);

    channelFactory->addFeaturesForTextChats(
            Tp::Features()
            << Tp::TextChannel::FeatureMessageQueue
            << Tp::TextChannel::FeatureMessageSentSignal
            );
    channelFactory->addFeaturesForTextChatrooms(
            Tp::Features()
            << Tp::TextChannel::FeatureMessageQueue
            << Tp::TextChannel::FeatureMessageSentSignal
            );

    Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create(
                Tp::Features()
                    << Tp::Contact::FeatureAlias
                    << Tp::Contact::FeatureAvatarData
                );

    m_accountmanager = Tp::AccountManager::create(accountFactory);
    connect(m_accountmanager->becomeReady(), &Tp::PendingReady::finished, this, &Telepathy::onAccountManagerReady);

    registrar = Tp::ClientRegistrar::create(accountFactory,
                                            connectionFactory,
                                            channelFactory,
                                            contactFactory);

    auto tphandler = new TelepathyHandler(Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::textChat() << Tp::ChannelClassSpec::textChatroom());

    Tp::AbstractClientPtr handler = Tp::AbstractClientPtr::dynamicCast(
            Tp::SharedPtr<TelepathyHandler>(tphandler));

    tphandler->setTelepathyParent(this);

    registrar->registerClient(handler, "Conversations");
}

/* When the account manager is ready, we will get a list of our accounts and
 * store them in our accounts structure */
void Telepathy::onAccountManagerReady(Tp::PendingOperation *op) {
    qDebug() << "onAccountManagerReady";

    auto validaccounts = m_accountmanager->validAccounts();
    auto l = validaccounts->accounts();

    Tp::AccountPtr acc;

    for (int i = 0; i < l.count(); i++) {
        acc = l[i];
        auto myacc = new TelepathyAccount(acc);
        accounts << myacc;

        /* Connect this account signal to our general TP instance */
        connect(myacc, &TelepathyAccount::databaseAddition, this, &Telepathy::onDatabaseAddition);
    }

    emit accountManagerReady();
}

void Telepathy::onDatabaseAddition(const QSharedPointer<ChatMessage> &msg) {
    emit databaseAddition(msg);
}

/* Convenience function to send a message to a contact, the local_uid specifies
 * what accounts to send from, the remote_uid specifies whom to send to - either
 * a person or a channel identifier */
/* TODO: We might want to change the signature of this function */
void Telepathy::sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
    qDebug() << local_uid << remote_uid;

    for (TelepathyAccount *ma : accounts) {
        auto acc = ma->acc;

        /* TODO: Let's revisit this and see if we want to use the backend_name
           of this format */
        QByteArray backend_name = (acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName()).toLocal8Bit();

        if (backend_name == local_uid) {
            qDebug() << backend_name;
            ma->sendMessage(remote_uid, message);
        }
    }
}

Telepathy::~Telepathy()
{
}

TelepathyHandler::TelepathyHandler(const Tp::ChannelClassSpecList &channelFilter)
    : Tp::AbstractClientHandler(channelFilter) {
    // XXX: Do we want to do anything here?

}

void TelepathyHandler::setTelepathyParent(Telepathy* parent) {
    m_telepathy_parent = parent;
    return;
}

bool TelepathyHandler::bypassApproval() const {
    return false;
}

void TelepathyHandler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                                      const Tp::AccountPtr &account,
                                      const Tp::ConnectionPtr &connection,
                                      const QList<Tp::ChannelPtr> &channels,
                                      const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                                      const QDateTime &userActionTime,
                                      const Tp::AbstractClientHandler::HandlerInfo &handlerInfo) {
    qDebug() << "handleChannels";

    foreach (Tp::ChannelPtr channelptr, channels) {
        /* Do we have to check if we already have an existing channel
         * (I would think not, but maybe we do want to check for that) */
        for (TelepathyAccount *ma : m_telepathy_parent->accounts) {
            if (ma->acc == account) {
                auto mychan = new TelepathyChannel(channelptr, ma);
                ma->channels << mychan;
            }
        }
    }

    context->setFinished();
}



/* TP account class, maintains list of channels and will pass signals along
 * might log the messages to rtcom from here, unless we decide to do that in the
 * channel class */
TelepathyAccount::TelepathyAccount(Tp::AccountPtr macc) : QObject(nullptr) {
    acc = macc;

    connect(acc.data(),
            SIGNAL(onlinenessChanged(bool)),
            SLOT(onOnline(bool)));

    connect(acc->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccReady(Tp::PendingOperation*)));

    m_nickname = acc->nickname();
    m_backend_name = acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName();
    m_protocol_name = acc->protocolName();
}

bool TelepathyAccount::log_event(time_t epoch, const QString &text, bool outgoing, const Tp::TextChannelPtr &channel, const QString &remote_uid) {
    char* channel_str = NULL;
    QString channel_qstr = QString();
    QByteArray channel_ba = channel->targetId().toLocal8Bit();
    if (channel->targetHandleType() == Tp::HandleTypeContact) {
    } else {
        channel_str = channel_ba.data();
        channel_qstr = channel->targetId();
    }

    /* TODO: How does this work for SMS? Need to test/think about this,
     * Fremantle uses just a number for SMS it looks like, or sometimes a name?
     * Let's double check. It seems to be the last 6-7 digits from a phone
     * number (uhhh) */
    QByteArray group_uid = (acc->objectPath().replace("/org/freedesktop/Telepathy/Account/", "") + "-" + channel->targetId()).toLocal8Bit();

    /* TODO: remote_name != remote_uid, we shouldn't make them equal,
     * but let's do it for now
     * I think we need to do a lookup in osso_abook */
    create_event(epoch, epoch, m_nickname.toLocal8Bit().data(),
                 m_backend_name.toLocal8Bit().data(),
                 remote_uid.toLocal8Bit(), remote_uid.toLocal8Bit(), /* TODO: remote_name */
                 text.toLocal8Bit(), outgoing, m_protocol_name.toStdString().c_str(),
                 channel_str, group_uid,
                 0 /* TODO: flags */);

    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    auto *msg = new ChatMessage(1, service, group_uid,
            m_nickname, remote_uid, remote_uid,
            "" /* remote_abook_uid */,
            text, "" /* icon_name */,
            epoch, 0,
            /* group_title */ "",
            channel_qstr,
            "-1", outgoing, 0);

    QSharedPointer<ChatMessage> ptr(msg);
    emit databaseAddition(ptr);

    return TRUE;
}

/* Slot for when we have received a message */
void TelepathyAccount::onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageReceived" << message.received() << message.senderNickname() << message.text();
    qDebug() << "isDeliveryReport" << message.isDeliveryReport();

    if (message.isDeliveryReport()) {
        // TODO: We do not want to reply to it not write anything for now
        // Later we want to update the rtcom db with the delivery report
        return;
    }

    auto epoch = message.received().toTime_t();
    auto remote_uid = message.senderNickname();
    auto text = message.text().toLocal8Bit();

    qDebug() << "log_event";
    log_event(epoch, text, false, channel, remote_uid);
}

/* When we have managed to send a message */
void TelepathyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageSent" << message.text();

    auto epoch = message.sent().toTime_t();
    // TODO: what is the remote_uid if we send a message to a channel anyway?

    auto remote_uid = channel->targetId();
    auto text = message.text().toLocal8Bit();

    qDebug() << "log_event";
    log_event(epoch, text, true, channel, remote_uid);
}

void TelepathyAccount::onOnline(bool online) {
    /* We might want to present whether a chat is online or not (if it is not
     * online, we can't send messages? */
    qDebug() << "onOnline: " << online;
}

void TelepathyAccount::joinChannel(const QString &channel) {
    qDebug() << "joinChannel" << channel;
    auto *pending = acc->ensureTextChatroom(channel);
}

void TelepathyAccount::onAccReady(Tp::PendingOperation *op) {
    //qDebug() << "onAccReady, isError:" << op->isError() << "Display name:" << acc->displayName() << "(Online:" << acc->isOnline() << ", serviceName:" << acc->serviceName() << ", objectPath: " << acc->objectPath() << ", busName:" << acc->busName() << ")";

#if 0
    if (acc->isOnline()) {
        joinChannel("##maemotest");
        joinChannel("#maemo-leste");
    }
#endif

}

void TelepathyAccount::sendMessage(const QString &remote_uid, const QString &message) {
    qDebug() << "sendMessage: remote_uid:" << remote_uid;
    bool found = FALSE;
    /* Find existing channel, otherwise create one using ensureTextChat and the
     * lambda */
    foreach (TelepathyChannel* channel, channels) {
        Tp::TextChannel* a_channel = (Tp::TextChannel*) channel->m_channel.data();
        /* TODO: This has problems with case sensitivity */
        qDebug() << "Testing against:" << a_channel->targetId();
        if (remote_uid.toLower() == a_channel->targetId().toLower()) {
            qDebug() << "TelepathyAccount::sendMessage: Found matching channel";
            channel->sendMessage(message);
            found = TRUE;
            break;
        }
    }

    if (!found) {
        auto *pending = acc->ensureTextChat(remote_uid);

        connect(pending, &Tp::PendingChannelRequest::finished, [=](Tp::PendingOperation *op){
                auto *_pending = (Tp::PendingChannelRequest*)op;
                auto chanrequest = _pending->channelRequest();
                auto channel = chanrequest->channel();
                auto text_channel = (Tp::TextChannel*)channel.data();
                text_channel->send(message);
                });
    }
}

TelepathyAccount::~TelepathyAccount()
{
}


TelepathyChannel::TelepathyChannel(Tp::ChannelPtr mchannel, TelepathyAccount* macc) : QObject(nullptr) {
    m_account = macc;
    m_channel = mchannel;

    connect(m_channel->becomeReady(),
        SIGNAL(finished(Tp::PendingOperation*)),
        SLOT(onChannelReady(Tp::PendingOperation*)));
}

void TelepathyChannel::onChannelReady(Tp::PendingOperation *op) {

    qDebug() << "onChannelReady, isError:" << op->isError();
    if (op->isError()) {
        qDebug() << "onChannelReady, errorName:" << op->errorName();
        qDebug() << "onChannelReady, errorMessage:" << op->errorMessage();
    }

    Tp::TextChannel* channel = (Tp::TextChannel*) m_channel.data();
    qDebug() << "handle type" << channel->targetHandleType();

    if (channel->targetHandleType() == Tp::HandleTypeContact) {
        qDebug() << "handle type contact";
        Tp::TextChannel* channel = (Tp::TextChannel*) m_channel.data();
    
        connect(channel,
                SIGNAL(messageReceived(const Tp::ReceivedMessage&)),
                SLOT(onChanMessageReceived(const Tp::ReceivedMessage&)));
    
        connect(channel,
                SIGNAL(pendingMessageRemoved(const Tp::ReceivedMessage&)),
                SLOT(onChanPendingMessageRemoved(const Tp::ReceivedMessage&)));
    
        connect(channel,
                SIGNAL(messageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &)),
                SLOT(onChanMessageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &)));

        /* There might be pending messages, we should probably do this also for
         * group channels, but maybe after their 'contacts' are added .*/
        foreach (Tp::ReceivedMessage msg, channel->messageQueue()) {
            onChanMessageReceived(msg);
        }
    } else {
        auto pending = channel->groupAddContacts(QList<Tp::ContactPtr>() << channel->connection()->selfContact());
        connect(pending,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onGroupAddContacts(Tp::PendingOperation*)));
    }
}

/* Once this is done we have a notion of contacts */
void TelepathyChannel::onGroupAddContacts(Tp::PendingOperation *op) {
    qDebug() << "onGroupAddContacts, isError:" << op->isError();
    if (op->isError()) {
        qDebug() << "onGroupAddContacts, errorName:" << op->errorName();
        qDebug() << "onGroupAddContacts, errorMessage:" << op->errorMessage();
    }

    Tp::TextChannel* channel = (Tp::TextChannel*) m_channel.data();

    connect(channel,
            SIGNAL(messageReceived(const Tp::ReceivedMessage&)),
            SLOT(onChanMessageReceived(const Tp::ReceivedMessage&)));

    connect(channel,
            SIGNAL(pendingMessageRemoved(const Tp::ReceivedMessage&)),
            SLOT(onChanPendingMessageRemoved(const Tp::ReceivedMessage&)));

    connect(channel,
            SIGNAL(messageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &)),
            SLOT(onChanMessageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &)));

    /* TODO: DRY with code in TelepathyChannel::onChannelReady */
    /* There might be pending messages, we should probably do this also for
     * group channels, but maybe after their 'contacts' are added .*/
    foreach (Tp::ReceivedMessage msg, channel->messageQueue()) {
        onChanMessageReceived(msg);
    }
}

/* Called when we have received a message on the specific channel
 * Currently this does the logging
 */
void TelepathyChannel::onChanMessageReceived(const Tp::ReceivedMessage &message) {
    qDebug() << "onChanMessageReceived" << message.received() << message.senderNickname() << message.text();
    qDebug() << "channel targetID:" << m_channel->targetId();
    Tp::TextChannel* channel = (Tp::TextChannel*) m_channel.data();

    auto messages = QList<Tp::ReceivedMessage>() << message;
    channel->acknowledge(messages);

    m_account->onMessageReceived(message, (Tp::TextChannelPtr)channel);
    //emit m_account->databaseAddition(ptr);
}

/* This fires when a message is removed from the messageQueue, but that would
 * typically be us, so I don't think we need to do anything here */
void TelepathyChannel::onChanPendingMessageRemoved(const Tp::ReceivedMessage &message) {
    qDebug() << "onChanPendingMessageRemoved" << message.received() << message.senderNickname() << message.text();
}

/* When we have sent a message on a channel
 * TODO: Implement logging here */
void TelepathyChannel::onChanMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken) {
    qDebug() << "onChanMessageSent";

    Tp::TextChannel* channel = (Tp::TextChannel*) m_channel.data();
    m_account->onMessageSent(message, flags, sentMessageToken, (Tp::TextChannelPtr)channel);
    // TODO: Log our own message here instead of in the
    // TelepathyAccount::onMessageSent

    // TODO: emit signals
    //emit m_account->databaseAddition(ptr);
}

/* If we already have a channel, send is easy */
void TelepathyChannel::sendMessage(const QString &message) {
    Tp::TextChannel* channel = (Tp::TextChannel*) m_channel.data();
    channel->send(message);
}


TelepathyChannel::~TelepathyChannel()
{
}
