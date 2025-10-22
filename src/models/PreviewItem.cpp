#include "PreviewItem.h"

#include "conversations.h"

PreviewItem::SaveResult PreviewItem::saveFile(const QString &destPath, const bool overwrite) const {
  m_lastError.clear();

  if (QFile::exists(destPath)) {
    if (!overwrite) {
      m_lastError = "File already exists";
      return SaveResult::AlreadyExists;
    }
    if (!QFile::remove(destPath)) {
      m_lastError = QString("Could not remove existing file: %1").arg(destPath);
      return SaveResult::RemoveFailed;
    }
  }

  if (!QFile::copy(filePath, destPath)) {
    m_lastError = QString("Failed to copy file to %1").arg(destPath);
    return SaveResult::CopyFailed;
  }

  return SaveResult::Success;
}

int PreviewItem::height(const unsigned int relative_width) const {
  if (state == State::Idle)
    return 0;

  // dynamic heights for images
  const bool inlineImages = config()->get(ConfigKeys::LinkPreviewImageEnabled).toBool();
  if (displayType == DisplayType::Image && inlineImages) {
    auto size = Conversations::instance()->previewImageProvider->getSize(filePath);
    auto _width = size.width();
    auto _height = size.height();

    if (_width > 0 && _height > 0) {
      const double aspect = static_cast<double>(_width) / _height;
      if (aspect >= 1.2)
        return 160;
      if (aspect <= 0.85)
        return 280;
      return 220;
    }
  }

  return m_defaultHeight;
}
