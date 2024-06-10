#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QStringList>
#include <QDir>

#include <glib.h>
#include <glib/gstdio.h>

#include "qtcsv/stringdata.h"
#include "qtcsv/reader.h"
#include "qtcsv/writer.h"

#include "models/ChatMessage.h"
#include "lib/rtcom.h"


class ChatModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ChatModelRoles {
        NameRole = Qt::UserRole + 1,
        DateRole,
        HourRole,
        MessageRole,
        isHeadRole,
        isLastRole,
        OutgoingRole,
        GroupUIDRole,
        LocalUIDRole,
        RemoteUIDRole,
        RemoteNameRole,
        IconNameRole,
        EventIDRole,
        ServiceIDRole,
        OverviewNameRole,
        ChannelRole,
        ChatEventRole,
        JoinEventRole,
        LeaveEventRole,
        MessageReadRole,
        displayTimestampRole,
        shouldHardWordWrapRole
    };

    explicit ChatModel(QObject *parent = nullptr);
    ~ChatModel() override {
      this->clear();
    }

    // rtcom
    static QList<QString> localUIDs();
    unsigned int getMessages(const QString &service_id, const QString &group_uid);
    unsigned int getMessages(const QString &service_id, const QString &group_uid, int limit, int offset);
    Q_INVOKABLE unsigned int getPage(int custom_limit=0);
    Q_INVOKABLE unsigned int searchMessages(const QString &search);
    Q_INVOKABLE unsigned int searchMessages(const QString &search, const QString &group_uid);

    void setGroupUID(const QString &group_uid) { m_group_uid = group_uid; }
    void setServiceID(const QString &service_id) { m_service_id = service_id; }

    void prependMessage(ChatMessage *message);
    void prependMessage(const QSharedPointer<ChatMessage> &message);
    void appendMessage(ChatMessage *message);
    void appendMessage(const QSharedPointer<ChatMessage> &message);
    bool setMessagesRead();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QList<QSharedPointer<ChatMessage>> chats;

    // endless scroll exhausted
    Q_PROPERTY(bool exhausted READ is_exhausted NOTIFY exhaustedChanged);
    Q_PROPERTY(int limit MEMBER m_limit NOTIFY limitChanged);
    Q_PROPERTY(int offset MEMBER m_offset NOTIFY offsetChanged);
    Q_PROPERTY(int count READ count NOTIFY countChanged);
    Q_PROPERTY(QString group_uid MEMBER m_group_uid CONSTANT);
    int count() const { return m_count; }
    bool is_exhausted() {
      return m_exhausted;
    }

    Q_INVOKABLE int eventIdToIdx(int msg);

    Q_INVOKABLE void clear();
    static void exportChatToCsv(const QString &service, const QString &group_uid, QObject *parent);

signals:
    void exhaustedChanged();
    void limitChanged();
    void offsetChanged();
    void countChanged();
    void messageRead(unsigned int event_id);

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    QString m_group_uid;
    QString m_service_id;

    int m_limit = 25;
    int m_offset = 0;
    int m_count = 0;
    bool m_exhausted = false;
};
