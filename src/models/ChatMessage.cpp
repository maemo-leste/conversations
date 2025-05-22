#include <QObject>
#include <QDebug>

#include "models/ChatMessage.h"
#include "conversations.h"

ChatMessage::ChatMessage(rtcom_qt::ChatMessageEntry* raw_msg, QObject *parent) :
    QObject(parent),
    m_raw(raw_msg) {
  m_date = QDateTime::fromTime_t(raw_msg->timestamp);
  m_cid = QString("%1%2").arg(QString::number(raw_msg->outgoing), QString::fromStdString(raw_msg->remote_uid));

  const auto ctx = Conversations::instance();
  connect(ctx->telepathy, &Telepathy::messageFlagsChanged, this, &ChatMessage::onMessageFlagsChanged);
  m_persistent_uid = local_remote_uid();
}

void ChatMessage::onMessageFlagsChanged(unsigned int event_id, unsigned int flag) {
  if (this->event_id() == event_id) {
    this->m_raw->flags = flag;
    emit messageFlagsChanged(event_id);
  }
}

QString ChatMessage::text() const {
  if(join_event())
    return QString(tr("%1 joined the groupchat")).arg(remote_uid());

  if(leave_event())
    return QString(tr("%1 has left the groupchat")).arg(remote_uid());

  QString text = "";
  // @TODO: uncomment this when we want to visually show delivery reports in the UI
  // we cannot uncomment this, because connection managers, for example XMPP, do
  // not work properly: they only send delivery reports whilst in groupchats,
  // and not 1:1 chats.
  // if (m_raw->flags & rtcom_qt::RTCOM_EL_FLAG_SMS_PENDING)
  //   text += "[Sending…] ";
  if (m_raw->flags & rtcom_qt::RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR)
    text += "[Sending temporarily failed] ";
  else if (m_raw->flags & rtcom_qt::RTCOM_EL_FLAG_SMS_PERMANENT_ERROR)
    text += "[Sending permanently failed] ";

  text += QString::fromStdString(m_raw->text);
  return text;
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
  if (m_raw->outgoing) // self
    return "me";

  // 1. ask abook
  if (auto result = abook_qt::get_display_name(m_raw->local_uid, m_raw->remote_uid, m_raw->group_uid); !result.empty())
    return QString::fromStdString(result);
  // 2. rtcom db remote name
  if(!m_raw->remote_name.empty())
    return QString::fromStdString(m_raw->remote_name);
  // 3. fallback; remote_uid
  return QString::fromStdString(m_raw->remote_uid);
}

QString ChatMessage::name_counterparty() const {
  // This chatmessage maybe us (self), but we need the counterparty name of this conversation
  // 1. ask abook
  if (const auto group_uid_str = QString::fromStdString(m_raw->group_uid); group_uid_str.contains("-")) {
    const auto _remote_uid = group_uid_str.split("-").at(1);
    auto _remote_uid_str = _remote_uid.toStdString();
    if (auto result = abook_qt::get_display_name(m_raw->local_uid, _remote_uid_str, m_raw->group_uid); !result.empty())
      return QString::fromStdString(result);
  }

  // 2. fallback
  return name();
}

QString ChatMessage::name_channel() const {
  if(m_raw->channel.empty())
    return {};

  const auto group_uid_str = m_raw->group_uid;
  const auto _group_uid_str = group_uid_str.c_str();
  if(auto room_name = rtcom_qt::get_room_name(_group_uid_str); !room_name.empty())
    return QString::fromStdString(room_name);
  return QString::fromStdString(m_raw->channel);
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
  const auto overview_name = this->name_channel();
  QString richtext;
  // Stylesheet: overview/overviewRichDelegate.css
  richtext += QString("<span class=\"header\">%1</b>").arg(!m_raw->channel.empty() ? this->name_channel() : this->name_counterparty());
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
