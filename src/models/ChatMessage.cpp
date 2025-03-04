#include <QObject>
#include <QDebug>

#include "models/ChatMessage.h"

ChatMessage::ChatMessage(rtcom_qt::ChatMessageEntry* raw_msg, QObject *parent) :
    QObject(parent),
    m_raw(raw_msg) {
  m_date = QDateTime::fromTime_t(raw_msg->timestamp);
  m_cid = QString("%1%2").arg(QString::number(raw_msg->outgoing), QString::fromStdString(raw_msg->remote_uid));

  m_persistent_uid = local_remote_uid();
}

QString ChatMessage::text() const {
  if(join_event())
    return QString(tr("%1 joined the groupchat")).arg(remote_uid());

  if(leave_event())
    return QString(tr("%1 has left the groupchat")).arg(remote_uid());

  return QString::fromStdString(m_raw->text);
}

QString ChatMessage::textSnippet() const {
  constexpr unsigned int max_length = 36;
  if(m_raw->text.length() >= max_length) {
    QString snippet = QString::fromStdString(m_raw->text).mid(0, max_length) + "...";
    return snippet;
  }
  return QString::fromStdString(m_raw->text);
}
QString ChatMessage::name() const {
  auto remote_name = QString::fromStdString(m_raw->remote_name);
  if(!m_raw->remote_name.empty()) return remote_name;
  return QString::fromStdString(m_raw->remote_uid);
}
QString ChatMessage::overview_name() const {
  if(!m_raw->channel.empty()) {
    const auto channel_str = m_raw->channel;
    const auto _channel_str = channel_str.c_str();
    auto room_name = rtcom_qt::get_room_name(_channel_str);
    if(!room_name.empty())
      return QString::fromStdString(room_name);
    return QString::fromStdString(m_raw->channel);
  }

  if(!m_raw->remote_name.empty()) return QString::fromStdString(m_raw->remote_name);
  return QString::fromStdString(m_raw->remote_uid);
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
  if (outgoing())
    return false;

  const auto uid = local_remote_uid();
  const std::string avatar_token = abook_qt::get_avatar_token(local_uid().toStdString(), remote_uid().toStdString());
  auto has = !avatar_token.empty() && avatar_token != "0";
  return has;
}

QString ChatMessage::avatar() {
  const auto persistent_uid = local_remote_uid();
  if (abook_qt::ROSTER.contains(persistent_uid.toStdString())) {
    const auto token = abook_qt::get_avatar_token(local_uid().toStdString(), remote_uid().toStdString());
    auto url = "image://avatar/" + m_persistent_uid + "?token=" + QString::fromStdString(token);
    return url;
  }

  qWarning() << "avatar requested, but could not be found in the ROSTER";
  return {};
}

bool ChatMessage::displayTimestamp() const {
  // returns False if the previous message is:
  // - from the same author
  // - occured within 30 secs
  // Used in the UI to save some vertical space in chat message bubbles.
  if(this->isHead() || previous == nullptr)
    return true;
  if(previous->cid() == m_cid) {
    constexpr unsigned int max_delta = 30;
    const auto delta = previous->date().secsTo(m_date);
    return delta >= max_delta;
  }
  return false;
}

bool ChatMessage::shouldHardWordWrap() const {
  const auto text = QString::fromStdString(m_raw->text);
  if(text.length() <= 24) return false;
  for(const auto &word: text.split(" "))
    if(word.length() >= 24)
      return true;
  return false;
}

void ChatMessage::generateOverviewItemDelegateRichText(){
  const auto overview_name = this->overview_name();
  // Stylesheet: overview/overviewRichDelegate.css
  auto richtext = QString("<span class=\"header\">%1</b>").arg(this->overview_name());
  richtext += QString("<span class=\"small\">&nbsp;&nbsp;%1</span>").arg(QString::fromStdString(this->m_raw->protocol));
  richtext += QString("<span class=\"small text-muted\">&nbsp;&nbsp;%1 %2</span>").arg(this->datestr(), this->hourstr());
  richtext += "<br>";
  
  // do not allow messages to provide their own tags as they end up in QTextDocument @ lib/QRichItemDelegate.cpp
  auto textSnippet = this->textSnippet();
  textSnippet = textSnippet.replace("<", "");

  richtext += QString("<span class=\"text-muted\">%1</span>").arg(textSnippet);
  overviewItemDelegateRichText = richtext;
}

ChatMessage::~ChatMessage() {
  delete m_raw;
// #ifdef DEBUG
//  qDebug() << "ChatMessage::destructor";
// #endif
}

QStringList ChatMessage::weblinks() const {
  const auto text = QString::fromStdString(m_raw->text);
  if(!text.contains("http"))
    return {};
  return Utils::extractWebLinks(text);
}
