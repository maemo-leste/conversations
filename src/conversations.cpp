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

Conversations::Conversations(QCommandLineParser *cmdargs, IPC *ipc) {
  Notification::init(QApplication::applicationName());

  this->telepathy =new Telepathy(this);
  this->cmdargs = cmdargs;

  this->ipc = ipc;
  connect(ipc, &IPC::commandReceived, this, &Conversations::onIPCReceived);

  // chat overview models
  overviewModel = new OverviewModel(this);
  overviewModel->onLoad();
  connect(telepathy, &Telepathy::accountManagerReady, overviewModel, &OverviewModel::onLoad);
  // @TODO: do not refresh the whole overview table on new messages, edit specific entries 
  // instead. For now though, it is not that important as it is still performant enough.
  connect(telepathy, &Telepathy::databaseAddition, overviewModel, &OverviewModel::onLoad);
  overviewProxyModel = new OverviewProxyModel(this);
  overviewProxyModel->setSourceModel(overviewModel);
  overviewProxyModel->setSortRole(OverviewModel::TimeRole);
  overviewProxyModel->sort(OverviewModel::TimeRole, Qt::DescendingOrder);
  overviewProxyModel->setDynamicSortFilter(true);

  // Paths
  pathGenericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  configRoot = QDir::homePath();
  accountName = qgetenv("USER");
  homeDir = QDir::homePath();
  configDirectory = QString("%1/.config/%2/").arg(configRoot, QCoreApplication::applicationName());
  createConfigDirectory(configDirectory);

  textScaling = config()->get(ConfigKeys::TextScaling).toFloat();

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

  connect(telepathy, &Telepathy::databaseAddition, this, &Conversations::onDatabaseAddition);

  inheritSystemTheme = config()->get(ConfigKeys::EnableInheritSystemTheme).toBool();
  emit inheritSystemThemeChanged(inheritSystemTheme);

  displayGroupchatJoinLeave = config()->get(ConfigKeys::EnableDisplayGroupchatJoinLeave).toBool();
  emit displayGroupchatJoinLeaveChanged(displayGroupchatJoinLeave);

  this->onGetAvailableServiceAccounts();
}

// get a list of 'service accounts' (AKA protocols) from both TP and rtcom
void Conversations::onGetAvailableServiceAccounts() {
  serviceAccounts.clear();

  RTComElQuery *query = qtrtcom::startQuery(0, 0, RTCOM_EL_QUERY_GROUP_BY_EVENTS_LOCAL_UID);
  if(!rtcom_el_query_prepare(query, NULL)) {
    qCritical() << "Could not prepare query";
    g_object_unref(query);
  } else {
    auto items = iterateRtComEvents(query);
    for(auto &item: items) {
      auto local_uid = item->local_uid();
      auto *sa = ServiceAccount::fromRtComUID(local_uid);
      QSharedPointer<ServiceAccount> ptr(sa);
      serviceAccounts << ptr;
    }
    qDeleteAll(items);
    g_object_unref(query);
  }

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
    QWidget *chatWindow = MainWindow::getChatWindow(msg->group_uid());

    if (chatWindow && chatWindow->isActiveWindow())
      return;

    if (!notificationMap.contains(msg->group_uid()))
      notificationMap[msg->group_uid()] = msg;

    auto title = QString("Message from %1").arg(msg->remote_name());
    auto *notification = Notification::issue(title, msg->textSnippet(), msg);
    connect(notification, &Notification::clicked, this, &Conversations::onNotificationClicked);
  }
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

void Conversations::onSendOutgoingMessage(const QString &local_uid, const QString &remote_uid, const QString &message) {
  telepathy->sendMessage(local_uid, remote_uid, message);
}

void Conversations::setWindowTitle(const QString &title) {
  emit setTitle(title);
}

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

Conversations::~Conversations() {

}
