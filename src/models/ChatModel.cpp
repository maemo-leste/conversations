#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <ranges>

#include "lib/rtcom/rtcom_public.h"
#include "lib/rtcom/rtcom_models.h"
#include "lib/webpreviewmodel.h"

#include "models/ChatModel.h"
#include "conversations.h"

ChatModel::ChatModel(const bool has_preview_capability, QObject *parent)
    : m_has_preview_capability(has_preview_capability), QAbstractListModel(parent) {
  if(m_has_preview_capability) {
    const auto *ctx = Conversations::instance();
    connect(ctx, &Conversations::enableLinkPreviewEnabledToggled, [this](bool enabled) {
      const QModelIndex topLeft = index(0, 0);
      const QModelIndex bottomRight = index(rowCount() - 1, 0);
      emit dataChanged(topLeft, bottomRight, { previewRole });
    });
  }
}

void ChatModel::prependMessage(const QSharedPointer<ChatMessage> &message) {
  if(!chats.isEmpty()) {
    const auto n = chats.at(0);
    message->next = n;
    n->previous = message;

    if (n->date().date() != message->date().date())
      n->set_new_day(true);
  }

  beginInsertRows(QModelIndex(), 0, 0);
  chats.prepend(message);
  endInsertRows();

  m_count += 1;
  m_offset += 1;
  this->countChanged();
}

void ChatModel::appendMessage(const QSharedPointer<ChatMessage> &message) {
  connect(message.data(), &ChatMessage::messageFlagsChanged, this, &ChatModel::onMessageRowChanged);
  const int idx = rowCount();
  if(idx != 0 && !chats.isEmpty()) {
    auto prev = chats.at(idx - 1);
    prev->next = message;
    message->previous = prev;

    if (prev->date().date() != message->date().date())
      message->set_new_day(true);
  }

  beginInsertRows(QModelIndex(), idx, rowCount());
  chats.append(message);
  endInsertRows();

  m_count += 1;
  m_offset += 1;
  this->countChanged();
}

int ChatModel::rowCount(const QModelIndex & parent) const {
  Q_UNUSED(parent);
  return chats.count();
}

QVariant ChatModel::data(const QModelIndex &index, int role) const {
  if (index.row() < 0 || index.row() >= chats.count())
    return QVariant();

  const QSharedPointer<ChatMessage> message = chats[index.row()];
  if (role == OverviewNameRole) {
      if (!message->channel().isEmpty()) {
          return message->channel();
      } else if (!message->remote_name().isEmpty()) {
          return message->remote_name();
      } else {
          return message->remote_uid();
      }
  } else if (role == GroupUIDRole)
    return message->group_uid();
  else if (role == LocalUIDRole)
    return message->local_uid();
  else if (role == RemoteNameRole)
    return message->remote_name();
  else if (role == NameRole)
    return message->name();
  else if (role == RemoteUIDRole)
      return message->remote_uid();
  else if (role == DateRole)
    return message->datestr();
  else if (role == HourRole)
    return message->hourstr();
  else if (role == MessageRole)
    return message->text();
  else if (role == isHeadRole)
    return message->isHead();
  else if (role == newDayRole)
    return message->new_day();
  else if (role == RawDateRole)
    return message->date();
  else if (role == AvatarRole)
    return message->avatar();
  else if (role == AvatarImageRole) {
    auto img = message->avatarImage();
    if(!img.isNull()) return img;
    else return QVariant();
  }
  else if (role == hasAvatarRole)
    return message->hasAvatar();
  else if (role == isLastRole)
    return message->isLast();
  else if (role == OutgoingRole)
    return message->outgoing();
  else if (role == IconNameRole)
    return message->icon_name();
  else if (role == EventIDRole)
    return message->event_id();
  else if (role == ServiceIDRole)
    return message->service();
  else if (role == ChannelRole)
    return message->channel();
  else if (role == ChatEventRole)
    return message->chat_event();
  else if (role == JoinEventRole)
    return message->join_event();
  else if (role == LeaveEventRole)
    return message->leave_event();
  else if (role == MessageReadRole)
    return message->message_read();
  else if (role == displayTimestampRole)
    return message->displayTimestamp();
  else if (role == shouldHardWordWrapRole)
    return message->shouldHardWordWrap();
  else if (role == weblinksRole) {
    return message->weblinks();
  }
  else if (role == weblinksCountRole) {
    return message->weblinks_count();
  }
  else if (role == previewRole) {
    if (!m_has_preview_capability) return {};
    auto linkPreviewEnabled = config()->get(ConfigKeys::LinkPreviewEnabled).toBool();
    if (!linkPreviewEnabled)
      return {};

    const auto links = message->weblinks();
    if (links.isEmpty())
      return {};

    const auto event_id = message->event_id();
    const bool requires_user_interaction = config()->get(ConfigKeys::LinkPreviewRequiresUserInteraction).toBool();

    if (!webPreviewCache.contains(event_id)) {
      auto *previewModel = new PreviewModel(event_id);
      const auto ptr = QSharedPointer<PreviewModel>(previewModel);

      connect(previewModel, &PreviewModel::previewItemClicked, this, &ChatModel::previewItemClicked);
      ptr->addLinks(links);
      webPreviewCache[event_id] = ptr;

      if (!requires_user_interaction)
        ptr->buttonPressed();
    } else {
      const auto previewModel = webPreviewCache[event_id];
      connect(previewModel.data(), &PreviewModel::previewItemClicked, this, &ChatModel::previewItemClicked, Qt::UniqueConnection);

      // auto-fetch
      if (!requires_user_interaction && previewModel->state() == PreviewModel::USER_WAIT) {
        previewModel->buttonPressed();
      }
    }

    return QVariant::fromValue(webPreviewCache[event_id].data());
  }
  return QVariant();
}

void ChatModel::exportChatToCsv(const QString &service, const QString &group_uid, QObject *parent) {
  qDebug() << __FUNCTION__;

  auto *model = new ChatModel(false, parent);
  model->setGroupUID(group_uid);
  model->setServiceID(service);

  while(model->getPage(500) > 0) {
    qDebug() << "fetching";
  }

  QString csv;
  QtCSV::StringData data;
  auto now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");

  for(const QSharedPointer<ChatMessage> &chat: model->chats) {
    QStringList row;
    row << QString::number(chat->epoch()) << chat->service() << chat->fulldate() << chat->remote_uid() << chat->name() << chat->text();
    data.addRow(row);
  }

  const auto fn = QString("%1-%2-%3.csv").arg(service).arg(now).arg(group_uid);
  const auto path = QString("%1/MyDocs/%2").arg(QDir::homePath()).arg(fn);

  qDebug() << "writing to: " << path;
  QtCSV::Writer::write(path, data);
  model->deleteLater();
}

QHash<int, QByteArray> ChatModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[GroupUIDRole] = "group_uid";
  roles[LocalUIDRole] = "local_uid";
  roles[RemoteUIDRole] = "remote_uid";
  roles[RemoteNameRole] = "remote_name";
  roles[NameRole] = "name";
  roles[DateRole] = "datestr";
  roles[RawDateRole] = "rawdate";
  roles[HourRole] = "hourstr";
  roles[MessageRole] = "message";
  roles[isHeadRole] = "isHead";
  roles[isLastRole] = "isLast";
  roles[hasAvatarRole] = "hasAvatar";
  roles[AvatarRole] = "avatar";
  roles[AvatarImageRole] = "avatar_image";
  roles[OutgoingRole] = "outgoing";
  roles[IconNameRole] = "icon_name";
  roles[EventIDRole] = "event_id";
  roles[ServiceIDRole] = "service_id";
  roles[OverviewNameRole] = "overview_name";
  roles[ChannelRole] = "channel";
  roles[ChatEventRole] = "chat_event";
  roles[JoinEventRole] = "join_event";
  roles[LeaveEventRole] = "leave_event";
  roles[MessageReadRole] = "message_read";
  roles[displayTimestampRole] = "display_timestamp";
  roles[shouldHardWordWrapRole] = "hardWordWrap";
  roles[weblinksRole] = "weblinks";
  roles[weblinksCountRole] = "weblinks_count";
  roles[previewRole] = "preview";
  roles[newDayRole] = "new_day";
  return roles;
}

void ChatModel::clear() {
  qDebug() << "Clearing chatModel" << this->chats.size();
  beginResetModel();

  for(const QSharedPointer<ChatMessage> &msg: this->chats) {
    msg->next.clear();
    msg->previous.clear();
  }

  this->chats.clear();
  m_count = 0;
  endResetModel();
  this->countChanged();
}

unsigned int ChatModel::getMessages(const QString &service_id, const QString &group_uid) {
  const auto count = this->getMessages(service_id, group_uid, m_limit, m_offset);
  if(count < m_limit) {
    m_exhausted = true;
    emit exhaustedChanged();
  }
  return count;
}

unsigned int ChatModel::searchMessages(const QString &search) {
  return this->searchMessages(search, "");
}

unsigned int ChatModel::searchMessages(const QString &search, const QString &group_uid) {
  this->clear();

  auto results = rtcom_qt::search_messages(search.toStdString(), group_uid.toStdString());

  for(auto const &message: results) {
    this->appendMessage(QSharedPointer<ChatMessage>(new ChatMessage(message)));
  }

  return results.size();
}

unsigned int ChatModel::getMessages(const QString &service_id, const QString &group_uid, const int limit, const int offset) {
  m_group_uid = group_uid;
  m_service_id = service_id;

  auto results = rtcom_qt::get_messages(service_id.toStdString(), group_uid.toStdString(), limit, offset);

  bool prepend = offset != 0;
  if(prepend) {
    for(auto const &message: results) {
      this->prependMessage(QSharedPointer<ChatMessage>(new ChatMessage(message)));
    }
  } else {
    for (const auto entry: std::ranges::reverse_view(results)) {
      this->appendMessage(QSharedPointer<ChatMessage>(new ChatMessage(entry)));
    }
  }

  return results.size();
}

int ChatModel::eventIdToIdx(int event_id) {
  for(int i = 0; i != chats.length(); i++)
    if(chats[i]->event_id() == event_id)
      return i;
  return -1;
}

bool ChatModel::setMessagesRead() {
  // set 'message_read' for messages in current buffer
  bool rtn = false;
  for(auto const &msg: chats) {
    if(!msg->message_read()) {
      const auto event_id = msg->event_id();
      msg->set_message_read();
      emit messageRead(event_id);
      rtn = true;
    }
  }

  return rtn;
}

unsigned int ChatModel::getPage(int custom_limit) {
  // Query database and prepend additional message(s)
  qDebug() << __FUNCTION__ << "limit:" << m_limit << " offset:" << m_offset;

  int limit = m_limit;
  if(custom_limit)
    limit = custom_limit;

  const auto count = this->getMessages(m_service_id, m_group_uid, limit, m_offset);
  emit offsetChanged();

  if(count < limit) {
    m_exhausted = true;
    emit exhaustedChanged();
  }

  return count;
}

void ChatModel::onMessageRowChanged(const unsigned int event_id) {
  for (int row = 0; row < chats.size(); ++row) {
    if (const auto& chat = chats[row]; chat->event_id() == event_id) {
      emit dataChanged(index(row, 0), index(row, 0));
      break;
    }
  }
}