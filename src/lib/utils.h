#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdexcept>

#include <QObject>
#include <QSettings>
#include <QRegExp>
#include <QQmlContext>
#include <QStandardItemModel>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QApplication>
#include <QTextCharFormat>

using namespace std;
using std::runtime_error;

class Utils
{

public:
    static bool fileExists(const QString &path);
    static QByteArray fileOpen(const QString &path);
    static QByteArray fileOpenQRC(const QString &path);
    static qint64 fileModifiedAge(const QString &path);
    static bool fileWrite(const QString &path, const QString &data);
    static bool validateJSON(const QByteArray &blob);
    static bool readJsonFile(QIODevice &device, QSettings::SettingsMap &map);
    static bool writeJsonFile(QIODevice &device, const QSettings::SettingsMap &map);
    static bool dirExists(const QString &path);
    static QString barrayToString(const QByteArray &data);
    static QString formatBytes(quint64 bytes);
    static double roundUp(double value, int decimal_places);
    static QMap<QString, QLocale> localeCache;
    static QString protocolToRTCOMServiceID(const QString &protocol);
    static QMap<QString, QString> readSystemConfig(const QString &path);
    static bool protocolIsTelephone(const QString &protocol);
    static QJsonObject getUserGroupChatChannels();
    static int IPCOpen(const std::string &path);
    static bool IPCSend(int sock, const QString &message);
    static QStringList extractWebLinks(const QString &content);
};

class Conversations;