#include "PreviewImageProvider.h"
#include <QPixmap>

QMap<QString, QSize> previewImageSizes = {};

#ifdef QUICK
PreviewImageProvider::PreviewImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

QPixmap PreviewImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) {
  QPixmap pix(id);
  if(size)
    *size=pix.size();
  return pix;
}

void PreviewImageProvider::generateSize(const QString &path) {
  const QPixmap pix(path);
  previewImageSizes[path] = pix.size();
}

QSize PreviewImageProvider::getSize(const QString &path) {
  if (previewImageSizes.contains(path))
    return previewImageSizes[path];
  return {};
}
#endif