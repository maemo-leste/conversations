#include <QObject>
#include <QDebug>

#include "models/ChatMessage.h"

ChatMessage::ChatMessage(  // @TODO: cleanup constructor
    const int event_id, const QString &service, const QString &group_uid,
    const QString &local_uid, const QString &remote_uid, const QString &remote_name,
    const QString &remote_ebook_uid, const QString &text, const QString &icon_name,
    const int timestamp, const int count, const QString &group_title,
    const QString &event_type, bool outgoing, const int flags, QObject *parent) :
    QObject(parent),
    m_event_id(event_id),
    m_service(service),
    m_group_uid(group_uid),
    m_local_uid(local_uid),
    m_remote_uid(remote_uid),
    m_remote_name(remote_name),
    m_remote_ebook_uid(remote_ebook_uid),
    m_text(text),
    m_icon_name(icon_name),
    m_count(count),
    m_group_title(group_title),
    m_event_type(event_type),
    m_outgoing(outgoing),
    m_flags(flags) {
  m_date = QDateTime::fromTime_t(timestamp);
  m_text = m_text.trimmed();
  m_cid = QString("%1%2").arg(QString::number(outgoing), remote_uid);
}

QString ChatMessage::service() const { return m_service; }
int ChatMessage::event_id() const { return m_event_id; }
QString ChatMessage::group_uid() const { return m_group_uid; }
QString ChatMessage::local_uid() const { return m_local_uid; }
QString ChatMessage::remote_uid() const { return m_remote_uid; }
QString ChatMessage::remote_name() const { return m_remote_name; }
QString ChatMessage::remote_ebook_uid() const { return m_remote_ebook_uid; }
QString ChatMessage::text() const { return m_text; }
QString ChatMessage::textSnippet() const {
  auto max_length = 32;
  if(m_text.length() >= max_length) {
    QString snippet = m_text.mid(0, max_length) + "...";
    return snippet;
  }
  return m_text;
}
QString ChatMessage::icon_name() const { return m_icon_name; }
int ChatMessage::count() const { return m_count; }
QString ChatMessage::group_title() const { return m_group_title; }
QString ChatMessage::event_type() const { return m_event_type; }
bool ChatMessage::outgoing() const { return m_outgoing; }
int ChatMessage::flags() const { return m_flags; }
QString ChatMessage::cid() const { return m_cid; }
QDateTime ChatMessage::date() const { return m_date; }
QString ChatMessage::hourstr() const { return m_date.toString("hh:mm"); }
QString ChatMessage::datestr() const { return m_date.toString("dd/MM/yyyy"); }
QString ChatMessage::name() const {
  if(!m_remote_name.isEmpty()) return m_remote_name;
  return m_remote_uid;
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
