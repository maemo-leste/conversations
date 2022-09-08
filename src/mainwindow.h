#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#include <QQuickWidget>
#include <QQuickView>
#include <QQmlContext>
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "chatwindow.h"
#include "searchwindow.h"
#include "settings.h"
#include "lib/config.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(Conversations *ctx, QWidget *parent = nullptr);
    static MainWindow *getInstance();
    static Conversations *getContext();
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
    void onOpenChatWindow(const QString &remote_uid);
    void onOpenChatWindow(const QSharedPointer<ChatMessage> &msg);
    void onOpenSettingsWindow();
    void onOpenSearchWindow();
    void onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg);
    void onQuitApplication();
    void onShowApplication();
    void onHideApplication();
    void onChatWindowClosed();
    void onTPAccountManagerReady();

signals:
    void requestOverviewSearchWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QQuickWidget *m_quickWidget = nullptr;
    Conversations *m_ctx;
    static MainWindow *pMainWindow;
    ChatWindow *m_chatWindow = nullptr;
    Settings *m_settings = nullptr;
    SearchWindow *m_searchWindow = nullptr;
    bool m_autoHideWindow = true;

    void createQml();
    void destroyQml();
};

#endif