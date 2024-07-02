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
ChatWindow::ChatWindow(
  Conversations *ctx,
  const QString &local_uid,   // e.g idle/irc/myself
  const QString &remote_uid,  // e.g cool_username (counterparty)
  const QString &group_uid,   // e.g idle/irc/myself-##maemotest
  const QString &channel,     // e.g ##maemotest
  const QString &service_uid, // e.g RTCOM_EL_SERVICE_SMS
  QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatWindow),
    local_uid(local_uid),
    remote_uid(remote_uid),
    group_uid(group_uid),
    channel(channel),
    service_uid(service_uid),
    m_windowFocusTimer(new QTimer(this)),
    groupchat(!channel.isEmpty()),
    m_ctx(ctx) {
  pChatWindow = this;
  ui->setupUi(this);
  ui->menuBar->hide();

  qDebug() << "ChatWindow()";
  qDebug() << "local_uid:" << local_uid;
  qDebug() << "group_uid:" << group_uid;
  qDebug() << "remote_uid:" << remote_uid;
  qDebug() << "channel:" << channel;
  qDebug() << "service:" << service_uid;
  qDebug() << "groupchat:" << groupchat;

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
  this->chatModel->getMessages(service_uid, group_uid);

  // QML
  auto *qctx = ui->quick->rootContext();
  qctx->setContextProperty("chatWindow", this);
  qctx->setContextProperty("chatModel", this->chatModel);
  qctx->setContextProperty("ctx", m_ctx);
  qctx->setContextProperty("theme", m_ctx->theme);
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  qctx->setContextProperty("fixedFont", fixedFont);
  ui->quick->setAttribute(Qt::WA_AlwaysStackOnTop);

  // theme
  auto theme = config()->get(ConfigKeys::ChatTheme).toString();
  if(theme == "chatty")
    ui->quick->setSource(QUrl("qrc:/chatty/chatty.qml"));
  else if(theme == "irssi")
    ui->quick->setSource(QUrl("qrc:/irssi/irssi.qml"));
  else
    ui->quick->setSource(QUrl("qrc:/whatsthat/whatsthat.qml"));

  // auto-close inactivity timer
  m_windowFocusTimer->setInterval(1000);
  connect(m_windowFocusTimer, &QTimer::timeout, [this] {
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
  connect(m_ctx, &Conversations::enterKeySendsChatToggled, this, &ChatWindow::onEnterKeySendsChatToggled);
  auto autoCloseChatWindowsEnabled = config()->get(ConfigKeys::EnableAutoCloseChatWindows).toBool();
  this->onAutoCloseChatWindowsChanged(autoCloseChatWindowsEnabled);

  connect(this->ui->btnSend, &QPushButton::clicked, this, &ChatWindow::onGatherMessage);
  connect(m_ctx->telepathy, &Telepathy::databaseAddition, this, &ChatWindow::onDatabaseAddition);

  // groupchat
  if(groupchat) {
    this->onSetupGroupchat();
  } else {
    ui->actionLeave_channel->setVisible(false);
    ui->actionAuto_join_groupchat->setVisible(false);
  }

  connect(ui->actionAuto_join_groupchat, &QAction::triggered, this, &ChatWindow::onAutoJoinToggled);
  connect(ui->actionLeave_channel, &QAction::triggered, this, &ChatWindow::onGroupchatJoinLeaveRequested);
  connect(ui->actionClear_chat, &QAction::triggered, this, &ChatWindow::onChatRequestClear);
  connect(ui->actionDelete_chat, &QAction::triggered, this, &ChatWindow::onChatRequestDelete);
  connect(m_ctx->telepathy, &Telepathy::channelJoined, this, &ChatWindow::onChannelJoinedOrLeft);
  connect(m_ctx->telepathy, &Telepathy::channelLeft, this, &ChatWindow::onChannelJoinedOrLeft);

  connect(ui->actionExportChatToCsv, &QAction::triggered, this, &ChatWindow::onExportToCsv);
  connect(ui->actionSearchChat, &QAction::triggered, this, &ChatWindow::onOpenSearchWindow);
  connect((QObject*)ui->quick->rootObject(),
          SIGNAL(chatPreReady()), this,
          SLOT(onChatPreReady()));
  connect((QObject*)ui->quick->rootObject(),
          SIGNAL(showMessageContextMenu(int, QVariant)), this,
          SLOT(onShowMessageContextMenu(int, QVariant)));

  this->detectActiveChannel();
  this->onSetWindowTitle();
}

void ChatWindow::onShowMessageContextMenu(int event_id, QVariant point) {
  this->showMessageContextMenu(event_id, point.toPoint());
}

void ChatWindow::onChatRequestDelete() {
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(this, "Delete", "Delete this chat?", QMessageBox::Yes|QMessageBox::No);
  if(reply == QMessageBox::Yes) {
    this->onChatDelete();
  }
}

void ChatWindow::onChatRequestClear() {
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(this, "Clear", "Clear chat history?", QMessageBox::Yes|QMessageBox::No);
  if(reply == QMessageBox::Yes) {
    this->onChatClear();
  }
}

void ChatWindow::onChatDelete() {
  auto group_uid_str = group_uid.toStdString();
  auto _group_uid = group_uid_str.c_str();
  qtrtcom::deleteEvents(_group_uid);
  this->chatModel->clear();

  m_ctx->telepathy->deleteChannel(local_uid, channel);

  this->close();
}

void ChatWindow::onChatClear() {
  auto group_uid_str = group_uid.toStdString();
  auto _group_uid = group_uid_str.c_str();
  qtrtcom::deleteEvents(_group_uid);
  this->chatModel->clear();
  this->chatModel->getMessages(service_uid, group_uid);
  emit chatCleared();
}

void ChatWindow::onAutoJoinToggled() {
  m_auto_join = !m_auto_join;
  auto result = m_ctx->state->setAutoJoin(local_uid, channel, m_auto_join);
  if(!result)
    return;

  // join while we are at it
  auto chan = m_ctx->telepathy->channelByName(local_uid, channel);
  if(m_auto_join && chan.isNull())
    m_ctx->telepathy->joinChannel(local_uid, channel);

  // ui text
  if(m_auto_join) {
    ui->actionAuto_join_groupchat->setText("Disable auto-join");
  } else {
    ui->actionAuto_join_groupchat->setText("Enable auto-join");
  }
}

void ChatWindow::onExportToCsv() {
    qDebug() << __FUNCTION__;
    ChatModel::exportChatToCsv(service_uid, group_uid, this);
    QMessageBox msgBox;
    msgBox.setText(QString("File written to ~/MyDocs/"));
    msgBox.exec();
}

void ChatWindow::onAutoCloseChatWindowsChanged(bool enabled) {
  m_windowFocus = 0;
  enabled ? m_windowFocusTimer->start() : m_windowFocusTimer->stop();
}

void ChatWindow::onEnterKeySendsChatToggled(bool enabled) {
  m_enterKeySendsChat = enabled;
}

void ChatWindow::onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg) {
  m_searchWindow->close();
  m_searchWindow->deleteLater();
}

void ChatWindow::onOpenSearchWindow() {
  m_searchWindow = new SearchWindow(m_ctx, group_uid, this);
  m_searchWindow->show();

  connect(m_searchWindow, &SearchWindow::searchResultClicked, this, &ChatWindow::onSearchResultClicked);
  connect(m_searchWindow, &SearchWindow::searchResultClicked, this, &ChatWindow::onCloseSearchWindow);
}

void ChatWindow::onSearchResultClicked(const QSharedPointer<ChatMessage> &msg) {
  this->setHighlight(msg->event_id());
}

void ChatWindow::setHighlight(const unsigned int event_id) {
  emit chatPreReady();

  fillBufferUntil(event_id);
  emit jumpToMessage(event_id);

  emit chatPostReady();
}

void ChatWindow::fillBufferUntil(const unsigned int event_id) const {
  unsigned int limit = 0;
  const unsigned int perPage = 100;

  while(chatModel->eventIdToIdx(event_id) == -1) {
    chatModel->getPage(perPage);
    limit += 1;
  }
}

void ChatWindow::onChatPreReady() {
  emit chatPreReady();
  emit scrollDown();
  emit chatPostReady();
}

void ChatWindow::onDatabaseAddition(const QSharedPointer<ChatMessage> &msg) {
  if(local_uid != msg->local_uid() || group_uid != msg->group_uid())  // is this message for this chatwindow?
    return;

  if(!this->chatModel)
    return;

  this->chatModel->appendMessage(msg);

  if(m_windowActive) {
    if(this->chatModel->setMessagesRead())
      m_ctx->overviewModel->onLoad();  // refresh overview
  }
}

QString ChatWindow::remoteId() const {
  return groupchat ? channel : remote_uid;
}

void ChatWindow::onGatherMessage() {
  QString _msg = this->ui->chatBox->toPlainText();
  _msg = _msg.trimmed();
  if(_msg.isEmpty())
    return;

  emit sendMessage(local_uid, remoteId(), _msg);

  this->ui->chatBox->clear();
  this->ui->chatBox->setFocus();
}

void ChatWindow::onGroupchatJoinLeaveRequested() {
  auto chan = m_ctx->telepathy->channelByName(local_uid, channel);
  if(!chan.isNull()) {
    m_ctx->telepathy->leaveChannel(local_uid, channel);
  } else {
    m_ctx->telepathy->joinChannel(local_uid, channel);
  }
}

void ChatWindow::onSetupGroupchat() {  // do some UI stuff in case this is a groupchat
  if(!groupchat)
    return;

  // join/leave groupchat
  auto participantOfChannel = m_ctx->telepathy->channelByName(local_uid, channel);
  if(!participantOfChannel.isNull()) {
    ui->actionLeave_channel->setText("Leave groupchat");
  } else {
    ui->actionLeave_channel->setText("Join groupchat");
  }

  // auto_join text
  m_auto_join = m_ctx->state->getAutoJoin(local_uid, channel);
  QString auto_join_text = m_auto_join ? "Disable auto-join" : "Enable auto-join";
  ui->actionAuto_join_groupchat->setText(auto_join_text);
}

void ChatWindow::onChannelJoinedOrLeft(const QString &_local_uid, const QString &_channel) {
  if(_local_uid == local_uid && _channel == channel) {
    // change some UI
    this->detectActiveChannel();
    this->onSetWindowTitle();
    onSetupGroupchat();
  }
}

void ChatWindow::onSetWindowTitle() {
  QString protocol = "Unknown";
  QString windowTitle;

  QString remote_name;
  if(!this->chatModel->chats.isEmpty())
    remote_name = this->chatModel->chats.last()->remote_name();

  if(local_uid.count('/') == 2)
    windowTitle += local_uid.split("/").at(1);
  else
    windowTitle += protocol.toUpper();

  if(groupchat) {
      windowTitle +=  " - " + channel;
  } else {
    if(!remote_name.isEmpty())
      windowTitle += " - " + remote_name;
    else
      windowTitle += " - " + remote_uid;
  }

  if(!m_active && groupchat)
    windowTitle += " (not joined)";

  this->setWindowTitle(windowTitle);
}

void ChatWindow::detectActiveChannel() {
  m_active = !m_ctx->telepathy->channelByName(local_uid, channel).isNull();
}

Conversations *ChatWindow::getContext() {
  return pChatWindow->m_ctx;
}

void ChatWindow::setChatState(Tp::ChannelChatState state) const {
  if(local_uid.isEmpty() || remote_uid.isEmpty())
    return;
  m_ctx->telepathy->setChatState(local_uid, remoteId(), state);
}

bool ChatWindow::eventFilter(QObject *watched, QEvent *event) {
  switch(event->type()) {
    case QKeyEvent::KeyPress:
    {
      auto *ke = static_cast<QKeyEvent*>(event);

      if(m_enterKeySendsChat &&
         (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)) {
          this->onGatherMessage();
          return true;
      }

      break;
    }
    case QEvent::WindowActivate:
      setChatState(Tp::ChannelChatStateActive);
      break;
    case QEvent::WindowDeactivate:
      setChatState(Tp::ChannelChatStateInactive);
      break;
  }

  return QMainWindow::eventFilter(watched, event);
}

void ChatWindow::closeEvent(QCloseEvent *event) {
  setChatState(Tp::ChannelChatStateInactive);
  this->chatModel->clear();
  emit closed(group_uid);
  QWidget::closeEvent(event);
}

void ChatWindow::showMessageContextMenu(unsigned int event_id, QPoint point) {
  QMenu contextMenu("Context menu", this);

  QSharedPointer<ChatMessage> msg;
  for(const auto &_msg: chatModel->chats) {
    if(_msg->event_id() == event_id) {
      msg = _msg;
    }
  }

  if(!msg) {
    qWarning() << "message not found";
    return;
  }

  QAction actionNew("Reply", this);
  connect(&actionNew, &QAction::triggered, [this, msg]{
    this->ui->chatBox->setText(
      QString("> %1\n").arg(msg->text())
    );

    QTextCursor cursor = this->ui->chatBox->textCursor();
    cursor.movePosition(QTextCursor::End);
    this->ui->chatBox->setTextCursor(cursor);

    this->ui->chatBox->setFocus();
  });
  contextMenu.addAction(&actionNew);

  QAction actionOpen("Copy Text", this);
  connect(&actionOpen, &QAction::triggered, [msg]{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(msg->text());

    QMessageBox msgBox;
    msgBox.setText(QString("Text copied."));
    msgBox.exec();
  });
  contextMenu.addAction(&actionOpen);

  // submenu; copy weblinks
  if(!msg->weblinks().isEmpty()) {
    auto copyLinkMenu = new QMenu("Copy Link", &contextMenu);

    for(unsigned int i = 0; i != msg->weblinks().size(); i++) {
      auto weblink = msg->weblinks().at(i);

      auto actionCopyLink = new QAction(QString("Link #%1").arg(QString::number(i)), &contextMenu);
      connect(actionCopyLink, &QAction::triggered, [this, weblink] {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(weblink);

        QMessageBox msgBox;
        msgBox.setText(QString("Link copied."));
        msgBox.exec();
      });

      copyLinkMenu->addAction(actionCopyLink);
    }
    contextMenu.addMenu(copyLinkMenu);
  }

  // submenu; open weblinks
  if(!msg->weblinks().isEmpty()) {
    auto openLinkMenu = new QMenu("Open Link", &contextMenu);

    for(unsigned int i = 0; i != msg->weblinks().size(); i++) {
      auto weblink = msg->weblinks().at(i);

      auto actionOpenLink = new QAction(QString("Link #%1").arg(QString::number(i)), &contextMenu);
      connect(actionOpenLink, &QAction::triggered, [this, weblink] {
        auto webLinkStr = weblink.toStdString();
        GError *error = NULL;
        HildonURIAction *action = hildon_uri_get_default_action_by_uri(webLinkStr.c_str(), &error);

        if(error != NULL) {
         qWarning() << QString("Could not get default action by uri, error: %1->'%s'").arg(
            QString::number(error->code),
            QString::fromLocal8Bit(error->message));
        } else {
          hildon_uri_open(webLinkStr.c_str(), action, &error);
          if(error != NULL)
            qWarning() << weblink << "-" << QString::fromLocal8Bit(error->message);
        }

        g_error_free(error);
      });

      openLinkMenu->addAction(actionOpenLink);
    }
    contextMenu.addMenu(openLinkMenu);
  }

  contextMenu.exec(mapToGlobal({point.x(), point.y()}));
}


void ChatWindow::changeEvent(QEvent *event) {
  if(event->type() == QEvent::ActivationChange) {
    // set message_read
    auto changed = m_windowActive != this->isActiveWindow();
    m_windowActive = this->isActiveWindow();

    if(m_windowActive) {
      if(this->chatModel->setMessagesRead())
        m_ctx->overviewModel->onLoad();  // refresh overview
    }
  }
}

ChatWindow::~ChatWindow() {
  qDebug() << "destroying chatWindow";
  delete ui;
}
