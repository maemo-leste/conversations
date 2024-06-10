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
    emit openChannelWindow(local_uid, remote_uid, group_uid, channel, service);
}

void Telepathy::onNewAccount(const Tp::AccountPtr &account) {
    auto myacc = new TelepathyAccount(account, this);
    qDebug() << "onNewAccount" << myacc->getLocalUid();
    accounts << myacc;

    /* Connect this account signal to our general TP instance */
    connect(myacc, &TelepathyAccount::databaseAddition, this, &Telepathy::onDatabaseAddition);
    connect(myacc, &TelepathyAccount::openChannelWindow, this, &Telepathy::onOpenChannelWindow);
    connect(myacc, &TelepathyAccount::channelJoined, this, &Telepathy::channelJoined);
    connect(myacc, &TelepathyAccount::channelLeft, this, &Telepathy::channelLeft);
    connect(myacc, &TelepathyAccount::removed, this, &Telepathy::onAccountRemoved);

    emit accountAdded(myacc);
}

void Telepathy::onAccountRemoved(TelepathyAccount* account) {
    qDebug() << "onAccountRemoved" << account;

    accounts.removeOne(account);
    delete account;
}

AccountChannelPtr Telepathy::channelByName(const QString &backend_name, const QString &remote_id) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return nullptr;
    if(!account->channels.contains(remote_id))
      return nullptr;
    return account->channels[remote_id];
}

TelepathyAccount* Telepathy::accountByName(const QString &backend_name) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return nullptr;
    return account;
}

void Telepathy::joinChannel(const QString &backend_name, const QString &remote_id, bool persistent) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return;
    account->joinChannel(remote_id, persistent);
}

void Telepathy::leaveChannel(const QString &backend_name, const QString &remote_id) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return;
    account->leaveChannel(remote_id);
}

void Telepathy::deleteChannel(const QString &backend_name, const QString &remote_id) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return;
    account->leaveChannel(remote_id);
    account->channels.remove(remote_id);

    TelepathyAccount::configRemove(backend_name, remote_id);
    emit channelDeleted(backend_name, remote_id);
}

bool Telepathy::participantOfChannel(const QString &backend_name, const QString &remote_id) {
    auto account = rtcomLocalUidToAccount(backend_name);

    if(account == nullptr)
        return false;
    else if(!account->channels.contains(remote_id))
        return false;

    return account->channels[remote_id]->hasActiveChannel();
}

void Telepathy::sendMessage(const QString &backend_name, const QString &remote_id, const QString &message) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return;
    account->sendMessage(remote_id, message);
}

void Telepathy::setChatState(const QString &backend_name, const QString &remote_id, Tp::ChannelChatState state)
{
  auto account = rtcomLocalUidToAccount(backend_name);
  if(account == nullptr)
      return;
  account->setChatState(remote_id, state);
}

TelepathyAccount* Telepathy::rtcomLocalUidToAccount(const QString &backend_name) {
    /* Find account given the backend_name */
    for (TelepathyAccount *account: accounts) {
        if(account->name == backend_name)
            return account;
    }
    qWarning() << "could not find associated active tp account by backend_name" << backend_name;
    return nullptr;
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
        Tp::ChannelPtr matching_channel;
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
                matching_requestptr = (Tp::ChannelRequest *)requestptr.data();
                break;
            }
        }

        for (TelepathyAccount *ma : m_telepathy_parent->accounts) {
            if (ma->acc != account)
                continue;

            matching_account = ma;
            matching_channel = ma->hasChannel(channelptr->targetId());

            if (!matching_channel) {
                matching_channel = channelptr;

                auto mychan = new TelepathyChannel(channelptr, ma);
                auto channel_name = mychan->m_channel->targetId();

                if(!ma->channels.contains(channel_name)) {
                    qDebug() << "handleChannels(), new channel:" << channel_name;
                    ma->channels[channel_name] = AccountChannelPtr(new AccountChannel);
                }

                ma->channels[channel_name]->name = channel_name;
                ma->channels[channel_name]->tpChannel = mychan;
                emit ma->channelJoined(ma->getLocalUid(), channel_name);
            } else {
              qWarning() << "handleChannels: channel exists already" << matching_channel;
            }

            break;
        }

        /* Update channel request ptr with channel */
        if (matching_channel) {
            if (matching_requestptr) {
                matching_requestptr->succeeded(matching_channel);
                matching_account->TpOpenChannelWindow(Tp::TextChannelPtr::staticCast(matching_channel));
            }
        }
    }

    context->setFinished();
}


/* TP account class, maintains list of channels and will pass signals along
 * might log the messages to rtcom from here, unless we decide to do that in the
 * channel class */
TelepathyAccount::TelepathyAccount(Tp::AccountPtr macc, QObject *parent) : QObject(parent) {
    acc = macc;
    name = getLocalUid();  // backend_name, e.g: 'idle/irc/oftc_2dsander0'
    m_nickname = acc->nickname();
    m_protocol_name = acc->protocolName();
    m_parent = static_cast<Telepathy*>(parent);

    // read and set channel properties saved in user config
    configRead();

    connect(acc.data(), &Tp::Account::removed, this, &TelepathyAccount::onRemoved);
    connect(acc.data(), &Tp::Account::onlinenessChanged, this, &TelepathyAccount::onOnline);
    connect(acc->becomeReady(), &Tp::PendingReady::finished, this, &TelepathyAccount::onAccReady);
}

void TelepathyAccount::TpOpenChannelWindow(Tp::TextChannelPtr channel) {
    auto local_uid = name;
    auto remote_uid = getRemoteUid(Tp::TextChannelPtr(channel));
    auto group_uid = getGroupUid(Tp::TextChannelPtr(channel));
    auto service = getServiceName();
    QString channelstr;

    if (channel->targetHandleType() != Tp::HandleTypeContact) {
        channelstr = channel->targetId();
    }

    emit openChannelWindow(local_uid, remote_uid, group_uid, service, channelstr);
}

QString TelepathyAccount::getGroupUid(Tp::TextChannelPtr channel) {
    if (acc->cmName() == "ring") {
        /* Fremantle uses just the last 7 digits as a group ID and does not
         * include the account path. This is not great, because I think this can
         * cause collisions with different numbers that have the same last 7
         * digits, but Fremantle has been doing it for many years, so let's do
         * what Fremantle does for now. By doing what Fremantle does, we can
         * guarantee compatibility with existing Fremantle rtcom databases */
        return channel->targetId().right(7);
    } else {
        return acc->objectPath().replace("/org/freedesktop/Telepathy/Account/", "") + "-" + channel->targetId();
    }
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

    QString remote_name_q = QString(remote_name);
    if (!remote_name && (remote_alias != nullptr)) {
        remote_name_q = QString(remote_alias);
    }

    std::string remote_name_str;
    if(!remote_name) {
      remote_name_str = remote_alias.toStdString();
      remote_name = remote_name_str.c_str();
    }

    auto self_name_str = m_nickname.toStdString();
    auto self_name = self_name_str.c_str();
    auto protocol_str = m_protocol_name.toStdString();
    auto protocol = protocol_str.c_str();
    auto backend_name_str = name.toStdString();
    auto backend_name = backend_name_str.c_str();

    unsigned int event_id = qtrtcom::registerMessage(
        epoch, epoch, self_name, backend_name,
        remote_uid.toLocal8Bit(), remote_name, abook_uid,
        text.toLocal8Bit(), outgoing, protocol,
        channel_str, group_uid);
    if(event_id < 0) {
      qWarning() << "log_event insertion error";
      return FALSE;
    }

    // @TODO: duplicate code like in onJoinChannel, refactor
    auto service = getServiceName();
    auto event_type = Utils::protocolIsTelephone(protocol) ? "RTCOM_EL_EVENTTYPE_SMS_MESSAGE" : "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE";

    auto *chatMessage = new ChatMessage({
        .event_id = event_id,  /* TODO: event id is wrong here but should not matter? or does it? */
        .service = service,
        .group_uid = group_uid,
        .local_uid = backend_name,
        .remote_uid = remote_uid,
        .remote_name = remote_name_q,
        .remote_ebook_uid = "",
        .text = text,
        .icon_name = "",
        .timestamp = epoch,
        .count = 0,
        .group_title = "",
        .channel = channel_qstr,
        .event_type = event_type,
        .outgoing = outgoing,
        .is_read = false,
        .flags = 0
      });

    QSharedPointer<ChatMessage> ptr(chatMessage);
    emit databaseAddition(ptr);

    return TRUE;
}

/* Slot for when we have received a message */
void TelepathyAccount::onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel) {
    QDateTime dt = message.sent().isValid() ? message.sent() : message.received();
    bool isScrollback = message.isScrollback();
    bool isDeliveryReport = message.isDeliveryReport();

    qDebug() << "onMessageReceived" << dt << message.senderNickname() << message.text();
    qDebug() << "isDeliveryReport" << isDeliveryReport;
    qDebug() << "isScrollback" << isScrollback;
    qDebug() << "isRescued" << message.isRescued();

    if (isDeliveryReport) {
        // TODO: We do not want to reply to it not write anything for now
        // Later we want to update the rtcom db with the delivery report
        return;
    }

    auto groupSelfContact = channel->groupSelfContact();
    auto remote_uid = message.sender()->id();
    auto remote_alias = message.sender()->alias();
    auto text = message.text().toLocal8Bit();
    bool outgoing = isScrollback &&
                    (groupSelfContact->handle() == message.sender()->handle() ||
                     groupSelfContact->id() == remote_uid);

    qint64 epoch = dt.toMSecsSinceEpoch();
    const QString &channel_str = channel->targetId();

    if (channels.contains(channel_str) && epoch <= channels[channel_str]->date_last_message)
        return;

    channels[channel_str]->date_last_message = epoch;
    m_parent->configSave();

    log_event(dt.toTime_t(), text, outgoing, channel, remote_uid, remote_alias);
}

/* When we have managed to send a message */
void TelepathyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageSent" << message.text();

    auto epoch = message.sent().toTime_t();
    QString remote_uid = getRemoteUid(channel);
    auto text = message.text().toLocal8Bit();

    log_event(epoch, text, true, channel, remote_uid, nullptr);
}

void TelepathyAccount::onOnline(bool online) {
    /* We might want to present whether a chat is online or not (if it is not
     * online, we can't send messages? */
    qDebug() << "onOnline: " << online;
    if(online)
      this->joinSavedGroupChats();
}

void TelepathyAccount::joinChannel(const QString &remote_id, bool persistent) {
    if(!channels.contains(remote_id)) {
        qDebug() << "joinChannel(), new channel:" << remote_id;
        channels[remote_id] = AccountChannelPtr(new AccountChannel);
        channels[remote_id]->name = remote_id;
        channels[remote_id]->auto_join = persistent;
    } else {
        channels[remote_id]->auto_join = persistent;
    }

    m_parent->configSave();
    this->_joinChannel(remote_id);
}

void TelepathyAccount::_joinChannel(const QString &remote_id) {
    qDebug() << "_joinChannel" << remote_id;
    auto *pending = acc->ensureTextChatroom(remote_id);

    connect(pending, &Tp::PendingChannelRequest::channelRequestCreated, this, [this, remote_id](const Tp::ChannelRequestPtr &channelRequest) {
      if(channelRequest->isValid()) {
        // @TODO: for now, we do not register chat join/leave events
        //this->onChannelJoined(channelRequest, remote_id);
      }
      else {
        qWarning() << "_joinChannel failed for " << remote_id;
      }
    });
}


// register in rtcom
void TelepathyAccount::onChannelJoined(const Tp::ChannelRequestPtr &channelRequest, QString channel) {
    if(!channelRequest->isValid()) {  // @TODO: handle error
      return;
    }

    auto abook_uid = nullptr;  // @TODO: ?

    auto local_uid_str = name.toStdString();
    auto _local_uid = local_uid_str.c_str();

    auto remote_uid_str  = m_nickname.toStdString();
    auto _remote_uid = remote_uid_str.c_str();

    std::string channel_str = channel.toStdString();
    const char *_channel = channel_str.c_str();

    std::string protocol_str = m_protocol_name.toStdString();
    const char *_protocol = protocol_str.c_str();

    auto group_uid = QString("%1-%2").arg(name, channel);
    auto group_uid_str = group_uid.toStdString();
    auto _group_uid = group_uid_str.c_str();

    time_t now = QDateTime::currentDateTime().toTime_t();
    const char* remote_name = nullptr;

    qtrtcom::registerChatJoin(now, now, _remote_uid, _local_uid,
                         _remote_uid, remote_name, abook_uid,
                         "join", _protocol, _channel, _group_uid);

    // @TODO: duplicate code like in log_event, refactor
    auto service = getServiceName();
    auto text = QString("%1 joined the groupchat").arg(m_nickname);

    auto *chatMessage = new ChatMessage({
        .event_id = 1,  /* TODO: event id is wrong here but should not matter */
        .service = service,
        .group_uid = group_uid,
        .local_uid = name,
        .remote_uid = m_nickname,
        .remote_name = QString(remote_name),
        .remote_ebook_uid = "",
        .text = text,
        .icon_name = "",
        .timestamp = now,
        .count = 0,
        .group_title = "",
        .channel = channel,
        .event_type = "-1",
        .outgoing = false,
        .is_read = true,
        .flags = 0
      });

    QSharedPointer<ChatMessage> ptr(chatMessage);
    emit databaseAddition(ptr);
}

// register in rtcom
void TelepathyAccount::onChannelLeft(QString channel) {
    auto abook_uid = nullptr;  // @TODO: ?

    auto local_uid_str = name.toStdString();
    auto _local_uid = local_uid_str.c_str();

    auto remote_uid_str  = m_nickname.toStdString();
    auto _remote_uid = remote_uid_str.c_str();

    std::string channel_str = channel.toStdString();
    const char *_channel = channel_str.c_str();

    std::string protocol_str = m_protocol_name.toStdString();
    const char *_protocol = protocol_str.c_str();

    auto group_uid = QString("%1-%2").arg(name, channel);
    auto group_uid_str = group_uid.toStdString();
    auto _group_uid = group_uid_str.c_str();

    time_t now = QDateTime::currentDateTime().toTime_t();
    const char* remote_name = nullptr;

    qtrtcom::registerChatLeave(now, now, _remote_uid, _local_uid,
                              _remote_uid, remote_name, abook_uid,
                              "left", _protocol, _channel, _group_uid);

    // @TODO: duplicate code like in log_event, refactor
    auto service = getServiceName();
    auto text = QString("%1 has left the groupchat").arg(m_nickname);

    auto *chatMessage = new ChatMessage({
        .event_id = 1,  /* TODO: event id is wrong here but should not matter? or does it? */
        .service = service,
        .group_uid = group_uid,
        .local_uid = name,
        .remote_uid = m_nickname,
        .remote_name = QString(remote_name),
        .remote_ebook_uid = "",
        .text = text,
        .icon_name = "",
        .timestamp = now,
        .count = 0,
        .group_title = "",
        .channel = channel,
        .event_type = "-1",
        .outgoing = false,
        .is_read = true,
        .flags = 0
      });

    QSharedPointer<ChatMessage> ptr(chatMessage);
    emit databaseAddition(ptr);
}

void TelepathyAccount::onAccReady(Tp::PendingOperation *op) {
  this->joinSavedGroupChats();
}

Tp::ChannelPtr TelepathyAccount::hasChannel(const QString &remote_id) {
    Tp::ChannelPtr channel;

    if (channels.contains(remote_id)) {
        auto _channel = channels[remote_id];

        if (_channel->tpChannel) {
            auto a_channel = _channel->tpChannel->m_channel;

            if (remote_id == a_channel->targetId())
                channel = a_channel;
        }
    }

    return channel;
}

void TelepathyAccount::leaveChannel(const QString &channel) {
    auto leave_message = "";
    if(!channels.contains(channel))
        return;

    auto tpChannel = channels[channel]->tpChannel;
    if(tpChannel == nullptr)
        return;

    // request groupchat leave
    auto pending = ((Tp::TextChannel *)tpChannel->m_channel.data())->requestLeave(leave_message);
    connect(pending, &Tp::PendingOperation::finished, [this, channel](Tp::PendingOperation *op) {
        if(op->isError()) {
            // @TODO: do something useful
            qWarning() << "leaveChannel" << channel << op->errorMessage();
        }

        if(channels.contains(channel)) {
            // we keep `channels[channel]` (AccountChannelPtr) around; we use it for other purposes too, and can be re-used.
            channels[channel]->tpChannel = nullptr;
        }

        // @TODO: for now, we do not register chat join/leave events
        //this->onChannelLeft(channel);
        m_parent->configSave();   // user-config registration
        emit channelLeft(name, channel);
    });
}

// called by `TelepathyChannel::onInvalidated`
void TelepathyAccount::_removeChannel(TelepathyChannel *chanptr) {
    channels.remove(chanptr->m_channel->targetId());
    chanptr->deleteLater();
}

void TelepathyAccount::sendMessage(const QString &remote_id, const QString &message) {
    qDebug() << "sendMessage: remote_id:" << remote_id;
    Tp::TextChannelPtr channel = Tp::TextChannelPtr::staticCast(hasChannel(remote_id));
    if (channel) {
        channel->send(message);
        return;
    }

    auto *pending = acc->ensureTextChat(remote_id);

    connect(pending, &Tp::PendingChannelRequest::finished, [message](Tp::PendingOperation *op){
            auto *_pending = (Tp::PendingChannelRequest *)op;
            auto chanrequest = _pending->channelRequest();
            auto channel = chanrequest->channel();
            auto text_channel = (Tp::TextChannel *)channel.data();
            text_channel->send(message);
    });
}

// Ensure AccountChannelPtr* exists, even though it may not be associated with any
// TelepathyChannel* instance. We need it to represent e.g channels from the config.
void TelepathyAccount::ensureChannel(const QString &remote_id) {
  if(channels.contains(remote_id)) 
    return;
  channels[remote_id] = AccountChannelPtr(new AccountChannel);
  channels[remote_id]->name = remote_id;
}

void TelepathyAccount::setAutoJoin(const QString &remote_id, bool autoJoin) {
  if(!channels.contains(remote_id))
    this->ensureChannel(remote_id);

  channels[remote_id]->auto_join = autoJoin;
  qDebug() << "setAutoJoin channel:" << remote_id << "set to" << autoJoin;
  m_parent->configSave();
}

void TelepathyAccount::setChatState(const QString &remote_id, Tp::ChannelChatState state)
{
  qDebug() << "setChatState: remote_id:" << remote_id;
  Tp::TextChannelPtr channel = Tp::TextChannelPtr::staticCast(hasChannel(remote_id));

  if (channel && channel->hasChatStateInterface())
    channel->requestChatState(state);
}

void TelepathyAccount::onRemoved() {
    emit removed(this);
}

void TelepathyAccount::joinSavedGroupChats() {
    for(const auto &_channel: channels) {
        if(_channel->auto_join) {
            qDebug() << "account:" << name << "joining:" << _channel->name << "- from user config";
            this->_joinChannel(_channel->name);
        }
    }
}

QString TelepathyAccount::getServiceName() {
  return Utils::protocolToRTCOMServiceID(m_protocol_name);
}

TelepathyAccount::~TelepathyAccount() {
}

TelepathyChannel::TelepathyChannel(const Tp::ChannelPtr &mchannel, TelepathyAccount* macc) : QObject(nullptr) {
    m_account = macc;
    m_channel = mchannel;

    connect(m_channel->becomeReady(),
        SIGNAL(finished(Tp::PendingOperation*)),
        SLOT(onChannelReady(Tp::PendingOperation*)));

    connect(m_channel.data(), &Tp::DBusProxy::invalidated, [this]() {
        m_account->_removeChannel(this);
    });
}

void TelepathyChannel::onChannelReady(Tp::PendingOperation *op) {

    qDebug() << "onChannelReady, isError:" << op->isError();
    if (op->isError()) {
        qDebug() << "onChannelReady, errorName:" << op->errorName();
        qDebug() << "onChannelReady, errorMessage:" << op->errorMessage();
    }

    Tp::TextChannel *channel = (Tp::TextChannel *)m_channel.data();

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

    auto *channel = Tp::TextChannelPtr::staticCast(m_channel).data();

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
    auto channel = Tp::TextChannelPtr::staticCast(m_channel);

    auto messages = QList<Tp::ReceivedMessage>() << message;
    channel->acknowledge(messages);

    m_account->onMessageReceived(message, channel);
}

/* This fires when a message is removed from the messageQueue, but that would
 * typically be us, so I don't think we need to do anything here */
void TelepathyChannel::onChanPendingMessageRemoved(const Tp::ReceivedMessage &message) {
}

/* When we have sent a message on a channel */
void TelepathyChannel::onChanMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken) const {
    qDebug() << "onChanMessageSent";

    m_account->onMessageSent(message, flags, sentMessageToken,
                             Tp::TextChannelPtr::staticCast(m_channel));
}

/* If we already have a channel, send is easy */
void TelepathyChannel::sendMessage(const QString &message) const {
    Tp::TextChannelPtr::staticCast(m_channel)->send(message);
}

TelepathyChannel::~TelepathyChannel() = default;


void Telepathy::configSave() {
    qDebug() << "configSave()";
    auto obj = Utils::getUserGroupChatChannels();

    for(const auto &account: accounts) {
        QJsonObject obj_account;
        QJsonArray  obj_channels;

        for(const auto &channel: account->channels.keys()) {
            auto ac = account->channels[channel];
            QJsonObject obj_channel;
            obj_channel["name"] = ac->name;
            obj_channel["auto_join"] = ac->auto_join;
            // @TODO: unfortunately we need to use QString to represent a qint64, Qt6 has QJsonValue::toInteger()
            obj_channel["date_created"] = QString::number(ac->date_created);
            obj_channel["date_last_message"] = QString::number(ac->date_last_message);
            obj_channels << obj_channel;
        }

        obj_account["channels"] = obj_channels;
        obj[account->name] = obj_account;
    }

    auto dumps = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    config()->set(ConfigKeys::GroupChatChannels, dumps);
}

void TelepathyAccount::configRead() {
    qDebug() << "configRead()";
    auto obj = Utils::getUserGroupChatChannels();

    for(const auto &_name: obj.keys()) {
        if(_name != name)
            continue;

        auto obj_account = obj[_name].toObject();
        if(!obj_account.contains("channels"))
            return;

        auto account_channels = obj_account["channels"].toArray();
        for (const auto &chan: account_channels) {
            auto obj_channel = chan.toObject();
            auto channel = obj_channel["name"].toString();
            auto auto_join = obj_channel["auto_join"].toBool();

            qint64 date_created = 0;
            if(obj_channel.contains("date_created"))
                date_created = obj_channel["date_created"].toString().toLongLong();

            qint64 date_last_message = 0;
            if(obj_channel.contains("date_last_message"))
                date_last_message = obj_channel["date_last_message"].toString().toLongLong();

            if(!channels.contains(channel)) {
                qDebug() << "readGroupchatChannels(), new channel:" << channel << "date_created" << date_created;
                auto ac = AccountChannelPtr(new AccountChannel());
                ac->name = channel;
                ac->date_created = date_created;
                ac->date_last_message = date_last_message;
                ac->auto_join = auto_join;
                channels[channel] = ac;
            } else {
                (channels[channel])->auto_join = auto_join;
                channels[channel]->date_created = date_created;
                channels[channel]->date_last_message = date_last_message;
            }
        }
    }
}

void TelepathyAccount::configRemove(const QString &backend_name, const QString &remote_id) {
    qDebug() << "configRemove()" << backend_name << remote_id;
    bool found = false;
    auto obj = Utils::getUserGroupChatChannels();
    for(const auto &_name: obj.keys()) {
        if(_name != backend_name)
            continue;

        auto obj_account = obj[_name].toObject();
        if(!obj_account.contains("channels"))
            return;

        auto account_channels = obj_account["channels"].toArray();
        for (int i = 0; i != account_channels.size(); i += 1) {
            auto obj_channel = account_channels.at(i).toObject();
            auto chan_name = obj_channel["name"].toString();
            qDebug() << chan_name;
            if(chan_name == remote_id) {
                account_channels.removeAt(i);
                found = true;
                break;
            }
        }

        if(found) {
            obj_account["channels"] = account_channels;
            obj[_name] = obj_account;
            break;
        }
    }

    if(!found) {
        qDebug() << "failed to locate" << remote_id << "by" << backend_name;
        return;
    }

    auto dumps = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    config()->set(ConfigKeys::GroupChatChannels, dumps);
    qDebug() << "removed" << remote_id << "from config";
}
