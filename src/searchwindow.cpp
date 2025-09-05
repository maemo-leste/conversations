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
  m_overviewProxyModel->setDynamicSortFilter(true);
  connect(m_overviewModel, &OverviewModel::overviewRowClicked, [=](const QSharedPointer<ChatMessage>& msg){
    searchResultClicked(msg);
  });

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  // setup overview, re-use model from mainwindow
  m_overviewWidget = new OverviewWidget(m_ctx, m_overviewProxyModel, this);
  ui->centralWidget->layout()->addWidget(m_overviewWidget);
  m_overviewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  if (group_uid.isEmpty()) {
    drawContactsSearch();
  } else {
    ui->radio_contacts->hide();
    drawContentSearch();
  }

  connect(ui->radio_contacts, &QRadioButton::pressed, [this]() {
      drawContactsSearch();
  });

  connect(ui->radio_content, &QRadioButton::pressed, [this]() {
      drawContentSearch();
  });

  connect(ui->line_search, &QLineEdit::textChanged, this, &SearchWindow::onSearch);
  setInfoLabel();
}

void SearchWindow::onSearch(const QString& text) {
  QString term = text;
  term = term.toLower();
  term = term.replace("%", "");
  emit search_termChanged();

  search_term = "%%" + term + "%%";
  const bool search_contents = ui->radio_content->isChecked();
  const auto mininum_characters = search_contents ? 2 : 1;

  if (term.length() < mininum_characters) {
    this->m_overviewModel->onClear();
    setInfoLabel();
    return;
  }

  if (search_contents) {
    this->m_overviewModel->loadSearchMessages(search_term, m_group_uid.isEmpty() ? "" : m_group_uid);
  } else {
    // contact search
    auto messages = this->m_overviewModel->getOverviewMessages();
    QList<ChatMessage*> filtered_messages;
    for (const auto& message: messages) {
      const auto *raw = message->raw();
      QString name = !raw->channel.empty() ? message->name_channel() : message->name_counterparty();
      if (name.toLower().contains(term)) {
        qDebug() << "hit" << name << ":" << term;
        filtered_messages.append(message);
      }
    }
    m_overviewModel->setMessages(filtered_messages);
  }

  setInfoLabel();
}

void SearchWindow::setInfoLabel() {
  const bool search_contents = ui->radio_content->isChecked();
  const auto mininum_characters = search_contents ? 2 : 1;
  auto term = ui->line_search->text();
  term = term.replace("%", "");

  if (term.length() < mininum_characters) {
    ui->lbl_info->setText(QString("Minimum search characters: %1").arg(QString::number(mininum_characters)));
    ui->lbl_info->show();
    return;
  }

  if (m_overviewModel->rowCount() == 0) {
    ui->lbl_info->setText("No results");
    ui->lbl_info->show();
  } else {
    ui->lbl_info->hide();
  }
}

void SearchWindow::drawContactsSearch() {
  ui->radio_contacts->setChecked(true);
  ui->radio_content->setChecked(false);
  m_overviewModel->onClear();
  setInfoLabel();
}

void SearchWindow::drawContentSearch() {
  ui->radio_contacts->setChecked(false);
  ui->radio_content->setChecked(true);
  m_overviewModel->onClear();
  setInfoLabel();
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
  m_overviewModel->onClear();
  m_overviewModel->deleteLater();
  m_overviewProxyModel->deleteLater();
  delete ui;
}
