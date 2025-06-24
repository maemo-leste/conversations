#pragma once

#include <QNetworkAccessManager>
#include <QCommandLineParser>
#include <QStorageInfo>
#include <QApplication>
#include <algorithm>
#include <QFileInfo>
#include <iostream>
#include <QProcess>
#include <QObject>
#include <QTimer>
#include <random>
#include <chrono>
#include <array>

#include "conversations.h"

namespace conversations_logger {
  extern Conversations *logger_ctx;
  extern QFile *logFile;
  extern QTextStream *logStream;
  extern bool syslog_enabled;

  void conversationsMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
}