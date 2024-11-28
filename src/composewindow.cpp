#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QQmlContext>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QCheckBox>

#include "composewindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_composewindow.h"

Compose * Compose::pCompose = nullptr;

Compose::Compose(Conversations *ctx, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ComposeWindow),
    m_ctx(ctx) {
  pCompose = this;
  ui->setupUi(this);

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  // @TODO: this code will be removed, we will start new messages from the addressbook instead
  for(auto acc: m_ctx->telepathy->accounts) {
    ui->comboAccount->addItem(acc->name(), QList<QVariant>() << acc->local_uid);
  }

  if(!m_ctx->telepathy->accounts.empty())
    ui->comboAccount->setCurrentIndex(0);

  connect(this->ui->btnSend, &QPushButton::clicked, [this] {
    // account combobox
    auto account_combo_data = ui->comboAccount->currentData().value<QList<QVariant>>();
    auto account_local_uid = account_combo_data.first().toString();

    auto to = ui->line_to->text().trimmed();
    QString msg = ui->text_msg->toPlainText().trimmed();

    qDebug() << "sending message from" << account_local_uid << "->" << to;
    emit message(account_local_uid, to, msg);
  });
}

void Compose::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

Compose::~Compose() {
  delete ui;
}
