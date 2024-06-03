#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QQmlContext>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QTextEdit>

#include "searchwindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_searchwindow.h"


SearchWindow * SearchWindow::pSearchWindow = nullptr;
SearchWindow::SearchWindow(Conversations *ctx, QString group_uid, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SearchWindow),
    m_group_uid(group_uid),
    m_ctx(ctx) {
  pSearchWindow = this;
  ui->setupUi(this);

  this->searchModel = new ChatModel(this);

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  auto *qctx = ui->quick->rootContext();
  qctx->setContextProperty("searchWindow", this);
  qctx->setContextProperty("chatSearchModel", this->searchModel);
  qctx->setContextProperty("ctx", m_ctx);
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  qctx->setContextProperty("fixedFont", fixedFont);

  ui->quick->setSource(QUrl("qrc:/qml/Search.qml"));

  connect((QObject*)ui->quick->rootObject(), SIGNAL(itemClicked(int)), this, SLOT(onItemClicked(int)));
}

void SearchWindow::onItemClicked(int idx) {
  auto msg = searchModel->chats.at(idx);
  emit searchResultClicked(msg);
}

Conversations *SearchWindow::getContext(){
  return pSearchWindow->m_ctx;
}

SearchWindow::~SearchWindow() {
  qDebug() << "destroying chatWindow";
  delete ui;
}
