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

#include "chatwindow.h"
#include "mainwindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_chatwindow.h"


ChatWindow * ChatWindow::pChatWindow = nullptr;
ChatWindow::ChatWindow(Conversations *ctx, QSharedPointer<ChatMessage> msg, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatWindow),
    m_chatMessage(msg),
    m_windowFocusTimer(new QTimer(this)),
    m_ctx(ctx) {
  pChatWindow = this;
  ui->setupUi(this);
  ui->menuBar->hide();

  // [window]
  // title
  QString protocol = "Unknown";
  if(m_chatMessage->local_uid().count('/') == 2)
    protocol = m_chatMessage->local_uid().split("/").at(1);
  this->setWindowTitle(QString("%1 - %2").arg(protocol.toUpper(), m_chatMessage->remote_uid()));
  // properties
  setProperty("X-Maemo-Orientation", 2);
  setProperty("X-Maemo-StackedWindow", 0);

  // [chatBox]
  ui->chatBox->setFocus();
  // force chatEdit widget to 1 line (visually)
  QFontMetrics metrics(ui->chatBox->font());
  int lineHeight = metrics.lineSpacing();
  int margins = 25;  // ew, hardcoded.
  ui->chatBox->setFixedHeight(lineHeight + (margins*2));
  // catch Enter/RETURN
  ui->chatBox->installEventFilter(this);
  m_enterKeySendsChat = config()->get(ConfigKeys::EnterKeySendsChat).toBool();

  this->chatModel = new ChatModel(this);
  this->chatModel->getMessages(m_chatMessage->service(), m_chatMessage->group_uid());
  if(m_chatMessage->isSearchResult)
    fillBufferUntil(m_chatMessage);

  auto *qctx = ui->quick->rootContext();
  qctx->setContextProperty("chatWindow", this);
  qctx->setContextProperty("chatModel", this->chatModel);
  qctx->setContextProperty("ctx", m_ctx);
  qctx->setContextProperty("theme", m_ctx->theme);
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  qctx->setContextProperty("fixedFont", fixedFont);
  MainWindow::qmlInjectPalette(qctx, m_ctx);

  ui->quick->setAttribute(Qt::WA_AlwaysStackOnTop);

  auto theme = config()->get(ConfigKeys::ChatTheme).toString();
  if(theme == "chatty")
    ui->quick->setSource(QUrl("qrc:/chatty/chatty.qml"));
  else if(theme == "irssi")
    ui->quick->setSource(QUrl("qrc:/irssi/irssi.qml"));
  else
    ui->quick->setSource(QUrl("qrc:/whatsthat/whatsthat.qml"));

  // auto-close inactivity timer
  m_windowFocusTimer->setInterval(1000);
  connect(m_windowFocusTimer, &QTimer::timeout, [=] {
     auto *window = QApplication::activeWindow();
     if(window == nullptr || window->windowTitle() != this->windowTitle()) {
       m_windowFocus += 1;
       if(m_windowFocus == 60*15) {  // 15 minutes
         this->close();
       }
     } else {
      m_windowFocus = 0;
    }
  });
  connect(m_ctx, &Conversations::autoCloseChatWindowsChanged, this, &ChatWindow::onAutoCloseChatWindowsChanged);
  auto autoCloseChatWindowsEnabled = config()->get(ConfigKeys::EnableAutoCloseChatWindows).toBool();
  this->onAutoCloseChatWindowsChanged(autoCloseChatWindowsEnabled);

  connect(this->ui->btnSend, &QPushButton::clicked, this, &ChatWindow::onGatherMessage);
  connect(m_ctx->telepathy, &Telepathy::databaseAddition, this, &ChatWindow::onDatabaseAddition);

  connect(ui->actionExportChatToCsv, &QAction::triggered, this, &ChatWindow::onExportToCsv);
  connect(ui->actionSearchChat, &QAction::triggered, this, &ChatWindow::onOpenSearchWindow);
  connect((QObject*)ui->quick->rootObject(),
          SIGNAL(chatPreReady()), this,
          SLOT(onChatPreReady()));
}

void ChatWindow::onExportToCsv() {
    qDebug() << __FUNCTION__;
    ChatModel::exportChatToCsv(m_chatMessage->service(), m_chatMessage->group_uid(), this);
    QMessageBox msgBox;
    msgBox.setText(QString("File written to ~/MyDocs/"));
    msgBox.exec();
}

void ChatWindow::onAutoCloseChatWindowsChanged(bool enabled) {
  m_windowFocus = 0;
  enabled ? m_windowFocusTimer->start() : m_windowFocusTimer->stop();
}

void ChatWindow::onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg) {
  m_searchWindow->close();
  m_searchWindow->deleteLater();
}

void ChatWindow::onOpenSearchWindow() {
  m_searchWindow = new SearchWindow(m_ctx, m_chatMessage->group_uid(), this);
  m_searchWindow->show();

  connect(m_searchWindow,
          SIGNAL(searchResultClicked(QSharedPointer<ChatMessage>)), this,
          SLOT(onSearchResultClicked(QSharedPointer<ChatMessage>)));

  connect(m_searchWindow, &SearchWindow::searchResultClicked, this, &ChatWindow::onCloseSearchWindow);
}

void ChatWindow::onSearchResultClicked(const QSharedPointer<ChatMessage> &msg) {
  m_chatMessage = msg;
  this->onChatPreReady();
}

void ChatWindow::fillBufferUntil(const QSharedPointer<ChatMessage> &msg) const {
  // fill the message buffer list until we find the relevant message
  qDebug() << __FUNCTION__;
  unsigned int limit = 0;
  const unsigned int perPage = 100;

  while(chatModel->eventIdToIdx(msg->event_id()) == -1) {
    chatModel->getPage(perPage);
    limit += 1;
  }
}

void ChatWindow::onChatPreReady() {
  qDebug() << __FUNCTION__;
  emit chatPreReady();

  // check if we need to 'jump' to a 'requested' message;
  if(m_chatMessage->isSearchResult) {
    fillBufferUntil(m_chatMessage);
    qDebug() << "jump to message " << m_chatMessage->event_id();
    emit jumpToMessage(m_chatMessage->event_id());
  } else {
    qDebug() << "forcing scroll down";
    emit scrollDown();
  }

  emit chatPostReady();
}

void ChatWindow::onDatabaseAddition(const QSharedPointer<ChatMessage> &msg) {
  if(m_chatMessage->local_uid() == msg->local_uid() && m_chatMessage->group_uid() == msg->group_uid()) {
    this->chatModel->appendMessage(msg);
  }
}

void ChatWindow::onGatherMessage() {
  QString _msg = this->ui->chatBox->toPlainText();
  _msg = _msg.trimmed();
  if(_msg.isEmpty())
    return;

  if (m_chatMessage->channel() != "") {
    emit sendMessage(m_chatMessage->local_uid(), m_chatMessage->channel(), _msg);
  } else {
    emit sendMessage(m_chatMessage->local_uid(), m_chatMessage->remote_uid(), _msg);
  }

  this->ui->chatBox->clear();
}

Conversations *ChatWindow::getContext() {
  return pChatWindow->m_ctx;
}

bool ChatWindow::eventFilter(QObject *watched, QEvent *event) {
  if(event->type() == QKeyEvent::KeyPress) {
    auto *ke = static_cast<QKeyEvent*>(event);
    if(m_enterKeySendsChat &&
       (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)) {
      this->onGatherMessage();
      return true;
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void ChatWindow::closeEvent(QCloseEvent *event) {
  this->chatModel->clear();
  emit closed(m_chatMessage->group_uid());
  m_chatMessage.clear();
  QWidget::closeEvent(event);
}

ChatWindow::~ChatWindow() {
  qDebug() << "destroying chatWindow";
  delete ui;
}
