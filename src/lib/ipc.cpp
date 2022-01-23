#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/ipc.h"
#include "lib/utils.h"
#include "lib/globals.h"

IPC::IPC(QObject *parent) : m_server(new QLocalServer(this)), QObject(parent) {
  m_server->setSocketOptions(QLocalServer::UserAccessOption);

  auto socketPath = QString("/tmp/conversations-%1.sock").arg(getenv("USER"));
  m_socketFile = QFileInfo(socketPath);
  m_socketPath = socketPath;
}

void IPC::bind() {
  if(Utils::fileExists(m_socketPath)) {
    QFile file(m_socketPath);
    file.remove();
  }

  if(!m_server->listen(m_socketPath)) {
    qDebug() << QString("Unable to start IPC server in \"%1\": %2").arg(m_socketPath, m_server->errorString());
  }

  connect(m_server, &QLocalServer::newConnection, this, &IPC::handleConnection);
  qDebug() << "listening" << m_socketPath;
}

QIODevice* IPC::open() {
  auto *ls = new QLocalSocket();
  qDebug() << "open unix socket @" << m_socketPath;
  ls->connectToServer(m_socketPath, QIODevice::WriteOnly);
  ls->waitForConnected(250);
  return ls;
}

int IPC::send(QIODevice* ls, const QString &cmdString) {
  QByteArray buffer;
  buffer = buffer.append(cmdString.toUtf8());
  ls->write(buffer);
  if(!ls->waitForBytesWritten(1000)) {
    qCritical() << "error sending command" << ls->errorString();
    return 0;
  }
  return 1;
}

void IPC::handleConnection(){
  QLocalSocket *clientConnection = m_server->nextPendingConnection();
  connect(clientConnection, &QLocalSocket::disconnected, clientConnection, &QLocalSocket::deleteLater);

  clientConnection->waitForReadyRead(2);
  QByteArray cmdArray = clientConnection->readAll();
  QString cmdString = QTextCodec::codecForMib(106)->toUnicode(cmdArray);  // UTF-8
  qDebug() << "received" << cmdString;

  emit commandReceived(cmdString);

  clientConnection->close();
  delete clientConnection;
}
