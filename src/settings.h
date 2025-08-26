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
    class Settings;
}

class Settings : public QMainWindow {
    Q_OBJECT

public:
    explicit Settings(Conversations *ctx, QWidget *parent = nullptr);
    static Conversations *getContext();
    ~Settings() override;
    Ui::Settings *ui;

signals:
    void textScalingChanged();
    void autoCloseChatWindowsChanged(bool enabled);
    void inheritSystemThemeToggled(bool enabled);
    void enableDisplayGroupchatJoinLeaveToggled(bool enabled);
    void enableDisplayAvatarsToggled(bool enabled);
    void enableGPUAccel(bool enabled);
    void enableDisplayChatGradientToggled(bool enabled);
    void enableSlimToggled(bool enabled);
    void enterKeySendsChatToggled(bool enabled);
    void enableLinkPreviewEnabledToggled(bool enabled);
    void enableLinkPreviewImageEnabledToggled(bool enabled);
    void enableLinkPreviewRequiresUserInteractionToggled(bool enabled);

private slots:
    void onTextScalingValueChanged(int val);

private:
    Conversations *m_ctx;
    static Settings *pSettings;
    void closeEvent(QCloseEvent *event) override;
};

