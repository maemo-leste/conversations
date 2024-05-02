#include <QObject>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QStandardPaths>

#include "logger.h"

Conversations *logger_ctx = nullptr;

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

    auto fn = QString(context.file);
    int eeg =  1;
    auto fnspl = fn.split("/");
    logMessage += QString("[%1::%2] %3").arg(fnspl.last()).arg(context.line).arg(msg);

    QTextStream stream(type != QtInfoMsg ? stderr : stdout);
    stream << logMessage << "\n";

    if(logger_ctx != nullptr)
      logger_ctx->applicationLog(logMessage);

    // in case we want to write it somewhere
    // QFile outFile("log.txt");
    // if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    //     QTextStream fileStream(&outFile);
    //     fileStream << logMessage << endl;
    // }
}