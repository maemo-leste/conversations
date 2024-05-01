#include <QObject>
#include <QDebug>

#include "models/ChatMessage.h"

ChatMessage::ChatMessage(ChatMessageParams params, QObject *parent) :
    QObject(parent),
    m_params(params) {
  m_date = QDateTime::fromTime_t(m_params.timestamp);
  m_params.text = m_params.text.trimmed();
  m_cid = QString("%1%2").arg(QString::number(params.outgoing), params.remote_uid);
}

QString ChatMessage::text() const {
  if(join_event()) {
    return QString("%1 joined the groupchat").arg(remote_uid());
  } else if(leave_event()) {
    return QString("%1 has left the groupchat").arg(remote_uid());
  }
  return m_params.text;
}

QString ChatMessage::textSnippet() const {
  auto max_length = 32;
  if(m_params.text.length() >= max_length) {
    QString snippet = m_params.text.mid(0, max_length) + "...";
    return snippet;
  }
  return m_params.text;
}
QString ChatMessage::name() const {
  if(!m_params.remote_name.isEmpty()) return m_params.remote_name;
  return m_params.remote_uid;
}
QString ChatMessage::overview_name() const {
  if(!m_params.channel.isEmpty()) return m_params.channel;
  if(!m_params.remote_name.isEmpty()) return m_params.remote_name;
  return m_params.remote_uid;
}
bool ChatMessage::isHead() const {
  if(previous == nullptr) return true;
  if(previous->cid() == m_cid) return false;
  return true;
}
bool ChatMessage::isLast() const {
  if(previous == nullptr || next == nullptr) return false;
  if(previous->cid() == m_cid && next->cid() != m_cid) {
    return true;
  }
  return false;
}

ChatMessage::~ChatMessage() {
//#ifdef DEBUG
//  qDebug() << "ChatMessage::destructor";
//#endif
}
