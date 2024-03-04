#pragma once

#include <QObject>
#include <QDebug>
#include <QDateTime>

class ChatMessage : public QObject
{
Q_OBJECT

public:
    explicit ChatMessage(int event_id, const QString &service, const QString &group_uid,
                const QString &local_uid, const QString &remote_uid, const QString &remote_name,
                const QString &remote_ebook_uid, const QString &text, const QString &icon_name,
                int timestamp, int count, const QString &group_title, const QString &channel,
                const QString &event_type, bool outgoing, int flags, QObject *parent = nullptr);
    ~ChatMessage() override;

    int event_id() const;
    QString service() const;
    QString group_uid() const;
    QString local_uid() const;
    QString remote_uid() const;
    QString remote_name() const;
    QString remote_ebook_uid() const;
    QString text() const;
    QString textSnippet() const;
    QString icon_name() const;
    QDateTime date() const;
    int count() const;
    QString group_title() const;
    QString channel() const;
    QString event_type() const;
    bool outgoing() const;
    int flags() const;
    QString cid() const;
    QString name() const;
    QString overview_name() const;

    QString fulldate() const { return m_date.toString("yyyy-MM-dd hh:mm:ss"); }
    QString hourstr() const { return m_date.toString("hh:mm"); }
    QString datestr() const { return m_date.toString("dd/MM/yyyy"); }
    time_t epoch() const { return m_date.toTime_t(); }
    bool message_read() const {
      // @TODO: do something with flags here
      if(flags() != 0) return true;
      return false;
    }

    bool chat_event() const { auto et = this->event_type(); return et == "RTCOM_EL_EVENTTYPE_SMS_MESSAGE" || et == "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"; }
    bool join_event() const { return this->event_type() == "RTCOM_EL_EVENTTYPE_CHAT_JOIN"; }  // groupchat join
    bool leave_event() const { return this->event_type() == "RTCOM_EL_EVENTTYPE_CHAT_LEAVE"; }  // groupchat leave

    bool isHead() const;
    bool isLast() const;
    QSharedPointer<ChatMessage> previous = nullptr;
    QSharedPointer<ChatMessage> next = nullptr;

    bool isSearchResult = false;

private:
    int m_event_id;
    QString m_service;
    QString m_group_uid;
    QString m_local_uid;
    QString m_remote_uid;
    QString m_remote_name;
    QString m_remote_ebook_uid;
    QString m_text;
    QString m_cid;
    QString m_icon_name;
    QDateTime m_date;
    int m_count;
    QString m_group_title;
    QString m_channel;
    QString m_event_type;
    bool m_outgoing;
    int m_flags;
};
