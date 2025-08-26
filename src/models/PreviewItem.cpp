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

  bool inlineImages = config()->get(ConfigKeys::LinkPreviewImageEnabled).toBool();
  if (displayType == DisplayType::Image && inlineImages) {
    return 120;
  }

  // @TODO: dynamic heights, lets do static for now
  // if (relative_width != 0 &&
  //     itemType == ImageType &&
  //     displayType == ImageDisplay) {
  //
  //   auto size = Conversations::instance()->previewImageProvider->getSize(filePath);
  //   if (size.width() > 0 && size.height() > 0) {
  //     auto _height = size.height() * relative_width / size.width();
  //     return _height;
  //   }
  // }

  return m_defaultHeight;
}
