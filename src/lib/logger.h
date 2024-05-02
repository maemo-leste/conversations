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

extern Conversations *logger_ctx;

void conversationsMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);