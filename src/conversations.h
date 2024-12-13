#pragma once

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QTimer>

#include "lib/colors.h"
#include "lib/http.h"
#include "lib/config.h"
#include "lib/ipc.h"
#include "lib/tp/tp.h"
#include "lib/state.h"
#include "lib/libnotify-qt/Notification.h"
#include "models/ChatModel.h"
#include "models/ContactItem.h"
#include "models/ChatMessage.h"
#include "overview/OverviewModel.h"

class Conversations : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString configRootconfigDirectory MEMBER configDirectory);
    Q_PROPERTY(QString configRoot MEMBER configRoot);
    Q_PROPERTY(QString pathGenericData MEMBER pathGenericData);
    Q_PROPERTY(QString homeDir MEMBER homeDir);
    Q_PROPERTY(QString accountName MEMBER accountName);
    Q_PROPERTY(bool isDebug MEMBER isDebug NOTIFY debugChanged);
    Q_PROPERTY(float isMaemo MEMBER isMaemo NOTIFY isMaemoChanged);
    Q_PROPERTY(float scaleFactor MEMBER textScaling NOTIFY textScalingChanged);
    Q_PROPERTY(bool inheritSystemTheme MEMBER inheritSystemTheme NOTIFY inheritSystemThemeChanged);
    Q_PROPERTY(bool displayGroupchatJoinLeave MEMBER displayGroupchatJoinLeave NOTIFY displayGroupchatJoinLeaveChanged);
    Q_PROPERTY(bool displayAvatars MEMBER displayAvatars NOTIFY displayAvatarsChanged);

public:
    explicit Conversations(QCommandLineParser *cmdargs, IPC *ipc);
    ~Conversations() override;
    bool isDebug = false;
    bool isMaemo = false;
    bool isBackground = false;

    QCommandLineParser *cmdargs;
    IPC *ipc;
    ConfigState *state;

    QString configDirectory;
    QString configRoot;
    QString pathGenericData;
    QString homeDir;
    QString accountName;
    QString applicationPath;

    HildonTheme *theme;
    bool inheritSystemTheme;
    float textScaling = 1.0;
    bool displayGroupchatJoinLeave;
    bool displayAvatars;

    static void createConfigDirectory(const QString &dir) ;

    ChatModel *chatOverviewModel;
    OverviewModel *overviewModel;
    RequestModel *requestModel;
    OverviewProxyModel *overviewProxyModel;
    Telepathy *telepathy;
    QList<QSharedPointer<ServiceAccount>> serviceAccounts;

    // keep track of previous libnotify broadcasts to prevent notification spam
    QMap<QString, QSharedPointer<ChatMessage>> notificationMap;  // remote_uid, context

    void setWindowTitle(const QString &title);
    Q_INVOKABLE QString ossoIconLookup(const QString &filename); // /usr/share/icons/hicolor/48x48/hildon/

    static void configRemove(const QString &backend_name, const QString &remote_id);

    Q_INVOKABLE void singleShot(int msec, QJSValue callback);

signals:
    void applicationLog(QString msg);
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
    void databaseAddition(const QSharedPointer<ChatMessage> &msg);
    void notificationClicked(const QSharedPointer<ChatMessage> &msg);
    void autoCloseChatWindowsChanged(bool enabled);
    void inheritSystemThemeChanged(bool enabled);
    void displayGroupchatJoinLeaveChanged(bool enabled);
    void displayAvatarsChanged(bool enabled);
    void displayChatGradientChanged(bool enabled);
    void enterKeySendsChatToggled(bool enabled);
    void hildonThemeChanged();

public slots:
    void onSendOutgoingMessage(const QString &local_uid, const QString &remote_uid, const QString &message);
    void onTextScalingChanged();
    void onIPCReceived(const QString &cmd);
    void onDatabaseAddition(const QSharedPointer<ChatMessage> &msg);
    void onNotificationClicked(const QSharedPointer<ChatMessage> &msg);
    void onGetAvailableServiceAccounts();

private slots:
    void onApplicationLog(QString msg);

private:
    QMap<QString, QString> ossoIconCache;
    std::chrono::seconds m_hibernateDetectInterval{300};
    std::chrono::time_point<std::chrono::steady_clock> m_hibernatePreviousTime;
};
