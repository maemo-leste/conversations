#pragma once
#include <sys/prctl.h>
#include <signal.h>

#include <QProcess>
#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QJsonArray>
#include <QtCore/QTextStream>
#include <QtNetwork/QLocalSocket>

class KotkiServerProcess final : public QProcess {
protected:
  void setupChildProcess() override {
    prctl(PR_SET_PDEATHSIG, SIGKILL);
    QProcess::setupChildProcess();
  }
};

class KotkiClient final : public QObject {
Q_OBJECT

public:
  explicit KotkiClient(const QString &socketPath = "/tmp/kotki_server.socket", QObject *parent = nullptr);

  QJsonArray listModels(int timeoutMs = 3000);
  void clearListModelsCache();
  QJsonObject translate(const QString &text, const QString &lang, int timeoutMs = 3000) const;

  bool startServer();
  bool stopServer();
  bool isServerRunning() const;

private:
  QByteArray make_request_raw(const QJsonObject &request, int timeout) const;

  QString m_socketPath;
  QJsonArray m_listModelsCache;

  KotkiServerProcess m_proc;
  bool m_running=false;
};