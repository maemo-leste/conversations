#include <QObject>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <syslog.h>

#include "logger.h"

namespace conversations_logger {
  Conversations *logger_ctx = nullptr;
  QFile *logFile = nullptr;
  QTextStream *logStream = nullptr;
  bool syslog_enabled = false;

  void conversationsMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString log_message;
    int syslog_prio;
    switch (type) {
      case QtDebugMsg: log_message = "[D] "; syslog_prio = LOG_DEBUG; break;
      case QtInfoMsg: log_message = "[I] "; syslog_prio = LOG_INFO; break;
      case QtWarningMsg: log_message = "[W] "; syslog_prio = LOG_WARNING; break;
      case QtCriticalMsg: log_message = "[C] "; syslog_prio = LOG_ERR; break;
      case QtFatalMsg: log_message = "[F] "; syslog_prio = LOG_CRIT; break;
      default: log_message = "[U] "; syslog_prio = LOG_NOTICE; break;
    }

    const auto fn = QString(context.file);
    auto fnspl = fn.split("/");
    log_message += QString("[%1::%2] %3").arg(fnspl.last()).arg(context.line).arg(msg);

    QTextStream stream(type != QtInfoMsg ? stderr : stdout);
    stream << log_message << "\n";

    if(logFile && logFile->isOpen()) {
      if(!logStream) logStream = new QTextStream(logFile);
      *logStream << log_message << Qt::endl;
    }

    if(logger_ctx)
      logger_ctx->applicationLog(log_message);

    if(syslog_enabled) {
      openlog("Conversations", LOG_PID | LOG_CONS, LOG_USER);
      syslog(syslog_prio, "%s", log_message.toUtf8().constData());
      closelog();
    }
  }
}