#pragma once
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

namespace Ui {
    class ComposeWindow;
}

class Compose : public QMainWindow {
    Q_OBJECT

public:
    explicit Compose(Conversations *ctx, QWidget *parent = nullptr);
    static Conversations *getContext();
    ~Compose() override;
    void onContactPicked(const std::string &persistent_uid) const;
    Ui::ComposeWindow *ui;

signals:
    void autoCloseChatWindowsChanged(bool enabled);
    void message(QString account, QString to, QString msg);

private:
    Conversations *m_ctx;
    static Compose *pCompose;
    void closeEvent(QCloseEvent *event) override;
};
