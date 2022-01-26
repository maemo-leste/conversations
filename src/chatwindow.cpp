#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QQmlContext>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>

#include "chatwindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_chatwindow.h"


ChatWindow * ChatWindow::pChatWindow = nullptr;

ChatWindow::ChatWindow(Conversations *ctx, const QString &group_uid, const QString &local_uid, const QString &remote_uid, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatWindow),
    m_group_uid(group_uid),
    m_local_uid(local_uid),
    m_remote_uid(remote_uid),
    m_ctx(ctx) {
  pChatWindow = this;
  ui->setupUi(this);

  this->chatModel = new ChatModel(this);
  this->chatModel->getMessages(remote_uid);

#ifdef MAEMO
  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);
#endif

  auto *qctx = ui->quick->rootContext();
  qctx->setContextProperty("chatModel", this->chatModel);
  qctx->setContextProperty("ctx", m_ctx);
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  qctx->setContextProperty("fixedFont", fixedFont);

  auto theme = config()->get(ConfigKeys::ChatTheme).toString();
  if(theme == "chatty")
    ui->quick->setSource(QUrl("qrc:/chatty/chatty.qml"));
  else if(theme == "irssi")
    ui->quick->setSource(QUrl("qrc:/irssi/irssi.qml"));
  else
    ui->quick->setSource(QUrl("qrc:/whatsthat/whatsthat.qml"));

  connect(this->ui->btnSend, &QPushButton::clicked, this, &ChatWindow::onGatherMessage);

  connect(m_ctx->telepathy, &Sender::databaseAddition, this, &ChatWindow::onDatabaseAddition);
}

void ChatWindow::onDatabaseAddition(ChatMessage *msg) {
  if(m_local_uid == msg->local_uid()) {
    this->chatModel->appendMessage(msg);
  }
}

void ChatWindow::onGatherMessage() {
  QString _msg = this->ui->chatBox->text();
  _msg = _msg.trimmed();
  if(_msg.isEmpty())
    return;
  emit sendMessage(m_local_uid, m_remote_uid, _msg);

  this->ui->chatBox->clear();
}

Conversations *ChatWindow::getContext(){
  return pChatWindow->m_ctx;
}

void ChatWindow::closeEvent(QCloseEvent *event) {
  this->chatModel->clear();
  emit closed();
  QWidget::closeEvent(event);
}

ChatWindow::~ChatWindow() {
  delete ui;
}
