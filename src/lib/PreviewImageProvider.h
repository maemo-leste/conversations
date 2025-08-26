#pragma once
#ifdef QUICK
#include <QQmlContext>
#include <QQuickImageProvider>
#endif
#include <QMap>
#include <QSize>

extern QMap<QString,QSize> previewImageSizes;

#ifdef QUICK
class PreviewImageProvider : public QQuickImageProvider {
public:
  PreviewImageProvider();
  QPixmap requestPixmap(const QString &id,QSize *size,const QSize &requestedSize) override;
  static void generateSize(const QString &path);
  static QSize getSize(const QString &path);
};
#endif