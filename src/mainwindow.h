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
    void onOpenChatWindow(const QString &remote_uid);
    void onOpenChatWindow(const QString &group_uid, const QString &local_uid, const QString &remote_uid);
    void onOpenSettingsWindow();
    void onShowApplication();
    void onHideApplication();
    void onChatWindowClosed();

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

    void createQml();
    void destroyQml();
};

#endif