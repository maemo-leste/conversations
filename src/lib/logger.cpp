#include <QObject>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QCommandLineParser>
#include <QStandardPaths>

#include "logger.h"

Conversations *logger_ctx = nullptr;
QFile *logFile = nullptr;
QTextStream *logStream = nullptr;

void conversationsMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString logMessage;
    switch (type) {
    case QtDebugMsg:
        logMessage = "[D] ";
        break;
    case QtInfoMsg:
        logMessage = "[I] ";
        break;
    case QtWarningMsg:
        logMessage = "[W] ";
        break;
    case QtCriticalMsg:
        logMessage = "[C] ";
        break;
    default:
        logMessage = "[U] ";
        break;
    }

    const auto fn = QString(context.file);
    auto fnspl = fn.split("/");
    logMessage += QString("[%1::%2] %3").arg(fnspl.last()).arg(context.line).arg(msg);

    // to console
    QTextStream stream(type != QtInfoMsg ? stderr : stdout);
    stream << logMessage << "\n";

    // to file
    if(logFile != nullptr && logFile->isOpen()) {
      if(logStream == nullptr)
        logStream = new QTextStream(logFile);
      *logStream << logMessage << Qt::endl;
    }

    // to app
    if(logger_ctx != nullptr)
      logger_ctx->applicationLog(logMessage);
}