#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/tp.h"
#include "lib/utils.h"


Telepathy::Telepathy(QObject *parent) : QObject(parent) {
#if 0
    m_accountmanager = Tp::AccountManager::create(Tp::AccountFactory::create(QDBusConnection::sessionBus(), Tp::Account::FeatureCore),
        Tp::ConnectionFactory::create(QDBusConnection::sessionBus(), Tp::Connection::FeatureCore | Tp::Connection::FeatureSelfContact)
            );
#endif


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

void Telepathy::onAccountManagerReady(Tp::PendingOperation *op) {
    qDebug() << "onAccountManagerReady";

    auto validaccounts = m_accountmanager->validAccounts();
    auto l = validaccounts->accounts();

    Tp::AccountPtr acc;

    for (int i = 0; i < l.count(); i++) {
        acc = l[i];
        auto myacc = new TelepathyAccount(acc);
        accounts << myacc;

        connect(myacc, &TelepathyAccount::databaseAddition, this, &Telepathy::onDatabaseAddition);
    }

    emit accountManagerReady();
}

void Telepathy::onDatabaseAddition(const QSharedPointer<ChatMessage> &msg) {
    emit databaseAddition(msg);
}

void Telepathy::sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
    qDebug() << local_uid << remote_uid;

    for (TelepathyAccount *ma : accounts) {
        auto acc = ma->acc;

        QByteArray backend_name = (acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName()).toLocal8Bit();

        if (backend_name == local_uid) {
            qDebug() << backend_name;
            ma->sendMessage(local_uid, remote_uid, message);
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
        Tp::TextChannel* channel_ = (Tp::TextChannel*)channelptr.data();

        // TODO: If we actually implement this properly, then we can stop using
        // the SimpleObservers, which would be a big improvement /
        // simplification
        if (channel_->targetHandleType() == Tp::HandleTypeContact) {
            qDebug() << "handleChannels: skipping this channel, it's a contact, not a groupdm/channel";
            continue;
        }
        for (TelepathyAccount *ma : m_telepathy_parent->accounts) {
            if (ma->acc == account) {
                auto mychan = new TelepathyChannel(channelptr, ma);
                ma->channels << mychan;
            }
        }
    }

    context->setFinished();
}




TelepathyAccount::TelepathyAccount(Tp::AccountPtr macc) : QObject(nullptr) {
    acc = macc;
    textobserver = Tp::SimpleTextObserver::create(acc);

    connect(acc.data(),
            SIGNAL(onlinenessChanged(bool)),
            SLOT(onOnline(bool)));

    connect(acc->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccReady(Tp::PendingOperation*)));

    connect(textobserver.data(),
            SIGNAL(messageReceived(const Tp::ReceivedMessage&, const Tp::TextChannelPtr&)),
            SLOT(onMessageReceived(const Tp::ReceivedMessage&, const Tp::TextChannelPtr&)));

    connect(textobserver.data(), SIGNAL(messageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &, const Tp::TextChannelPtr &)),
            SLOT(onMessageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &, const Tp::TextChannelPtr &)));

    channelobserver = Tp::SimpleObserver::create(acc, Tp::ChannelClassSpec::textChatroom());
    connect(channelobserver.data(),
            SIGNAL(newChannels(const QList< Tp::ChannelPtr > &)),
            SLOT(onNewChannels(const QList< Tp::ChannelPtr > &)));


    m_nickname = acc->nickname();
    m_local_uid = acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName();
    m_protocol_name = acc->protocolName();
}

void TelepathyAccount::onNewChannels(const QList< Tp::ChannelPtr > &channels) {
    Tp::TextChannel* channel_ = (Tp::TextChannel*)channels[0].data();
    qDebug() << "onNewChannels" << channel_;

    //channel_->send("Maemo conversations says hi");
}

void TelepathyAccount::onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageReceived" << message.received() << message.senderNickname() << message.text();
    qDebug() << "isDeliveryReport" << message.isDeliveryReport();

    if (message.isDeliveryReport()) {
        // TODO: We do not want to reply to it not write anything for now
        // Later we want to update the rtcom db with the delivery report
        return;
    }

    QByteArray self_name = acc->nickname().toLocal8Bit();
    QByteArray backend_name = (acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName()).toLocal8Bit();
    QByteArray remote_uid = message.senderNickname().toLocal8Bit();
    QByteArray text = message.text().toLocal8Bit();

    // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's do it for now
    auto epoch = message.received().toTime_t();
    create_event(epoch, epoch,
         self_name.data(),
         backend_name.data(),
         remote_uid,
         remote_uid,
         text,
         false,
         m_protocol_name.toStdString().c_str(),
         NULL /* channel */,
         NULL /* TODO: group_uid */,
         0 /* TODO: flags */);

    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    auto *msg = new ChatMessage(1, service, "", backend_name, remote_uid, remote_uid, "", text, "", epoch, 0, "", "-1", false, 0);
    QSharedPointer<ChatMessage> ptr(msg);
    emit databaseAddition(ptr);
}

void TelepathyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageSent" << message.text();

    auto remote_uid = channel->targetContact()->id().toLocal8Bit();
    auto text = message.text().toLocal8Bit();

    // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's do it for now
    auto epoch = message.sent().toTime_t();
    create_event(epoch, epoch,
                 m_nickname.toLocal8Bit().data(),
                 m_local_uid.toLocal8Bit().data(),
                 remote_uid,
                 remote_uid,
                 text, true,
                 m_protocol_name.toStdString().c_str(),
                 NULL /* channel */,
         NULL /* TODO: group_uid */,
         0 /* TODO: flags */ );

    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    auto *msg = new ChatMessage(1, service, "", m_local_uid, remote_uid, remote_uid, "", text, "", epoch, 0, "", "-1", true, 0);
    QSharedPointer<ChatMessage> ptr(msg);
    emit databaseAddition(ptr);
}

void TelepathyAccount::onOnline(bool online) {
    //qDebug() << "onOnline: " << online;
}

void TelepathyAccount::joinChannel(const QString &channel) {
    qDebug() << "joinChannel" << channel;
    auto *pending = acc->ensureTextChatroom(channel);
}

void TelepathyAccount::onAccReady(Tp::PendingOperation *op) {
    qDebug() << "onAccReady, isError:" << op->isError() << "Display name:" << acc->displayName() << "(Online:" << acc->isOnline() << ", serviceName:" << acc->serviceName() << ", objectPath: " << acc->objectPath() << ", busName:" << acc->busName() << ")";


#if 1
    if (acc->isOnline()) {
        joinChannel("##maemotest");
    }
#endif

}

void TelepathyAccount::sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
    auto *pending = acc->ensureTextChat(remote_uid);

    connect(pending, &Tp::PendingChannelRequest::finished, [=](Tp::PendingOperation *op){
            auto *_pending = (Tp::PendingChannelRequest*)op;
            auto chanrequest = _pending->channelRequest();
            auto channel = chanrequest->channel();
            auto text_channel = (Tp::TextChannel*)channel.data();
            text_channel->send(message);
            });
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

    auto pending = channel->groupAddContacts(QList<Tp::ContactPtr>() << channel->connection()->selfContact());
    connect(pending,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onGroupAddContacts(Tp::PendingOperation*)));
}

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

}

void TelepathyChannel::onChanMessageReceived(const Tp::ReceivedMessage &message) {
    qDebug() << "onChanMessageReceived" << message.received() << message.senderNickname() << message.text();
    qDebug() << "channel targetID:" << m_channel->targetId();

    auto acc = m_account->acc;
    QByteArray self_name = acc->nickname().toLocal8Bit();
    QByteArray backend_name = (acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName()).toLocal8Bit();
    QByteArray remote_uid = message.senderNickname().toLocal8Bit();
    QByteArray text = message.text().toLocal8Bit();
    QByteArray channel = m_channel->targetId().toLocal8Bit();
    QByteArray group_uid = (acc->objectPath().replace("/org/freedesktop/Telepathy/Account/", "") + "-" + m_channel->targetId()).toLocal8Bit();

    // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's do it for now
    auto epoch = message.received().toTime_t();
    create_event(epoch, epoch, self_name.data(), backend_name.data(), remote_uid, remote_uid, text, false, m_account->protocolName().toStdString().c_str(), channel, group_uid, 3 /* TODO BETTER FLAGS */);

    //emit m_account->databaseAddition(ptr);
}

void TelepathyChannel::onChanPendingMessageRemoved(const Tp::ReceivedMessage &message) {
    qDebug() << "onChanPendingMessageRemoved" << message.received() << message.senderNickname() << message.text();
}

void TelepathyChannel::onChanMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken) {
    qDebug() << "onChanMessageSent";

    // TODO: Log our own message here?
    // TODO: emit signals
    //emit m_account->databaseAddition(ptr);
}

void TelepathyChannel::sendMessage(const QString &message) {
    Tp::TextChannel* channel = (Tp::TextChannel*) m_channel.data();
    channel->send(message);
}


TelepathyChannel::~TelepathyChannel()
{
}
