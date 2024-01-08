#undef signals
/*Work around issues with signals being defined earlier */
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include "lib/abook.h"

#include <QGuiApplication>
#include <QSslSocket>
#include <QResource>
#include <QtCore>

#include <unistd.h>
#include <sys/types.h>
#include "conversations.h"
#include "lib/globals.h"
#include "lib/ipc.h"
#include "lib/rtcom.h"
#include "conv-intl.h"
#include "config-conversations.h"
#include "mainwindow.h"

#ifdef Q_WS_MAEMO_5
#include <QDBusInterface>
#include <QDBusMessage>
#endif

#include <libosso-abook/osso-abook.h>

#include "rtcom-eventlogger/eventlogger-query.h"

int main(int argc, char *argv[]) {
  Q_INIT_RESOURCE(assets);
  Q_INIT_RESOURCE(whatsthat);
  Q_INIT_RESOURCE(chatty);
  Q_INIT_RESOURCE(irssi);

#ifdef DEBUG
  Tp::enableDebug(true);
  Tp::enableWarnings(true);

  // For remote debugging (QEMU) with CLion, the environment variables need
  // to be correctly set such that e.g. dbus will work. We can execute
  // this hack to dump the environ to a file, then reads it below.
  // This snippet runs when `-D CMAKE_DEBUG_TYPE=Debug`

  // /bin/sh -c 'nohup /tmp/tmp.fCTLrJeUgW/cmake-build-debug/bin/conversations >/dev/null 2>&1 &'; sleep 2; cat "/proc/`pidof conversations`/environ" | tr "\0" "\n" > /home/user/env.sh; kill -9 "`pidof conversations`"
  setuid(1000);
  QString path_env_file = "/home/user/env.sh";
  qDebug() << "trying to read ENV from" << path_env_file << ", if it exists";
  auto env_file = Utils::fileOpen(path_env_file);
  for(auto &line: Utils::barrayToString(env_file).split("\n")) {
    line = line.replace("export ", "");
    int pos = line.indexOf("=");
    auto key = line.left(pos);
    auto val = line.remove(0, pos + 1);

    if(val.startsWith("\""))
      val = val.mid(1);
    if(val.endsWith("\""))
      val = val.mid(0, val.length() - 1);

    if(val.isEmpty() || key.isEmpty()) continue;
    qputenv(key.toStdString().c_str(), val.toStdString().c_str());
  }
#endif

  intl("conversations-ui");
  qputenv("QML_DISABLE_DISK_CACHE", "1");

  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setApplicationName("conversations");
  QApplication::setOrganizationDomain("https://maemo-leste.github.io/");
  QApplication::setOrganizationName("Maemo Leste");
  QApplication::setQuitOnLastWindowClosed(false);  // allow conversations to operate in the background
  QApplication::setApplicationVersion(CONVERSATIONS_VERSION);

  QApplication app(argc, argv);
  app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

  QMap<QString, QString> info;
  info["Qt"] = QT_VERSION_STR;
  info["Conversations"] = CONVERSATIONS_VERSION;
#ifndef QT_NO_SSL
  info["SSL"] = QSslSocket::sslLibraryVersionString();
  info["SSL build"] = QSslSocket::sslLibraryBuildVersionString();
#endif

  for (const auto &k: info.keys())
    qWarning().nospace().noquote() << QString("%1: %2").arg(k, info[k]);

  hildon_init();

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
    auto res = IPC::send(ls, "makeActive");
    qInfo() << "an instance of conversations is already active, sending `makeActive` and exiting the current process";
    return res;
  }

  // Listen on IPC
  QTimer::singleShot(0, ipc, [=]{ ipc->bind(); });

  // initialize application
  auto *ctx = new Conversations(&parser, ipc);
  ctx->applicationPath = argv_.at(0);
  ctx->isDebug = debugMode;
#ifdef MAEMO
  ctx->isMaemo = true;
#endif
  /* TODO: check for failure */
  conv_abook_init();
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
