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

#include "conversations.h"
#include "chatwindow.h"
#include "mainwindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#ifdef QUICK
#include "ui_chatwindow.h"
#else
#include "ui_chatwindow_widgets.h"
#endif

ChatWindow * ChatWindow::pChatWindow = nullptr;
ChatWindow::ChatWindow(
  Conversations *ctx,
  const QString &local_uid,   // e.g idle/irc/myself
  const QString &remote_uid,  // e.g cool_username (counterparty)
  const QString &group_uid,   // e.g idle/irc/myself-##maemotest
  const QString &channel,     // e.g ##maemotest
  const QString &service_uid, // e.g RTCOM_EL_SERVICE_SMS
  QWidget *parent) :
    QConversationsMainWindow(ctx, parent),
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
  m_enterKeySendsChat = config()->get(ConfigKeys::EnterKeySendsChat).toBool();
  onDisplayChatBox();

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

#ifdef QUICK
   this->chatModel = new ChatModel(true, this);
#else
  this->chatModel = new ChatModel(false, this);
#endif
   connect(this->chatModel, &ChatModel::previewItemClicked, this, &ChatWindow::onPreviewItemClicked);
#ifndef QUICK
  this->chatModel->setLimit(-1);
#endif
   this->chatModel->getMessages(service_uid, group_uid);

   linkPreviewRequiresUserInteraction = config()->get(ConfigKeys::LinkPreviewRequiresUserInteraction).toBool();

   // QML
#ifdef QUICK
   const auto gpu_accel = config()->get(ConfigKeys::EnableGPUAccel).toBool();
   bgMatrixRainEnabled = gpu_accel && config()->get(ConfigKeys::EnableMatrixRainBackground).toBool();
   connect(m_ctx, &Conversations::bgMatrixRainEnabledChanged, this, &ChatWindow::onBgMatrixRainEnabledChanged);

   ui->quick->installEventFilter(this);
   auto *qctx = ui->quick->rootContext();
   qctx->setContextProperty("chatWindow", this);
   qctx->setContextProperty("chatModel", this->chatModel);
   qctx->setContextProperty("ctx", m_ctx);
   qctx->setContextProperty("theme", m_ctx->theme);
   const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);  // has no effect on Leste?
   qctx->setContextProperty("fixedFont", fixedFont);
   ui->quick->setAttribute(Qt::WA_AlwaysStackOnTop);
   ui->quick->engine()->addImageProvider("avatar", new AvatarImageProvider);
   ui->quick->engine()->addImageProvider("previewImage", new PreviewImageProvider);

   // QML theme
   const auto theme = config()->get(ConfigKeys::ChatTheme).toString();
   if(theme == "chatty")
     ui->quick->setSource(QUrl("qrc:/chatty/chatty.qml"));
   else if(theme == "irssi")
     ui->quick->setSource(QUrl("qrc:/irssi/irssi.qml"));
   else
     ui->quick->setSource(QUrl("qrc:/whatsthat/whatsthat.qml"));
#endif

  // auto-close inactivity timer
  m_windowFocusTimer->setInterval(1000);
  connect(m_windowFocusTimer, &QTimer::timeout, [this] {
     const auto *window = QApplication::activeWindow();
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
    ui->actionParticipants->setEnabled(true);
  } else {
    ui->actionLeave_channel->setVisible(false);
    ui->actionAuto_join_groupchat->setVisible(false);
    ui->actionParticipants->setEnabled(false);
  }

  connect(ui->actionAuto_join_groupchat, &QAction::triggered, this, &ChatWindow::onAutoJoinToggled);
  connect(ui->actionLeave_channel, &QAction::triggered, this, &ChatWindow::onGroupchatJoinLeaveRequested);
  connect(ui->actionClear_chat, &QAction::triggered, this, &ChatWindow::onChatRequestClear);
  connect(ui->actionDelete_chat, &QAction::triggered, this, &ChatWindow::onChatRequestDelete);
  connect(ui->actionIgnore_notifications, &QAction::triggered, this, &ChatWindow::onIgnoreNotificationsToggled);
  connect(m_ctx->telepathy, &Telepathy::channelJoined, this, &ChatWindow::onChannelJoinedOrLeft);
  connect(m_ctx->telepathy, &Telepathy::channelLeft, this, &ChatWindow::onChannelJoinedOrLeft);
  connect(ui->actionParticipants, &QAction::triggered, this, &ChatWindow::onOpenTpContactsWindow);

  connect(ui->actionExportChatToCsv, &QAction::triggered, this, &ChatWindow::onExportToCsv);
  connect(ui->actionSearchChat, &QAction::triggered, this, &ChatWindow::onOpenSearchWindow);
#ifdef QUICK
  connect((QObject*)ui->quick->rootObject(),
          SIGNAL(chatPreReady()), this,
          SLOT(onChatPreReady()));
  connect((QObject*)ui->quick->rootObject(),
          SIGNAL(showMessageContextMenu(int, QVariant)), this,
          SLOT(onShowMessageContextMenu(int, QVariant)));
#endif
  this->detectActiveChannel();
  this->onSetWindowTitle();
  this->onSetupAuthorizeActions();

  // menu: add/remove friend
  connect(m_ctx->telepathy, &Telepathy::contactsChanged, this, &ChatWindow::onSetupAuthorizeActions);
  connect(m_ctx->telepathy, &Telepathy::rosterChanged, this, &ChatWindow::onSetupAuthorizeActions);
  // @TODO: should add/remove friends be handled by conversations or osso-abook?
  // connect(ui->actionAddFriend, &QAction::triggered, this, &ChatWindow::onAddFriend);
  // connect(ui->actionRemoveFriend, &QAction::triggered, this, &ChatWindow::onRemoveFriend);
  // connect(ui->actionAcceptFriendRequest, &QAction::triggered, this, &ChatWindow::onAcceptFriend);
  // connect(ui->actionRejectFriendRequest, &QAction::triggered, this, &ChatWindow::onRejectFriend);

  connect(m_ctx, &Conversations::avatarChanged, this, &ChatWindow::onAvatarChanged);

  // ignore notifications
  m_ignore_notifications = m_ctx->state->getNotificationsIgnore(local_uid, !channel.isEmpty() ? channel : remote_uid);
  if(m_ignore_notifications) {
    ui->actionIgnore_notifications->setText("Enable notifications");
  } else {
    ui->actionIgnore_notifications->setText("Ignore notifications");
  }

  m_windowHeight = this->height();
  this->show();

  // chatBox
  ui->chatBox_multi->setFocus();
  // hack to calculate initial line height
  // 1) send event synchronously
  // 2) textBox generates some richText
  // 3) calculate height based on that
  // 4) reset
  // other approaches have failed
  // note: depends on this->show()
  QKeyEvent press(QEvent::KeyPress, Qt::Key_E, Qt::NoModifier, "e");
  QKeyEvent release(QEvent::KeyRelease, Qt::Key_E, Qt::NoModifier, "e");
  QCoreApplication::sendEvent(ui->chatBox_multi, &press);
  QCoreApplication::sendEvent(ui->chatBox_multi, &release);
  dynamicInputTextHeight(ui->chatBox_multi);
  ui->chatBox_multi->setText("");
  //
  connect(ui->chatBox_multi, &QTextEdit::textChanged, this, [this] {
    dynamicInputTextHeight(ui->chatBox_multi);
  });
  // manually handle keys: enter, and shift+enter
  ui->chatBox_multi->installEventFilter(this);

#ifndef QUICK
  setupChatWidget();
  connect(this->chatModel, &ChatModel::countChanged, this, [this]() {
    if (this->chatModel->chats.size() == 0) return;
    auto html = generateChatHTML(this->chatModel->chats.last());
    ui->chat->moveCursor(QTextCursor::End);
    ui->chat->insertHtml(html);
  });
#endif
}

#ifndef QUICK
QString ChatWindow::generateChatHTML(const QSharedPointer<ChatMessage> &chats) {
  return QString("<span class=\"date\">[%1]</span> <b>%2</b>: %3<br>").arg(
    chats->partialdate(),
    Utils::escapeHtml(chats->name()),
    Utils::escapeHtml(chats->text())
  );
}

void ChatWindow::setupChatWidget() {
  QString html = "";
  ui->chat->document()->setDefaultStyleSheet(".date{color:grey;}");
  for (const auto& chat: this->chatModel->chats) {
    html += generateChatHTML(chat);
  }
  ui->chat->setHtml(html);
  ui->chat->moveCursor(QTextCursor::End);
}
#endif

void ChatWindow::dynamicInputTextHeight(QTextEdit *edit) const {
  if (!edit || !edit->parentWidget())
    return;

  const QSizeF doc_size = edit->document()->documentLayout()->documentSize();
  int h = qCeil(doc_size.height()
                + edit->contentsMargins().top()
                + edit->contentsMargins().bottom())
                + 4;

  const int line_height = edit->fontMetrics().height()
    + edit->contentsMargins().top()
    + edit->contentsMargins().bottom()
    + 4;

  // min height
  const int min_h = line_height + 2;

  // max height = 50% window
  const int max_h = m_windowHeight / 2;

  h = qBound(min_h, h, max_h);
  edit->setFixedHeight(h);
}

void ChatWindow::onAvatarChanged(const std::string& abook_uid) {
  const TelepathyAccountPtr acc = Conversations::instance()->telepathy->accountByName(local_uid);
  if (acc.isNull())
    return;

  const auto remote_uid_str = remote_uid.toStdString();
  const auto protocol = acc->protocolName().toStdString();

  if (abook_qt::get_abook_uid(protocol, remote_uid_str) == abook_uid) {
    emit avatarChanged();
  }
}

void ChatWindow::onBgMatrixRainEnabledChanged(bool enabled) {
  bgMatrixRainEnabled = enabled;
  emit bgMatrixRainEnabledChanged();
}

void ChatWindow::onIgnoreNotificationsToggled() {
  m_ignore_notifications = !m_ignore_notifications;
  if(auto result = m_ctx->state->setNotificationsIgnore(local_uid, channel, m_ignore_notifications); !result)
    return;

  // ui text
  if(m_ignore_notifications) {
    ui->actionIgnore_notifications->setText("Enable notifications");
  } else {
    ui->actionIgnore_notifications->setText("Ignore notifications");
  }
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
  rtcom_qt::delete_events(group_uid.toStdString());
  this->chatModel->clear();

  const auto key = channel.isEmpty() ? remote_uid : channel;
  if (!m_ctx->telepathy->deleteChannel(local_uid, key)) {
    qWarning() << "Failed to find/delete Tp channel, perhaps wrong channel key";
  }

  configState->deleteItem(local_uid, key);
  Conversations::instance()->overviewModel->loadOverviewMessages();
  this->close();
}

void ChatWindow::onChatClear() {
  auto group_uid_str = group_uid.toStdString();
  auto _group_uid = group_uid_str.c_str();
  rtcom_qt::delete_events(_group_uid);
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
  onDisplayChatBox();
}

void ChatWindow::onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg) {
  m_searchWindow->close();
  m_searchWindow->deleteLater();
}

void ChatWindow::onClosePreviewWindow(const QSharedPointer<ChatMessage> &msg) {
  m_previewWindow->close();
  m_previewWindow->deleteLater();
}

void ChatWindow::onOpenSearchWindow() {
  m_searchWindow = new SearchWindow(m_ctx, group_uid, this);
  m_searchWindow->show();

  connect(m_searchWindow, &SearchWindow::searchResultClicked, this, &ChatWindow::onSearchResultClicked);
  connect(m_searchWindow, &SearchWindow::searchResultClicked, this, &ChatWindow::onCloseSearchWindow);
}

void ChatWindow::onOpenPreviewWindow(QSharedPointer<PreviewItem> item) {
  m_previewWindow = new PreviewWindow(m_ctx, item, this);
  m_previewWindow->show();
}

void ChatWindow::onOpenTpContactsWindow() {
  const auto acc = m_ctx->telepathy->accountByName(local_uid);
  const TelepathyChannelPtr ptr = acc->hasChannel(channel);
  if (!ptr.isNull()) {
    m_tpContactsWindow = new TpContactsWindow(ptr, this);

    connect(m_tpContactsWindow, &TpContactsWindow::contactClicked, [this, acc](const Tp::ContactPtr &contact) {
      // note: Tp's ContactPtr->id is unparsed
      // Tp source (contact.h) has: // TODO filter: exact, prefix, substring match
      // auto _remote_uid = contact->id();
      // if (_remote_uid.contains("/"))
      //   _remote_uid = _remote_uid.split("/").at(1);

      // note: XMPP MUCs (rooms) have different user IDs (JIDs) for participants.
      // starting a chat against this user ID works, but the remote ID is off.
      // https://git.maemo.org/leste/conversations/issues/37
      acc->ensureTextChat(contact);
    });

    m_tpContactsWindow->show();
  }
}

void ChatWindow::onCloseTpContactsWindow() {
  m_tpContactsWindow->close();
  m_tpContactsWindow->deleteLater();
}

void ChatWindow::onSearchResultClicked(const QSharedPointer<ChatMessage> &msg) {
  this->setHighlight(msg->event_id());
}

void ChatWindow::setHighlight(const unsigned int event_id) {
  is_pinned = false;
  emit isPinnedChanged();

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
      m_ctx->overviewModel->loadOverviewMessages();  // refresh overview
  }
}

QString ChatWindow::remoteId() const {
  return groupchat ? channel : remote_uid;
}

void ChatWindow::onGatherMessage() {
  emit avatarChanged();
  QString msg;

  msg = this->ui->chatBox_multi->toPlainText();
  this->ui->chatBox_multi->clear();

  msg = msg.trimmed();
  if(msg.isEmpty())
    return;

  emit sendMessage(local_uid, remoteId(), msg);

  // apparently need to wait a bit
  QTimer::singleShot(50, this, [this] {
      this->ui->chatBox_multi->setFocus();
  });
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
  QStringList parts;

  const TelepathyAccountPtr acc = Conversations::instance()->telepathy->accountByName(local_uid);
  if (acc.isNull())
    return;

  parts << acc->protocolName();

  if (!channel.isEmpty()) {
    auto group_uid_str = group_uid.toStdString();
    if(auto room_name = rtcom_qt::get_room_name(group_uid_str); !room_name.empty()) {
      parts << QString::fromStdString(room_name);
    } else {
      parts << channel;
    }
  } else {
    if(local_uid.count('/') == 2) {
      auto _group_uid = local_uid.split("/").at(2);
      if (_group_uid.contains("-"))
        parts << _group_uid.split("-").at(1);
      else
        parts << remote_uid;
    }
  }

  if(!m_active && groupchat)
    parts << QString(" (not joined)");

  this->setWindowTitle(parts.join(" - "));
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
  const auto event_type = event->type();
#ifdef QUICK
  if (watched == ui->quick) {
    if (event_type == QEvent::MouseButtonPress || event_type == QEvent::TouchBegin) {
      emit isPressed();
    } else if (event_type == QEvent::MouseButtonRelease || event_type == QEvent::TouchEnd) {
      emit isReleased();
    }

    return QObject::eventFilter(watched, event);
  }
#endif
  switch (event_type) {
    case QEvent::KeyPress: {
      auto *ke = static_cast<QKeyEvent*>(event);

      if (m_enterKeySendsChat &&
          (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) &&
          (ke->modifiers() == Qt::NoModifier)) {

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
  if (this->chatModel)
    this->chatModel->clear();

  emit closed(group_uid);
  QWidget::closeEvent(event);
}

void ChatWindow::onPreviewItemClicked(const QSharedPointer<PreviewItem> &item, const QPoint point) {
  const bool preview_window =
    item->itemType == PreviewItem::ItemType::Image &&
    item->displayType == PreviewItem::DisplayType::Image;

  const bool open_from_disk =
    item->state == PreviewItem::State::Downloaded &&
    item->itemType != PreviewItem::ItemType::Html;

  if (preview_window && open_from_disk)
    return this->onOpenPreviewWindow(item);

  QString ref_type = item->itemType == PreviewItem::ItemType::Html ? "link" : "file";
  if (ref_type == "file" && item->state != PreviewItem::State::Downloaded)
    ref_type = "link";
  QString ref_title = ref_type == "link" ? "Open in browser" : "Open from disk";
  if (preview_window)
    ref_title = "Preview file";

  QDialog dialog(this);
  dialog.setWindowTitle(QString("Attachment"));
  dialog.setModal(true);
  //
  QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
  //
  QLabel *label = new QLabel(QString(item->url.toString()));
  mainLayout->addWidget(label);
  //
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  mainLayout->addLayout(buttonLayout);
  //
  QPushButton *openButton = new QPushButton(ref_title, &dialog);
  QPushButton *copyUrlButton = new QPushButton("Copy URL", &dialog);
  QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
  buttonLayout->addWidget(cancelButton);
  buttonLayout->addWidget(copyUrlButton);
  buttonLayout->addWidget(openButton);
  //
  connect(openButton, &QPushButton::clicked, [&dialog] { dialog.done(1); });
  connect(copyUrlButton, &QPushButton::clicked, [&dialog] { dialog.done(2); });
  connect(cancelButton, &QPushButton::clicked, [&dialog] { dialog.done(0); });
  // dialog
  if (const int result = dialog.exec(); result == 1) {
    if (preview_window) {
      this->onOpenPreviewWindow(item);
    } else {
      GError *error = NULL;
      const QString ref = open_from_disk ? item->filePath : item->url.toString();
      hildon_uri_open(ref.toStdString().c_str(), NULL, &error);
    }
  } else if (result == 2) {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(item->url.toString());

    QMessageBox _msgBox;
    _msgBox.setText(QString("URL copied."));
    _msgBox.exec();
  } else {
    // dismiss
  }
}

void ChatWindow::onDisplayChatBox() {
  ui->chatBox_multi->show();
}

void ChatWindow::showMessageContextMenu(const unsigned int event_id, const QPoint point) {
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
  connect(&actionNew, &QAction::triggered, [this, msg] {
    const auto text_reply = QString("> %1\n").arg(msg->text());
    ui->chatBox_multi->setText(text_reply);
    QTextCursor cursor = ui->chatBox_multi->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chatBox_multi->setTextCursor(cursor);
    ui->chatBox_multi->setFocus();
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
    const auto copyLinkMenu = new QMenu("Copy Link", &contextMenu);

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
    const auto openLinkMenu = new QMenu("Open Link", &contextMenu);

    for(unsigned int i = 0; i != msg->weblinks().size(); i++) {
      auto weblink = msg->weblinks().at(i);

      auto actionOpenLink = new QAction(QString("Link #%1").arg(QString::number(i)), &contextMenu);
      connect(actionOpenLink, &QAction::triggered, [this, weblink] {
        auto webLinkStr = weblink.toStdString();
        GError *error = NULL;

        if(error != NULL) {
         qWarning() << QString("Could not get default action by uri, error: %1->'%s'").arg(
            QString::number(error->code),
            QString::fromLocal8Bit(error->message));
        } else {
          hildon_uri_open(webLinkStr.c_str(), NULL, &error);
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
        m_ctx->overviewModel->loadOverviewMessages();  // refresh overview
    }
  }
}

void ChatWindow::resizeEvent(QResizeEvent *event) {
  m_windowHeight = this->height();
  dynamicInputTextHeight(ui->chatBox_multi);
  QMainWindow::resizeEvent(event);
}

void ChatWindow::onAddFriend() {
  m_ctx->telepathy->authorizeContact(local_uid, remote_uid);
}

void ChatWindow::onRemoveFriend() {
  m_ctx->telepathy->removeContact(local_uid, remote_uid);
}

void ChatWindow::onAcceptFriend() {
  m_ctx->telepathy->authorizeContact(local_uid, remote_uid);
}

void ChatWindow::onRejectFriend() {
  m_ctx->telepathy->removeContact(local_uid, remote_uid);
}

void ChatWindow::onSetupAuthorizeActions() {
  // if(m_ctx->telepathy->has_feature_friends(local_uid)) {
  //   qDebug() << "this protocol does not support roster friends";
  //   return;
  // }
  //
  // // @TODO: avatar
  // const auto persistent_uid = (local_uid + "-" + remote_uid).toStdString();
  // if(abook_qt::ROSTER.contains(persistent_uid)) {
  //   const QSharedPointer<ContactItem> item = abook_roster_cache[persistent_uid];
  //
  //   if(item->subscribed() == "yes") {
  //     ui->menuAuthorize->setEnabled(false);
  //   } else {
  //     ui->menuAuthorize->setEnabled(true);
  //   }
  // }
}

ChatWindow::~ChatWindow() {
  qDebug() << "destroying chatWindow";
  delete ui;
}
