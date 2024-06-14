#pragma once

#include <QtCore>
#include <QLocalServer>

class IPC : public QObject
{
  Q_OBJECT
public:
  explicit IPC(QObject *parent = nullptr);
  QIODevice *open();
  static int send(QIODevice* ls, const QString &cmdString);

public slots:
  void bind();

signals:
  void commandReceived(const QString &cmd);

private:
  QLocalServer *m_server;
  QString m_queuedCmd;
  QFileInfo m_socketFile;
  QString m_socketPath;
};
