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

#include "window_joinchannel.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_window_joinchannel.h"

JoinChannel * JoinChannel::pJoinChannel = nullptr;

JoinChannel::JoinChannel(Conversations *ctx, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::JoinChannelWindow),
    m_ctx(ctx) {
  pJoinChannel = this;
  ui->setupUi(this);

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  // @TODO: deal with additions/removals of accounts at runtime
  for(auto acc: m_ctx->telepathy->accounts) {
    ui->comboAccount->addItem(acc->name(), QList<QVariant>() << acc->local_uid);
  }

  if(!m_ctx->telepathy->accounts.empty()) {
    ui->comboAccount->setCurrentIndex(0);
    ui->lblAccountWarning->hide();
  } else {
    ui->lblAccountWarning->setText("No accounts known yet");
    ui->lblAccountWarning->show();
  }

  connect(this->ui->btnJoinChannel, &QPushButton::clicked, [this] {
    // account combobox
    auto account_combo_data = ui->comboAccount->currentData().value<QList<QVariant>>();
    auto account_local_uid = account_combo_data.first().toString();

    // channel
    QString channel = ui->lineEdit_channel->text().trimmed();

    // channel auto-join
    bool persistent = ui->checkbox_autoJoin->isChecked();

    if(channel.isEmpty()) {
      this->messageBox("channel cannot be empty");
      return;
    } else if (account_local_uid.isEmpty()) {
      this->messageBox("account cannot be empty");
      return;
    }

    m_ctx->state->setAutoJoin(account_local_uid, channel, persistent);
    emit joinChannel(account_local_uid, channel);
  });
}

void JoinChannel::messageBox(QString msg) {
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();
}

void JoinChannel::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

JoinChannel::~JoinChannel() {
  delete ui;
}
