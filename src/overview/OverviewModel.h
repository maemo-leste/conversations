#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QPixmap>
#include <QStringList>
#include <QDir>

#include <glib.h>
#include <glib/gstdio.h>

#include "lib/utils.h"
#include "lib/rtcom/rtcom_public.h"
#include "lib/tp/tp.h"
#include "lib/state.h"
#include "lib/abook/abook_contact.h"
#include "lib/QRichItemDelegate.h"
#include "models/ChatMessage.h"

class ServiceAccount
{
public:
  explicit ServiceAccount();
  static ServiceAccount* fromRtComUID(const QString &local_uid);
  static ServiceAccount* fromTpProtocol(const QString &local_uid, const QString &protocol);
  void setName(const QString &protocol);
  QString title;
  QString protocol;
  QString uid;
};


class OverviewModel;
class OverviewProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
  explicit OverviewProxyModel(QObject *parent = nullptr);
  void setProtocolFilter(QString protocol);
  void setNameFilter(QString name);
  void removeProtocolFilter();

public slots:
  void onOverviewRowClicked(uint32_t idx);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
  int rowCount(const QModelIndex &parent) const override;
  bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
  QHash<int, QByteArray> roleNames() const {
     return sourceModel()->roleNames();
  }

private:
  QString m_nameFilter = "";
  QString m_protocolFilter = "";
};


class OverviewModel : public QAbstractListModel
{
Q_OBJECT

public:
  enum class OverviewModelRoles {
    MsgStatusIcon = Qt::UserRole + 1,
    ContentRole,
    OverviewNameRole,
    ProtocolRole,
    AvatarIcon,
    PresenceIcon,
    ChatTypeIcon,
    TimeRole
  };

  enum class Columns {
    MsgStatusColumn = 0,
    ChatTypeColumn,
    ContentColumn,
    PresenceColumn,
    AvatarColumn,
    TimeColumn,
    COUNT
  };

  explicit OverviewModel(Telepathy *tp, ConfigState* state, QObject *parent = nullptr);
  ~OverviewModel() override {
    this->onClear();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  void updateMessage(int row, QSharedPointer<ChatMessage> &msg);
  void _dataChanged(const size_t row);
public:
  QList<QSharedPointer<ChatMessage>> messages;

public slots:
  void loadSearchMessages(const QString& needle, const QString& group_uid = "");
  QList<ChatMessage*> getOverviewMessages();
  void setMessages(QList<ChatMessage*> lst);
  void loadOverviewMessages();
  void onClear();
  void onDatabaseAddition(QSharedPointer<ChatMessage> &msg);
  void onContacsChanged(std::vector<std::shared_ptr<abook_qt::AbookContact>> contacts);
  void onAvatarChanged(const std::string &abook_uid);

signals:
  void overviewRowClicked(const QSharedPointer<ChatMessage> &ptr);

protected:
  QHash<int, QByteArray> roleNames() const;

private:
  void preloadPixmaps();
private:
  Telepathy *m_tp = nullptr;
  ConfigState *m_state = nullptr;
  QMap<QString, QIcon> m_icons;
};
