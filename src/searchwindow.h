#ifndef SEARCHWINDOW_H
#define SEARCHWINDOW_H

#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#ifdef QUICK
#include <QQuickItem>
#endif
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"
#include "models/ChatModel.h"
#include "models/ChatMessage.h"
#include "overview/overviewwidget.h"

namespace Ui {
    class SearchWindow;
}

class SearchWindow : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(QString search_term MEMBER search_term NOTIFY search_termChanged);

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

  void search_termChanged();

private slots:
  void onItemClicked(int idx);

protected:
  void closeEvent(QCloseEvent *event) override;

private:
    void drawContactsSearch();
    void drawContentSearch();
    void resetSearch();
#ifdef QUICK
    void setupQML();
#endif

    bool m_qml = false;
    Conversations *m_ctx;
    static SearchWindow *pSearchWindow;
    QString m_group_uid = "";

    QString search_term;

    OverviewWidget* m_overviewWidget = nullptr;
    OverviewProxyModel* m_overviewProxyModel = nullptr;
};

#endif
