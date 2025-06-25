#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#ifdef QUICK
#include <QQmlContext>
#endif
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QTextEdit>

#include "searchwindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_searchwindow_widgets.h"

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

  m_overviewModel = new OverviewModel(m_ctx->telepathy, m_ctx->state, this);
  m_overviewProxyModel = new OverviewProxyModel(this);
  m_overviewProxyModel->setSourceModel(m_overviewModel);
  m_overviewProxyModel->setSortRole(OverviewModel::TimeRole);
  m_overviewProxyModel->sort(OverviewModel::TimeRole, Qt::DescendingOrder);
  m_overviewProxyModel->setDynamicSortFilter(true);
  connect(m_overviewModel, &OverviewModel::overviewRowClicked, [=](const QSharedPointer<ChatMessage>& msg){
    searchResultClicked(msg);
  });

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
    const bool search_contents = ui->radio_content->isChecked();
    const auto mininum_characters = search_contents ? 3 : 1;

    if(search_term.length() >= mininum_characters) {
      if(search_contents)
        if (m_group_uid.isEmpty())
          this->m_overviewModel->loadSearchMessages("%%" + search_term + "%%");
        else
          this->m_overviewModel->loadSearchMessages("%%" + search_term + "%%", m_group_uid);
      else
        m_overviewProxyModel->setNameFilter(search_term);
    } else {  // clear
      if(search_contents)
        m_overviewModel->onClear();
      else
        m_overviewProxyModel->setNameFilter("");
    }
  });

  // setup overview, re-use model from mainwindow
  m_overviewWidget = new OverviewWidget(m_ctx, m_overviewProxyModel, this);
  ui->centralWidget->layout()->addWidget(m_overviewWidget);
  m_overviewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SearchWindow::drawContactsSearch() {
  ui->radio_contacts->setChecked(true);
  ui->radio_content->setChecked(false);
  resetSearch();
}

void SearchWindow::drawContentSearch() {
  ui->radio_contacts->setChecked(false);
  ui->radio_content->setChecked(true);
  resetSearch();
}

void SearchWindow::resetSearch() {
  ui->line_search->setText("");
  emit search_termChanged();
  m_overviewProxyModel->setNameFilter("");
  m_overviewModel->loadOverviewMessages();
}

Conversations *SearchWindow::getContext(){
  return pSearchWindow->m_ctx;
}

void SearchWindow::closeEvent(QCloseEvent *event) {
  qDebug() << "closing";
  QWidget::closeEvent(event);
}

SearchWindow::~SearchWindow() {
  qDebug() << "destroying SearchWindow";
  resetSearch();
  m_overviewModel->deleteLater();
  m_overviewProxyModel->deleteLater();
  delete ui;
}
