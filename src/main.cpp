#include "lib/abook/abook_public.h"

#include <QGuiApplication>
#include <QSslSocket>
#include <QResource>
#include <QtCore>

#include <unistd.h>
#include <sys/types.h>
#include "conversations.h"
#include "lib/globals.h"
#include "lib/ipc.h"
#include "lib/rtcom/rtcom_public.h"
#include "conv-intl.h"
#include "config-conversations.h"
#include "mainwindow.h"
#include "lib/utils.h"
#include "lib/clion_debug.h"

#ifdef Q_WS_MAEMO_5
#include <QDBusInterface>
#include <QDBusMessage>
#endif

#include <hildon/hildon-main.h>
#include "lib/logger.h"

int main(int argc, char *argv[]) {
  // are we already running?
  if(const int ipc_sock = Utils::IPCOpen("/tmp/conversations-user.sock"); ipc_sock >= 0) {
    while(*argv != nullptr) {  // remote-uid via args?
      if(auto arg = QString::fromUtf8(*argv, strlen(*argv)); arg.contains(globals::reRemoteUID)) {
        return Utils::IPCSend(ipc_sock, arg);
      }
      argv++;
    }

    qInfo() << "an instance of conversations is already active, sending `makeActive` and exiting the current process";
    return Utils::IPCSend(ipc_sock, "makeActive");
  }

#ifdef QUICK
  Q_INIT_RESOURCE(assets);
  Q_INIT_RESOURCE(whatsthat);
  Q_INIT_RESOURCE(chatty);
  Q_INIT_RESOURCE(irssi);
#endif

#ifdef DEBUG
  // Tp::enableDebug(true);
  // Tp::enableWarnings(true);
  // qputenv("G_MESSAGES_DEBUG", "all");
  // qputenv("OSSO_ABOOK_DEBUG", "all");
#else
  Tp::enableDebug(false);
  Tp::enableWarnings(false);
#endif

#ifdef DEBUG
  clion_debug_setup();
#endif

  intl("conversations-ui");

#ifdef DISABLE_QML_DISK_CACHE
  qputenv("QML_DISABLE_DISK_CACHE", "1");
#endif

  QApplication::setApplicationName("conversations");
  QApplication::setOrganizationDomain("https://maemo-leste.github.io/");
  QApplication::setOrganizationName("Maemo Leste");
  QApplication::setQuitOnLastWindowClosed(false);  // allow conversations to operate in the background
  QApplication::setApplicationVersion(CONVERSATIONS_VERSION);

  const QApplication app(argc, argv);

  // logging
  const QString logPath = "/tmp/conversations.log";
  logFile = new QFile(logPath);
  if(!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    qWarning() << QString("could not open logfile: %1").arg(logPath);

  qInstallMessageHandler(conversationsMessageHandler);
  app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
  app.setAttribute(Qt::AA_CompressTabletEvents);
  app.setAttribute(Qt::AA_CompressHighFrequencyEvents);

  // handle program arguments
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.setApplicationDescription("Communications");

  const QCommandLineOption backgroundModeOption(QStringList() << "background", "Start without spawning the GUI.");
  parser.addOption(backgroundModeOption);

  const QCommandLineOption debugModeOption(QStringList() << "debug", "Start in debug mode.");
  parser.addOption(debugModeOption);

  const QCommandLineOption versionOption(QStringList() << "version", "Print version.");
  parser.addOption(versionOption);

  QStringList argv_;
  for(int i = 0; i != argc; i++)
    argv_ << QString::fromStdString(argv[i]);

  if(const auto parsed = parser.parse(argv_); !parsed) {
    qCritical() << parser.errorText();
    exit(1);
  }

  const QStringList args = parser.positionalArguments();
  if(parser.isSet(versionOption)) {
    printf("%s\n", qPrintable(QString("conversations %1").arg(CONVERSATIONS_VERSION)));
    exit(0);
  }

  QMap<QString, QString> info;
  info["Qt"] = QT_VERSION_STR;
  info["Conversations"] = CONVERSATIONS_VERSION;
#ifndef QT_NO_SSL
  info["SSL"] = QSslSocket::sslLibraryVersionString();
  info["SSL build"] = QSslSocket::sslLibraryBuildVersionString();
#endif

  for (const auto &k: info.keys())
    qWarning().nospace().noquote() << QString("%1: %2").arg(k, info[k]);

  const bool debugMode = parser.isSet(debugModeOption);
  parser.process(app);

  // Listen on IPC
  auto *ipc = new IPC();
  QTimer::singleShot(0, ipc, [ipc]{ ipc->bind(); });

  // init hildon (is this required?)
  hildon_init();

  // init abook
  if (!abook_qt::abook_init())
    throw std::runtime_error("cannot initialize abook");
  abook_qt::abook_init_contact_roster();

  // initialize application
  auto *ctx = new Conversations(&parser, ipc);
  logger_ctx = ctx;
  ctx->applicationPath = argv_.at(0);
  ctx->isDebug = debugMode;
  ctx->isMaemo = true;  // @TODO: remove

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

  // load Telepathy last, ensures the app is
  // setup so we can act on all signals
  ctx->telepathy->init();

  return QApplication::exec();
}
