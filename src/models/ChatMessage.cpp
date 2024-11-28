#include <QObject>
#include <QDebug>

#include "models/ChatMessage.h"

ChatMessage::ChatMessage(ChatMessageParams params, QObject *parent) :
    QObject(parent),
    m_params(params) {
  m_date = QDateTime::fromTime_t(m_params.timestamp);
  m_params.text = m_params.text.trimmed();
  m_cid = QString("%1%2").arg(QString::number(params.outgoing), params.remote_uid);

  // extract protocol from local_uid
  if(m_params.local_uid.count("/") == 2) {
    protocol = m_params.local_uid.split("/").at(1);
  }

  m_persistent_uid = local_remote_uid();
}

QString ChatMessage::text() const {
  if(join_event()) {
    return QString(tr("%1 joined the groupchat")).arg(remote_uid());
  } else if(leave_event()) {
    return QString(tr("%1 has left the groupchat")).arg(remote_uid());
  }
  return m_params.text;
}

QString ChatMessage::textSnippet() const {
  auto max_length = 36;
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
  if(!m_params.channel.isEmpty()) {
    auto channel_str = m_params.channel.toStdString();
    auto _channel_str = channel_str.c_str();
    auto room_name = qtrtcom::getRoomName(_channel_str);
    if(!room_name.isEmpty())
      return room_name;

    return m_params.channel;
  }
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

bool ChatMessage::hasAvatar() {
  if(abook_roster_cache.contains(m_persistent_uid))
    return abook_roster_cache[m_persistent_uid]->hasAvatar();
  return false;
}

QImage& ChatMessage::avatarImage() {
  if(abook_roster_cache.contains(m_persistent_uid)) {
    auto contact_item = abook_roster_cache[m_persistent_uid];
    if(contact_item->hasAvatar())
      return contact_item->avatar();
  }
  static QImage empty;
  return empty;
}

QString ChatMessage::avatar() {
  if(abook_roster_cache.contains(m_persistent_uid)) {
    auto contact_item = abook_roster_cache[m_persistent_uid];
    return "image://avatar/" + m_persistent_uid + "?token=" + contact_item->avatar_token_hex();
  }
}

bool ChatMessage::displayTimestamp() const {
  // returns False if the previous message is:
  // - from the same author
  // - occured within 30 secs
  // Used in the UI to save some vertical space in chat message bubbles.
  const unsigned int max_delta = 30;
  if(this->isHead() || previous == nullptr)
    return true;
  if(previous->cid() == m_cid) {
    auto delta = previous->date().secsTo(m_date);
    return delta >= max_delta;
  }
  return false;
}

bool ChatMessage::shouldHardWordWrap() const {
  if(m_params.text.length() <= 32) return false;
  for(const auto &word: m_params.text.split(" "))
    if(word.length() >= 32)
      return true;
  return false;
}

void ChatMessage::generateOverviewItemDelegateRichText(){
  const auto overview_name = this->overview_name();
  // Stylesheet: overview/overviewRichDelegate.css
  auto richtext = QString("<span class=\"header\">%1</b>").arg(this->overview_name());
  richtext += QString("<span class=\"small\">&nbsp;&nbsp;%1</span>").arg(this->protocol);
  richtext += QString("<span class=\"small text-muted\">&nbsp;&nbsp;%1 %2</span>").arg(this->datestr(), this->hourstr());
  richtext += "<br>";
  
  // do not allow messages to provide their own tags as they end up in QTextDocument @ lib/QRichItemDelegate.cpp
  auto textSnippet = this->textSnippet();
  textSnippet = textSnippet.replace("<", "");

  richtext += QString("<span class=\"text-muted\">%1</span>").arg(textSnippet);
  overviewItemDelegateRichText = richtext;
}

ChatMessage::~ChatMessage() {
// #ifdef DEBUG
//  qDebug() << "ChatMessage::destructor";
// #endif
}

QList<ChatMessage*> iterateRtComEvents(RTComElQuery *query) {
  QList<ChatMessage *> results;
  RTComElIter *it = rtcom_el_get_events(qtrtcom::rtcomel(), query);

  if(it && rtcom_el_iter_first(it)) {
    do {
      GHashTable *values = NULL;

      values = rtcom_el_iter_get_value_map(
          it,
          "id",
          "service",
          "group-uid",
          "local-uid",
          "remote-uid",
          "remote-name",
          "remote-ebook-uid",
          "content",
          "icon-name",
          "start-time",
          "event-count",
          "group-title",
          "channel",
          "event-type",
          "outgoing",
          "is-read",
          "flags",
          NULL);

      auto *item = new ChatMessage({
        .event_id = LOOKUP_INT("id"),
        .service = LOOKUP_STR("service"),
        .group_uid = LOOKUP_STR("group-uid"),
        .local_uid = LOOKUP_STR("local-uid"),
        .remote_uid = LOOKUP_STR("remote-uid"),
        .remote_name = LOOKUP_STR("remote-name"),
        .remote_ebook_uid = LOOKUP_STR("remote-ebook-uid"),
        .text = LOOKUP_STR("content"),
        .icon_name = LOOKUP_STR("icon-name"),
        .timestamp = LOOKUP_INT("start-time"),
        .count = LOOKUP_INT("event-count"),
        .group_title = LOOKUP_STR("group-title"),
        .channel = LOOKUP_STR("channel"),
        .event_type = LOOKUP_STR("event-type"),
        .outgoing = LOOKUP_BOOL("outgoing"),
        .is_read = LOOKUP_BOOL("is-read"),
        .flags = LOOKUP_INT("flags")
      });

      g_hash_table_destroy(values);
      results << item;
    } while (rtcom_el_iter_next(it));

    g_object_unref(it);
  } else {
    qCritical() << "Failed to init iterator to start";
  }

  return results;
}

QStringList ChatMessage::weblinks() {
  if(!m_params.text.contains("http")) 
    return {};
  return Utils::extractWebLinks(m_params.text);
}

