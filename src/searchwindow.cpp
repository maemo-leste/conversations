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
  setAttribute(Qt::WA_DeleteOnClose);
  pSearchWindow = this;
  ui->setupUi(this);

  ui->line_search->setFocus();

  this->searchModel = new ChatModel(this);

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  // contacts view is default
  drawContactsSearch();

  connect(ui->radio_contacts, &QRadioButton::pressed, [this]() {
      drawContactsSearch();
  });

  connect(ui->radio_content, &QRadioButton::pressed, [this]() {
      drawContentSearch();
  });

  connect(ui->line_search, &QLineEdit::textChanged, [this](const QString &text) {
    QString term = text;
    term = term.replace("%", "");
    emit search_termChanged();

    search_term = term;

    if(search_term.length() >= 3) {
      if(ui->radio_content->isChecked())
        this->searchModel->searchMessages("%%" + search_term + "%%");
      else
        m_ctx->overviewProxyModel->setNameFilter(search_term);
    } else {  // clear
      if(ui->radio_content->isChecked())
        this->searchModel->clear();
      else
        m_ctx->overviewProxyModel->setNameFilter("");
    }
  });

  // setup overview, re-use model from mainwindow
  m_overviewWidget = new OverviewWidget(m_ctx,  this);
  ui->frame_contacts->layout()->addWidget(m_overviewWidget);
}

void SearchWindow::drawContactsSearch() {
  ui->frame_contacts->show();
  ui->frame_content->hide();
  ui->radio_contacts->setChecked(true);
  ui->radio_content->setChecked(false);
  resetSearch();
}

void SearchWindow::drawContentSearch() {
  if(!m_qml)
    setupQML();

  ui->frame_contacts->hide();
  ui->frame_content->show();
  ui->radio_contacts->setChecked(false);
  ui->radio_content->setChecked(true);
  resetSearch();
}

void SearchWindow::resetSearch() {
  ui->line_search->setText("");
  emit search_termChanged();
  m_ctx->overviewProxyModel->setNameFilter("");
  this->searchModel->clear();
}

void SearchWindow::onItemClicked(int idx) {
  auto msg = searchModel->chats.at(idx);
  emit searchResultClicked(msg);
}

Conversations *SearchWindow::getContext(){
  return pSearchWindow->m_ctx;
}

void SearchWindow::closeEvent(QCloseEvent *event) {
  qDebug() << "closing";
  QWidget::closeEvent(event);
}

void SearchWindow::setupQML() {
  auto *qctx = ui->quick->rootContext();
  qctx->setContextProperty("searchWindow", this);
  qctx->setContextProperty("chatSearchModel", this->searchModel);
  qctx->setContextProperty("ctx", m_ctx);
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  qctx->setContextProperty("fixedFont", fixedFont);

  ui->quick->setSource(QUrl("qrc:/qml/Search.qml"));

  connect((QObject*)ui->quick->rootObject(), SIGNAL(itemClicked(int)), this, SLOT(onItemClicked(int)));
  m_qml = true;
}

SearchWindow::~SearchWindow() {
  qDebug() << "destroying SearchWindow";
  resetSearch();
  delete ui;
}
