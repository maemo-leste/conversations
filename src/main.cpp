#include <QGuiApplication>
#include <QSslSocket>
#include <QResource>
#include <QtCore>

#include "conversations.h"
#include "lib/globals.h"
#include "lib/ipc.h"
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
  Q_INIT_RESOURCE(irssi);

  intl("conversations-ui");
  qputenv("QML_DISABLE_DISK_CACHE", "1");
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setApplicationName("conversations");
  QApplication::setOrganizationDomain("https://maemo-leste.github.io/");
  QApplication::setOrganizationName("Maemo Leste");
  QApplication::setQuitOnLastWindowClosed(false);  // allow conversations to operate in the background
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

  QCommandLineOption backgroundModeOption(QStringList() << "background", "Start without spawning the GUI.");
  parser.addOption(backgroundModeOption);

  QCommandLineOption debugModeOption(QStringList() << "debug", "Start in debug mode.");
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

  // check if conversations is already running
  auto *ipc = new IPC();
  QIODevice *ls = ipc->open();

  if(ls->isOpen()) {
    // pass argv[] to 1st instance and exit
    for (const auto &arg: args) {
      if (arg.isEmpty() || arg.length() >= 128) continue;
      if (arg.contains(globals::reRemoteUID)) {
        return IPC::send(ls, arg);
      }
    }
    return IPC::send(ls, "makeActive");
  }

  // Listen on IPC
  QTimer::singleShot(0, ipc, SLOT(bind()));

  // initialize application
  auto *ctx = new Conversations(&parser, ipc);
  ctx->applicationPath = argv_.at(0);
  ctx->isDebug = debugMode;
#ifdef MAEMO
  ctx->isMaemo = true;
#endif
  auto *mainWindow = new MainWindow(ctx);
  if(!parser.isSet(backgroundModeOption))
    mainWindow->onShowApplication();

  // handle positional startup arguments
  for (const auto &arg: args) {
    if(arg.contains(globals::reRemoteUID)) {
      ctx->onIPCReceived(arg);
      break;
    }
  }

  return QApplication::exec();
}
