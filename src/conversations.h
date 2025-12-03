#pragma once

#include <stdint.h>

#include <QObject>
#include <QProcess>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QTimer>

#include "lib/colors.h"
#include "lib/config.h"
#include "lib/ipc.h"
#include "lib/tp/tp.h"
#include "lib/state.h"
#include "lib/abook/abook_public.h"
#include "lib/PreviewImageProvider.h"
#include "lib/libnotify-qt/Notification.h"
#include "models/ChatModel.h"
#include "models/ContactItem.h"
#include "models/ChatMessage.h"
#include "overview/OverviewModel.h"

class Conversations;
extern Conversations* CTX;

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
    Q_PROPERTY(bool displayChatGradient MEMBER displayChatGradient NOTIFY displayChatGradientChanged);
    Q_PROPERTY(bool displayGroupchatJoinLeave MEMBER displayGroupchatJoinLeave NOTIFY displayGroupchatJoinLeaveChanged);
    Q_PROPERTY(bool displayAvatars MEMBER displayAvatars NOTIFY displayAvatarsChanged);

public:
    explicit Conversations(QCommandLineParser *cmdargs, IPC *ipc);
    ~Conversations() override;
    static Conversations* instance() {
      if (CTX == nullptr) throw std::runtime_error("ctx is null");
      return CTX;
    }

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
    bool displayChatGradient;
    bool displayAvatars;

    static void createConfigDirectory(const QString &dir) ;

    ChatModel *chatOverviewModel;
    OverviewModel *overviewModel;
    OverviewProxyModel *overviewProxyModel;
    Telepathy *telepathy;
    QList<QSharedPointer<ServiceAccount>> serviceAccounts;
    AvatarImageProvider* avatarProvider = nullptr;
#ifdef QUICK
    PreviewImageProvider* previewImageProvider = nullptr;
#endif
    // keep track of previous libnotify broadcasts to prevent notification spam
    QMap<QString, uint64_t> notificationMap;  // remote_uid, context

    void setWindowTitle(const QString &title);
    Q_INVOKABLE QString ossoIconLookup(const QString &filename); // /usr/share/icons/hicolor/48x48/hildon/
    void OSnotify(const QString& title, const QString& message, const QSharedPointer<ChatMessage> &msg_obj);

    static void configRemove(const QString &backend_name, const QString &remote_id);

    void stopNewVersionTimer() const { m_newVersionCheckTimer->stop(); }
    void startNewVersionTimer() const { m_newVersionCheckTimer->start(); }

#ifdef QUICK
    Q_INVOKABLE void singleShot(int msec, QJSValue callback) const;
#endif

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
    void openChatWindow(QString remote_uid);
    void reloadOverview();
    void contactsChanged(std::vector<std::shared_ptr<abook_qt::AbookContact>> contacts);
    void databaseAddition(const QSharedPointer<ChatMessage> &msg);
    void notificationClicked(const QSharedPointer<ChatMessage> &msg);
    void autoCloseChatWindowsChanged(bool enabled);
    void inheritSystemThemeChanged(bool enabled);
    void displayGroupchatJoinLeaveChanged(bool enabled);
    void displayAvatarsChanged(bool enabled);
    void displayChatGradientChanged(bool enabled);
    void enterKeySendsChatToggled(bool enabled);
    void hildonThemeChanged();
    void avatarChanged(const std::string &abook_uid);
    void abookReady();
    void enableLinkPreviewEnabledToggled(bool enabled);
    void enableLinkPreviewImageEnabledToggled(bool enabled);
    void enableLinkPreviewRequiresUserInteractionToggled(bool enabled);
    void attachmentMaxDownloadSizeChanged(int val);
    void bgMatrixRainEnabledChanged(bool enabled);
    void differentVersionAvailable();

public slots:
    void onContactsChanged(std::vector<std::shared_ptr<abook_qt::AbookContact>> contacts);
    void onAvatarChanged(const std::string &abook_uid);
    void onSendOutgoingMessage(const QString &local_uid, const QString &remote_uid, const QString &message);
    void onTextScalingChanged();
    void onIPCReceived(const QString &cmd);
    void onDatabaseAddition(const QSharedPointer<ChatMessage> &msg);
    void onNotificationClicked(const QSharedPointer<ChatMessage> &msg);
    void onGetAvailableServiceAccounts();
    void onAbookReady();

private slots:
    void onCheckNewVersion();
    void onApplicationLog(QString msg);

private:
    QMap<QString, QString> ossoIconCache;
    std::chrono::seconds m_hibernateDetectInterval{300};
    std::chrono::time_point<std::chrono::steady_clock> m_hibernatePreviousTime;
    QTimer *m_newVersionCheckTimer;
};
