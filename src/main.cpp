#include "lib/abook/abook_public.h"

#include <QGuiApplication>
#include <QSslSocket>
#include <QResource>
#include <QtCore>

#include <unistd.h>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <iostream>
#include <sys/types.h>
#include "conversations.h"
#include "lib/globals.h"
#include "lib/ipc.h"
#include "lib/config.h"
#include "lib/rtcom/rtcom_public.h"
#include "conv-intl.h"
#include "config-conversations.h"
#include "mainwindow.h"
#include "lib/utils.h"
#ifdef DEBUG
#include "lib/clion_debug.h"
#endif

#ifdef Q_WS_MAEMO_5
#include <QDBusInterface>
#include <QDBusMessage>
#endif

#include <hildon/hildon-main.h>
#include "lib/logger.h"
#include "lib/logger_std/logger_std.h"
#include "lib/utils_c.h"

#define PATH_CONV INSTALL_PREFIX_QUOTED "/bin/conversations_qml"
#define PATH_CONV_SLIM INSTALL_PREFIX_QUOTED "/bin/conversations_slim"

int main(int argc, char *argv[]) {
  // do not run twice
  const int qml_count = active_proc_count_by_path(PATH_CONV);
  const int slim_count = active_proc_count_by_path(PATH_CONV_SLIM);

  if (qml_count + slim_count != 0 && (qml_count + slim_count != 1 || qml_count > 1 || slim_count > 1)) {
    printf("already running? trying wakeup\n");
    const char *ipc_message = argc > 1 ? argv[1] : "makeActive";
    return ipc_try_wakeup(ipc_message);  // regardless, exit and refuse to run twice
  }

#ifdef ENABLE_DEBUG_TIMINGS
  globals::logger_std_init();
#endif
  CLOCK_MEASURE_START(start_total);
  CLOCK_MEASURE_START(start_pre);
  Q_INIT_RESOURCE(assets);
#ifdef QUICK
  CLOCK_MEASURE_START(start_init_qrc);
  Q_INIT_RESOURCE(whatsthat);
  Q_INIT_RESOURCE(chatty);
  Q_INIT_RESOURCE(irssi);
  CLOCK_MEASURE_END(start_init_qrc, "main::Q_INIT_RESOURCE");
#endif

#ifdef DEBUG
  clion_debug_setup();
#endif

  // ensure config directory exists
  const std::string home = std::getenv("HOME") ? std::getenv("HOME") : "/home/user";
  const std::filesystem::path configDir = std::filesystem::path(home) / ".config" / "conversations";
  std::filesystem::create_directories(configDir);

  // if these paths exist, we add additional logging
  const std::filesystem::path path_enable_glib_logging = configDir / ".log_glib";
  const std::filesystem::path path_enable_tp_logging = configDir / ".log_tp";
  if (std::filesystem::exists(path_enable_glib_logging)) {
    qputenv("G_MESSAGES_DEBUG", "all");
  }
  if (std::filesystem::exists(path_enable_tp_logging)) {
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
  } else {
    Tp::enableDebug(false);
    Tp::enableWarnings(false);
  }

  CLOCK_MEASURE_START(start_osso_intl);
  intl("conversations-ui");
  CLOCK_MEASURE_END(start_osso_intl, "main::osso-intl.h init");

  QApplication::setApplicationName("conversations");
  QApplication::setOrganizationDomain("https://maemo-leste.github.io/");
  QApplication::setOrganizationName("Maemo Leste");
  QApplication::setQuitOnLastWindowClosed(false);  // allow conversations to operate in the background
  QApplication::setApplicationVersion(CONVERSATIONS_VERSION);

  CLOCK_MEASURE_START(start_cfg);
  if(const auto gpu = config()->get(ConfigKeys::EnableGPUAccel).toBool(); !gpu)
    qputenv("QT_QUICK_BACKEND", "software");
  CLOCK_MEASURE_END(start_cfg, "main::config init");

#ifdef DISABLE_QML_DISK_CACHE
  qputenv("QML_DISABLE_DISK_CACHE", "1");
#endif

  CLOCK_MEASURE_START(start_create_app);
  const QApplication app(argc, argv);
  CLOCK_MEASURE_END(start_create_app, "main::create app");

  // logging
  CLOCK_MEASURE_START(start_tmp_logging);
  if (config()->get(ConfigKeys::EnableLogSyslog).toBool())
    conversations_logger::syslog_enabled = true;
  if (config()->get(ConfigKeys::EnableLogWrite).toBool()) {
    conversations_logger::logFile = new QFile(QString("%1/conversations.log").arg(QString::fromStdString(configDir)));
    if(!conversations_logger::logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
      qWarning() << QString("could not open logfile");
  }
  CLOCK_MEASURE_END(start_tmp_logging, "main::start_tmp_logging");

  CLOCK_MEASURE_START(start_log_handler);
  qInstallMessageHandler(conversations_logger::conversationsMessageHandler);
  app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
  app.setAttribute(Qt::AA_CompressTabletEvents);
  app.setAttribute(Qt::AA_CompressHighFrequencyEvents);
  CLOCK_MEASURE_END(start_log_handler, "main::start_log_handler");

  // handle program arguments
  CLOCK_MEASURE_START(start_arg_parse);
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

  for (const auto &k: info.keys())
    qWarning().nospace().noquote() << QString("%1: %2").arg(k, info[k]);

  const bool debugMode = parser.isSet(debugModeOption);
  parser.process(app);
  CLOCK_MEASURE_END(start_arg_parse, "main::arg parse");

  // Listen on IPC
  CLOCK_MEASURE_START(start_ipc_bind);
  auto *ipc = new IPC();
  QTimer::singleShot(0, ipc, [ipc]{ ipc->bind(); });
  CLOCK_MEASURE_END(start_ipc_bind, "main::ipc bind");
  CLOCK_MEASURE_END(start_pre, "main::pre");

  // init hildon (is this required?)
  CLOCK_MEASURE_START(start_hildon_init);
  hildon_gtk_init(&argc, &argv);
  CLOCK_MEASURE_END(start_hildon_init, "main::hildon_init");

  // initialize application
  CLOCK_MEASURE_START(start_ctx);
  auto *ctx = new Conversations(&parser, ipc);
  conversations_logger::logger_ctx = ctx;
  ctx->applicationPath = argv_.at(0);
  ctx->isDebug = debugMode;
  ctx->isMaemo = true;  // @TODO: remove
  CLOCK_MEASURE_END(start_ctx, "main::ctx_init");

  CLOCK_MEASURE_START(start_mainwindow);
  auto *mainWindow = new MainWindow(ctx);
  if(!parser.isSet(backgroundModeOption))
    mainWindow->onShowApplication();
  CLOCK_MEASURE_END(start_mainwindow, "main::new mainwindow()");

  // handle positional startup arguments
  for (const auto &arg: args) {
    ctx->onIPCReceived(arg);
    break;
  }

  // load Telepathy last, ensures the app is setup so we can act on all signals
  CLOCK_MEASURE_START(start_tp_init);
  ctx->telepathy->init();
  CLOCK_MEASURE_END(start_tp_init, "main::telepathy_init");

  CLOCK_MEASURE_END(start_total, "main::done");
  return QApplication::exec();
}
