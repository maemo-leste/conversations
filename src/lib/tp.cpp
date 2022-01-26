#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/tp.h"
#include "lib/utils.h"


Sender::Sender(QObject *parent) : QObject(parent)
{
  acc = Tp::Account::create(TP_QT_ACCOUNT_MANAGER_BUS_NAME, "/org/freedesktop/Telepathy/Account/idle/irc/dscbot0");
  //"/org/freedesktop/Telepathy/Account/ring/tel/ring");
  //"/org/freedesktop/Telepathy/Account/idle/irc/fremantle0");

  registrar = Tp::ClientRegistrar::create();
  Tp::AbstractClientPtr handler = Tp::AbstractClientPtr::dynamicCast(
      Tp::SharedPtr<MyHandler>(new MyHandler(
          Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::textChat())));
  registrar->registerClient(handler, "myhandler");

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

void Sender::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel)
{
  qDebug() << "onMessageSent" << message.text();

  auto self_name = acc->nickname().toLocal8Bit();
  auto backend_name = (acc->cmName() + "/" + acc->protocolName() + "/" + acc->displayName()).toLocal8Bit();

  auto remote_uid = channel->targetContact()->id().toLocal8Bit();
  auto text = message.text().toLocal8Bit();

  // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's
  // do it for now
  create_event(message.sent().toTime_t(), self_name.data(), backend_name.data(), remote_uid, remote_uid, text, true, true);
}

void Sender::onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel)
{
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

  // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's
  // do it for now
  create_event(message.received().toTime_t(), self_name.data(), backend_name.data(), remote_uid, remote_uid, text, false, true);

  channel->send("echo " + message.text());

}

void Sender::onOnline(bool online)
{
  qDebug() << "onOnline: " << online;

  if (online) {
    //connect((Tp::PendingOperation*)acc->connection()->lowlevel()->requestConnect(),
//    connect((Tp::PendingOperation*)acc->connection()->becomeReady(),
//            SIGNAL(finished(Tp::PendingOperation*)),
//            SLOT(onConnectionReady(Tp::PendingOperation*)));
  }
}

void Sender::onAccReady(Tp::PendingOperation *op)
{
  qDebug() << "onAccReady, isError:" << op->isError();
  qDebug() << "onAccReady, connection:" << acc->connection();
  qDebug() << "Display name:" << acc->displayName();
  qDebug() << "connectsAutomatically" << acc->connectsAutomatically();
  qDebug() << "isOnline" << acc->isOnline();

  connect((Tp::PendingOperation*)acc->setConnectsAutomatically(true),
          SIGNAL(finished(Tp::PendingOperation*)),
          SLOT(onAutoConnectSet(Tp::PendingOperation*)));

  //qDebug() << "currentPresence" << acc->currentPresence();
  //connect((Tp::PendingOperation*)acc->setRequestedPresence(Tp::Presence::available()),
}

void Sender::onAutoConnectSet(Tp::PendingOperation *op) {
  qDebug() << "onAutoConnectSet, isError:" << op->isError();

  connect((Tp::PendingOperation*)acc->setAutomaticPresence(Tp::Presence::available()),
          SIGNAL(finished(Tp::PendingOperation*)),
          SLOT(onPresence(Tp::PendingOperation*)));
}


void Sender::onPresence(Tp::PendingOperation *op) {
  qDebug() << "onPresence, isError:" << op->isError();

  qDebug() << "onPresence, connection:" << acc->connection();

  acc->reconnect(); // Let's not check the result for now
}

void Sender::onConnectionReady(Tp::PendingOperation *op)
{
  qDebug() << "onConnectionReady, isError:" << op->isError();

  auto conn = ((Tp::PendingConnection*)op)->connection();

  qDebug() << "connection status:" << conn->status();
  auto low_level_conn = conn->lowlevel();

  qDebug() << "lowLevel conn: " << low_level_conn;
  auto pending = low_level_conn->requestHandles(Tp::HandleTypeContact, QStringList{"dsc_"});
  //auto pending = low_level_conn->requestHandles(Tp::HandleTypeContact, QStringList{"wizzup", "dsc_"});

  connect((Tp::PendingOperation*)pending,
          SIGNAL(finished(Tp::PendingOperation*)),
          SLOT(onHandles(Tp::PendingOperation*)));
}

void Sender::onHandles(Tp::PendingOperation *op) {
  qDebug() << "onHandles, isError:" << op->isError();

  auto opera = ((Tp::PendingHandles*)op);

  Tp::ReferencedHandles handles = opera->handles();

  qDebug() << "validNames:" << opera->validNames();
  qDebug() << "handles toList:" << handles.toList();

  connect(acc->connection()->contactManager()->contactsForHandles(handles),
          SIGNAL(finished(Tp::PendingOperation*)),
          SLOT(onContacts(Tp::PendingOperation*)));
}


void Sender::onContacts(Tp::PendingOperation *op) {
  qDebug() << "onContacts, isError:" << op->isError();
  if (op->isError()) {
    qDebug() << op->errorName() << op->errorMessage();
  }

  Tp::PendingContacts* pending = (Tp::PendingContacts*)op;
  auto contacts = pending->contacts();
  auto conn = contacts.value(0)->manager()->connection();
  auto contact = contacts.value(0);

  qDebug() << "contact id 0: " << contact->id();

  connect(acc->ensureTextChat(contact->id()),
          SIGNAL(finished(Tp::PendingOperation*)),
          SLOT(onChannel(Tp::PendingOperation*)));

  //connect(acc->ensureTextChatroom("#hetgrotebos"),
  //        SIGNAL(finished(Tp::PendingOperation*)),
  //        SLOT(onChannelGroup(Tp::PendingOperation*)));
}

void Sender::onChannel(Tp::PendingOperation *op) {
  qDebug() << "onChannel, isError:" << op->isError();
  if (op->isError()) {
    qDebug() << op->errorName() << op->errorMessage();
  }

  Tp::PendingChannelRequest* pending = (Tp::PendingChannelRequest*)op;
  auto chanrequest = pending->channelRequest();

  //wizzupchan = (Tp::TextChannelPtr)chanrequest->channel().data();
  auto wizzupchan = (Tp::TextChannel*)chanrequest->channel().data();

  qDebug() << "Sending to channel" << wizzupchan;

  wizzupchan->send("Telepathy decided to send you this message without you initiating anything");
  //wizzupchan->send("Telepathy kan nu ook hallo zeggen zonder napraten");
}

void Sender::onChannelGroup(Tp::PendingOperation *op) {
  qDebug() << "onChannel, isError:" << op->isError();
  if (op->isError()) {
    qDebug() << op->errorName() << op->errorMessage();
  }

  Tp::PendingChannelRequest* pending = (Tp::PendingChannelRequest*)op;
  auto chanrequest = pending->channelRequest();

  //wizzupchan = (Tp::TextChannelPtr)chanrequest->channel().data();
  hgbchan = (Tp::TextChannel*)chanrequest->channel().data();

  //qDebug() << "Sending to channel" << wizzupchan;

  //wizzupchan->send("Nog een test");
  //wizzupchan->send("Telepathy kan nu ook hallo zeggen zonder napraten");
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