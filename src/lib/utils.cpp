#include <sys/stat.h>

#include <QScreen>
#include <QMessageBox>
#include <QtNetwork>
#include <QClipboard>

#include "utils.h"
#include "lib/config.h"
#include "models/ChatMessage.h"

DeviceType DEVICE_TYPE = DeviceType::DEFAULT;

bool Utils::fileExists(const QString &path) {
    QFileInfo check_file(path);
    return check_file.exists() && check_file.isFile();
}

qint64 Utils::fileSize(const QString &path) {
  struct stat fileInfo{};
  if (stat(path.toStdString().c_str(), &fileInfo) == 0)
    return fileInfo.st_size;
  return -1;
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
    return QString::fromUtf8(data);
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

QString Utils::escapeHtml(const QString& text) {
  QString escaped = text;
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  return escaped;
}

static bool utils_isEmojiScalar(char32_t cp) {
  return (cp >= 0x1F000 && cp <= 0x1FAFF) ||
         (cp >= 0x2600  && cp <= 0x27BF)  ||
         (cp >= 0x2B00  && cp <= 0x2BFF)  ||
         (cp >= 0x2300  && cp <= 0x23FF)  ||
         (cp >= 0x2194  && cp <= 0x21AA)  ||
         (cp >= 0x25AA  && cp <= 0x25FE)  ||
         cp == 0x203C || cp == 0x2049 || cp == 0x2122 || cp == 0x2139 ||
         cp == 0x24C2 || cp == 0x3030 || cp == 0x303D || cp == 0x3297 || cp == 0x3299;
}

static bool utils_isRegionalIndicator(char32_t cp) { return cp >= 0x1F1E6 && cp <= 0x1F1FF; }

QList<QPair<int, int>> Utils::emojiRanges(const QString &s) {
  QList<QPair<int, int>> ranges;

  int i = 0;
  while (i < s.length()) {
    char32_t cp;
    int len = 1;
    if (s[i].isHighSurrogate() && i + 1 < s.length() && s[i + 1].isLowSurrogate()) {
      cp = QChar::surrogateToUcs4(s[i], s[i + 1]);
      len = 2;
    } else {
      cp = s[i].unicode();
    }

    const bool nextIsVS16 = (i + len < s.length() && s[i + len].unicode() == 0xFE0F);
    const bool keycapBase = (cp == '#' || cp == '*' || (cp >= '0' && cp <= '9'));

    if (keycapBase) {
      int j = i + len;
      if (j < s.length() && s[j].unicode() == 0xFE0F) j++;
      if (j < s.length() && s[j].unicode() == 0x20E3) {
        ranges.append({i, j + 1});
        i = j + 1;
        continue;
      }
    }

    if (utils_isEmojiScalar(cp) || utils_isRegionalIndicator(cp) || nextIsVS16) {
      const int start = i;
      i += len;
      for (;;) {
        if (i >= s.length())
          break;
        const ushort u = s[i].unicode();
        if (u == 0xFE0F || u == 0x20E3) { i++; continue; }
        if (u == 0x200D) {
          i++;
          if (i < s.length())
            i += (s[i].isHighSurrogate() && i + 1 < s.length()) ? 2 : 1;
          continue;
        }
        if (s[i].isHighSurrogate() && i + 1 < s.length() && s[i + 1].isLowSurrogate()) {
          const char32_t m = QChar::surrogateToUcs4(s[i], s[i + 1]);
          if ((m >= 0x1F3FB && m <= 0x1F3FF) || utils_isRegionalIndicator(m)) { i += 2; continue; }
        }
        break;
      }
      ranges.append({start, i});
      continue;
    }

    i += len;
  }

  return ranges;
}

QString Utils::emojiHtml(const QString &text, const QString &family) {
  const auto escape = [](const QString &in) -> QString {
    QString out = in;
    out.replace("&", "&amp;");
    out.replace("<", "&lt;");
    out.replace(">", "&gt;");
    out.replace("\n", "<br/>");
    return out;
  };

  if (family.isEmpty())
    return escape(text);

  const QList<QPair<int, int>> ranges = emojiRanges(text);
  if (ranges.isEmpty())
    return escape(text);

  const QString open = QStringLiteral("<span style=\"font-family:'%1'\">").arg(family);
  const QString close = QStringLiteral("</span>");

  QString out;
  int pos = 0;
  for (const auto &r : ranges) {
    if (r.first > pos)
      out += escape(text.mid(pos, r.first - pos));
    out += open;
    out += escape(text.mid(r.first, r.second - r.first));
    out += close;
    pos = r.second;
  }
  if (pos < text.length())
    out += escape(text.mid(pos));

  return out;
}

double Utils::roundUp(double value, int decimal_places) {
    const double multiplier = std::pow(10.0, decimal_places);
    return std::ceil(value * multiplier) / multiplier;
}

bool Utils::protocolIsTelephone(const QString &protocol) {
  return protocol == "sms" || protocol == "tel" || protocol == "ofono" || protocol == "RTCOM_EL_EVENTTYPE_SMS_MESSAGE";
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

QList<QMap<QString, QString>> Utils::parseDebianChangelog(const QString &text) {
  QList<QMap<QString, QString>> result;
  const QRegularExpression headerRegex(R"(conversations \(([^)]+)\) unstable; urgency=.*)");
  const QRegularExpression metaRegex(R"(^\s+-- .+? <.+?>\s+.+$)");
  QStringList lines = text.split('\n');
  QMap<QString, QString> entry;
  QStringList bodyLines;

  for (const QString &line : lines) {
    if (auto headerMatch = headerRegex.match(line); headerMatch.hasMatch()) {
      if (!entry.isEmpty()) {
        auto body = bodyLines.join('\n').trimmed();
        if (!body.startsWith("  "))
          body = "  " + body;
        entry["body"] = body;
        result.append(entry);
        entry.clear();
        bodyLines.clear();
      }
      entry["version"] = headerMatch.captured(1);
      continue;
    }
    if (metaRegex.match(line).hasMatch()) {
      entry["meta"] = line.trimmed();
      continue;
    }
    if (!line.trimmed().isEmpty() || !bodyLines.isEmpty()) {
      bodyLines.append(line);
    }
  }
  if (!entry.isEmpty()) {
    auto body = bodyLines.join('\n').trimmed();
    if (!body.startsWith("  "))
      body = "  " + body;
    entry["body"] = body;

    if (!entry["meta"].contains("Jenkins Auto"))
      result.append(entry);
  }

  return result;
}

void Utils::init_device_type() {
    QFile file("/etc/hostname");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "Could not open /etc/hostname";
      DEVICE_TYPE = DEFAULT;
      return;
    }

    QTextStream in(&file);
    QString hostname = in.readLine().trimmed().toLower();

    if (hostname.contains("n900")) {
      DEVICE_TYPE = N900;
    } else if (hostname.contains("droid4")) {
      DEVICE_TYPE = DROID4;
    } else if (hostname.contains("pinephone")) {
      DEVICE_TYPE = PINEPHONE;
    } else {
      DEVICE_TYPE = DEFAULT;
    }
}

QString Utils::extractTitleFromHtml(const QString &filePath, const int max_len) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Cannot open file:" << filePath;
    return {};
  }

  QTextStream in(&file);
  in.setEncoding(QStringConverter::Utf8);
  const QString content = in.readAll();
  file.close();

  const QRegularExpression re("<title>(.*?)</title>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
  QRegularExpressionMatch match = re.match(content);
  if (match.hasMatch()) {
    QString title = match.captured(1).trimmed();
    if (title.length() > max_len)
      title = title.left(max_len);
    return title;
  }
  return {};
}
