#pragma once
#include <QObject>
#include <QSettings>
#include <QRegExp>
#include <QQmlContext>
#include <QStandardItemModel>
#include <QApplication>
#include <QTextCharFormat>

#include "lib/libnotify-qt/Notification.h"

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
    static Notification* notification(QString title, QString message, const QSharedPointer<ChatMessage> &msg);
    static QString protocolToRTCOMServiceID(const QString &protocol);
    static void qmlInjectPalette(QQmlContext *qctx, Conversations *m_ctx);
};

class Conversations;