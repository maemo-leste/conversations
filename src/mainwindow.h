#pragma once
#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#include <QtCore>
#include <QLabel>
#include <QtGui>
#include <QFileInfo>
#include <QPalette>

#include <malloc.h>
#include <iostream>

#include "conversations.h"
#include "chatwindow.h"
#include "searchwindow.h"
#include "settings.h"
#include "composewindow.h"
#include "window_joinchannel.h"
#include "overview/overviewwidget.h"
#include "lib/config.h"
#include "lib/libnotify-qt/Notification.h"

namespace Ui {
    class MainWindow;
}

struct FilterProtocolItem {
    QString title;
    QString filterKey;
    QAction *action;
};

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(Conversations *ctx, QWidget *parent = nullptr);
    static MainWindow *getInstance();
    static Conversations *getContext();
    static QWidget *getChatWindow(const QString &group_uid);
    ~MainWindow() override;
    Ui::MainWindow *ui;

    qreal screenDpiRef;
    QRect screenGeo;
    QRect screenRect;
    qreal screenDpi;
    qreal screenDpiPhysical;
    qreal screenRatio;

public slots:
    void onOpenChatWindow(int idx);
    void onOpenChatWindow(QString local_uid, QString remote_uid, QString group_uid, QString channel, QString service);
    void onOpenChatWindow(const QSharedPointer<ChatMessage> &msg);
    void onOpenChatWindowWithHighlight(const QSharedPointer<ChatMessage> &msg);
    void onOpenSettingsWindow();
    void onOpenComposeWindow();
    void onOpenSearchWindow();
    void onOpenJoinChatWindow();
    void onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg);
    void onQuitApplication();
    void onShowApplication();
    void onHideApplication();
    void onChatWindowClosed(const QString &remote_uid);
    void onTPAccountManagerReady();
    void onNotificationClicked(const QSharedPointer<ChatMessage> &msg);
    void onFriendRequest(QSharedPointer<ContactItem> item);

private slots:
    void onProtocolFilterClicked(const QString service);
    void onShowWelcomePage() const;
    void onShowOverviewPage() const;
    void onDeterminePage() const;
    void onShowEmptyDbPage() const;

signals:
    void requestOverviewSearchWindow();
    void inheritSystemThemeChanged(bool toggled);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    OverviewWidget *m_widgetOverview = nullptr;
    Conversations *m_ctx;
    static MainWindow *pMainWindow;
    QMap<QString, ChatWindow*> m_chatWindows;
    Settings *m_settings = nullptr;
    Compose *m_compose = nullptr;
    JoinChannel *m_joinchannel = nullptr;
    SearchWindow *m_searchWindow = nullptr;
    bool m_autoHideWindow = true;
    QMap<QString, FilterProtocolItem*> m_filterProtocols;
    QActionGroup *m_filters;
    void onSetupUIAccounts();
    QAction *addProtocol(const QString title, const QString service);
};
