#ifndef CONVTP_H
#define CONVTP_H

#include <QtCore>
#include <QLocalServer>
#include <QtDBus/QtDBus>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
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
#ifdef RTCOM
#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>
#endif

class UserX : public QObject
{
  Q_OBJECT

public:
  explicit UserX(Tp::AccountPtr &acc, QObject *parent = nullptr);

public slots:
  void onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);
  void onAccReady(Tp::PendingOperation *op);
  void onOnline(bool online);
  void onConnectionReady(Tp::PendingOperation *op);
  void onPresence(Tp::PendingOperation *op);

signals:
  void messageWritten();

private:
  QString xx;
  Tp::AccountPtr m_acc;
  Tp::SimpleTextObserverPtr m_observer;
};

class ConvTelepathy : public QObject
{
  Q_OBJECT

public:
  explicit ConvTelepathy(QObject *parent = nullptr);

private slots:
  void onAccountManagerReady(Tp::PendingOperation *op);

signals:
  void databaseChanged();  // changes to rtcom occurred

private:
  Tp::ClientRegistrarPtr m_registrar;
  Tp::AccountManagerPtr m_accountManager;

  QList<UserX*> m_users;
};

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


#endif // CONVTP_H
