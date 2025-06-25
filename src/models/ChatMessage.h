#pragma once

#include <QObject>
#include <QDebug>
#include <QImage>
#include <QDateTime>

#include "utils.h"
#include "lib/rtcom/rtcom_public.h"
#include "lib/abook/abook_public.h"
#include "lib/abook/abook_roster.h"

class ChatMessage : public QObject {
Q_OBJECT

public:
    explicit ChatMessage(rtcom_qt::ChatMessageEntry* raw_msg, QObject *parent = nullptr);
    ~ChatMessage() override;

    unsigned int event_id() const { return m_raw->event_id; }
    QString service() const { return QString::fromStdString(m_raw->service); }
    QString group_uid() const { return QString::fromStdString(m_raw->group_uid); }
    QString channel() const { return QString::fromStdString(m_raw->channel); }
    QString local_uid() const { return QString::fromStdString(m_raw->local_uid); }
    QString remote_uid() const { return QString::fromStdString(m_raw->remote_uid); }
    QString local_remote_uid() const { return QString::fromStdString(m_raw->local_uid + "-" + m_raw->remote_uid); }
    QString remote_name() const { return QString::fromStdString(m_raw->remote_name); }
    QString remote_ebook_uid() const { return QString::fromStdString(m_raw->remote_ebook_uid); }
    QString icon_name() const { return QString::fromStdString(m_raw->icon_name); }
    unsigned int count() const { return m_raw->count; }
    QString group_title() const { return QString::fromStdString(m_raw->group_title); }
    QString event_type() const { return QString::fromStdString(m_raw->event_type); }
    bool outgoing() const { return m_raw->outgoing; }
    unsigned int flags() const { return m_raw->flags; }
    QString cid() const { return m_cid; }
    QDateTime date() const { return m_date; }
    QString protocol() const { return QString::fromStdString(m_raw->protocol); }

    QString text() const;
    QString textSnippet() const;

    QString name() const;
    bool matchesName(const QString& name) const;
    QString name_counterparty() const;
    QString name_channel() const;
    QString generateOverviewItemDelegateRichText();

    QString fulldate() const { return m_date.toString("yyyy-MM-dd hh:mm:ss"); }
    QString partialdate() const { return m_date.toString("MM-dd hh:mm:ss"); }
    QString hourstr() const { return m_date.toString("hh:mm"); }
    QString datestr() const { return m_date.toString("dd/MM/yyyy"); }
    time_t epoch() const { return m_date.toTime_t(); }
    void set_message_read() {
      rtcom_qt::set_read(this->event_id(), true);
      m_raw->is_read = true;
    }
    bool message_read() { return m_raw->is_read; }

    bool chat_event() const { return m_raw->event_type == "RTCOM_EL_EVENTTYPE_SMS_MESSAGE" || m_raw->event_type == "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"; }
    bool join_event() const { return m_raw->event_type == "RTCOM_EL_EVENTTYPE_CHAT_JOIN"; }  // groupchat join
    bool leave_event() const { return m_raw->event_type == "RTCOM_EL_EVENTTYPE_CHAT_LEAVE"; }  // groupchat leave

    bool hasAvatar();
    QString avatar();
    QPixmap __attribute__((always_inline)) avatarImage() const {
      const auto local_uid_str = local_uid().toStdString();
      const auto remote_uid_str = remote_uid().toStdString();

      const std::string avatar_token = abook_qt::get_avatar_token(local_uid_str, remote_uid_str);
      if (!avatar_token.empty() && avatar_token != "0") {
        QPixmap pixmap;
        auto result = Utils::get_avatar(local_uid_str, remote_uid_str, avatar_token, pixmap);

        if (!result)
          return {};

        return pixmap;
      }

      return {};
    }

    bool isHead() const;
    bool isLast() const;
    bool displayTimestamp() const;
    bool shouldHardWordWrap() const;

    QStringList weblinks() const;

    QSharedPointer<ChatMessage> previous = nullptr;
    QSharedPointer<ChatMessage> next = nullptr;

private slots:
    void onMessageFlagsChanged(unsigned int event_id, unsigned int flag);

signals:
    void messageFlagsChanged(unsigned int event_id);

private:
    rtcom_qt::ChatMessageEntry* m_raw = nullptr;
    QString m_persistent_uid;
    QDateTime m_date;
    QString m_cid;
};

