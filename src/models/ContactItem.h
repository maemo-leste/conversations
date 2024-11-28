#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QPixmap>
#include <QStringList>
#include <QDir>
#include <QDebug>

class ContactItem : public QObject {
Q_OBJECT

public:
    explicit ContactItem(QString local_uid, QString remote_uid, QObject *parent = nullptr);
    explicit ContactItem(QString persistent_uid, QObject *parent = nullptr);
    ~ContactItem() override;

    QString id() const { return m_local_uid + "-" + m_remote_uid; }
    QString group_uid() const { return m_group_uid; }
    QString local_uid() const { return m_local_uid; }
    QString remote_uid() const { return m_remote_uid; }

    QString presence() const { return m_presence; }
    QString subscribed() const { return m_subscribed; }
    QString published() const { return m_published; }

    bool is_blocked() const { return m_is_blocked; }
    bool can_block() const { return m_can_block; }
    bool can_auth() const { return m_can_auth; }

    void setPresence(QString presence, bool &dirty);
    void setSubscribed(QString subscribed, bool &dirty);
    void setPublished(QString published, bool &dirty);
    void setIsBlocked(bool is_blocked, bool &dirty);
    void setCanBlock(bool can_block, bool &dirty);
    void setCanAuth(bool can_auth, bool &dirty);

    QImage& avatar() { return m_avatar; }
    bool canSetNewAvatar(void* token);
    void setAvatar(void* token, QImage &img);
    bool hasAvatar();
    QString avatar_token_hex() { return m_avatar_token_hex; };

    void print();

signals:
    void avatarChanged();

private:
    QString m_group_uid;
    QString m_local_uid;
    QString m_remote_uid;

    QString m_presence;
    QString m_subscribed;
    QString m_published;

    bool m_is_blocked = false;
    bool m_can_block = false;
    bool m_can_auth = false;

    void* m_avatar_token = nullptr;
    QString m_avatar_token_hex;
    QImage m_avatar;
};
