#pragma once

#include <QFile>
#include <QObject>
#include <QUrl>
#include <QMessageBox>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

#include "lib/utils.h"

class PreviewItem : public QObject {
  Q_OBJECT

public:
  enum class ItemType {
    Error,
    Audio,
    Video,
    Image,
    Html,
    Pdf,
    Unknown
  };

  enum class DisplayType {
    Text,
    Image
  };

  enum class State {
    Idle,
    Head,
    Downloaded
  };

  enum class SaveResult {
    Success,
    AlreadyExists,
    RemoveFailed,
    CopyFailed
  };

  explicit PreviewItem(QObject *parent = nullptr) : QObject(parent) {}

  QString name;
  QUrl url;
  ItemType itemType { ItemType::Unknown };
  QString contentType;
  qint64 contentLength { -1 };
  QString error;
  QString filePath;
  State state { State::Idle };
  DisplayType displayType { DisplayType::Text };
  QString title;
  QString subtitle;
  bool itemVisible { true };

  SaveResult saveFile(const QString &destPath, bool overwrite) const;
  [[nodiscard]] QString lastError() const { return m_lastError; }

  [[nodiscard]] QString icon() const {
    if (hasError()) {
      return "qrc:/FileDead.svg";
    }

    switch (itemType) {
      case ItemType::Image: return "qrc:/Image.svg";
      case ItemType::Audio: return "qrc:/AudioStreamPlayer.svg";
      case ItemType::Video: return "qrc:/VideoStreamPlayer.svg";
      case ItemType::Html:  return "qrc:/NavigationLink2D.svg";
      case ItemType::Pdf:   return "qrc:/PlaceholderTexture2D.svg";
      default:              return "qrc:/NavigationLink2D.svg";
    }
  }

  [[nodiscard]] QString ext() const {
    switch (itemType) {
      case ItemType::Image: return "jpg";
      case ItemType::Audio: return "audio";
      case ItemType::Video: return "video";
      case ItemType::Html:  return "html";
      case ItemType::Pdf:   return "pdf";
      default:              return "file";
    }
  }

  [[nodiscard]] QString previewHTMLTitle() const {
    if (!QFile::exists(filePath))
      return {};
    return Utils::extractTitleFromHtml(filePath);
  }

  [[nodiscard]] bool hasError() const { return !error.isEmpty(); }
  [[nodiscard]] int height(unsigned int relative_width = 0) const;

private:
  const unsigned int m_defaultHeight { 52 };
  mutable QString m_lastError;
};
