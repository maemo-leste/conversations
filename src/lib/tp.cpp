#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/tp.h"
#include "lib/utils.h"


Sender::Sender(QObject *parent) : QObject(parent) {
  m_accountmanager = Tp::AccountManager::create(Tp::AccountFactory::create(QDBusConnection::sessionBus(), Tp::Account::FeatureCore));
  connect(m_accountmanager->becomeReady(), &Tp::PendingReady::finished, this, &Sender::onAccountManagerReady);

  registrar = Tp::ClientRegistrar::create();
  Tp::AbstractClientPtr handler = Tp::AbstractClientPtr::dynamicCast(
      Tp::SharedPtr<MyHandler>(new MyHandler(
          Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::textChat())));
  registrar->registerClient(handler, "myhandler");
}

void Sender::onAccountManagerReady(Tp::PendingOperation *op) {
  qDebug() << "onAccountManagerReady";

  auto validaccounts = m_accountmanager->validAccounts();
  auto l = validaccounts->accounts();

  qDebug() << "account count:" << l.count();

  Tp::AccountPtr acc;

  for (int i = 0; i < l.count(); i++) {
      acc = l[i];
      auto myacc = new MyAccount(acc);
      accs << myacc;

      connect(myacc, SIGNAL(databaseAddition(ChatMessage *)), SLOT(onDatabaseAddition(ChatMessage *)));
  }
}

void Sender::onDatabaseAddition(ChatMessage *msg) {
    emit databaseAddition(msg);
}

void Sender::sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
    qDebug() << local_uid << remote_uid;

    for (MyAccount *ma : accs) {
        auto acc = ma->acc;

        QByteArray backend_name = (acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName()).toLocal8Bit();

        if (backend_name == local_uid) {
            qDebug() << backend_name;
            ma->sendMessage(local_uid, remote_uid, message);
        }

    }
}



Sender::~Sender()
{
}


MyHandler::MyHandler(const Tp::ChannelClassSpecList &channelFilter)
    : Tp::AbstractClientHandler(channelFilter) { }

bool MyHandler::bypassApproval() const {
  return false;
}

void MyHandler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                               const Tp::AccountPtr &account,
                               const Tp::ConnectionPtr &connection,
                               const QList<Tp::ChannelPtr> &channels,
                               const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                               const QDateTime &userActionTime,
                               const Tp::AbstractClientHandler::HandlerInfo &handlerInfo)
{
  // do something
  context->setFinished();
}


MyAccount::MyAccount(Tp::AccountPtr macc) : QObject(nullptr) {
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


}

void MyAccount::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel) {
  qDebug() << "onMessageSent" << message.text();

  auto self_name = acc->nickname().toLocal8Bit();
  auto backend_name = (acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName()).toLocal8Bit();

  auto remote_uid = channel->targetContact()->id().toLocal8Bit();
  auto text = message.text().toLocal8Bit();

  // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's do it for now
  auto epoch = message.sent().toTime_t();
  create_event(epoch, self_name.data(), backend_name.data(), remote_uid, remote_uid, text, true, true);

  auto *item = new ChatMessage(1, "3", "", backend_name, remote_uid, remote_uid, "", text, "", epoch, 0, "", "11", true, 0);
  emit databaseAddition(item);
}

void MyAccount::onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel) {
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
  //QByteArray remote_uid = message.sender()->id().toLocal8Bit();
  QByteArray text = message.text().toLocal8Bit();

  // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's do it for now
  auto epoch = message.received().toTime_t();
  create_event(epoch, self_name.data(), backend_name.data(), remote_uid, remote_uid, text, false, true);

  auto *item = new ChatMessage(1, "3", "", backend_name, remote_uid, remote_uid, "", text, "", epoch, 0, "", "11", false, 0);
  emit databaseAddition(item);
}

void MyAccount::onOnline(bool online)
{
  qDebug() << "onOnline: " << online;

  if (online) {
  }
}

void MyAccount::onAccReady(Tp::PendingOperation *op)
{
  qDebug() << "onAccReady, isError:" << op->isError();
  qDebug() << "onAccReady, connection:" << acc->connection();
  qDebug() << "Display name:" << acc->displayName();
  qDebug() << "connectsAutomatically" << acc->connectsAutomatically();
  qDebug() << "isOnline" << acc->isOnline();

  // XXX: note that the account is ready, use that for some guards
}

// void MyAccount::onHandles(Tp::PendingOperation *op) {
//   qDebug() << "onHandles, isError:" << op->isError();
// 
//   auto opera = ((Tp::PendingHandles*)op);
// 
//   Tp::ReferencedHandles handles = opera->handles();
// 
//   qDebug() << "validNames:" << opera->validNames();
//   qDebug() << "handles toList:" << handles.toList();
// 
//   connect(acc->connection()->contactManager()->contactsForHandles(handles),
//           SIGNAL(finished(Tp::PendingOperation*)),
//           SLOT(onContacts(Tp::PendingOperation*)));
// }
// 
// void MyAccount::onContacts(Tp::PendingOperation *op) {
//   qDebug() << "onContacts, isError:" << op->isError();
//   if (op->isError()) {
//     qDebug() << op->errorName() << op->errorMessage();
//   }
// 
//   Tp::PendingContacts* pending = (Tp::PendingContacts*)op;
//   auto contacts = pending->contacts();
//   auto conn = contacts.value(0)->manager()->connection();
//   auto contact = contacts.value(0);
// 
//   qDebug() << "contact id 0: " << contact->id();
// 
//   //connect(acc->ensureTextChat(contact->id()),
//   //        SIGNAL(finished(Tp::PendingOperation*)),
//   //        SLOT(onChannel(Tp::PendingOperation*)));
// 
//   //connect(acc->ensureTextChatroom("#hetgrotebos"),
//   //        SIGNAL(finished(Tp::PendingOperation*)),
//   //        SLOT(onChannelGroup(Tp::PendingOperation*)));
// }
// 
// void MyAccount::onChannel(Tp::PendingOperation *op) {
//   qDebug() << "onChannel, isError:" << op->isError();
//   if (op->isError()) {
//     qDebug() << op->errorName() << op->errorMessage();
//   }
// 
//   Tp::PendingChannelRequest* pending = (Tp::PendingChannelRequest*)op;
//   auto chanrequest = pending->channelRequest();
// 
//   //wizzupchan = (Tp::TextChannelPtr)chanrequest->channel().data();
//   auto wizzupchan = (Tp::TextChannel*)chanrequest->channel().data();
// 
//   qDebug() << "Sending to channel" << wizzupchan;
// 
//   wizzupchan->send("Telepathy decided to send you this message without you initiating anything");
//   //wizzupchan->send("Telepathy kan nu ook hallo zeggen zonder napraten");
// }
// 
// void MyAccount::onChannelGroup(Tp::PendingOperation *op) {
//   qDebug() << "onChannel, isError:" << op->isError();
//   if (op->isError()) {
//     qDebug() << op->errorName() << op->errorMessage();
//   }
// 
//   Tp::PendingChannelRequest* pending = (Tp::PendingChannelRequest*)op;
//   auto chanrequest = pending->channelRequest();
// 
//   //wizzupchan = (Tp::TextChannelPtr)chanrequest->channel().data();
//   hgbchan = (Tp::TextChannel*)chanrequest->channel().data();
// 
//   //qDebug() << "Sending to channel" << wizzupchan;
// 
//   //wizzupchan->send("Nog een test");
//   //wizzupchan->send("Telepathy kan nu ook hallo zeggen zonder napraten");
// }

void MyAccount::sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
  auto *pending = acc->ensureTextChat(remote_uid);

  connect(pending, &Tp::PendingChannelRequest::finished, [=](Tp::PendingOperation *op){
    auto *_pending = (Tp::PendingChannelRequest*)op;
    auto chanrequest = _pending->channelRequest();
    auto lolchan = (Tp::TextChannel*)chanrequest->channel().data();
    lolchan->send(message);
  });
}

MyAccount::~MyAccount()
{
}
