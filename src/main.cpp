#include <QGuiApplication>
#include <QSslSocket>
#include <QResource>
#include <QtCore>

#include "conversations.h"
#include "conv-intl.h"
#include "config-conversations.h"
#include "mainwindow.h"

#ifdef Q_WS_MAEMO_5
#include <QDBusInterface>
#include <QDBusMessage>
#endif

int main(int argc, char *argv[]) {
  Q_INIT_RESOURCE(assets);
  Q_INIT_RESOURCE(whatsthat);
  Q_INIT_RESOURCE(chatty);
#ifdef MAEMO
  Q_INIT_RESOURCE(assets_maemo);
#endif

  intl("conversations-ui");
  qputenv("QML_DISABLE_DISK_CACHE", "1");
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setApplicationName("conversations");
  QApplication::setOrganizationDomain("https://maemo-leste.github.io/");
  QApplication::setOrganizationName("Maemo Leste");
  QApplication::setApplicationVersion(CONVERSATIONS_VERSION);

  QApplication app(argc, argv);

  QMap<QString, QString> info;
  info["Qt"] = QT_VERSION_STR;
  info["Conversations"] = CONVERSATIONS_VERSION;
#ifndef QT_NO_SSL
  info["SSL"] = QSslSocket::sslLibraryVersionString();
  info["SSL build"] = QSslSocket::sslLibraryBuildVersionString();
#endif

  for (const auto &k: info.keys())
    qWarning().nospace().noquote() << QString("%1: %2").arg(k, info[k]);

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();
  parser.setApplicationDescription("Communications");

  QCommandLineOption debugModeOption(QStringList() << "debug", "Run program in debug mode.");
  parser.addOption(debugModeOption);

  QStringList argv_;
  for(int i = 0; i != argc; i++)
    argv_ << QString::fromStdString(argv[i]);

  auto parsed = parser.parse(argv_);
  if(!parsed) {
    qCritical() << parser.errorText();
    exit(1);
  }

  const QStringList args = parser.positionalArguments();
  bool debugMode = parser.isSet(debugModeOption);

  parser.process(app);
  auto *ctx = new Conversations(&parser);
  ctx->applicationPath = argv_.at(0);
  ctx->isDebug = debugMode;
#ifdef MAEMO
  ctx->isMaemo = true;
#endif
  auto *mainWindow = new MainWindow(ctx);
  return QApplication::exec();
}
