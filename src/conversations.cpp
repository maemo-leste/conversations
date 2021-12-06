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

Conversations::Conversations(QCommandLineParser *cmdargs) {
  this->cmdargs = cmdargs;

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

  chatOverviewModel = new ChatModel();
  this->chatOverviewModel->getOverviewMessages();
}

void Conversations::onSendMessage(const QString &message) {
//  auto _msg = message.toUtf8();
//  auto date_t = QDateTime::currentDateTime();
//
//  auto *chat_obj = new ChatMessage("_self", date_t, _msg);
//  chatModel->appendMessage(chat_obj);
}

void Conversations::setWindowTitle(const QString &title) {
  emit setTitle(title);
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

Conversations::~Conversations() {}
