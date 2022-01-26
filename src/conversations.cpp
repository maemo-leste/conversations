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

Conversations::Conversations(QCommandLineParser *cmdargs, IPC *ipc) :
      telepathy(new ConvTelepathy(this)) {
  this->cmdargs = cmdargs;

  this->ipc = ipc;
  connect(ipc, &IPC::commandReceived, this, &Conversations::onIPCReceived);

  // Paths
  pathGenericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  configRoot = QDir::homePath();
  accountName = qgetenv("USER");
  homeDir = QDir::homePath();
  configDirectory = QString("%1/.config/%2/").arg(configRoot, QCoreApplication::applicationName());

  // Create some directories
  createConfigDirectory(configDirectory);

  m_textScaling = config()->get(ConfigKeys::TextScaling).toFloat();

  if(this->isDebug) {
    qDebug() << "configRoot: " << configRoot;
    qDebug() << "homeDir: " << homeDir;
    qDebug() << "configDirectory: " << configDirectory;
  }

  Tp::registerTypes();
  Tp::enableDebug(true);
  Tp::enableWarnings(true);

  chatOverviewModel = new ChatModel();
  this->chatOverviewModel->onGetOverviewMessages();

//  connect(telepathy, &ConvTelepathy::databaseChanged, this->chatOverviewModel, [=]{
//    this->chatOverviewModel->onGetOverviewMessages();
//  });
}

void Conversations::onIPCReceived(const QString &cmd) {
  if(cmd.contains(globals::reRemoteUID)) {
    emit showApplication();
    emit openChatWindow(cmd);
  } else if(cmd == "makeActive") {
    emit showApplication();
  }
}

void Conversations::onSendOutgoingMessage(const QString &message) {
//  auto _msg = message.toUtf8();
//  auto date_t = QDateTime::currentDateTime();
//
//  auto *chat_obj = new ChatMessage("_self", date_t, _msg);
//  chatModel->appendMessage(chat_obj);
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
