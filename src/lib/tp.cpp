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

    qDebug() << "account count:" << l.count();

    Tp::AccountPtr acc;

    for (int i = 0; i < l.count(); i++) {
        acc = l[i];
        auto myacc = new TelepathyAccount(acc);
        accounts << myacc;

        connect(myacc, SIGNAL(databaseAddition(ChatMessage *)), SLOT(onDatabaseAddition(ChatMessage *)));
    }

    emit accountManagerReady();
}

void Telepathy::onDatabaseAddition(ChatMessage *msg) {
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
    // XXX: this stub needs implementing
    auto channelptr = channels[0];
    Tp::TextChannel* channel_ = (Tp::TextChannel*)channelptr.data();

    qDebug() << "HANDLECHANNELS" << channel_;

    for (TelepathyAccount *ma : m_telepathy_parent->accounts) {
        qDebug() << "CMP" << ma->acc << "|" <<  account;
        qDebug() << "CMP2" << ma->nickname() << ma->localUid() << ma->protocolName();
        if (ma->nickname() == "wizzupvm") {
        //if (ma->nickname() == "merlijn@xmpp.wajer.org") {
            //if (ma->acc == account) {

            ma->m_testchan = channelptr;
            ma->connect(channel_->becomeReady(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(onChannelReady(Tp::PendingOperation*)));
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

void TelepathyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
    qDebug() << "onMessageSent" << message.text();

    auto remote_uid = channel->targetContact()->id().toLocal8Bit();
    auto text = message.text().toLocal8Bit();

    // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's do it for now
    auto epoch = message.sent().toTime_t();
    create_event(epoch,
                 m_nickname.toLocal8Bit().data(),
                 m_local_uid.toLocal8Bit().data(),
                 remote_uid,
                 remote_uid,
                 text, true,
                 m_protocol_name.toStdString().c_str());

    auto *item = new ChatMessage(1, "-1", "", m_local_uid, remote_uid, remote_uid, "", text, "", epoch, 0, "", "-1", true, 0);
    emit databaseAddition(item);
}

void TelepathyAccount::onChanMessageReceived(const Tp::ReceivedMessage &message) {
    qDebug() << "onChanMessageReceived" << message.received() << message.senderNickname() << message.text();

    Tp::TextChannel* channel = (Tp::TextChannel*) m_testchan.data();
    channel->send("Echo: " + message.text());
}

void TelepathyAccount::onChanPendingMessageRemoved(const Tp::ReceivedMessage &message) {
    qDebug() << "onChanPendingMessageRemoved" << message.received() << message.senderNickname() << message.text();
}

void TelepathyAccount::onChanMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken) {
    qDebug() << "onChanMessageSent";
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
    create_event(epoch, self_name.data(), backend_name.data(), remote_uid, remote_uid, text, false, m_protocol_name.toStdString().c_str());

    auto *item = new ChatMessage(1, "-1", "", backend_name, remote_uid, remote_uid, "", text, "", epoch, 0, "", "-1", false, 0);
    emit databaseAddition(item);
}

void TelepathyAccount::onOnline(bool online) {
    qDebug() << "onOnline: " << online;
}

void TelepathyAccount::onChannelReady(Tp::PendingOperation *op) {
    qDebug() << "onChannelReady, isError:" << op->isError();
    if (op->isError()) {
        qDebug() << "onChannelReady, errorName:" << op->errorName();
        qDebug() << "onChannelReady, errorMessage:" << op->errorMessage();
    }

    Tp::TextChannel* channel = (Tp::TextChannel*) m_testchan.data();

    qDebug() << "Calling groupAddContacts";
    auto pending = channel->groupAddContacts(QList<Tp::ContactPtr>() << channel->connection()->selfContact());
    connect(pending,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onGroupAddContacts(Tp::PendingOperation*)));
}

void TelepathyAccount::onGroupAddContacts(Tp::PendingOperation *op) {
    qDebug() << "onGroupAddContacts, isError:" << op->isError();
    if (op->isError()) {
        qDebug() << "onGroupAddContacts, errorName:" << op->errorName();
        qDebug() << "onGroupAddContacts, errorMessage:" << op->errorMessage();
    }

    // XXX: We need the channel object here to set up the signals
    //Tp::ChannelPtr chan = op->object();

    Tp::TextChannel* channel = (Tp::TextChannel*) m_testchan.data();

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

void TelepathyAccount::onAccReady(Tp::PendingOperation *op) {
    qDebug() << "onAccReady, isError:" << op->isError();
    qDebug() << "onAccReady, connection:" << acc->connection();
    qDebug() << "Display name:" << acc->displayName();
    qDebug() << "connectsAutomatically" << acc->connectsAutomatically();
    qDebug() << "isOnline" << acc->isOnline();
    qDebug() << "serviceName" << acc->serviceName();

    qDebug() << "channelObserver channels" << channelobserver->channels();

    if (acc->isOnline()) {
        qDebug() << "requested ##maemotest";
        //auto *pending = acc->ensureTextChatroom("test@conference.xmpp.wajer.org");
        //auto *pending = acc->ensureTextChatroom("#maemo-leste");
        auto *pending = acc->ensureTextChatroom("##maemotest");
    }


    // XXX: note that the account is ready, use that for some guards
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
