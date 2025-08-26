#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include "downloadpreview.h"
#include "conversations.h"

WebPreviewHTTP::WebPreviewHTTP(QObject *parent) :
  QObject(parent) {
  connect(&m_timeoutTimer, &QTimer::timeout, this, &WebPreviewHTTP::onTimeout);
}

void WebPreviewHTTP::head(const QSharedPointer<PreviewItem> item) {
  QNetworkRequest request(item->url);
  QNetworkReply *reply = m_manager.head(request);
  reply->setProperty("headUrl", item->url);

  QTimer *timeout = new QTimer(this);
  timeout->setSingleShot(true);
  connect(timeout, &QTimer::timeout, this, [this, timeout, reply] {
    if (reply && reply->isRunning()) {
      reply->abort();
      HttpHeadResult result;
      result.url = reply->url();
      result.valid = false;
      result.status_code = 0;
      result.errorString = "timed out";
      emit headFinished(result);
    }
    timeout->stop();
    timeout->deleteLater();
  });

  timeout->start(5000);

  const HeadContext ctx { reply, timeout };
  m_heads.insert(reply, ctx);

  connect(reply, &QNetworkReply::finished, this, &WebPreviewHTTP::onHeadFinished);
}

void WebPreviewHTTP::download(const QSharedPointer<PreviewItem> item) {
  if (item->contentLength >= m_maxDownloadSize) {
    const QString msg = "File too large for preview";
    qWarning() << msg;
    emit downloadError(msg, item->url);
    return;
  }

  const QString fileName = hashUrl(item->url) + "." + item->ext();
  const QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  QDir().mkpath(downloadDir);
  const QString filePath = downloadDir + QDir::separator() + fileName;

  if (QFile::exists(filePath)) {
    if (const QPixmap pix(filePath); !pix.isNull()) {
      emit downloadFinished(filePath, item->url);
      return;
    }
  }

  const auto file = new QFile(filePath);
  qDebug() << filePath;
  if (!file->open(QIODevice::WriteOnly)) {
    emit downloadError(QString("Cannot open file for writing: %1").arg(filePath), item->url);
    delete file;
    return;
  }

  QNetworkRequest request(item->url);
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

  QNetworkReply *reply = m_manager.get(request);
  reply->setProperty("downloadUrl", item->url);
  reply->setProperty("filePath", filePath);

  const DownloadCtx ctx { reply, file };
  m_downloads.insert(reply, ctx);

  connect(reply, &QNetworkReply::readyRead, this, &WebPreviewHTTP::onDownloadReadyRead);
  connect(reply, &QNetworkReply::finished, this, &WebPreviewHTTP::onDownloadFinished);
  connect(reply, &QNetworkReply::downloadProgress, this, &WebPreviewHTTP::onDownloadProgress);
}

void WebPreviewHTTP::onHeadFinished() {
  auto *reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply || !m_heads.contains(reply))
    return;

  HeadContext ctx = m_heads.take(reply);
  if (ctx.timeout) {
    ctx.timeout->stop();
    ctx.timeout->deleteLater();
  }

  HttpHeadResult result;
  result.url = reply->property("headUrl").toUrl();

  if (reply->error() != QNetworkReply::NoError) {
    result.valid = false;
    result.errorString = reply->errorString();
  } else {
    result.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    result.contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    if (result.contentLength >= m_maxDownloadSize) {
      result.errorString = "File too large";
      result.valid = false;
    } else {
      result.valid = true;
    }
  }

  result.status_code = status_code(reply);

  emit headFinished(result);
  reply->deleteLater();
}

void WebPreviewHTTP::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
  auto *reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply) return;
  const QUrl url = reply->property("downloadUrl").toUrl();
  emit downloadProgress(bytesReceived, bytesTotal, url);

  if (m_maxDownloadSize > 0 && bytesReceived > m_maxDownloadSize) {
    reply->abort();
    emit downloadError(
        QString("Download exceeded maximum size (%1 bytes)").arg(m_maxDownloadSize), url);
  }
}

void WebPreviewHTTP::onDownloadReadyRead() {
  auto *reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply || !m_downloads.contains(reply)) return;

  DownloadCtx &ctx = m_downloads[reply];
  if (ctx.file)
    ctx.file->write(reply->readAll());
}

void WebPreviewHTTP::onDownloadFinished() {
  auto *reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply || !m_downloads.contains(reply)) return;

  DownloadCtx ctx = m_downloads.take(reply);
  const QUrl url = reply->property("downloadUrl").toUrl();
  const QString filePath = reply->property("filePath").toString();

  if (reply->error() != QNetworkReply::NoError) {
    emit downloadError(reply->errorString(), url);
  } else if (ctx.file) {
    ctx.file->close();
    emit downloadFinished(filePath, url);
  }

  if (ctx.file) {
    delete ctx.file;
  }

  reply->deleteLater();
}

void WebPreviewHTTP::onTimeout() {
  if (m_currentReply) {
    m_currentReply->abort();
    emit errorOccurred("HEAD request timed out after 5 seconds");
  }
  m_timeoutTimer.stop();
}

int WebPreviewHTTP::status_code(const QNetworkReply* reply) {
  if (!reply)
    return -1;

  if (const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute); status.isValid())
    return status.toInt();
  return -1;
}

QString WebPreviewHTTP::hashUrl(const QUrl &url) {
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(url.toString().toUtf8());
  return QString(hash.result().toHex());
}
