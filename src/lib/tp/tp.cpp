#include "lib/abook/abook_public.h"
#include "lib/abook/abook_roster.h"
#include "lib/abook/abook_contact.h"

#include <QCoreApplication>
#include <QDebug>

#include "lib/tp/tp.h"

#include "rtcom_models.h"
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

Telepathy::Telepathy(QObject *parent) : QObject(parent) {}

void Telepathy::init() {
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

    // @TODO: avatar?
    // conv_abook_func_roster_updated = std::bind(&Telepathy::onRosterChanged, this);
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

void Telepathy::getContact(QString local_uid, QString remote_uid, std::function<void(Tp::ContactPtr)> cb) {
    const TelepathyAccountPtr account = accountByName(local_uid);
    if(!account) {
        qCritical() << "No account associated with" << local_uid;
        return;
    }

    Tp::ConnectionPtr connection = account->acc->connection();
    Tp::PendingContacts *pcontacts = connection->contactManager()->contactsForIdentifiers(QStringList() << remote_uid);

    connect(pcontacts, &Tp::PendingContacts::finished, [this, cb, remote_uid](Tp::PendingOperation* op) {
        if(op->isError()) {
            qWarning() << "contactsForIdentifiers" << remote_uid << op->errorMessage();
            return;
        }

        Tp::PendingContacts *pcontacts = qobject_cast<Tp::PendingContacts*>(op);
        QList<Tp::ContactPtr> contacts = pcontacts->contacts();

        if(pcontacts->identifiers().size() != 1 || contacts.size() != 1 || !contacts.first()) {
            qWarning() << "could not fetch contact for remote_uid" << remote_uid;
            return;
        }

        const QString username = pcontacts->identifiers().first();
        qDebug() << "username" << username;
        Tp::ContactPtr contact = contacts.first();

        cb(contact);
    });
}

void Telepathy::authorizeContact(const QString &local_uid, const QString &remote_uid) {
    qDebug() << "authorizeContact" << local_uid << remote_uid;

    auto lambda = [this, local_uid, remote_uid](Tp::ContactPtr contact) {
        QString message = "";  // empty for now
        Tp::PendingOperation *auth_op = contact->authorizePresencePublication(message);
        connect(auth_op, &Tp::PendingOperation::finished, [this, message, contact](Tp::PendingOperation *op) {

            if (contact->subscriptionState() != Tp::Contact::PresenceStateYes) {
                Tp::PendingOperation* req_op = contact->requestPresenceSubscription(message);
                if (contact->subscriptionState() != Tp::Contact::PresenceStateYes) {
                    connect(req_op, &Tp::PendingOperation::finished, [this, message, contact](Tp::PendingOperation *op) {

                    });
                }
            }
        });
    };

    getContact(local_uid, remote_uid, lambda);
}

void Telepathy::denyContact(const QString &local_uid, const QString &remote_uid) {
    qDebug() << "denyContact" << local_uid << remote_uid;

    auto lambda = [this, local_uid, remote_uid](Tp::ContactPtr contact) {
        if (contact->publishState() != Tp::Contact::PresenceStateNo) {
            Tp::PendingOperation* op = contact->removePresencePublication();
            connect(op, &Tp::PendingOperation::finished, [this, contact](Tp::PendingOperation *_op) {
                if(_op->isError()) {
                    qWarning() << "removePresencePublication failed" << _op->errorMessage();
                } else {
                    qDebug() << "denying contact";
                }
            });
        } else {
            qWarning() << "ignoring deny operation, contact publishState is PresenceStateNo";
        }
    };

    getContact(local_uid, remote_uid, lambda);
}

void Telepathy::removeContact(const QString &local_uid, const QString &remote_uid) {
    qDebug() << "removeContact" << local_uid << remote_uid;

    auto lambda = [this, local_uid, remote_uid](Tp::ContactPtr contact) {
        if(contact->subscriptionState() != Tp::Contact::PresenceStateNo) {
            // the contact cant see our presence and we cant see their presence
            const Tp::PendingOperation* pub_op = contact->removePresencePublication();
            const Tp::PendingOperation* sub_op = contact->removePresenceSubscription();

            connect(pub_op, &Tp::PendingOperation::finished, [this, contact](Tp::PendingOperation *_op) {
                if(_op->isError()) {
                    qWarning() << "removePresencePublication failed" << _op->errorMessage();
                } else {
                    qDebug() << "removed publication for contact";
                }
            });

            connect(sub_op, &Tp::PendingOperation::finished, [this, contact](Tp::PendingOperation *_op) {
                if(_op->isError()) {
                    qWarning() << "removePresenceSubscription failed" << _op->errorMessage();
                } else {
                    qDebug() << "removed subscription for contact";
                }
            });
        } else {
            qWarning() << "ignoring remove operation, contact subscriptionState is PresenceStateNo";
        }
    };

    getContact(local_uid, remote_uid, lambda);
}

void Telepathy::blockContact(const QString &local_uid, const QString &remote_uid, bool block) {
    qDebug() << "blockContact" << local_uid << remote_uid;

    auto lambda = [this, local_uid, block, remote_uid](Tp::ContactPtr contact) {
        const Tp::PendingOperation* op = block ? contact->block() : contact->unblock();
        connect(op, &Tp::PendingOperation::finished, [this, contact, block](Tp::PendingOperation *_op) {
            if(_op->isError()) {
                qWarning() << "blockContact failed" << _op->errorMessage();
                return;
            } else {
                qDebug() << QString("block (%1)").arg(block) << "for contact";
            }
        });
    };

    getContact(local_uid, remote_uid, lambda);
}

bool Telepathy::has_feature_friends(const QString &local_uid) {
    TelepathyAccountPtr account = accountByName(local_uid);
    if(!account) {
        qCritical() << "Could not find account for feature_friends" << local_uid;
        return false;
    }

    return account->has_feature_friends();
}

void Telepathy::onDatabaseAddition(QSharedPointer<ChatMessage> &msg) {
    emit databaseAddition(msg);
}

void Telepathy::onRosterChanged() {
    qDebug() << "======= EMIT onRosterChanged()";
    emit rosterChanged();
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
    connect(telepathyAccount, &TelepathyAccount::errorMessage, this, &Telepathy::errorMessage);
    connect(telepathyAccount, &TelepathyAccount::removed, [this](TelepathyAccount *ptr) {
      this->onAccountRemoved(ptr->local_uid);
    });

    emit accountAdded(accountPtr);
}

void Telepathy::onAccountRemoved(const QString &local_uid) {
    qDebug() << "onAccountRemoved" << local_uid;
    const auto ptr = this->accountByName(local_uid);
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

TelepathyAccountPtr Telepathy::accountByPtr(const Tp::AccountPtr &ptr) {
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

bool Telepathy::deleteChannel(const QString &local_uid, const QString &remote_uid) {
    auto account = rtcomLocalUidToAccount(local_uid);
    if(!account)
        return false;
    account->leaveChannel(remote_uid);
    account->channels.remove(remote_uid);

    emit channelDeleted(local_uid, remote_uid);
    return true;
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
        // find associated TelepathyAccountPtr
        TelepathyAccountPtr accountPtr = m_tp->accountByPtr(account);
        if(!accountPtr)
            throw std::runtime_error("no matching TelepathyAccountPtr debug me");

        QVariantMap props = channelPtr->immutableProperties();
        QString remote_uid = props.value(QString("%1.TargetID").arg(TP_QT_IFACE_CHANNEL)).toString();
        if(remote_uid.isEmpty()) {
            qWarning() << "handleChannels cannot get TargetID (remote_uid) for channel";
            continue;
        }

        // https://telepathy.freedesktop.org/doc/telepathy-qt/a00879.html
        Tp::HandleType handleType = (Tp::HandleType) props.value(QString("%1.TargetHandleType").arg(TP_QT_IFACE_CHANNEL)).toInt();
        const bool isRoom = handleType == Tp::HandleTypeRoom;

        QString initiator_id = props.value(QString("%1.InitiatorID").arg(TP_QT_IFACE_CHANNEL)).toString();
        QString room_name = props.value(QString("%1.Interface.Room2.RoomName").arg(TP_QT_IFACE_CHANNEL)).toString();

        // https://telepathy.freedesktop.org/spec/index.html#Channel-Types
        QString channelType = props.value(QString("%1.ChannelType").arg(TP_QT_IFACE_CHANNEL)).toString();
        if(!channelType.endsWith(".Text")) {
            qWarning() << "skipping unsupported channel type" << channelType;
            continue;
        }

        // Matrix always needs a room name
        if(accountPtr->protocolName() == "matrix") {
            if(room_name.isEmpty()) {
                qWarning() << "matrix channel offered without room alias, skipping";
                continue;
            }

            if(!room_name.startsWith("#") && !room_name.startsWith("@")) {
                qWarning() << "matrix channel offered a faulty room alias, skipping, room_name was" << room_name;
                continue;
            }
        }

        // create a TelepathyChannelPtr
        if(!accountPtr->hasChannel(remote_uid)) {
            qDebug() << "creating TelepathyChannelPtr()";
            auto tcPtr = TelepathyChannelPtr(new TelepathyChannel(remote_uid, accountPtr, channelPtr, handleType));

            tcPtr->isRoom = isRoom;
            if(!room_name.isEmpty()) {  // register room name in rtcom
                auto remote_uid_str = remote_uid.toStdString();
                const auto _remote_uid = remote_uid_str.c_str();

                auto room_name_str = room_name.toStdString();
                const auto _room_name = room_name_str.c_str();

                rtcom_qt::set_room_name(_remote_uid, _room_name);
                tcPtr->room_name = room_name;
            }

            accountPtr->channels[remote_uid] = tcPtr;

            // handle invalidated signal
            QObject::connect(channelPtr.data(), &Tp::DBusProxy::invalidated, [accountPtr, remote_uid] {
                accountPtr->removeChannel(remote_uid);
            });

            // announce we joined a channel (valid for both room, and 1:1 contact)
            emit accountPtr->channelJoined(accountPtr->local_uid, remote_uid);
        } else { // we already have a ChannelPtr, replace if the old one is not valid
            const auto old_channel = accountPtr->hasChannel(remote_uid);

            if (old_channel && !old_channel->m_channel->isValid() && channelPtr->isValid()) {
                qDebug() << "setting new ChannelPtr, replacing the old";
                old_channel->setChannelPtr(channelPtr);
                emit accountPtr->channelJoined(accountPtr->local_uid, remote_uid);
            }
        }

        // channel joined from 'external' Tp client (e.g addresbook), request chatWindow
        if(!isAutoJoin)
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

    onConnectionChanged(acc->connection());

    connect(acc.data(), &Tp::Account::removed, this, &TelepathyAccount::onRemoved);
    connect(acc.data(), &Tp::Account::onlinenessChanged, this, &TelepathyAccount::onOnline);
    connect(acc.data(), &Tp::Account::connectionChanged, this, &TelepathyAccount::onConnectionChanged);
    connect(acc->becomeReady(), &Tp::PendingReady::finished, this, &TelepathyAccount::onAccReady);
}

void TelepathyAccount::onConnectionChanged(const Tp::ConnectionPtr &conn) {
    if(conn.isNull() || conn->isReady(Tp::Connection::FeatureRoster))
        return;

    Tp::Features connectionFeatures;
    connectionFeatures << Tp::Connection::FeatureRoster;

    // only request roster groups if we support it, otherwise it can error and not finish becoming ready
    if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS))
        connectionFeatures << Tp::Connection::FeatureRosterGroups;

    if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE))
        connectionFeatures << Tp::Connection::FeatureSimplePresence;

    // `bool TelepathyAccount::has_feature_friends()` uses this
    if (conn->hasInterface(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACTS))
        m_feature_friends = true;

    Tp::PendingReady *op = conn->becomeReady(connectionFeatures);
    op->setProperty("connection", QVariant::fromValue<Tp::ConnectionPtr>(conn));
    connect(op, SIGNAL(finished(Tp::PendingOperation*)), SLOT(onConnectionReady(Tp::PendingOperation*)));
}

void TelepathyAccount::onConnectionReady(Tp::PendingOperation *op) {
    m_connection = op->property("connection").value<Tp::ConnectionPtr>();
}

void TelepathyAccount::TpOpenChannelWindow(Tp::TextChannelPtr channel) {
    const auto remote_uid = getRemoteUid(channel);
    const auto group_uid = getGroupUid(channel);
    const auto service = getServiceName();
    QString channelstr;

    if (channel->targetHandleType() != Tp::HandleTypeContact) {
        channelstr = channel->targetId();
    }

    emit openChannelWindow(local_uid, remote_uid, group_uid, service, channelstr);
}

QString TelepathyAccount::getGroupUid(const TelepathyChannelPtr &channel) {
    auto text_channel = Tp::TextChannelPtr::staticCast(channel->m_channel);
    return getGroupUid(text_channel);
}

QString TelepathyAccount::getRoomName(const TelepathyChannelPtr &channel) {
    for(const auto &c: channels) {
        if(c == channel) {
            return c->room_name;
        }
    }
    return {};
}

QString TelepathyAccount::getGroupUid(Tp::TextChannelPtr channel) const {
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

QString TelepathyAccount::getLocalUid() const {
    return QString(acc->objectPath()).replace("/org/freedesktop/Telepathy/Account/", "");
}

QSharedPointer<ChatMessage> TelepathyAccount::log_event(time_t epoch, const QString &text, bool outgoing, const Tp::TextChannelPtr &channel, const QString &remote_uid, const QString &remote_alias, unsigned int flags) {
    std::string channel_str;

    QByteArray channel_ba = channel->targetId().toLocal8Bit();
    if (channel->targetHandleType() == Tp::HandleTypeContact) {
    } else {
        channel_str = channel_ba.data();
        // auto channel_qstr = channel->targetId();
    }

    const QByteArray group_uid = getGroupUid(channel).toLocal8Bit();

    std::string remote_name;
    const QString abook_uid = this->local_uid + "-" + remote_uid;
    const auto abook_uid_str = abook_uid.toStdString();

    if (abook_qt::ROSTER.contains(abook_uid_str) && !abook_qt::ROSTER[abook_uid_str]->display_name.empty()) {
        remote_name = abook_qt::ROSTER[abook_uid.toStdString()]->display_name;
    } else if (remote_alias != nullptr && !remote_alias.isEmpty()) {
        remote_name = remote_alias.toStdString();
    } else {
        remote_name = remote_uid.toStdString();
    }

    const auto self_name_str = m_nickname.toStdString();
    const auto protocol_str = m_protocol_name.toStdString();
    const auto local_uid_str = local_uid.toStdString();

    const auto service = getServiceName();

    rtcom_qt::ChatMessageEntry* new_message = rtcom_qt::register_message(
        epoch, epoch, self_name_str, local_uid_str,
        remote_uid.toStdString(), remote_name, abook_uid.toStdString(),
        text.toStdString(), outgoing, protocol_str,
        channel_str, group_uid.toStdString(), flags);

    if (!new_message)
      return {};

    auto *chatMessage = new ChatMessage(new_message);

    QSharedPointer<ChatMessage> ptr(chatMessage);
    emit databaseAddition(ptr);

    return ptr;
}

/* Slot for when we have received a message */
void TelepathyAccount::onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel) {
    auto groupSelfContact = channel->groupSelfContact();
    bool is_scrollback = message.isScrollback();
    const bool is_delivery_report = message.isDeliveryReport();

    if (is_delivery_report) {
      const auto details = message.deliveryDetails();
      if(!details.hasOriginalToken()) {  // matching the delivery report to the original message is impossible without the token
        qWarning() << "delivery report without original message token received";
        return;
      }

      const auto token = details.originalToken().toStdString();

      // ask rtcom db
      const auto res = rtcom_qt::get_events_by_header("message-token", token);
      if (res.empty()) {
        qWarning() << "the original message for this delivery report was not found";
        return;
      }

      const unsigned int event_id = res[0];
      Tp::DeliveryStatus status = details.status();

      if (status == Tp::DeliveryStatusTemporarilyFailed) {
        emit m_parent->messageFlagsChanged(event_id, Tp::DeliveryStatusTemporarilyFailed);
        return rtcom_qt::toggle_event_flags(event_id, rtcom_qt::RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR);
      } else if (status == Tp::DeliveryStatusPermanentlyFailed) {
        emit m_parent->messageFlagsChanged(event_id, Tp::DeliveryStatusPermanentlyFailed);
        return rtcom_qt::toggle_event_flags(event_id, rtcom_qt::RTCOM_EL_FLAG_SMS_PERMANENT_ERROR);
      } else if (status == Tp::DeliveryStatusDelivered || Tp::DeliveryStatusAccepted || Tp::DeliveryStatusRead) {
        constexpr unsigned int flags = rtcom_qt::RTCOM_EL_FLAG_SMS_PENDING |
                                       rtcom_qt::RTCOM_EL_FLAG_SMS_PERMANENT_ERROR |
                                       rtcom_qt::RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR;
        emit m_parent->messageFlagsChanged(event_id, 0);
        return rtcom_qt::toggle_event_flags(event_id, flags, true /* unset */);
      }

      qWarning() << "delivery report; unhandled status:" << status;
      return;
    }

    auto remote_uid = message.sender()->id();
    auto remote_alias = message.sender()->alias();
    const bool outgoing = groupSelfContact->handle() == message.sender()->handle() || groupSelfContact->id() == remote_uid;

    const QDateTime dt = message.sent().isValid() ? message.sent() : message.received();
    qint64 epoch = dt.toMSecsSinceEpoch();

    const auto text = message.text().toLocal8Bit();
    qDebug() << "onMessageReceived" << dt << channel->targetId() << message.senderNickname() << message.text();
    qDebug() << "is_scrollback" << is_scrollback;
    qDebug() << "channel->targetId()" << channel->targetId();
    qDebug() << "isRescued" << message.isRescued();

    if(outgoing) {
        remote_uid = channel->groupSelfContact()->id();
        remote_alias = nullptr;
    }

    const ConfigStateItemPtr configItem = configState->getItem(local_uid, channel->targetId());
    if (is_scrollback && !Utils::protocolIsTelephone(m_protocol_name) && !configItem.isNull()) {
        // try to drop scrollbacks that we have already registered
        if (auto now = QDateTime::currentMSecsSinceEpoch(); epoch > now) {
            qWarning() << "WARNING: a message from the future detected for protocol:" << m_protocol_name << " remote_uid:" << remote_uid << "text: " << text << ", defaulting to current time (NOW)";
            epoch = now;
        }

        if (epoch <= configItem->date_last_message) {
            qDebug() << "dropping old message" << channel->targetId() << ":" << text;
            return;
        }
    }

    // handle case where delivery report was sent to another client, but our instance is not aware yet
    // of a message's new status. A scrollback of an outgoing message implies it was received by the server,
    // update our local db to reflect this.
    if (is_scrollback && outgoing) {
      // @TODO:
    }

    if (const auto result = log_event(dt.toTime_t(), text, outgoing, channel, remote_uid, remote_alias); result.isNull())
      qWarning() << "Failed to add a database event";
    else {
      // only write to state when insertion was successful
      configState->setLastMessageTimestamp(local_uid, channel->targetId(), epoch);
    }
}

/* When we have managed to send a message */
void TelepathyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageSent" << message.text();

    const time_t epoch = message.sent().toTime_t();
    const QString remote_uid = channel->groupSelfContact()->id();
    const auto text = message.text().toLocal8Bit();

    const auto result = log_event(epoch, text, true, channel, remote_uid, nullptr, rtcom_qt::RTCOM_EL_FLAG_SMS_PENDING);
    if (result.isNull()) {
      qWarning() << "Failed to add a database event";
    } else {
      if(auto message_token = message.messageToken().toStdString(); !message_token.empty())
        rtcom_qt::set_event_header(result->event_id(), "message-token", message_token);
    }
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

void TelepathyAccount::_joinChannel(const QString& remote_uid, const bool auto_join) {
    qDebug() << "_joinChannel" << remote_uid << "protocol" << protocolName();

    // set a 'null' datetime on auto_join
    const QDateTime dt = auto_join ? QDateTime() : QDateTime::currentDateTime();
    const auto *pending = acc->ensureTextChatroom(remote_uid, dt);

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
void TelepathyAccount::onChannelJoined(const Tp::ChannelRequestPtr &channelRequest, const QString& channel) {
    if(!channelRequest->isValid()) {
      qWarning() << "onChannelJoined: !channelRequest->isValid()";
      return;
    }
    this->onChannelJoinedOrLeft(true, channel);
}

void TelepathyAccount::onChannelJoinedOrLeft(bool joined, QString channel) {
    const std::string local_uid_str = local_uid.toStdString();
    const std::string remote_uid_str  = m_nickname.toStdString();
    const std::string abook_uid = this->local_uid.toStdString() + "-" + remote_uid_str;
    const std::string channel_str = channel.toStdString();
    const std::string protocol_str = m_protocol_name.toStdString();
    const auto group_uid = QString("%1-%2").arg(local_uid, channel);
    const std::string group_uid_str = group_uid.toStdString();
    const std::string text_content = joined ? "!joined!" : "!left!";

    const time_t now = QDateTime::currentDateTime().toTime_t();
    const std::string remote_name = "";

    auto* new_message = rtcom_qt::register_chat_leave(
      now, now, remote_uid_str, local_uid_str,
      remote_uid_str, remote_name, abook_uid,
      text_content, protocol_str, channel_str,
      group_uid_str);

    auto *chatMessage = new ChatMessage(new_message);
    QSharedPointer<ChatMessage> ptr(chatMessage);
    emit databaseAddition(ptr);
}

void TelepathyAccount::onAccReady(Tp::PendingOperation *op) {
  this->joinSavedGroupChats();
  emit accountReady(this);
}

TelepathyChannelPtr TelepathyAccount::hasChannel(const QString &remote_uid) {
    TelepathyChannelPtr channel;

    if (channels.contains(remote_uid) && channels[remote_uid]) {
        const auto isValid = channels[remote_uid]->m_channel->isValid();
        if (isValid) {
            const auto targetId = channels[remote_uid]->m_channel->targetId();
            if (targetId == remote_uid)
                channel = channels[remote_uid];
        }
  }

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
        //this->onChannelJoinedOrLeft(false, remote_uid);
        emit channelLeft(local_uid, remote_uid);
    });
}

void TelepathyAccount::removeChannel(const QString &remote_uid) {
  if(channels.contains(remote_uid)) {
    channels.remove(remote_uid);
  }

  emit channelLeft(local_uid, remote_uid);
}

void TelepathyAccount::sendMessage(QString remote_uid, const QString &message) {
    qDebug() << "sendMessage: remote_uid:" << remote_uid;
    auto chan = hasChannel(remote_uid);

    if(chan) {
        Tp::TextChannelPtr textChannel = Tp::TextChannelPtr::staticCast(chan->m_channel);
        textChannel->send(message);
        return;
    }

    // cannot create Matrix TextChannel for *group* rooms on-the-fly, need to specifically call joinChannel() first
    if(protocolName() == "matrix" && remote_uid.startsWith("#"))
        return;

    qDebug() << "did not have TextChannelPtr, calling acc->ensureTextChat()";
    auto *pending = acc->ensureTextChat(remote_uid);

    connect(pending, &Tp::PendingChannelRequest::finished, [message, this](Tp::PendingOperation *op){
        if(op->isError()) {
            auto err_msg = QString("ensureTextChat failed: %1").arg(op->errorMessage());
            qWarning() << err_msg;
            emit errorMessage(err_msg);
            return;
        }

        auto channel = reinterpret_cast<Tp::PendingChannelRequest *>(op)->channelRequest()->channel();
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

    qDebug() << "TelepathyChannel::TelepathyChannel constructor";
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
        qWarning() << "onChannelReady, errorName:" << op->errorName();
        qWarning() << "onChannelReady, errorMessage:" << op->errorMessage();
        return;
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

void TelepathyChannel::setChannelPtr(const Tp::ChannelPtr channelPtr) {
    qDebug() << "TelepathyChannel::setChannelPtr, replacing";
    m_channel->deleteLater();
    m_channel = channelPtr;
    connect(m_channel->becomeReady(),
        SIGNAL(finished(Tp::PendingOperation*)),
        SLOT(onChannelReady(Tp::PendingOperation*)));
}

/* If we already have a channel, send is easy */
void TelepathyChannel::sendMessage(const QString &message) const {
    Tp::TextChannelPtr::staticCast(m_channel)->send(message);
}

TelepathyChannel::~TelepathyChannel() {
  qDebug() << "RIP TelepathyChannel";
};
