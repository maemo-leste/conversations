#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/tp.h"
#include "lib/utils.h"

UserX::UserX(Tp::AccountPtr &acc, QObject *parent) : m_acc(acc), QObject(parent) {
  qDebug() << "============================================================";
  qDebug() << "============================================================";
  m_observer = Tp::SimpleTextObserver::create(m_acc);

  connect(m_observer.data(), &Tp::SimpleTextObserver::messageReceived, this, &UserX::onMessageReceived);

  auto *weg = acc->becomeReady();
  connect(weg, &Tp::PendingReady::finished, this, &UserX::onAccReady);

  auto *xz = m_acc.data();
  connect(xz, &Tp::Account::onlinenessChanged, this, &UserX::onOnline);
//  qDebug() << "currentPresence" << m_acc->currentPresence().status();
}

void UserX::onPresence(Tp::PendingOperation *op) {
  qDebug() << "onPresence, isError:" << op->isError();
  qDebug() << "onPresence, connection:" << m_acc->connection();
  m_acc->reconnect(); // Let's not check the result for now
}

void UserX::onOnline(bool online) {
  qDebug() << "onOnline: " << online;
  connect((Tp::PendingOperation*)m_acc->connection()->becomeReady(),
          SIGNAL(finished(Tp::PendingOperation*)),
          SLOT(onConnectionReady(Tp::PendingOperation*)));
}

void UserX::onConnectionReady(Tp::PendingOperation *op) {
  qDebug() << "onConnectionReady, isError:" << op->isError();
}

void UserX::onAccReady(Tp::PendingOperation *op)
{
  qDebug() << "==== onm_accReady, isError:" << op->isError();
  qDebug() << "==== onm_accReady, connection:" << m_acc->connection();
  qDebug() << "==== Display name:" << m_acc->displayName();
  qDebug() << "==== connectsAutomatically" << m_acc->connectsAutomatically();
  qDebug() << "==== isOnline" << m_acc->isOnline();

  connect((Tp::PendingOperation*)m_acc->setAutomaticPresence(Tp::Presence::available()),
          SIGNAL(finished(Tp::PendingOperation*)),
          SLOT(onPresence(Tp::PendingOperation*)));
}

void UserX::onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel) {
  if (message.isDeliveryReport()) {
    // TODO: We do not want to reply to it not write anything for now, later we
    // want to update the rtcom db with the delivery report
    qDebug() << "nope";
    return;
  }
  qDebug() << "yay";

  auto self_name = m_acc->nickname().toLocal8Bit();
  auto backend_name = (m_acc->cmName() + "/" + m_acc->protocolName() + "/" + m_acc->displayName()).toLocal8Bit();
  auto remote_uid = message.senderNickname().toLocal8Bit();
  //QByteArray remote_uid = message.sender()->id().toLocal8Bit();
  auto text = message.text().toLocal8Bit();

  // TODO: remote_name != remote_uid, we shouldn't make them equal, but let's do it for now
//  create_event(message.received().toTime_t(), self_name.data(), backend_name.data(), remote_uid, remote_uid, text, false, true);
//  emit messageWritten();

  channel->send("echo " + message.text());
}

ConvTelepathy::ConvTelepathy(QObject *parent) : QObject(parent) {
  auto acc = Tp::AccountFactory::create(
      QDBusConnection::sessionBus(),
      Tp::Account::FeatureCore);

  m_registrar = Tp::ClientRegistrar::create();
  Tp::AbstractClientPtr handler = Tp::AbstractClientPtr::dynamicCast(
      Tp::SharedPtr<MyHandler>(new MyHandler(
          Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::textChat())));
  m_registrar->registerClient(handler, "myhandler");

  m_accountManager = Tp::AccountManager::create(acc);
  connect(m_accountManager->becomeReady(), &Tp::PendingReady::finished, this, &ConvTelepathy::onAccountManagerReady);
}

void ConvTelepathy::onAccountManagerReady(Tp::PendingOperation *op) {
  for (Tp::AccountPtr &acc: m_accountManager->allAccounts()) {
    qDebug() << acc->nickname();
    auto *z = new UserX(acc);
    m_users.append(z);
    connect(z, &UserX::messageWritten, this, &ConvTelepathy::databaseChanged);
  }
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