#include <QScreen>
#include <QMessageBox>
#include <QtNetwork>
#include <QClipboard>

#include "utils.h"
#include "lib/config.h"
#include "models/ChatMessage.h"

bool Utils::fileExists(const QString &path) {
    QFileInfo check_file(path);
    return check_file.exists() && check_file.isFile();
}

bool Utils::dirExists(const QString &path) {
    QDir pathDir(path);
    return pathDir.exists();
}

QByteArray Utils::fileOpen(const QString &path) {
    QFile file(path);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        return QByteArray();
    }

    QByteArray data = file.readAll();
    file.close();
    return data;
}

qint64 Utils::fileModifiedAge(const QString &path) {
    QFileInfo fileInfo;
    fileInfo.setFile(path);
    return (QDateTime::currentSecsSinceEpoch() - fileInfo.lastModified().toSecsSinceEpoch());
}

QByteArray Utils::fileOpenQRC(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "error: " << file.errorString();
    }

    QByteArray data = file.readAll();
    file.close();
    return data;
}

bool Utils::fileWrite(const QString &path, const QString &data) {
    QFile file(path);
    if(file.open(QIODevice::WriteOnly)){
        QTextStream out(&file); out << data << "\n";
        file.close();
        return true;
    }
    return false;
}

bool Utils::validateJSON(const QByteArray &blob) {
    QJsonDocument doc = QJsonDocument::fromJson(blob);
    QString jsonString = doc.toJson(QJsonDocument::Indented);
    return !jsonString.isEmpty();
}

bool Utils::readJsonFile(QIODevice &device, QSettings::SettingsMap &map) {
    QJsonDocument json = QJsonDocument::fromJson(device.readAll());
    map = json.object().toVariantMap();
    return true;
}

bool Utils::writeJsonFile(QIODevice &device, const QSettings::SettingsMap &map) {
    device.write(QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Indented));
    return true;
}

QString Utils::barrayToString(const QByteArray &data) {
    return QString(QTextCodec::codecForMib(106)->toUnicode(data));
}

QString Utils::formatBytes(quint64 bytes)
{
    QVector<QString> sizes = { "B", "KB", "MB", "GB", "TB" };

    int i;
    double _data;
    for (i = 0; i < sizes.count() && bytes >= 10000; i++, bytes /= 1000)
        _data = bytes / 1000.0;

    if (_data < 0)
        _data = 0;

    // unrealistic
    if (_data > 10000)
        _data = 0;

    return QString("%1 %2").arg(QString::number(_data, 'f', 1), sizes[i]);
}

QMap<QString, QLocale> Utils::localeCache = {};

double Utils::roundUp(double value, int decimal_places) {
    const double multiplier = std::pow(10.0, decimal_places);
    return std::ceil(value * multiplier) / multiplier;
}

bool Utils::protocolIsTelephone(const QString &protocol) {
  return (protocol == "sms") || (protocol == "tel") || (protocol == "ofono");
}

QString Utils::protocolToRTCOMServiceID(const QString &protocol) {
  if(Utils::protocolIsTelephone(protocol))
      return "RTCOM_EL_SERVICE_SMS";
  return "RTCOM_EL_SERVICE_CHAT";
}

QMap<QString, QString> Utils::readSystemConfig(const QString &path) {
  QMap<QString, QString> rtn;
  if(!Utils::fileExists(path)) {
    qWarning() << "could not read: " << path;
    return rtn;
  }

  const auto lines = Utils::barrayToString(Utils::fileOpen(path));
  for(auto const &line: lines.split('\x0a')) {
    if (line.trimmed().length() == 0 || !line.contains('=')) continue;
    const auto spl = line.split('=');
    const auto &key = spl.at(0);
    const auto &val = spl.at(1);
    rtn[key] = val;
  }

  return rtn;
}

QJsonObject Utils::getUserGroupChatChannels() {
  auto data = config()->get(ConfigKeys::GroupChatChannels).toByteArray();
  if(data.isEmpty()) return {};

  auto doc = QJsonDocument::fromJson(data);
  if(doc.isNull() || !doc.isObject()) {
    qWarning() << "invalid json encountered parsing Config::autoJoinChatChannels";
    return {};
  }
  return doc.object();
}

int Utils::IPCOpen(const std::string &path) {
  int sock;
  struct sockaddr_un server;
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(sock < 0)
    return -1;

  server.sun_family = AF_UNIX;
  strcpy(server.sun_path, path.c_str());

  if(connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) < 0) {
    close(sock);
    return -1;
  }

  return sock;
}

bool Utils::IPCSend(int sock, const QString &message) {
  if(write(sock, message.toStdString().c_str(), message.length()) < 0) {
    close(sock);
    return true;
  }
  return false;
}

QStringList Utils::extractWebLinks(const QString &content) {
  QStringList rtn;

  QRegularExpression rx("((?:https?|ftp)://\\S+)");
  QRegularExpressionMatchIterator i = rx.globalMatch(content);

  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    rtn << match.captured(0);
  }

  return rtn;
}