#include <QObject>
#include <QDebug>

#ifdef RTCOM
#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>
#endif

#include "models/ChatModel.h"


ChatModel::ChatModel(QObject *parent)
    : QAbstractListModel(parent) {
}

void ChatModel::prependMessage(ChatMessage *message) {
  QSharedPointer<ChatMessage> ptr(message);
  this->prependMessage(ptr);
}

void ChatModel::prependMessage(const QSharedPointer<ChatMessage> &message) {
  if(!chats.isEmpty()) {
    auto n = chats.at(0);
    message->next = n;
    n->previous = message;
  }

  beginInsertRows(QModelIndex(), 0, 0);
  chats.prepend(message);
  endInsertRows();

  m_count += 1;
  m_offset += 1;
  this->countChanged();
}

void ChatModel::appendMessage(ChatMessage *message) {
  QSharedPointer<ChatMessage> ptr(message);
  return this->appendMessage(ptr);
}

void ChatModel::appendMessage(const QSharedPointer<ChatMessage> &message) {
  const int idx = rowCount();
  if(idx != 0 && !chats.isEmpty()) {
    auto prev = chats.at(idx - 1);
    prev->next = message;
    message->previous = prev;
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
  else if (role == RemoteNameRole || role == NameRole)
    return message->remote_name();
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
  return QVariant();
}

void ChatModel::exportChatToCsv(const QString &service, const QString &group_uid, QObject *parent) {
  qDebug() << __FUNCTION__;

  auto *model = new ChatModel(parent);
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
  roles[HourRole] = "hourstr";
  roles[MessageRole] = "message";
  roles[isHeadRole] = "isHead";
  roles[isLastRole] = "isLast";
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
  return roles;
}

void ChatModel::clear() {
  qDebug() << "Clearing chatModel";
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

void ChatModel::onProtocolFilter(QString protocol) {
  m_filterProtocol = protocol;
  onGetOverviewMessages();
}

void ChatModel::onGetOverviewMessages(const int limit, const int offset) {
  this->clear();

  RTComElQuery *query = qtrtcom::startQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_GROUP);
  bool query_prepared = FALSE;

  if(m_filterProtocol.isEmpty()) {
    gint service_id = rtcom_el_get_service_id(qtrtcom::rtcomel(), "RTCOM_EL_SERVICE_CALL");
    query_prepared = rtcom_el_query_prepare(query, "service-id", service_id, RTCOM_EL_OP_NOT_EQUAL,  NULL);
  } else {
    gint service_id = (m_filterProtocol == "sms" || m_filterProtocol == "tel" || m_filterProtocol == "ofono") ?
                      rtcom_el_get_service_id(qtrtcom::rtcomel(), "RTCOM_EL_SERVICE_SMS") :
                      rtcom_el_get_service_id(qtrtcom::rtcomel(), "RTCOM_EL_SERVICE_CHAT");

    QString filterProtocol = QString("%%/" + m_filterProtocol + "/%%");
    query_prepared = rtcom_el_query_prepare(query,
                                            "service-id", service_id, RTCOM_EL_OP_EQUAL,
                                            "local-uid", filterProtocol.toStdString().c_str(), RTCOM_EL_OP_STR_LIKE, NULL);
  }

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
    g_object_unref(query);
    return;
  }

  auto results = qtrtcom::iterateResults(query);

  g_object_unref(query);

  for (const auto &message: results)
    this->appendMessage(message);
}

unsigned int ChatModel::getMessages(const QString &service_id, const QString &group_uid) {
  auto count = this->getMessages(service_id, group_uid, m_limit, m_offset);
  if(count < m_limit) {
    m_exhausted = true;
    emit exhaustedChanged();
  }
  return count;
}

unsigned int ChatModel::searchMessages(const QString &search) {
  return this->searchMessages(search, nullptr);
}

unsigned int ChatModel::searchMessages(const QString &search, const QString &group_uid) {
#ifndef RTCOM
  return 0;
#else
  this->clear();

  RTComElQuery *query = qtrtcom::startQuery(20, 0, RTCOM_EL_QUERY_GROUP_BY_NONE);
  gint rtcom_sms_service_id = rtcom_el_get_service_id(qtrtcom::rtcomel(), "RTCOM_EL_SERVICE_SMS");
  bool query_prepared = FALSE;

  if(group_uid == nullptr) {
    query_prepared = rtcom_el_query_prepare(
      query,
      "free-text", search.toStdString().c_str(), RTCOM_EL_OP_STR_LIKE,
//      "service-id", rtcom_sms_service_id, RTCOM_EL_OP_EQUAL,
      NULL);
  } else {
    m_group_uid = group_uid;
    query_prepared = rtcom_el_query_prepare(
      query,
      "free-text", search.toStdString().c_str(), RTCOM_EL_OP_STR_LIKE,
      "group-uid", group_uid.toStdString().c_str(), RTCOM_EL_OP_EQUAL,
//      "service-id", rtcom_sms_service_id, RTCOM_EL_OP_EQUAL,
      NULL);
  }

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
    g_object_unref(query);
    return 0;
  }

  auto results = qtrtcom::iterateResults(query);

  g_object_unref(query);

  for(auto const &message: results) {
    this->appendMessage(message);
  }

  return results.length();
#endif
}

unsigned int ChatModel::getMessages(const QString &service_id, const QString &group_uid, const int limit, const int offset) {
#ifndef RTCOM
  return 0;
#else
  m_group_uid = group_uid;
  m_service_id = service_id;

  RTComElQuery *query = qtrtcom::startQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_NONE);
  gint sid = rtcom_el_get_service_id(qtrtcom::rtcomel(), m_service_id.toStdString().c_str());
  bool query_prepared = FALSE;
  query_prepared = rtcom_el_query_prepare(query,
                                          "group-uid", group_uid.toStdString().c_str(), RTCOM_EL_OP_EQUAL,
                                          "service-id", sid, RTCOM_EL_OP_EQUAL,
                                          NULL);

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
    g_object_unref(query);
    return 0;
  }

  auto results = qtrtcom::iterateResults(query);

  g_object_unref(query);

  bool prepend = offset != 0;
  if(prepend) {
    for(auto const &message: results) {
      this->prependMessage(message);
    }
  } else {
    QList<ChatMessage *>::const_iterator rIt;
    rIt = results.constEnd();

    while (rIt != results.constBegin()) {
      --rIt;
      if (offset == 0)
        this->appendMessage(*rIt);
    }
  }

  return results.length();
#endif
}

int ChatModel::eventIdToIdx(int event_id) {
  for(int i = 0; i != chats.length(); i++)
    if(chats[i]->event_id() == event_id)
      return i;
  return -1;
}

void ChatModel::onMessageRead(const int event_id) {
  emit messageRead(event_id);
}

unsigned int ChatModel::getPage(int custom_limit) {
  // Query database and prepend additional message(s)
  qDebug() << __FUNCTION__ << "limit:" << m_limit << " offset:" << m_offset;

  int limit = m_limit;
  if(custom_limit)
    limit = custom_limit;

  auto count = this->getMessages(m_service_id, m_group_uid, limit, m_offset);
  emit offsetChanged();

  if(count < limit) {
    m_exhausted = true;
    emit exhaustedChanged();
  }

  return count;
}
