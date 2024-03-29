#ifndef SEARCHWINDOW_H
#define SEARCHWINDOW_H

#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#include <QQuickItem>
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"
#include "models/ChatModel.h"
#include "models/ChatMessage.h"

namespace Ui {
    class SearchWindow;
}

class SearchWindow : public QMainWindow {
    Q_OBJECT

public:
    Ui::SearchWindow *ui;
    explicit SearchWindow(Conversations *ctx, QString group_uid, QWidget *parent = nullptr);
    static Conversations *getContext();
   ~SearchWindow() override;

    ChatModel *searchModel;
    Q_PROPERTY(QString group_uid MEMBER m_group_uid CONSTANT);

signals:
  void searchResultClicked(QSharedPointer<ChatMessage> msg);
  void closed();

private slots:
  void onItemClicked(int idx);

private:
    Conversations *m_ctx;
    static SearchWindow *pSearchWindow;
    QString m_group_uid = "";
};

#endif
