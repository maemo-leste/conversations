#include <QObject>
#include <QDir>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QCommandLineParser>
#include <QStandardPaths>

#include "conversations.h"
#include "lib/utils.h"
#include "lib/globals.h"
#include "mainwindow.h"

Conversations* CTX = nullptr;

Conversations::Conversations(QCommandLineParser *cmdargs, IPC *ipc) {
  CTX = this;
  Notification::init(QApplication::applicationName());

  // Paths
  pathGenericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  configRoot = QDir::homePath();
  accountName = qgetenv("USER");
  homeDir = QDir::homePath();
  configDirectory = QString("%1/.config/%2/").arg(configRoot, QCoreApplication::applicationName());
  createConfigDirectory(configDirectory);

  this->avatarProvider = new AvatarImageProvider;
  this->telepathy = new Telepathy(this);
  this->state = new ConfigState(QString("%1state.json").arg(configDirectory));
  configState = this->state;
  connect(this->state, &ConfigState::autoJoinChanged, this->telepathy, &Telepathy::onSetAutoJoin);

  this->displayAvatars = config()->get(ConfigKeys::EnableDisplayAvatars).toBool();
  this->displayGroupchatJoinLeave = config()->get(ConfigKeys::EnableDisplayGroupchatJoinLeave).toBool();

  this->cmdargs = cmdargs;

  this->ipc = ipc;
  connect(ipc, &IPC::commandReceived, this, &Conversations::onIPCReceived);

  // chat overview models
  overviewModel = new OverviewModel(this->telepathy, this->state, this);
  overviewModel->onLoad();
  connect(this->telepathy, &Telepathy::accountManagerReady, this->overviewModel, &OverviewModel::onLoad);
  connect(this->telepathy, &Telepathy::channelDeleted, this->state, &ConfigState::deleteItem);
  connect(this->state, &ConfigState::updated, this->overviewModel, &OverviewModel::onLoad);
  // update overview table on contact presence status changes
  connect(this->telepathy, &Telepathy::rosterChanged, this->overviewModel, &OverviewModel::onLoad);

  // Overview table updates
  connect(telepathy, &Telepathy::databaseAddition, overviewModel, &OverviewModel::onDatabaseAddition);
  connect(telepathy, &Telepathy::channelJoined, [this](QString local_uid, QString remote_uid) {
    overviewModel->onLoad();
  });
  connect(telepathy, &Telepathy::channelLeft, [this](QString local_uid, QString remote_uid) {
    overviewModel->onLoad();
  });
  connect(telepathy, &Telepathy::channelDeleted, [this](QString local_uid, QString remote_uid) {
    overviewModel->onLoad();
  });

  overviewProxyModel = new OverviewProxyModel(this);
  overviewProxyModel->setSourceModel(overviewModel);
  overviewProxyModel->setSortRole(OverviewModel::TimeRole);
  overviewProxyModel->sort(OverviewModel::TimeRole, Qt::DescendingOrder);
  overviewProxyModel->setDynamicSortFilter(true);

  textScaling = config()->get(ConfigKeys::TextScaling).toFloat();

  Tp::registerTypes();

  theme= new HildonTheme();
  qDebug() << "THEME: " << theme->name;
  chatOverviewModel = new ChatModel();

  connect(telepathy, &Telepathy::databaseAddition, this, &Conversations::onDatabaseAddition);

  inheritSystemTheme = config()->get(ConfigKeys::EnableInheritSystemTheme).toBool();
  emit inheritSystemThemeChanged(inheritSystemTheme);

  displayGroupchatJoinLeave = config()->get(ConfigKeys::EnableDisplayGroupchatJoinLeave).toBool();
  emit displayGroupchatJoinLeaveChanged(displayGroupchatJoinLeave);

  this->onGetAvailableServiceAccounts();

  // abook signals -> qt signals
  abook_qt::func_contactsChangedSignal =
    std::bind(&Conversations::onContactsChanged, this, std::placeholders::_1);
  abook_qt::func_avatarChangedSignal =
    std::bind(&Conversations::onAvatarChanged, this, std::placeholders::_1, std::placeholders::_2);

  // save config whilst quiting
  connect(qApp, &QCoreApplication::aboutToQuit, config(), &Config::sync);
}

// get a list of 'service accounts' (AKA protocols) from both TP and rtcom
void Conversations::onGetAvailableServiceAccounts() {
  serviceAccounts.clear();

  // rtcom
  for (const auto&item: rtcom_qt::get_service_accounts()) {
    ServiceAccount::fromRtComUID(QString::fromStdString(item));
  }

  // telepathy
  for(const auto &acc: this->telepathy->accounts) {
    auto *sa = ServiceAccount::fromTpProtocol(acc->getLocalUid(), acc->protocolName());
    QSharedPointer<ServiceAccount> ptr(sa);
    serviceAccounts << ptr;
  }
}

void Conversations::onDatabaseAddition(const QSharedPointer<ChatMessage> &msg) {
  // chat message notification
  auto notificationsEnabled = config()->get(ConfigKeys::EnableNotifications).toBool();
  if (notificationsEnabled && !msg->outgoing()) {
    auto uid = msg->protocol() + msg->group_uid();
    auto title = QString("Message from %1").arg(msg->remote_name());
    const auto body = msg->textSnippet();

    // SMS always gets a notification
    if (Utils::protocolIsTelephone(msg->protocol()))
      return this->OSnotify(title, body, msg);

    // No notification when this chat window is already opened, and in focus
    const QWidget *chatWindow = MainWindow::getChatWindow(msg->group_uid());
    if (chatWindow && chatWindow->isActiveWindow())
      return;

    // A message from a specific sender may only notify once per 30 sec
    if (!notificationMap.contains(uid)) {
      notificationMap[uid] = static_cast<uint64_t>(QDateTime::currentSecsSinceEpoch());
      return this->OSnotify(title, body, msg);
    }

    const auto now = static_cast<uint64_t>(QDateTime::currentSecsSinceEpoch());
    if (now - notificationMap[uid] >= 30) {
      notificationMap[uid] = static_cast<uint64_t>(QDateTime::currentSecsSinceEpoch());
      return this->OSnotify(title, body, msg);
    }
  }
}

void Conversations::OSnotify(const QString& title, const QString& message, const QSharedPointer<ChatMessage> &msg_obj) {
  const auto *notification = Notification::issue(title, message, msg_obj);
  connect(notification, &Notification::clicked, this, &Conversations::onNotificationClicked);
}

void Conversations::onNotificationClicked(const QSharedPointer<ChatMessage> &msg) {
  emit notificationClicked(msg);
}

void Conversations::onIPCReceived(const QString &cmd) {
  if(cmd.contains(globals::reRemoteUID)) {
    emit showApplication();
    emit openChatWindow(cmd);
  } else if(cmd == "makeActive") {
    emit showApplication();
  }
}

void Conversations::onAvatarChanged(std::string local_uid, std::string remote_uid) {
  emit avatarChanged(local_uid, remote_uid);
}

void Conversations::onContactsChanged(std::map<std::string, std::shared_ptr<AbookContact>> contacts) {
  emit contactsChanged(contacts);
}

void Conversations::onSendOutgoingMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
  telepathy->sendMessage(local_uid, remote_uid, message);
}

void Conversations::setWindowTitle(const QString &title) {
  emit setTitle(title);
}

#ifdef QUICK
void Conversations::singleShot(int msec, QJSValue callback) {
  QTimer::singleShot(msec, this, [callback] () mutable {
    if (callback.isCallable())
      callback.call();
  });
}
#endif

QString Conversations::ossoIconLookup(const QString &filename) {
  if(filename.isEmpty()) {
    qWarning() << "ossoIconLookup called with empty filename";
    return {};
  }

  auto fn = QString("/usr/share/icons/hicolor/48x48/hildon/%1").arg(filename);
  auto fn_qrc = QString("qrc:///%1").arg(filename);

  if(Utils::fileExists(fn)) {
    auto res = QString("file:///%1").arg(fn);
    ossoIconCache[filename] = res;
    return res;
  } else if(Utils::fileExists(fn_qrc)) {
    ossoIconCache[filename] = fn_qrc;
    return fn_qrc;
  }
  return QString("not_found_%1").arg(filename);
}

void Conversations::onTextScalingChanged() {
  textScaling = config()->get(ConfigKeys::TextScaling).toFloat();
  emit textScalingChanged();
}

void Conversations::createConfigDirectory(const QString &dir) {
  QStringList createDirs({dir});
  for(const auto &d: createDirs) {
    if(!Utils::dirExists(d)) {
      qDebug() << QString("Creating directory: %1").arg(d);
      if (!QDir().mkpath(d))
        throw std::runtime_error("Could not create directory " + d.toStdString());
    }
  }
}

void Conversations::onApplicationLog(QString msg) {}

Conversations::~Conversations() {

}
