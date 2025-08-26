#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>

#include "models/PreviewItem.h"

struct HttpHeadResult {
  QString contentType;
  qint64 contentLength;
  QUrl url;
  bool valid = false;
  QString errorString;
  int status_code = 0;
};

struct DownloadCtx {
  QNetworkReply *reply;
  QFile *file;
};

struct HeadContext {
  QNetworkReply *reply;
  QTimer *timeout;
};

class WebPreviewHTTP : public QObject {
  Q_OBJECT
public:
  explicit WebPreviewHTTP(QObject *parent = nullptr);

  void head(const QSharedPointer<PreviewItem> item);
  void download(const QSharedPointer<PreviewItem> item);
  void setMaxDownloadSize(qint64 bytes) { m_maxDownloadSize = bytes; }

signals:
  void errorOccurred(const QString &message);

  void headFinished(const HttpHeadResult &result);
  void downloadFinished(const QString &filePath, const QUrl &url);
  void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, const QUrl &url);
  void downloadError(const QString &message, const QUrl &url);

private slots:
  void onHeadFinished();
  void onDownloadReadyRead();
  void onDownloadFinished();
  void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
  void onTimeout();

private:
  static QString hashUrl(const QUrl &url);
  static int status_code(const QNetworkReply* reply);

  QMap<QNetworkReply*, QTimer*> m_headReplies;
  QHash<QNetworkReply*, DownloadCtx> m_downloads;
  QHash<QNetworkReply*, HeadContext> m_heads;

  QNetworkAccessManager m_manager;
  QNetworkReply *m_currentReply = nullptr;
  QFile *m_downloadFile = nullptr;
  QTimer m_timeoutTimer;
  qint64 m_maxDownloadSize = 2 * 1024 * 1024;
};
