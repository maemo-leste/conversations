#pragma once
#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QComboBox>
#include <QObject>
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"

namespace Ui {
    class JoinChannelWindow;
}

class JoinChannel : public QMainWindow {
    Q_OBJECT

public:
    explicit JoinChannel(Conversations *ctx, QWidget *parent = nullptr);
    static Conversations *getContext();
    ~JoinChannel() override;
    Ui::JoinChannelWindow *ui;

signals:
    void autoCloseChatWindowsChanged(bool enabled);
    void joinChannel(QString account, QString channel);

private:
    Conversations *m_ctx;
    static JoinChannel *pJoinChannel;
    void closeEvent(QCloseEvent *event) override;

    void messageBox(QString msg);
};
