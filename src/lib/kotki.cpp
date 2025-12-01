#include "kotki.h"
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDebug>

KotkiClient::KotkiClient(const QString &socketPath, QObject *parent) : QObject(parent), m_socketPath(socketPath) {}

bool KotkiClient::startServer() {
  if (m_running)
    return true;
  if (m_proc.state() != QProcess::NotRunning)
    return false;

  m_proc.setProgram("/usr/bin/kotki-server");
  m_proc.setProcessChannelMode(QProcess::ForwardedChannels);
  m_proc.start();

  if (!m_proc.waitForStarted(3000)) {
    qWarning() << "Failed to start kotki-server";
    return false;
  }

  qDebug() << "KotkiClient started";
  m_running = true;
  return true;
}

bool KotkiClient::stopServer() {
  if (!m_running)
    return false;

  m_proc.terminate();
  m_proc.waitForFinished();
  m_running = false;

  qDebug() << "KotkiClient stopped";
  return true;
}

bool KotkiClient::isServerRunning() const {
  return m_running;
}

QByteArray KotkiClient::make_request_raw(const QJsonObject &request, const int timeout) const {
  QLocalSocket socket;
  socket.connectToServer(m_socketPath);

  if (!socket.waitForConnected(timeout)) {
    qWarning() << "Connection failed:" << socket.errorString();
    return {};
  }

  const QJsonDocument doc(request);
  const QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

  if (socket.write(data) == -1) {
    qWarning() << "Write failed:" << socket.errorString();
    return {};
  }

  if (!socket.waitForBytesWritten(timeout)) {
    qWarning() << "Bytes not written:" << socket.errorString();
    return {};
  }

  if (!socket.waitForReadyRead(timeout)) {
    qWarning() << "No response from server";
    return {};
  }

  QByteArray responseData;
  while (socket.bytesAvailable() > 0 || socket.waitForReadyRead(100)) {
    responseData.append(socket.readAll());
  }

  socket.disconnectFromServer();
  return responseData;
}

QJsonArray KotkiClient::listModels(int timeoutMs) {
  if (!m_listModelsCache.isEmpty()) return m_listModelsCache;

  const QByteArray responseData = make_request_raw(QJsonObject{{"command","listModels"}}, timeoutMs);
  if (responseData.isEmpty())
    return {};

  const QJsonDocument doc = QJsonDocument::fromJson(responseData);
  if (!doc.isArray())
    return {};

  m_listModelsCache = doc.array();
  return m_listModelsCache;
}

void KotkiClient::clearListModelsCache() {
  m_listModelsCache = {};
}

QJsonObject KotkiClient::translate(const QString &text, const QString &lang, int timeoutMs) const {
  QByteArray responseData = make_request_raw(QJsonObject{{"command","translate"},{"text",text},{"lang",lang}}, timeoutMs);
  if (responseData.isEmpty()) return {{"error","No response"}};

  QJsonDocument doc = QJsonDocument::fromJson(responseData);
  if (!doc.isObject()) return {{"error","Invalid JSON response"}};

  return doc.object();
}
