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
#include <QCheckBox>

#include "composewindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_composewindow.h"

#include "lib/abook/abook_public.h"

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

  if (const auto path_icon_addrBook = "/usr/share/icons/hicolor/48x48/hildon/qgn_list_addressbook.png"; Utils::fileExists(path_icon_addrBook)) {
    ui->btnTo->setIcon(QIcon(path_icon_addrBook));
  }

  connect(ui->btnTo, &QPushButton::clicked, [this]() {
    auto cb = std::bind(&Compose::onContactPicked, this, std::placeholders::_1);
    abook_qt::new_dialog_contact_chooser(cb);
  });

  connect(this->ui->btnSend, &QPushButton::clicked, [this] {
    // account combobox
    auto account_combo_data = ui->comboAccount->currentData().value<QList<QVariant>>();
    const auto account_local_uid = account_combo_data.first().toString();

    const auto to = ui->line_to->text().trimmed();
    const QString msg = ui->text_msg->text().trimmed();

    qDebug() << "sending message from" << account_local_uid << "->" << to;
    emit message(account_local_uid, to, msg);
  });
}

// @TODO: from abook we cannot get the account, so that we can properly match. fix later
void Compose::onContactPicked(const std::string &persistent_uid) const {
  const QString pid = QString::fromStdString(persistent_uid);
  if (pid.count('/') != 2) return;
  const auto spl = pid.split("/");
  const auto account = spl.first();
  const auto remote = spl.last();
  if (remote.count('-') != 1) return;
  const auto remote_uid = remote.split("-").last();

  // set 'to' line
  ui->line_to->setText(remote_uid);

  // set combobox
  for (int idx = 0; idx < m_ctx->telepathy->accounts.size(); ++idx) {
    if (const auto acc = m_ctx->telepathy->accounts[idx]; acc->name().startsWith(account)) {
      ui->comboAccount->setCurrentIndex(idx);
      break;
    }
  }
}

void Compose::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

Compose::~Compose() {
  delete ui;
}
