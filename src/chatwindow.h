#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"
#include "models/ChatModel.h"

namespace Ui {
    class ChatWindow;
}

class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    Ui::ChatWindow *ui;
    explicit ChatWindow(Conversations *ctx, const QString &group_uid, const QString &local_uid, const QString &remote_uid, QWidget *parent = nullptr);
    static Conversations *getContext();
    ~ChatWindow() override;

    ChatModel *chatModel;

private slots:
    void onGatherMessage();

signals:
    void sendMessage(const QString &message);

private:
    Conversations *m_ctx;
    static ChatWindow *pChatWindow;
    QString m_remote_uid;
    QString m_local_uid;
    QString m_group_uid;
    void closeEvent(QCloseEvent *event) override;
};

#endif