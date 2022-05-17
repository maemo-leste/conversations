#ifndef CONVTP_H
#define CONVTP_H

#include <QtCore>
#include <QLocalServer>
#include <QtDBus/QtDBus>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ContactMessenger>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingSendMessage>
#include <TelepathyQt/SimpleTextObserver>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Types>
#include <TelepathyQt/ReceivedMessage>
#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Account>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ContactMessenger>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingSendMessage>
#include <TelepathyQt/SimpleTextObserver>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/Types>

#include "lib/utils.h"
#include "models/ChatMessage.h"
#ifdef RTCOM
#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>
#endif


class MyHandler : public Tp::AbstractClientHandler
{
public:
  MyHandler(const Tp::ChannelClassSpecList &channelFilter);
  ~MyHandler() { }
  bool bypassApproval() const;
  void handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                      const Tp::AccountPtr &account,
                      const Tp::ConnectionPtr &connection,
                      const QList<Tp::ChannelPtr> &channels,
                      const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                      const QDateTime &userActionTime,
                      const Tp::AbstractClientHandler::HandlerInfo &handlerInfo);
};


class MyAccount : public QObject
{
Q_OBJECT

public:
  explicit MyAccount(Tp::AccountPtr macc);
  ~MyAccount() override;

signals:
  void databaseAddition(ChatMessage *msg);

public slots:
  void sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message);

private slots:
  void onOnline(bool online);
  void onAccReady(Tp::PendingOperation *op);
  //void onHandles(Tp::PendingOperation *op);
  //void onContacts(Tp::PendingOperation *op);
  //void onChannel(Tp::PendingOperation *op);
  //void onChannelGroup(Tp::PendingOperation *op);

  void onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);
  void onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken, const Tp::TextChannelPtr &channel);

public:
  Tp::AccountPtr acc;

private:
  Tp::SimpleTextObserverPtr observer;
  Tp::ContactMessengerPtr messenger;
  Tp::TextChannel *hgbchan;
  Tp::AccountManagerPtr m_accountmanager;

  Tp::AbstractClientPtr clienthandler;
};


class Sender : public QObject
{
Q_OBJECT

public:
  explicit Sender(QObject *parent = nullptr);
  ~Sender() override;

signals:
  void databaseAddition(ChatMessage *msg);

public slots:
  void sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message);
  void onDatabaseAddition(ChatMessage *msg);

private slots:
  void onAccountManagerReady(Tp::PendingOperation *op);

private:
  QList<MyAccount*> accs;

  Tp::ClientRegistrarPtr registrar;
  Tp::AccountManagerPtr m_accountmanager;
  Tp::AbstractClientPtr clienthandler;
};


#endif // CONVTP_H
