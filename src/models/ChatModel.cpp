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
  if(!chats.isEmpty()) {
    auto *n = chats.at(0);
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
  const int idx = rowCount();
  if(idx != 0 && !chats.isEmpty()) {
    auto *prev = chats.at(idx - 1);
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

  const ChatMessage *message = chats[index.row()];
  if (role == GroupUIDRole)
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
  return QVariant();
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
  return roles;
}

void ChatModel::clear() {
  qDebug() << "Clearing chatModel";
  beginResetModel();

  qDeleteAll(this->chats.begin(), this->chats.end());
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

#ifdef RTCOM
  rtcom_query* query_struct = rtcomStartQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_CONTACT);
  bool query_prepared = FALSE;

  if(m_filterProtocol.isEmpty()) {
    gint service_id = rtcom_el_get_service_id(query_struct->el, "RTCOM_EL_SERVICE_CALL");
    query_prepared = rtcom_el_query_prepare(query_struct->query, "service-id", service_id, RTCOM_EL_OP_NOT_EQUAL,  NULL);
  } else {
    gint service_id = (m_filterProtocol == "sms" || m_filterProtocol == "tel" || m_filterProtocol == "ofono") ?
                      rtcom_el_get_service_id(query_struct->el, "RTCOM_EL_SERVICE_SMS") :
                      rtcom_el_get_service_id(query_struct->el, "RTCOM_EL_SERVICE_CHAT");

    QString filterProtocol = QString("%%/" + m_filterProtocol + "/%%");
    query_prepared = rtcom_el_query_prepare(query_struct->query,
                                            "service-id", service_id, RTCOM_EL_OP_EQUAL,
                                            "local-uid", filterProtocol.toStdString().c_str(), RTCOM_EL_OP_STR_LIKE, NULL);
  }

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
    g_object_unref(query_struct->query);
    delete query_struct;
    return;
  }

  auto results = rtcomIterateResults(query_struct);
  for (const auto &message: results)
    this->appendMessage(message);
#endif
}

unsigned int ChatModel::getMessages(const QString &service_id, const QString &remote_uid) {
  auto count = this->getMessages(service_id, remote_uid, m_limit, m_offset);
  if(count < m_limit) {
    m_exhausted = true;
    emit exhaustedChanged();
  }
  return count;
}

unsigned int ChatModel::searchMessages(const QString &search) {
  return this->searchMessages(search, nullptr);
}

unsigned int ChatModel::searchMessages(const QString &search, const QString &remote_uid) {
#ifndef RTCOM
  return 0;
#else
  this->clear();

  rtcom_query* query_struct = rtcomStartQuery(20, 0, RTCOM_EL_QUERY_GROUP_BY_NONE);
  gint rtcom_sms_service_id = rtcom_el_get_service_id(query_struct->el, "RTCOM_EL_SERVICE_SMS");
  bool query_prepared = FALSE;

  if(remote_uid == nullptr) {
    query_prepared = rtcom_el_query_prepare(
      query_struct->query,
      "free-text", search.toStdString().c_str(), RTCOM_EL_OP_STR_LIKE,
      "service-id", rtcom_sms_service_id, RTCOM_EL_OP_EQUAL,
      NULL);
  } else {
    m_remote_uid = remote_uid;
    query_prepared = rtcom_el_query_prepare(
      query_struct->query,
      "free-text", search.toStdString().c_str(), RTCOM_EL_OP_STR_LIKE,
      "remote-uid", remote_uid.toStdString().c_str(), RTCOM_EL_OP_EQUAL,
      "service-id", rtcom_sms_service_id, RTCOM_EL_OP_EQUAL,
      NULL);
  }

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
    g_object_unref(query_struct->query);
    delete query_struct;
    return 0;
  }

  auto results = rtcomIterateResults(query_struct);
  for(auto const &message: results) {
    this->appendMessage(message);
  }

  return results.length();
#endif
}

unsigned int ChatModel::getMessages(const QString &service_id, const QString &remote_uid, const int limit, const int offset) {
#ifndef RTCOM
  return 0;
#else
  m_remote_uid = remote_uid;
  m_service_id = service_id;

  rtcom_query* query_struct = rtcomStartQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_NONE);
  gint sid = rtcom_el_get_service_id(query_struct->el, m_service_id.toStdString().c_str());
  bool query_prepared = FALSE;
  query_prepared = rtcom_el_query_prepare(query_struct->query,
                                          "remote-uid", remote_uid.toStdString().c_str(), RTCOM_EL_OP_EQUAL,
                                          "service-id", sid, RTCOM_EL_OP_EQUAL,
                                          NULL);

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
    g_object_unref(query_struct->query);
    delete query_struct;
    return 0;
  }

  auto results = rtcomIterateResults(query_struct);

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

unsigned int ChatModel::getPage() {
  // called from QML for endless scroll
  qDebug() << __FUNCTION__ << "limit:" << m_limit << " offset:" << m_offset;

  auto count = this->getMessages(m_service_id, m_remote_uid, m_limit, m_offset);
  emit offsetChanged();

  if(count < m_limit) {
    m_exhausted = true;
    emit exhaustedChanged();
  }

  return count;
}
