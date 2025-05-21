#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QCheckBox>

#include "window_joinchannel.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include <QInputDialog>

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

  // button: choose account
  connect(ui->btn_accounts, &QPushButton::clicked, [this] {
    show_account_dialog();
  });

  // button: submit form
  connect(this->ui->btnJoinChannel, &QPushButton::clicked, [this] {
    submit();
  });

  // update UI on Telepathy account additions/removals
  const auto tp = Conversations::instance()->telepathy;
  connect(tp, &Telepathy::accountAdded, [this](TelepathyAccountPtr acc) { onAccountsChanged(); });
  connect(tp, &Telepathy::accountRemoved, [this] { onAccountsChanged(); });
  onAccountsChanged();
}

void JoinChannel::onAccountsChanged() {
  auto accounts = groupchat_accounts();
  const bool has_accounts = !accounts.isEmpty();
  ui->btn_accounts->setEnabled(has_accounts);

  if (has_accounts) {
    ui->lblAccountWarning->hide();
    if (!m_acc)
      selectItem(accounts.first());
  } else {
    m_acc.clear();
    ui->lblAccountWarning->setText("No accounts known yet");
    ui->lblAccountWarning->show();
  }
}

void JoinChannel::submit() {
  if (!m_acc)
    return this->messageBox("account cannot be empty");

  // @TODO: stop matching protocols w/ strings, we desperately need enum @ TelepathyAccount::
  const QString protocol = m_acc->protocolName();
  QString channel;
  const bool persistent = ui->checkbox_autoJoin->isChecked();

  if(protocol == "matrix") {
    auto matrix_channel = ui->line_matrix_channel->text().trimmed();
    if (matrix_channel.startsWith("#"))
      matrix_channel.remove(0, 1);
    const auto matrix_server = ui->line_matrix_server->text().trimmed();

    if (matrix_channel.isEmpty() || matrix_server.isEmpty())
      return this->messageBox("channel and/or server cannot be empty");

    channel = "#" + matrix_channel + ":" + matrix_server;
  } else if (protocol == "irc" or protocol == "idle") {
    auto irc_channel = ui->line_irc->text().trimmed();
    if (irc_channel.startsWith("#"))
      irc_channel.remove(0, 1);

    if (irc_channel.isEmpty())
      return this->messageBox("channel cannot be empty");

    channel = "#" + irc_channel;
  } else if (protocol == "xmpp" or protocol == "jabber" or protocol == "gabble") {
    auto xmpp_channel = ui->line_xmpp->text().trimmed();

    if (xmpp_channel.isEmpty())
      return this->messageBox("channel cannot be empty");

    if (m_acc->getServerHost().isEmpty())
      return this->messageBox("could not parse server_host from acc, this is a bug");

    channel = xmpp_channel + "@conference." + m_acc->getServerHost();
  } else {
    channel = ui->line_default->text().trimmed();
  }

  if (channel.isEmpty())
    return this->messageBox("could not parse channel, this is a bug");

  m_ctx->state->setAutoJoin(m_acc->local_uid, channel, persistent);
  emit joinChannel(m_acc->local_uid, channel);
}

void JoinChannel::messageBox(QString msg) {
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();
}

void JoinChannel::selectItem(const QString &item) {
  for(auto accounts = groupchat_accounts(); const auto acc: accounts) {
    if (acc->local_uid == item) {
      selectItem(acc);
      break;
    }
  }
}

void JoinChannel::selectItem(TelepathyAccountPtr account_ptr) {
  if (!account_ptr) return;

  ui->frame_default->hide();
  ui->frame_irc->hide();
  ui->frame_xmpp->hide();
  ui->frame_matrix->hide();

  m_acc = account_ptr;

  const auto protocol = m_acc->protocolName();
  qDebug() << protocol;
  ui->btn_accounts->setText(m_acc->protocolName() + " - " + m_acc->nickname());
  if (protocol == "irc") {
    ui->line_irc->setFocus();
    return ui->frame_irc->show();
  }

  if (protocol == "xmpp" or protocol == "jabber") {
    ui->line_xmpp->setFocus();
    return ui->frame_xmpp->show();
  }

  if (protocol == "matrix" or protocol == "tank") {
    ui->line_matrix_channel->setFocus();

    // @TODO: support getting Matrix connection host.tld @ TelepathyAccount::getServerHost()
    if (const QString server_host = m_acc->getServerHost(); !server_host.isEmpty())
      ui->line_matrix_server->setText(server_host);

    if (m_acc->getServerHost().isEmpty())
    return ui->frame_matrix->show();
  }

  if (protocol == "telegram") {
    // @TODO: if necessary...
    ui->line_default->setFocus();
    return ui->frame_default->show();
  }

  return ui->frame_default->show();
}

void JoinChannel::show_account_dialog() {
  QString selected_ui;

  QDialog dialog;
  dialog.setWindowTitle("Choose account");
  auto *mainLayout = new QVBoxLayout(&dialog);
  auto *scrollArea = new QScrollArea(&dialog);
  scrollArea->setWidgetResizable(true);
  mainLayout->addWidget(scrollArea);
  auto *container = new QWidget();
  auto *containerLayout = new QVBoxLayout(container);

  for(const auto acc: groupchat_accounts()) {
    auto name = acc->protocolName() + " - " + acc->nickname();
    auto uid = acc->local_uid;

    auto *button = new QPushButton(name, container);
    button->setProperty("uid", QVariant::fromValue(uid));
    containerLayout->addWidget(button);

    connect(button, &QPushButton::clicked, [&dialog, &selected_ui, uid] {
      selected_ui = uid;
      dialog.accept();
    });
  }

  container->setLayout(containerLayout);
  scrollArea->setWidget(container);

  if (dialog.exec() == QDialog::Accepted)
    selectItem(selected_ui);
}

void JoinChannel::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

JoinChannel::~JoinChannel() {
  delete ui;
}
