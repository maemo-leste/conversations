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

Conversations::Conversations(QCommandLineParser *cmdargs, IPC *ipc) {
  Notification::init(QApplication::applicationName());

  this->telepathy =new Telepathy(this);
  this->overviewServiceModel = new OverviewServiceModel(this);

  this->cmdargs = cmdargs;

  this->ipc = ipc;
  connect(ipc, &IPC::commandReceived, this, &Conversations::onIPCReceived);

  // Paths
  pathGenericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  configRoot = QDir::homePath();
  accountName = qgetenv("USER");
  homeDir = QDir::homePath();
  configDirectory = QString("%1/.config/%2/").arg(configRoot, QCoreApplication::applicationName());
  createConfigDirectory(configDirectory);

  m_textScaling = config()->get(ConfigKeys::TextScaling).toFloat();

  Tp::registerTypes();
#ifdef DEBUG
  Tp::enableDebug(true);
#else
  Tp::enableDebug(false);
#endif
  Tp::enableWarnings(true);

  theme= new HildonTheme();
  qDebug() << "THEME: " << theme->name;
  chatOverviewModel = new ChatModel();

  this->chatOverviewModel->onGetOverviewMessages();

  connect(telepathy, &Telepathy::databaseAddition, this, &Conversations::onDatabaseAddition);
  connect(overviewServiceModel, &OverviewServiceModel::protocolFilterChanged, chatOverviewModel, &ChatModel::onProtocolFilter);

  inheritSystemTheme = config()->get(ConfigKeys::EnableInheritSystemTheme).toBool();
  emit inheritSystemThemeChanged(inheritSystemTheme);

  displayGroupchatJoinLeave = config()->get(ConfigKeys::EnableDisplayGroupchatJoinLeave).toBool();
  emit displayGroupchatJoinLeaveChanged(displayGroupchatJoinLeave);
}

void Conversations::onDatabaseAddition(const QSharedPointer<ChatMessage> &msg) {
  this->chatOverviewModel->onGetOverviewMessages();

  // chat message notification
  auto notificationsEnabled = config()->get(ConfigKeys::EnableNotifications).toBool();
  if(isBackground && notificationsEnabled && !notificationMap.contains(msg->group_uid())) {
    notificationMap[msg->group_uid()] = msg;

    auto title = QString("Conversations - New message from %1").arg(msg->remote_name());
    auto *notification = Utils::notification(title, msg->textSnippet(), msg);
    connect(notification, &Notification::clicked, this, &Conversations::onNotificationClicked);
  }
}

void Conversations::onNotificationClicked(const QSharedPointer<ChatMessage> &msg) {
  if(!isBackground) return;
  emit notificationClicked(msg);
}

void Conversations::onMessageRead(const int event_id) {
  const gchar* flags = 0;  // @TODO: how to set the 'read' flag?
  //qtrtcom::setFlag(event_id, flags);
}

void Conversations::onIPCReceived(const QString &cmd) {
  if(cmd.contains(globals::reRemoteUID)) {
    emit showApplication();
    emit openChatWindow(cmd);
  } else if(cmd == "makeActive") {
    emit showApplication();
  }
}

void Conversations::onSendOutgoingMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
  telepathy->sendMessage(local_uid, remote_uid, message);
}

void Conversations::setWindowTitle(const QString &title) {
  emit setTitle(title);
}

QString Conversations::ossoIconLookup(const QString &filename) {
  if(ossoIconCache.contains(filename)) return ossoIconCache[filename];

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
  m_textScaling = config()->get(ConfigKeys::TextScaling).toFloat();
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

Conversations::~Conversations() {

}
