#pragma once

#include <QtCore>
#include <QUrl>
#include <QLocalServer>
#include <QtDBus/QtDBus>

// TODO: clean these up!
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
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/SimpleObserver>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/SimpleTextObserver>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingChannel>
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
#include <TelepathyQt/Account>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ContactMessenger>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingSendMessage>
#include <TelepathyQt/ChannelRequestHints>
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
#include "lib/config.h"
#include "lib/state.h"
#include "models/ChatMessage.h"

#include "lib/abook/abook_public.h"
#include "lib/rtcom/rtcom_public.h"
// #include <rtcom-eventlogger/eventlogger.h>

// needed to convert to Variant's
Q_DECLARE_METATYPE(Tp::AccountSetPtr);
Q_DECLARE_METATYPE(Tp::AccountPtr)
Q_DECLARE_METATYPE(Tp::AccountManagerPtr);
Q_DECLARE_METATYPE(Tp::ConnectionPtr);
Q_DECLARE_METATYPE(Tp::TextChannelPtr);
Q_DECLARE_METATYPE(Tp::ChannelPtr);

class Telepathy;
class TelepathyAccount;
class TelepathyChannel;
class TelepathyHandler;

typedef QSharedPointer<TelepathyChannel> TelepathyChannelPtr;
typedef QSharedPointer<TelepathyAccount> TelepathyAccountPtr;

class TelepathyAccount : public QObject {
  Q_OBJECT

public:
  explicit TelepathyAccount(Tp::AccountPtr macc, QObject *parent = nullptr);
  ~TelepathyAccount() override;

  const Tp::AccountPtr acc;
  QMap<QString, TelepathyChannelPtr> channels;

  const QString local_uid;

  QString name() const {
    // human-readable from local_uid
    auto decoded = QUrl::fromPercentEncoding(QString(local_uid).replace("_", "%").toUtf8());
    if (decoded.endsWith("0"))
      decoded.chop(1);
    return decoded;
  }

  QString nickname() const { return m_nickname; }
  QString protocolName() const { return m_protocol_name; }
  QString getServiceName();
  QString getServerHost();
  unsigned int getServerPort();

  bool isOnline = false;
  bool has_feature_friends() const { return m_feature_friends; };

signals:
  void databaseAddition(QSharedPointer<ChatMessage> &msg);
  void openChannelWindow(const QString &local_uid, const QString &remote_uid, const QString &group_uid,
                         const QString &service, const QString &channel);
  void removed(TelepathyAccount *account);
  void channelJoined(QString local_uid, QString remote_uid);
  void channelLeft(QString local_uid, QString remote_uid);
  void accountReady(TelepathyAccount *account);
  void errorMessage(const QString &errorMsg);
  void onlinenessChanged(bool online);

public slots:
  void sendMessage(QString remote_uid, const QString &message);
  void joinChannel(const QString &remote_uid);
  void leaveChannel(const QString &remote_uid);
  void removeChannel(const QString &remote_uid);
  void setChatState(const QString &remote_uid, Tp::ChannelChatState state);
  void onOnline(bool online);

  /* TODO: make these private again and use connect/emit */
  void onMessageReceived(const Tp::ReceivedMessage &message, const Tp::TextChannelPtr &channel);
  void onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags, const QString &sentMessageToken,
                     const Tp::TextChannelPtr &channel);

  void TpOpenChannelWindow(Tp::TextChannelPtr channel);
  void onConnectionChanged(const Tp::ConnectionPtr &conn);
  void onConnectionReady(Tp::PendingOperation *op);

  TelepathyChannelPtr hasChannel(const QString &remote_uid);

  QSharedPointer<ChatMessage> log_event(time_t epoch, const QString &text, bool outgoing,
                                        const Tp::TextChannelPtr &channel, const QString &remote_uid,
                                        const QString &remote_alias, unsigned int flags = 0);

  static QString getRemoteUid(Tp::TextChannelPtr channel);
  QString getGroupUid(const TelepathyChannelPtr &channel);
  QString getRoomName(const TelepathyChannelPtr &channel);
  QString getGroupUid(Tp::TextChannelPtr channel) const;
  QString getLocalUid() const;

private slots:
  void onAccReady(Tp::PendingOperation *op);
  void onChannelJoined(const Tp::ChannelRequestPtr &channelRequest, const QString &channel);
  void onChannelJoinedOrLeft(bool joined, QString channel);
  void onRemoved(void);

private:
  QString m_nickname;
  QString m_protocol_name;
  bool m_feature_friends = false;

  Tp::ConnectionPtr m_connection;

  Tp::SimpleTextObserverPtr textobserver;
  Tp::SimpleObserverPtr channelobserver;
  Tp::ContactMessengerPtr messenger;
  Tp::TextChannel *hgbchan;
  Tp::AccountManagerPtr m_accountmanager;

  Telepathy *m_parent;
  void _joinChannel(const QString &channel, bool auto_join = false);
  void joinSavedGroupChats();
  void configRead();
};

class TelepathyChannel : public QObject {
  Q_OBJECT

public:
  explicit TelepathyChannel(const QString &remote_uid, TelepathyAccountPtr accountPtr, const Tp::ChannelPtr channelPtr,
                            Tp::HandleType handleType);
  ~TelepathyChannel() override;

  QString remote_uid;
  QString room_name;
  Tp::HandleType handleType;
  bool isRoom;

  void setChannelPtr(Tp::ChannelPtr channel);

public slots:
  void sendMessage(const QString &message) const;

private slots:
  void onGroupAddContacts(Tp::PendingOperation *op);
  void onChannelReady(Tp::PendingOperation *op);

  void onChanMessageReceived(const Tp::ReceivedMessage &message);
  void onChanPendingMessageRemoved(const Tp::ReceivedMessage &message);
  void onChanMessageSent(const Tp::Message &, Tp::MessageSendingFlags, const QString &) const;
  // sendChannelMessage (if we cannot just use sendMessage)

public:
  TelepathyAccountPtr m_account;
  Tp::ChannelPtr m_channel;
};


class Telepathy : public QObject {
  Q_OBJECT

public:
  explicit Telepathy(QObject *parent = nullptr);
  void init();
  ~Telepathy() override;

  QList<TelepathyAccountPtr> accounts;

  TelepathyAccountPtr accountByName(const QString &local_uid);
  TelepathyAccountPtr accountByPtr(const Tp::AccountPtr &ptr);
  TelepathyChannelPtr channelByName(const QString &local_uid, const QString &remote_uid);
  void joinChannel(const QString &local_uid, const QString &remote_uid);
  void leaveChannel(const QString &local_uid, const QString &remote_uid);
  bool deleteChannel(const QString &local_uid, const QString &remote_uid);

  void getContact(QString local_uid, QString remote_uid, std::function<void(Tp::ContactPtr)> cb);
  void authorizeContact(const QString &local_uid, const QString &remote_uid);
  void denyContact(const QString &local_uid, const QString &remote_uid);
  void removeContact(const QString &local_uid, const QString &remote_uid);
  void blockContact(const QString &local_uid, const QString &remote_uid, bool block = true);
  bool has_feature_friends(const QString &local_uid);
  void onRosterChanged();

  TelepathyAccountPtr rtcomLocalUidToAccount(const QString &local_uid);

signals:
  void messageFlagsChanged(unsigned int event_id, unsigned int flag);
  void databaseAddition(QSharedPointer<ChatMessage> &msg);
  void openChannelWindow(const QString &local_uid, const QString &remote_uid, const QString &group_uid,
                         const QString &service, const QString &channel);
  void accountManagerReady();
  void accountAdded(TelepathyAccountPtr account);
  void accountReady(TelepathyAccountPtr account);
  void accountRemoved();
  void channelJoined(QString local_uid, QString channel);
  void channelLeft(QString local_uid, QString channel);
  void channelDeleted(QString local_uid, QString channel);
  void errorMessage(const QString &errorMsg);
  void contactsChanged();
  void rosterChanged();
  void onlinenessChanged(TelepathyAccountPtr account, bool online);

public slots:
  void sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message);
  void setChatState(const QString &local_uid, const QString &remote_uid, Tp::ChannelChatState state);

  void onDatabaseAddition(QSharedPointer<ChatMessage> &msg);
  void onOpenChannelWindow(const QString &local_uid, const QString &remote_uid, const QString &group_uid,
                           const QString &service, const QString &channel);

  void onNewAccount(const Tp::AccountPtr &account);
  void onAccountRemoved(const QString &local_uid);
  void onSetAutoJoin(const QString &local_uid, const QString &remote_uid, bool auto_join);

private slots:
  void onAccountManagerReady(Tp::PendingOperation *op);

private:
  Tp::ClientRegistrarPtr registrar;
  Tp::AccountManagerPtr m_accountmanager;
  Tp::AbstractClientPtr clienthandler;
};


class TelepathyHandler : public Tp::AbstractClientHandler {
public:
  TelepathyHandler(const Tp::ChannelClassSpecList &channelFilter);

  ~TelepathyHandler() {
  }

  bool bypassApproval() const;
  void handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                      const Tp::AccountPtr &account,
                      const Tp::ConnectionPtr &connection,
                      const QList<Tp::ChannelPtr> &channels,
                      const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                      const QDateTime &userActionTime,
                      const Tp::AbstractClientHandler::HandlerInfo &handlerInfo);

  void setTelepathyParent(Telepathy *parent);

public:
  Telepathy *m_tp;
};
