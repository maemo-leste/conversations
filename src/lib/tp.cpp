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
    connect(myacc, &TelepathyAccount::channelJoined, this, &Telepathy::channelJoined);
    connect(myacc, &TelepathyAccount::channelLeft, this, &Telepathy::channelLeft);
    connect(myacc, &TelepathyAccount::removed, this, &Telepathy::onAccountRemoved);
}

void Telepathy::onAccountRemoved(TelepathyAccount* account) {
    qDebug() << "onAccountRemoved" << account;

    accounts.removeOne(account);
    delete account;
}

void Telepathy::joinChannel(const QString &backend_name, const QString &channel, bool persistent) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return;
    account->joinChannel(channel, persistent);
}

void Telepathy::leaveChannel(const QString &backend_name, const QString &channel) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return;
    account->leaveChannel(channel);
}

bool Telepathy::participantOfChannel(const QString &backend_name, const QString &channel) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return false;

    if(account->channels.contains(channel))
      return true;
    return false;
}

void Telepathy::sendMessage(const QString &backend_name, const QString &remote_uid, const QString &message) {
    auto account = rtcomLocalUidToAccount(backend_name);
    if(account == nullptr)
        return;
    account->sendMessage(remote_uid, message);
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
                auto channel_name = mychan->m_channel.data()->targetId();

                if(!ma->channels.contains(channel_name)) {
                    ma->channels[channel_name] = new AccountChannel;
                }

                ma->channels[channel_name]->name = channel_name;
                ma->channels[channel_name]->tpChannel = mychan;
                emit ma->channelJoined(ma->getLocalUid(), channel_name);
            }

            break;
        }

        /* Update channel request ptr with channel */
        if (matching_channel) {
            if (matching_requestptr) {
                matching_requestptr->succeeded((Tp::ChannelPtr)matching_channel);
                // dont auto-open chat windows for now
                // matching_account->TpOpenChannelWindow(Tp::TextChannelPtr(matching_channel));
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
    name = getLocalUid();  // backend_name, e.g: 'idle/irc/oftc_2dsander0'
    m_nickname = acc->nickname();
    m_protocol_name = acc->protocolName();

    // read persistent channels saved in user config
    this->readGroupchatChannels();

    connect(acc.data(), &Tp::Account::removed, this, &TelepathyAccount::onRemoved);
    connect(acc.data(), &Tp::Account::onlinenessChanged, this, &TelepathyAccount::onOnline);
    connect(acc->becomeReady(), &Tp::PendingReady::finished, this, &TelepathyAccount::onAccReady);
}

void TelepathyAccount::TpOpenChannelWindow(Tp::TextChannelPtr channel) {
    auto local_uid = name;
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

    auto self_name_str = m_nickname.toStdString();
    auto self_name = self_name_str.c_str();
    auto protocol_str = m_protocol_name.toStdString();
    auto protocol = protocol_str.c_str();
    auto backend_name_str = name.toStdString();
    auto backend_name = backend_name_str.c_str();

    qtrtcom::registerMessage(
        epoch, epoch, self_name, backend_name,
        remote_uid.toLocal8Bit(), remote_name, abook_uid,
        text.toLocal8Bit(), outgoing, protocol,
        channel_str, group_uid);

    // @TODO: duplicate code like in onJoinChannel, refactor
    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    auto event_type = Utils::protocolIsTelephone(protocol) ? "RTCOM_EL_EVENTTYPE_SMS_MESSAGE" : "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE";

    auto *chatMessage = new ChatMessage({
        .event_id = 1,  /* TODO: event id is wrong here but should not matter? or does it? */
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

void TelepathyAccount::joinChannel(const QString &channel, bool persistent) {
    if(!channels.contains(channel)) {
        channels[channel] = new AccountChannel;
        channels[channel]->name = channel;
        channels[channel]->auto_join = persistent;
    } else {
        channels[channel]->auto_join = persistent;
    }

    this->writeGroupchatChannels();
    this->_joinChannel(channel);
}

void TelepathyAccount::_joinChannel(const QString &channel) {
    qDebug() << "_joinChannel" << channel;
    auto *pending = acc->ensureTextChatroom(channel);

    connect(pending, &Tp::PendingChannelRequest::channelRequestCreated, this, [=](const Tp::ChannelRequestPtr &channelRequest) {
      if(channelRequest->isValid()) {
        this->onChannelJoined(channelRequest, channel);
      }
      else {
        qWarning() << "_joinChannel failed for " << channel;
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
    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
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
    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
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
    for(const auto &_channel: channels) {
        if(_channel->auto_join) {
            qDebug() << "account:" << name << "joining:" << _channel->name << "- from user config";
            this->_joinChannel(_channel->name);
        }
    }
}

Tp::TextChannel* TelepathyAccount::hasChannel(const QString &remote_uid) {
    if(!channels.contains(remote_uid))
        return nullptr;

    auto _channel = channels[remote_uid];
    if(_channel->tpChannel == nullptr)
        return nullptr;

    auto *a_channel = (Tp::TextChannel*) _channel->tpChannel->m_channel.data();
    if (remote_uid == a_channel->targetId()) {
        return a_channel;
    }

    return nullptr;
}

void TelepathyAccount::leaveChannel(const QString &channel) {
    auto leave_message = "";
    if(!channels.contains(channel))
        return;

    auto tpChannel = channels[channel]->tpChannel;
    if(tpChannel == nullptr) {
        delete channels[channel];
        channels.remove(channel);
        this->writeGroupchatChannels();
        return;
    }

    // request groupchat leave
    auto pending = ((Tp::TextChannel*)tpChannel->m_channel.data())->requestLeave(leave_message);
    connect(pending, &Tp::PendingOperation::finished, [=](Tp::PendingOperation *op) {
        if(op->isError()) {
            // @TODO: do something useful
            qWarning() << "leaveChannel" << channel << op->errorMessage();
        }

        if(!channels.contains(channel)) {
            this->writeGroupchatChannels();
            return;
        }

        auto _channel = channels[channel];
        delete _channel;
        channels.remove(channel);

        this->onChannelLeft(channel);     // rtcom registration
        this->writeGroupchatChannels();   // user-config registration
        emit channelLeft(name, channel);
    });
}

// called by `TelepathyChannel::onInvalidated`
void TelepathyAccount::_removeChannel(TelepathyChannel* chanptr) {
    if(chanptr != nullptr)
        chanptr->deleteLater();
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

void TelepathyAccount::readGroupchatChannels() {
    auto data = config()->get(ConfigKeys::GroupChatChannels).toByteArray();
    if(data.isEmpty()) return;

    auto doc = QJsonDocument::fromJson(data);
    if(doc.isNull() || !doc.isObject()) {
        qWarning() << "invalid json encountered parsing Config::autoJoinChatChannels";
        return;
    }

    auto obj = doc.object();
    if(obj.contains(name)) {
        auto obj_account = obj[name].toObject();
        if(!obj_account.contains("channels"))
            return;

        auto account_channels = obj_account["channels"].toArray();
        for (const auto &chan: account_channels) {
            auto obj_channel = chan.toObject();
            auto channel = obj_channel["name"].toString();
            auto auto_join = obj_channel["auto_join"].toBool();

            if(!channels.contains(channel)) {
                channels[channel] = new AccountChannel;
                channels[channel]->name = channel;
                channels[channel]->auto_join = auto_join;
            }
        }
    }
}

void TelepathyAccount::writeGroupchatChannels() {
    QJsonObject obj_account;
    QJsonArray  obj_channels;
    for(const auto &channel: channels.keys()) {
        auto *ac = channels[channel];
        QJsonObject obj_channel;
        obj_channel["name"] = ac->name;
        obj_channel["auto_join"] = ac->auto_join;
        obj_channels << obj_channel;
    }
    obj_account["channels"] = obj_channels;

    auto data = config()->get(ConfigKeys::GroupChatChannels).toByteArray();
    auto doc = QJsonDocument::fromJson(data);
    if(!doc.isNull() && doc.isObject()) {
        auto obj = doc.object();
        obj[name] = obj_account;
        auto dumps = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        config()->set(ConfigKeys::GroupChatChannels, dumps);
    }
}

TelepathyAccount::~TelepathyAccount() {
    /* Let's assume telepathy will clean up it's own channels so we don't
     * work on the m_channel variable in this class */
    qDeleteAll(channels);
    channels.clear();
}

TelepathyChannel::TelepathyChannel(const Tp::ChannelPtr &mchannel, TelepathyAccount* macc) : QObject(nullptr) {
    m_account = macc;
    m_channel = mchannel;

    connect(m_channel->becomeReady(),
        SIGNAL(finished(Tp::PendingOperation*)),
        SLOT(onChannelReady(Tp::PendingOperation*)));

    connect(m_channel.data(),
            &Tp::DBusProxy::invalidated, this,
            &TelepathyChannel::onInvalidated);
}

void TelepathyChannel::onInvalidated(Tp::DBusProxy * proxy, const QString &errorName, const QString &errorMessage) {
    m_account->_removeChannel(this);
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
