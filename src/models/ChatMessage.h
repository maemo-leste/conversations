#pragma once

#include <QObject>
#include <QDebug>
#include <QDateTime>

#include "lib/rtcom.h"

struct ChatMessageParams {
    unsigned int event_id = -1;
    QString service;
    QString group_uid;

    QString local_uid;
    QString remote_uid;
    QString remote_name;
    QString remote_ebook_uid;
    QString text;
    QString icon_name;
    time_t timestamp = -1;
    unsigned int count = -1;
    QString group_title;
    QString channel;
    QString event_type;
    bool outgoing = true;
    bool is_read = false;
    unsigned int flags = -1;
};

class ChatMessage : public QObject
{
Q_OBJECT

public:
    explicit ChatMessage(ChatMessageParams params, QObject *parent = nullptr);
    ~ChatMessage() override;

    unsigned int event_id() const { return m_params.event_id; }
    QString service() const { return m_params.service; }
    QString group_uid() const { return m_params.group_uid; }
    QString channel() const { return m_params.channel; }
    QString local_uid() const { return m_params.local_uid; }
    QString remote_uid() const { return m_params.remote_uid; }
    QString remote_name() const { return m_params.remote_name; }
    QString remote_ebook_uid() const { return m_params.remote_ebook_uid; }
    QString icon_name() const { return m_params.icon_name; }
    unsigned int count() const { return m_params.count; }
    QString group_title() const { return m_params.group_title; }
    QString event_type() const { return m_params.event_type; }
    bool outgoing() const { return m_params.outgoing; }
    unsigned int flags() const { return m_params.flags; }
    QString cid() const { return m_cid; }
    QDateTime date() const { return m_date; }

    QString text() const;
    QString textSnippet() const;

    QString name() const;
    QString overview_name() const;

    QString fulldate() const { return m_date.toString("yyyy-MM-dd hh:mm:ss"); }
    QString hourstr() const { return m_date.toString("hh:mm"); }
    QString datestr() const { return m_date.toString("dd/MM/yyyy"); }
    time_t epoch() const { return m_date.toTime_t(); }
    void set_message_read() {
      qtrtcom::setRead(this->event_id(), true);
      m_params.is_read = true;
    }
    bool message_read() { return m_params.is_read; }

    bool chat_event() const { auto et = this->event_type(); return et == "RTCOM_EL_EVENTTYPE_SMS_MESSAGE" || et == "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"; }
    bool join_event() const { return this->event_type() == "RTCOM_EL_EVENTTYPE_CHAT_JOIN"; }  // groupchat join
    bool leave_event() const { return this->event_type() == "RTCOM_EL_EVENTTYPE_CHAT_LEAVE"; }  // groupchat leave

    bool isHead() const;
    bool isLast() const;
    QSharedPointer<ChatMessage> previous = nullptr;
    QSharedPointer<ChatMessage> next = nullptr;

    bool isSearchResult = false;

private:
    ChatMessageParams m_params;
    QDateTime m_date;
    QString m_cid;
};
