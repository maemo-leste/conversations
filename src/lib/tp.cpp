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
    auto *telepathyAccount = new TelepathyAccount(account, this);
    auto accountPtr = TelepathyAccountPtr(telepathyAccount);
    qDebug() << "onNewAccount" << accountPtr->local_uid;
    accounts << accountPtr;

    /* Connect this account signal to our general TP instance */
    connect(telepathyAccount, &TelepathyAccount::databaseAddition, this, &Telepathy::onDatabaseAddition);
    connect(telepathyAccount, &TelepathyAccount::openChannelWindow, this, &Telepathy::onOpenChannelWindow);
    connect(telepathyAccount, &TelepathyAccount::channelJoined, this, &Telepathy::channelJoined);
    connect(telepathyAccount, &TelepathyAccount::channelLeft, this, &Telepathy::channelLeft);
    connect(telepathyAccount, &TelepathyAccount::removed, [this](TelepathyAccount *ptr) {
      this->onAccountRemoved(ptr->local_uid);
    });

    emit accountAdded(accountPtr);
}

void Telepathy::onAccountRemoved(const QString &local_uid) {
    qDebug() << "onAccountRemoved" << local_uid;
    auto ptr = this->accountByName(local_uid);
    if(!ptr)
      throw std::runtime_error("debug me");

    accounts.removeOne(ptr);
    emit accountRemoved();
}

TelepathyChannelPtr Telepathy::channelByName(const QString &local_uid, const QString &remote_uid) {
    auto account = rtcomLocalUidToAccount(local_uid);
    if(!account || !account->channels.contains(remote_uid))
        return {};
    return account->channels[remote_uid];
}

TelepathyAccountPtr Telepathy::accountByName(const QString &local_uid) {
    auto account = rtcomLocalUidToAccount(local_uid);
    if(!account)
        return {};
    return account;
}

TelepathyAccountPtr Telepathy::accountByPtr(Tp::AccountPtr ptr) {
  for(const auto &accountPtr: accounts) {
      if(accountPtr->acc == ptr)
          return accountPtr;
  }
  return {};
}

void Telepathy::joinChannel(const QString &local_uid, const QString &remote_uid) {
    qDebug() << "Telepathy::joinChannel()" << local_uid << remote_uid;
    auto account = rtcomLocalUidToAccount(local_uid);
    if(!account)
        return;

    account->joinChannel(remote_uid);
}

void Telepathy::leaveChannel(const QString &local_uid, const QString &remote_uid) {
    auto account = rtcomLocalUidToAccount(local_uid);
    if(!account)
        return;

    account->leaveChannel(remote_uid);
}

void Telepathy::deleteChannel(const QString &local_uid, const QString &remote_uid) {
    auto account = rtcomLocalUidToAccount(local_uid);
    if(!account)
        return;
    account->leaveChannel(remote_uid);
    account->channels.remove(remote_uid);

    emit channelDeleted(local_uid, remote_uid);
}

void Telepathy::sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
    auto account = rtcomLocalUidToAccount(local_uid);
    if(!account)
        return;
    account->sendMessage(remote_uid, message);
}

void Telepathy::onSetAutoJoin(const QString &local_uid, const QString &remote_uid, bool auto_join) {
  qDebug() << "Telepathy::onSetAutoJoin()" << local_uid << remote_uid << auto_join;
  auto account = rtcomLocalUidToAccount(local_uid);
  if(!account)
      return;

  if(auto_join) {
    if(account->isOnline) {
      account->joinChannel(remote_uid);
    } else {
      qDebug() << "cannot auto-join" << remote_uid << "because" << local_uid << "is not online";
    }
  }
}

void Telepathy::setChatState(const QString &local_uid, const QString &remote_uid, Tp::ChannelChatState state) {
  auto account = rtcomLocalUidToAccount(local_uid);
  if(!account)
      return;
  account->setChatState(remote_uid, state);
}

TelepathyAccountPtr Telepathy::rtcomLocalUidToAccount(const QString &local_uid) {
    /* Find account given the local_uid */
    for (const auto &account: accounts) {
        if(account->local_uid == local_uid)
            return account;
    }

    qWarning() << "could not find associated active tp account by local_uid" << local_uid;
    return {};
}

Telepathy::~Telepathy() = default;

TelepathyHandler::TelepathyHandler(const Tp::ChannelClassSpecList &channelFilter)
    : Tp::AbstractClientHandler(channelFilter) {
    // XXX: Do we want to do anything here?

}

void TelepathyHandler::setTelepathyParent(Telepathy* parent) {
    m_tp = parent;
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
    bool isAutoJoin = !userActionTime.isValid();

    foreach (Tp::ChannelPtr channelPtr, channels) {
        QVariantMap props = channelPtr->immutableProperties();

        QString remote_uid = props.value(QString("%1.TargetID").arg(TP_QT_IFACE_CHANNEL)).toString();
        if(remote_uid.isEmpty()) {
            qWarning() << "handleChannels cannot get TargetID (remote_uid) for channel";
            continue;
        }

        QString room_name = props.value(QString("%1.Interface.Room2.RoomName").arg(TP_QT_IFACE_CHANNEL)).toString();

        // https://telepathy.freedesktop.org/spec/index.html#Channel-Types
        QString channelType = props.value(QString("%1.ChannelType").arg(TP_QT_IFACE_CHANNEL)).toString();
        if(!channelType.endsWith(".Text")) {
            qWarning() << "skipping unsupported channel type" << channelType;
            continue;
        }

        // https://telepathy.freedesktop.org/doc/telepathy-qt/a00879.html
        Tp::HandleType handleType = (Tp::HandleType) props.value(QString("%1.TargetHandleType").arg(TP_QT_IFACE_CHANNEL)).toInt();
        bool isRoom = handleType == Tp::HandleTypeRoom;

        // find associated TelepathyAccountPtr
        TelepathyAccountPtr accountPtr = m_tp->accountByPtr(account);
        if(!accountPtr)
            throw std::runtime_error("no matching TelepathyAccountPtr debug me");

        // create a TelepathyChannelPtr
        if(!accountPtr->hasChannel(remote_uid)) {
            auto tcPtr = TelepathyChannelPtr(new TelepathyChannel(remote_uid, accountPtr, channelPtr, handleType));

            tcPtr->isRoom = isRoom;
            if(!room_name.isEmpty()) {  // register room name
                auto remote_uid_str = remote_uid.toStdString();
                auto _remote_uid = remote_uid_str.c_str();

                auto room_name_str = room_name.toStdString();
                auto _room_name = room_name_str.c_str();

                qtrtcom::setRoomName(_remote_uid, _room_name);
                tcPtr->room_name = room_name;
            }

            accountPtr->channels[remote_uid] = tcPtr;

            // handle invalidated signal
            QObject::connect(channelPtr.data(), &Tp::DBusProxy::invalidated, [accountPtr, remote_uid] {
                accountPtr->removeChannel(remote_uid);
            });

            // announce we joined a channel (valid for both room, and 1:1 contact)
            emit accountPtr->channelJoined(accountPtr->local_uid, remote_uid);
        }

        if(!isAutoJoin)  // channel joined from 'external' Tp client (e.g addresbook), request chatWindow
          accountPtr->TpOpenChannelWindow(Tp::TextChannelPtr::staticCast(channelPtr));
    }

    context->setFinished();
}

/* TP account class, maintains list of channels and will pass signals along
 * might log the messages to rtcom from here, unless we decide to do that in the
 * channel class */
TelepathyAccount::TelepathyAccount(Tp::AccountPtr macc, QObject *parent) :
        acc(macc),
        local_uid(getLocalUid()),  // e.g: 'idle/irc/oftc_2dsander0'
        QObject(parent) {
    m_nickname = acc->nickname();
    m_protocol_name = acc->protocolName();
    m_parent = static_cast<Telepathy*>(parent);

    connect(acc.data(), &Tp::Account::removed, this, &TelepathyAccount::onRemoved);
    connect(acc.data(), &Tp::Account::onlinenessChanged, this, &TelepathyAccount::onOnline);
    connect(acc->becomeReady(), &Tp::PendingReady::finished, this, &TelepathyAccount::onAccReady);
}

void TelepathyAccount::TpOpenChannelWindow(Tp::TextChannelPtr channel) {
    auto remote_uid = getRemoteUid(channel);
    auto group_uid = getGroupUid(channel);
    auto service = getServiceName();
    QString channelstr;

    if (channel->targetHandleType() != Tp::HandleTypeContact) {
        channelstr = channel->targetId();
    }

    emit openChannelWindow(local_uid, remote_uid, group_uid, service, channelstr);
}

QString TelepathyAccount::getGroupUid(TelepathyChannelPtr channel) {
    auto text_channel = Tp::TextChannelPtr::staticCast(channel->m_channel);
    return getGroupUid(text_channel);
}

QString TelepathyAccount::getRoomName(TelepathyChannelPtr channel) {
    for(const auto &c: channels) {
        if(c == channel) {
            return c->room_name;
        }
    }
    return {};
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
        return QString(acc->objectPath()).replace("/org/freedesktop/Telepathy/Account/", "") + "-" + channel->targetId();
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
    return QString(acc->objectPath()).replace("/org/freedesktop/Telepathy/Account/", "");
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
    auto local_uid_str = local_uid.toStdString();
    auto local_uid = local_uid_str.c_str();

    unsigned int event_id = qtrtcom::registerMessage(
        epoch, epoch, self_name, local_uid,
        remote_uid.toLocal8Bit(), remote_name, abook_uid,
        text.toLocal8Bit(), outgoing, protocol,
        channel_str, group_uid);
    if(event_id < 0) {
      qWarning() << "log_event insertion error";
      return FALSE;
    }

    auto service = getServiceName();
    auto event_type = Utils::protocolIsTelephone(protocol) ? "RTCOM_EL_EVENTTYPE_SMS_MESSAGE" : "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE";

    auto *chatMessage = new ChatMessage({
        .event_id = (int) event_id,  /* TODO: event id is wrong here but should not matter? or does it? */
        .service = service,
        .group_uid = group_uid,
        .local_uid = local_uid,
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
    qint64 epoch = dt.toMSecsSinceEpoch();
    bool isScrollback = message.isScrollback();
    bool isDeliveryReport = message.isDeliveryReport();

    qDebug() << "onMessageReceived" << dt << channel->targetId() << message.senderNickname() << message.text();
    // qDebug() << "isDeliveryReport" << isDeliveryReport;
    // qDebug() << "isScrollback" << isScrollback;
    // qDebug() << "channel->targetId()" << channel->targetId();
    // qDebug() << "isRescued" << message.isRescued();

    if (isDeliveryReport) {
        // TODO: We do not want to reply to it not write anything for now
        // Later we want to update the rtcom db with the delivery report
        return;
    }

    auto groupSelfContact = channel->groupSelfContact();
    auto remote_uid = message.sender()->id();

    auto remote_alias = message.sender()->alias();
    auto text = message.text().toLocal8Bit();
    bool outgoing = groupSelfContact->handle() == message.sender()->handle() ||
                    groupSelfContact->id() == remote_uid;

    if(outgoing) {
        remote_uid = channel->targetId();
        remote_alias = nullptr;
    }

    // only insert newer messages, exit-early if neccesary
    ConfigStateItemPtr configItem = configState->getItem(local_uid, channel->targetId());
    if(configItem && epoch <= configItem->date_last_message) {
        qDebug() << "dropping old message" << channel->targetId() << ":" << text;
        return;
    }

    configState->setLastMessageTimestamp(local_uid, channel->targetId(), epoch);
    log_event(dt.toTime_t(), text, outgoing, channel, remote_uid, remote_alias);
}

/* When we have managed to send a message */
void TelepathyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageSent" << message.text();

    time_t epoch = message.sent().toTime_t();
    QString remote_uid = getRemoteUid(channel);
    auto text = message.text().toLocal8Bit();

    log_event(epoch, text, true, channel, remote_uid, nullptr);
}

void TelepathyAccount::onOnline(bool online) {
    qDebug() << "onOnline: " << online;
    isOnline = online;

    if(online)
        this->joinSavedGroupChats();
}

void TelepathyAccount::joinChannel(const QString &remote_uid) {
    this->_joinChannel(remote_uid);
}

void TelepathyAccount::_joinChannel(const QString &remote_uid, bool auto_join) {
    qDebug() << "_joinChannel" << remote_uid;

    // set a 'null' datetime on auto_join
    QDateTime dt = auto_join ? QDateTime() : QDateTime::currentDateTime();
    auto *pending = acc->ensureTextChatroom(remote_uid, dt);

    connect(pending, &Tp::PendingChannelRequest::channelRequestCreated, this, [this, remote_uid](const Tp::ChannelRequestPtr &channelRequest) {
      if(channelRequest->isValid()) {
        // @TODO: for now, we do not register chat join/leave events
        //this->onChannelJoined(channelRequest, remote_uid);
      }
      else {
        qWarning() << "_joinChannel failed for " << remote_uid;
      }
    });
}


// register in rtcom
void TelepathyAccount::onChannelJoined(const Tp::ChannelRequestPtr &channelRequest, QString channel) {
    if(!channelRequest->isValid()) {  // @TODO: handle error
      return;
    }

    auto abook_uid = nullptr;  // @TODO: ?

    auto local_uid_str = local_uid.toStdString();
    auto _local_uid = local_uid_str.c_str();

    auto remote_uid_str  = m_nickname.toStdString();
    auto _remote_uid = remote_uid_str.c_str();

    std::string channel_str = channel.toStdString();
    const char *_channel = channel_str.c_str();

    std::string protocol_str = m_protocol_name.toStdString();
    const char *_protocol = protocol_str.c_str();

    auto group_uid = QString("%1-%2").arg(local_uid, channel);
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
        .local_uid = local_uid,
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

    auto local_uid_str = local_uid.toStdString();
    auto _local_uid = local_uid_str.c_str();

    auto remote_uid_str  = m_nickname.toStdString();
    auto _remote_uid = remote_uid_str.c_str();

    std::string channel_str = channel.toStdString();
    const char *_channel = channel_str.c_str();

    std::string protocol_str = m_protocol_name.toStdString();
    const char *_protocol = protocol_str.c_str();

    auto group_uid = QString("%1-%2").arg(local_uid, channel);
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
        .event_id = 1,
        .service = service,
        .group_uid = group_uid,
        .local_uid = local_uid,
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
  emit accountReady(this);
}

TelepathyChannelPtr TelepathyAccount::hasChannel(const QString &remote_uid) {
    TelepathyChannelPtr channel;

    if(channels.contains(remote_uid) &&
          channels[remote_uid] &&
          channels[remote_uid]->m_channel->targetId() == remote_uid)
        channel = channels[remote_uid];

    return channel;
}

void TelepathyAccount::leaveChannel(const QString &remote_uid) {
    auto leave_message = "";
    if(!channels.contains(remote_uid) || !channels[remote_uid])
        return;

    auto tpChannel = channels[remote_uid];

    // request groupchat leave
    auto pending = ((Tp::TextChannel *)tpChannel->m_channel.data())->requestLeave(leave_message);
    connect(pending, &Tp::PendingOperation::finished, [this, remote_uid](Tp::PendingOperation *op) {
        if(op->isError()) {
            qWarning() << "leaveChannel" << remote_uid << op->errorMessage();
        }

        if(channels.contains(remote_uid) && channels[remote_uid])
            channels.remove(remote_uid);

        // @TODO: for now, we do not register chat join/leave events
        //this->onChannelLeft(remote_uid);
        emit channelLeft(local_uid, remote_uid);
    });
}

void TelepathyAccount::removeChannel(const QString &remote_uid) {
  if(channels.contains(remote_uid)) {
    channels.remove(remote_uid);
  }

  emit channelLeft(local_uid, remote_uid);
}

void TelepathyAccount::sendMessage(const QString &remote_uid, const QString &message) {
    qDebug() << "sendMessage: remote_uid:" << remote_uid;
    auto chan = hasChannel(remote_uid);

    if(chan) {
        Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::staticCast(chan->m_channel);
        textChannel->send(message);
        return;
    }

    auto *pending = acc->ensureTextChat(remote_uid);
    connect(pending, &Tp::PendingChannelRequest::finished, [message](Tp::PendingOperation *op){
            auto *_pending = (Tp::PendingChannelRequest *)op;
            auto chanrequest = _pending->channelRequest();
            auto channel = chanrequest->channel();
            auto text_channel = (Tp::TextChannel *)channel.data();
            text_channel->send(message);
    });
}

void TelepathyAccount::setChatState(const QString &remote_uid, Tp::ChannelChatState state) {
    auto chan = hasChannel(remote_uid);
    if(!chan) {
        qWarning() << "setChatState()" << remote_uid << "failed; no active channel";
        return;
    }

    Tp::TextChannelPtr channel = Tp::TextChannelPtr::staticCast(chan->m_channel);
    if (channel && channel->hasChatStateInterface())
        channel->requestChatState(state);
}

void TelepathyAccount::onRemoved() {
    emit removed(this);
}

// auto-join any user-defined persistent channels
// called from: onOnline() and onAccReady()
void TelepathyAccount::joinSavedGroupChats() {
    for(const ConfigStateItemPtr &configItem: configState->items) {
        if(configItem->local_uid == local_uid &&
           configItem->type == ConfigStateItemType::ConfigStateRoom &&
           configItem->auto_join) {
            if(hasChannel(configItem->remote_uid))
                continue;

            qDebug() << "account:" << local_uid << "auto-joining:" << configItem->remote_uid;
            this->_joinChannel(configItem->remote_uid, true);
        }
    }
}

QString TelepathyAccount::getServiceName() {
  return Utils::protocolToRTCOMServiceID(m_protocol_name);
}

TelepathyAccount::~TelepathyAccount() {}

TelepathyChannel::TelepathyChannel(const QString &remote_uid, TelepathyAccountPtr accountPtr, const Tp::ChannelPtr channelPtr, Tp::HandleType handleType) :
        remote_uid(remote_uid),
        m_account(accountPtr),
        m_channel(channelPtr),
        handleType(handleType),
        isRoom(handleType == Tp::HandleTypeRoom),
        QObject(nullptr) {
    auto group_uid = accountPtr->getGroupUid(Tp::TextChannelPtr::staticCast(m_channel));
    configState->addItem(accountPtr->local_uid, remote_uid, group_uid, isRoom ? ConfigStateItemType::ConfigStateRoom :
                                                                                ConfigStateItemType::ConfigStateContact);

    connect(m_channel->becomeReady(),
        SIGNAL(finished(Tp::PendingOperation*)),
        SLOT(onChannelReady(Tp::PendingOperation*)));
}

void TelepathyChannel::onChannelReady(Tp::PendingOperation *op) {
    qDebug() << "onChannelReady" << remote_uid << "isError:" << op->isError();
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

TelepathyChannel::~TelepathyChannel() {
  qDebug() << "RIP TelepathyChannel";
};
