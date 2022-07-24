#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/tp.h"
#include "lib/utils.h"


Telepathy::Telepathy(QObject *parent) : QObject(parent) {
    m_accountmanager = Tp::AccountManager::create(Tp::AccountFactory::create(QDBusConnection::sessionBus(), Tp::Account::FeatureCore));
    connect(m_accountmanager->becomeReady(), &Tp::PendingReady::finished, this, &Telepathy::onAccountManagerReady);

    registrar = Tp::ClientRegistrar::create();
    Tp::AbstractClientPtr handler = Tp::AbstractClientPtr::dynamicCast(
            Tp::SharedPtr<TelepathyHandler>(new TelepathyHandler(
                    Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::textChat() << Tp::ChannelClassSpec::textChatroom())));
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
    : Tp::AbstractClientHandler(channelFilter) { }

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
    // do something
    context->setFinished();
}

TelepathyAccount::TelepathyAccount(Tp::AccountPtr macc) : QObject(nullptr) {
    acc = macc;
    observer = Tp::SimpleTextObserver::create(acc);

    connect(acc.data(),
            SIGNAL(onlinenessChanged(bool)),
            SLOT(onOnline(bool)));

    connect(acc->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccReady(Tp::PendingOperation*)));

    connect(observer.data(),
            SIGNAL(messageReceived(const Tp::ReceivedMessage&, const Tp::TextChannelPtr&)),
            SLOT(onMessageReceived(const Tp::ReceivedMessage&, const Tp::TextChannelPtr&)));

    connect(observer.data(), SIGNAL(messageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &, const Tp::TextChannelPtr &)),
            SLOT(onMessageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &, const Tp::TextChannelPtr &)));

    m_nickname = acc->nickname();
    m_local_uid = acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName();
    m_protocol_name = acc->protocolName();
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

    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    auto *msg = new ChatMessage(1, service, "", m_local_uid, remote_uid, remote_uid, "", text, "", epoch, 0, "", "-1", true, 0);
    QSharedPointer<ChatMessage> ptr(msg);
    emit databaseAddition(ptr);
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

    auto service = Utils::protocolToRTCOMServiceID(m_protocol_name);
    auto *msg = new ChatMessage(1, service, "", backend_name, remote_uid, remote_uid, "", text, "", epoch, 0, "", "-1", false, 0);
    QSharedPointer<ChatMessage> ptr(msg);
    emit databaseAddition(ptr);
}

void TelepathyAccount::onOnline(bool online) {
    qDebug() << "onOnline: " << online;
}

void TelepathyAccount::onAccReady(Tp::PendingOperation *op) {
    qDebug() << "onAccReady, isError:" << op->isError();
    qDebug() << "onAccReady, connection:" << acc->connection();
    qDebug() << "Display name:" << acc->displayName();
    qDebug() << "connectsAutomatically" << acc->connectsAutomatically();
    qDebug() << "isOnline" << acc->isOnline();
    qDebug() << "serviceName" << acc->serviceName();

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
