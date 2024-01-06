#include "lib/abook.h"

#include <QCoreApplication>
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
 * - Figure out group_uid for SMS
 * - Message sending doesn't contain error codes
 * - Report whether message was delivered or not
 *
 * Soon(tm):
 * - Log channel joins/parts
 * - In the account manager, keep track of when accounts are *added* or *deleted*
 * - Deal with message history, if we can fetch it (say XMPP)
 * - Deal with self-messages sent by us from another client (see if we can get
 *   them for say XMPP)
 * - Deal with message read delivery reports
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
    auto validaccounts = m_accountmanager->validAccounts();
    auto l = validaccounts->accounts();

    Tp::AccountPtr acc;

    for (int i = 0; i < l.count(); i++) {
        onNewAccount(l[i]);
    }

    connect(m_accountmanager.data(), &Tp::AccountManager::newAccount, this, &Telepathy::onNewAccount);

    emit accountManagerReady();
}

void Telepathy::onDatabaseAddition(const QSharedPointer<ChatMessage> &msg) {
    emit databaseAddition(msg);
}

void Telepathy::onOpenChannelWindow(const QString& local_uid, const QString &remote_uid, const QString &group_uid, const QString& service, const QString& channel) {
    emit openChannelWindow(local_uid, remote_uid, group_uid, service, channel);
}

void Telepathy::onNewAccount(const Tp::AccountPtr &account) {
    qDebug() << "adding account" << account;
    auto myacc = new TelepathyAccount(account);
    accounts << myacc;

    /* Connect this account signal to our general TP instance */
    connect(myacc, &TelepathyAccount::databaseAddition, this, &Telepathy::onDatabaseAddition);
    connect(myacc, &TelepathyAccount::openChannelWindow, this, &Telepathy::onOpenChannelWindow);
    connect(myacc, &TelepathyAccount::removed, this, &Telepathy::onAccountRemoved);
}

void Telepathy::onAccountRemoved(TelepathyAccount* account) {
    qDebug() << "onAccountRemoved" << account;

    accounts.removeOne(account);
    delete account;
}

/* Convenience function to send a message to a contact, the local_uid specifies
 * what accounts to send from, the remote_uid specifies whom to send to - either
 * a person or a channel identifier */
void Telepathy::sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
    /* Find our account given the local_uid and then send the message */
    for (TelepathyAccount *ma : accounts) {
        auto acc = ma->acc;
        QByteArray backend_name = ma->getLocalUid().toLocal8Bit();

        if (backend_name == local_uid) {
            ma->sendMessage(remote_uid, message);
        }
    }
}

Telepathy::~Telepathy() = default;

TelepathyHandler::TelepathyHandler(const Tp::ChannelClassSpecList &channelFilter)
    : Tp::AbstractClientHandler(channelFilter) {
    // XXX: Do we want to do anything here?

}

void TelepathyHandler::setTelepathyParent(Telepathy* parent) {
    m_telepathy_parent = parent;
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
        /* TODO: Make this code just use the sharedptrs, I somehow got stuck at
         * making a nullptr shareptr and got frustrated, but in the end we cast it
         * back so it's stupid not just use the sharedptrs */
        Tp::ChannelRequest *matching_requestptr = nullptr;
        Tp::TextChannel *matching_channel = nullptr;
        TelepathyAccount *matching_account = nullptr;

        /* We don't need to check the account, I think we get handlechannels
         * for a specific account */
        for (Tp::ChannelRequestPtr requestptr: requestsSatisfied) {
            QString target_uid;
            for (Tp::QualifiedPropertyValueMap valuemap: requestptr->requests()) {
                if (valuemap.contains("org.freedesktop.Telepathy.Channel.TargetID")) {
                    auto target_variant = valuemap["org.freedesktop.Telepathy.Channel.TargetID"].variant();
                    // TODO: Check target_variant.userType?
                    target_uid = target_variant.toString();
                    break;
                }
            }

            if (target_uid == channelptr->targetId()) {
                matching_requestptr = (Tp::ChannelRequest*)requestptr.data();
                break;
            }
        }

        for (TelepathyAccount *ma : m_telepathy_parent->accounts) {
            if (ma->acc != account)
                continue;

            matching_account = ma;
            matching_channel = (Tp::TextChannel*)ma->hasChannel(channelptr->targetId());
            qDebug() << "handleChannels: channel exists already?" << matching_channel;

            if (matching_channel == nullptr) {
                matching_channel = (Tp::TextChannel*)channelptr.data();

                auto mychan = new TelepathyChannel(channelptr, ma);
                ma->channels << mychan;
            }

            break;
        }

        /* Update channel request ptr with channel */
        if (matching_channel) {
            if (matching_requestptr) {
                matching_requestptr->succeeded((Tp::ChannelPtr)matching_channel);
                matching_account->TpOpenChannelWindow(Tp::TextChannelPtr(matching_channel));
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

    connect(acc.data(), &Tp::Account::removed, this, &TelepathyAccount::onRemoved);
    connect(acc.data(), &Tp::Account::onlinenessChanged, this, &TelepathyAccount::onOnline);
    connect(acc->becomeReady(), &Tp::PendingReady::finished, this, &TelepathyAccount::onAccReady);

    m_nickname = acc->nickname();
    m_backend_name = getLocalUid();
    m_protocol_name = acc->protocolName();
}

void TelepathyAccount::TpOpenChannelWindow(Tp::TextChannelPtr channel) {
    auto local_uid = m_backend_name;
    auto remote_uid = getRemoteUid(Tp::TextChannelPtr(channel));
    auto group_uid = getGroupUid(Tp::TextChannelPtr(channel));
    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    QString channelstr;

    if (channel->targetHandleType() != Tp::HandleTypeContact) {
        channelstr = channel->targetId();
    }

    emit openChannelWindow(local_uid, remote_uid, group_uid, service, channelstr);
}

QString TelepathyAccount::getGroupUid(Tp::TextChannelPtr channel) {
    /* TODO: How does this work for SMS? Need to test/think about this,
     * Fremantle uses just a number for SMS it looks like, or sometimes a name?
     * Let's double check. It seems to be the last 6-7 digits from a phone
     * number (uhhh) */
    return acc->objectPath().replace("/org/freedesktop/Telepathy/Account/", "") + "-" + channel->targetId();
}

QString TelepathyAccount::getRemoteUid(Tp::TextChannelPtr channel) {
    if (channel->targetHandleType() == Tp::HandleTypeContact) {
        return channel->targetId();
    } else {
        return channel->groupSelfContact()->id();
    }
}

QString TelepathyAccount::getLocalUid() {
    return acc->objectPath().replace("/org/freedesktop/Telepathy/Account/", "");
}


bool TelepathyAccount::log_event(time_t epoch, const QString &text, bool outgoing, const Tp::TextChannelPtr &channel, const QString &remote_uid, const QString &remote_alias) {
    char* channel_str = nullptr;
    QString channel_qstr = QString();
    QByteArray channel_ba = channel->targetId().toLocal8Bit();
    if (channel->targetHandleType() == Tp::HandleTypeContact) {
    } else {
        channel_str = channel_ba.data();
        channel_qstr = channel->targetId();
    }

    QByteArray group_uid = getGroupUid(channel).toLocal8Bit();

    const char* remote_name = nullptr;
    const char* abook_uid = nullptr;
    OssoABookContact* contact = nullptr;

    if (m_protocol_name == "tel") {
        qDebug() << "conv_abook_lookup_tel";
        contact = conv_abook_lookup_tel(remote_uid.toLocal8Bit());
        if (contact) {
            remote_name = osso_abook_contact_get_display_name(contact);
            abook_uid = osso_abook_contact_get_uid(contact);
        }
    } else if (m_protocol_name == "sip") {
        qDebug() << "conv_abook_lookup_sip";
        contact = conv_abook_lookup_sip(remote_uid.toLocal8Bit());
        if (contact) {
            remote_name = osso_abook_contact_get_display_name(contact);
            abook_uid = osso_abook_contact_get_uid(contact);
        }
    } else {
        qDebug() << "conv_abook_lookup_im";
        contact = conv_abook_lookup_im(remote_uid.toLocal8Bit());
        if (contact) {
            remote_name = osso_abook_contact_get_display_name(contact);
            abook_uid = osso_abook_contact_get_uid(contact);
        }
    }

    if (!remote_name && (remote_alias != nullptr)) {
        remote_name = remote_alias.toLocal8Bit();
    }

    create_event(epoch, epoch, m_nickname.toLocal8Bit().data(),
                 m_backend_name.toLocal8Bit().data(),
                 remote_uid.toLocal8Bit(),
                 remote_name, abook_uid,
                 text.toLocal8Bit(), outgoing, m_protocol_name.toStdString().c_str(),
                 channel_str, group_uid,
                 0 /* TODO: flags */);

    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    auto *msg = new ChatMessage(1, /* TODO: event id is wrong here but should not matter */
            service, group_uid,
            m_backend_name, remote_uid, QString(remote_name),
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
    qDebug() << "isScrollback" << message.isScrollback();
    qDebug() << "isRescued" << message.isRescued();

    if (message.isDeliveryReport()) {
        // TODO: We do not want to reply to it not write anything for now
        // Later we want to update the rtcom db with the delivery report
        return;
    }

    auto epoch = message.received().toTime_t();
    auto remote_uid = message.sender()->id();
    auto remote_alias = message.sender()->alias();
    auto text = message.text().toLocal8Bit();

    qDebug() << "log_event";
    log_event(epoch, text, false, channel, remote_uid, remote_alias);
}

/* When we have managed to send a message */
void TelepathyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageSent" << message.text();

    auto epoch = message.sent().toTime_t();
    QString remote_uid = getRemoteUid(channel);
    auto text = message.text().toLocal8Bit();

    qDebug() << "log_event";
    log_event(epoch, text, true, channel, remote_uid, nullptr);
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
        //joinChannel("#merlijn-test-channel");
        //joinChannel("#maemo-leste");
    }
#endif

}

Tp::TextChannel* TelepathyAccount::hasChannel(const QString &remote_uid) {
    foreach (TelepathyChannel* channel, channels) {
        auto *a_channel = (Tp::TextChannel*) channel->m_channel.data();
        if (remote_uid == a_channel->targetId()) {
            return a_channel;
        }
    }

    return nullptr;
}

void TelepathyAccount::sendMessage(const QString &remote_uid, const QString &message) {
    qDebug() << "sendMessage: remote_uid:" << remote_uid;
    Tp::TextChannel* channel = hasChannel(remote_uid);
    if (channel) {
        channel->send(message);
        return;
    }

    auto *pending = acc->ensureTextChat(remote_uid);

    connect(pending, &Tp::PendingChannelRequest::finished, [=](Tp::PendingOperation *op){
            auto *_pending = (Tp::PendingChannelRequest*)op;
            auto chanrequest = _pending->channelRequest();
            auto channel = chanrequest->channel();
            auto text_channel = (Tp::TextChannel*)channel.data();
            text_channel->send(message);
    });
}

void TelepathyAccount::onRemoved() {
    emit removed(this);
}

TelepathyAccount::~TelepathyAccount() {
    foreach (TelepathyChannel* channel, channels) {
        /* Let's assume telepathy will clean up it's own channels so we don't
         * work on the m_channel variable in this class */
        delete channel;
    }
}

TelepathyChannel::TelepathyChannel(const Tp::ChannelPtr &mchannel, TelepathyAccount* macc) : QObject(nullptr) {
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

    Tp::TextChannel *channel = (Tp::TextChannel*)m_channel.data();

    if (channel->targetHandleType() == Tp::HandleTypeContact) {
        connect(channel, &Tp::TextChannel::messageReceived, this, &TelepathyChannel::onChanMessageReceived);
        connect(channel, &Tp::TextChannel::pendingMessageRemoved, this, &TelepathyChannel::onChanPendingMessageRemoved);
        connect(channel, &Tp::TextChannel::messageSent, this, &TelepathyChannel::onChanMessageSent);

        /* There might be pending messages, we should probably do this also for
         * group channels, but maybe after their 'contacts' are added .*/
        for (const Tp::ReceivedMessage &msg: channel->messageQueue()) {
            onChanMessageReceived(msg);
        }
    } else {
        auto pending = channel->groupAddContacts(QList<Tp::ContactPtr>() << channel->connection()->selfContact());
        connect(pending, &Tp::PendingOperation::finished, this, &TelepathyChannel::onGroupAddContacts);
    }
}

/* Once this is done we have a notion of contacts */
void TelepathyChannel::onGroupAddContacts(Tp::PendingOperation *op) {
    qDebug() << "onGroupAddContacts, isError:" << op->isError();
    if (op->isError()) {
        qDebug() << "onGroupAddContacts, errorName:" << op->errorName();
        qDebug() << "onGroupAddContacts, errorMessage:" << op->errorMessage();
    }

    auto *channel = (Tp::TextChannel*) m_channel.data();

    connect(channel, &Tp::TextChannel::messageReceived, this, &TelepathyChannel::onChanMessageReceived);
    connect(channel, &Tp::TextChannel::pendingMessageRemoved, this, &TelepathyChannel::onChanPendingMessageRemoved);
    connect(channel, &Tp::TextChannel::messageSent, this, &TelepathyChannel::onChanMessageSent);

    for (const Tp::ReceivedMessage &msg: channel->messageQueue()) {
        onChanMessageReceived(msg);
    }
}

/* Called when we have received a message on the specific channel
 * Currently this does the logging
 */
void TelepathyChannel::onChanMessageReceived(const Tp::ReceivedMessage &message) {
    qDebug() << "onChanMessageReceived" << message.received(); // << message.sender()->id() << message.text();
    qDebug() << "channel targetID:" << m_channel->targetId();
    auto *channel = (Tp::TextChannel*) m_channel.data();

    auto messages = QList<Tp::ReceivedMessage>() << message;
    channel->acknowledge(messages);

    m_account->onMessageReceived(message, (Tp::TextChannelPtr)channel);
}

/* This fires when a message is removed from the messageQueue, but that would
 * typically be us, so I don't think we need to do anything here */
void TelepathyChannel::onChanPendingMessageRemoved(const Tp::ReceivedMessage &message) {
}

/* When we have sent a message on a channel */
void TelepathyChannel::onChanMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken) const {
    qDebug() << "onChanMessageSent";

    auto *channel = (Tp::TextChannel*) m_channel.data();
    m_account->onMessageSent(message, flags, sentMessageToken, (Tp::TextChannelPtr)channel);
}

/* If we already have a channel, send is easy */
void TelepathyChannel::sendMessage(const QString &message) const {
    auto *channel = (Tp::TextChannel*) m_channel.data();
    channel->send(message);
}

TelepathyChannel::~TelepathyChannel() = default;
