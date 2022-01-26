#ifndef CONV_CONTEXT_H
#define CONV_CONTEXT_H

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QTimer>

#include "lib/http.h"
#include "lib/config.h"
#include "lib/ipc.h"
#include "lib/tp.h"
#include "models/ChatModel.h"

class Conversations : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString configRootconfigDirectory MEMBER configDirectory);
    Q_PROPERTY(QString configRoot MEMBER configRoot);
    Q_PROPERTY(QString pathGenericData MEMBER pathGenericData);
    Q_PROPERTY(QString homeDir MEMBER homeDir);
    Q_PROPERTY(QString accountName MEMBER accountName);
    Q_PROPERTY(bool isDebug MEMBER isDebug NOTIFY debugChanged);
    Q_PROPERTY(float scaleFactor MEMBER m_textScaling NOTIFY textScalingChanged);
    Q_PROPERTY(float isMaemo MEMBER isMaemo NOTIFY isMaemoChanged);

public:
    explicit Conversations(QCommandLineParser *cmdargs, IPC *ipc);
    ~Conversations() override;
    bool isDebug = false;
    bool isMaemo = false;

    QCommandLineParser *cmdargs;
    IPC *ipc;

    QString configDirectory;
    QString configRoot;
    QString pathGenericData;
    QString homeDir;
    QString accountName;
    QString applicationPath;

    static void createConfigDirectory(const QString &dir) ;

    ChatModel *chatOverviewModel;
    ConvTelepathy *telepathy;

    void setWindowTitle(const QString &title);
    Q_INVOKABLE QString ossoIconLookup(const QString &filename); // /usr/share/icons/hicolor/48x48/hildon/

signals:
    void clockSkewDetected();
    void setTitle(const QString &title); // set window title
    void showChat();
    void debugChanged();
    void textScalingChanged();
    void isMaemoChanged();
    void showApplication();
    void hideApplication();
    void openChatWindow(const QString &remote_uid);
    void reloadOverview();

public slots:
    void onSendOutgoingMessage(const QString &message);
    void onTextScalingChanged();
    void onIPCReceived(const QString &cmd);

private:
    float m_textScaling = 1.0;
    QMap<QString, QString> ossoIconCache;
    QTimer m_hibernateTimer;
    std::chrono::seconds m_hibernateDetectInterval{300};
    std::chrono::time_point<std::chrono::steady_clock> m_hibernatePreviousTime;
};

#endif // CONV_CONTEXT_H
