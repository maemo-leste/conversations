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
#include <QDebug>
#include <QSettings>
#include <QRegExp>
#include <QPixmapCache>
#ifdef QUICK
#include <QQmlContext>
#include <QQuickImageProvider>
#endif
#include <QStandardItemModel>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QApplication>
#include <QTextCharFormat>

#include "lib/abook/abook_public.h"
#include "lib/abook/abook_roster.h"
#include "lib/abook/abook_contact.h"

using namespace std;
using std::runtime_error;

enum DeviceType {
    DEFAULT,
    N900,
    DROID4,
    PINEPHONE
};

extern DeviceType DEVICE_TYPE;

class Utils {
public:
  static bool fileExists(const QString &path);
  static QByteArray fileOpen(const QString &path);
  static QByteArray fileOpenQRC(const QString &path);
  static qint64 fileModifiedAge(const QString &path);
  static void init_device_type();
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
  static QString escapeHtml(const QString& text);
  static QList<QMap<QString, QString>> parseDebianChangelog(const QString &text);

  static __attribute__((always_inline)) bool get_avatar(const std::string& local_uid, const std::string& remote_uid, const std::string& token, QPixmap& pixmap) {
    // check cache
    const auto cache_key = QString::fromStdString(local_uid + "-" + remote_uid + "-" + token);

    if (!QPixmapCache::find(cache_key, &pixmap)) {
      const AbookContactAvatar* avatar = abook_qt::abook_get_avatar(local_uid, remote_uid);
      if (avatar == nullptr)
        return false;

      QImage::Format format;
      if (avatar->n_channels == 3 && !avatar->has_alpha) {
        format = QImage::Format_RGB888;
      } else if (avatar->n_channels == 4 && avatar->has_alpha) {
        format = QImage::Format_RGBA8888;
      } else {
        qWarning() << "Unsupported avatar format for" << QString::fromStdString(local_uid);
        return false;
      }

      const QImage image(avatar->buf, avatar->width, avatar->height, avatar->rowstride, format);
      pixmap = QPixmap::fromImage(image.scaled(58, 58, Qt::KeepAspectRatio, Qt::SmoothTransformation));

      QPixmapCache::insert(cache_key, pixmap);
    }

    return true;
  }
};

#ifdef QUICK
class AvatarImageProvider : public QQuickImageProvider {
public:
  AvatarImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
#else
class AvatarImageProvider : public QObject {
Q_OBJECT
public:
  AvatarImageProvider() = default;
#endif

  QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
#ifdef QUICK
    override
#endif
    {
    qDebug() << "AvatarImageProvider::requestImage";
    const int last_slash = id.lastIndexOf("?token=");
    QString hex_str = id.mid(last_slash + 7);
    const QString persistent_uid = id.left(last_slash);

    const auto spl = persistent_uid.split("-");
    const auto local_uid_str = spl.at(0).toStdString();
    const auto remote_uid_str = spl.at(1).toStdString();

    if(abook_qt::ROSTER.contains(persistent_uid.toStdString())) {
      const std::string avatar_token = abook_qt::get_avatar_token(local_uid_str, remote_uid_str);
      if (!avatar_token.empty() && avatar_token != "0") {
        QPixmap pixmap;
        auto result = Utils::get_avatar(local_uid_str, remote_uid_str, avatar_token, pixmap);

        if (!result)
          return {};

        return pixmap;
      }
    }

    return {};
  }
};

class Conversations;
