#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

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

private slots:
    void onTextScalingValueChanged(int val);

private:
    Conversations *m_ctx;
    static Settings *pSettings;
    void closeEvent(QCloseEvent *event) override;
};

#endif
