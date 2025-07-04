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
  enum OverviewModelRoles {
    MsgStatusIcon = 0,
    ContentRole,
    OverviewNameRole,
    ProtocolRole,
    AvatarIcon,
    AvatarPadding,
    PresenceIcon,
    ChatTypeIcon,
    TimeRole,
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
public:
  QList<QSharedPointer<ChatMessage>> messages;

public slots:
  void loadSearchMessages(const QString& needle, const QString& group_uid = "");
  void loadOverviewMessages();
  void onClear();
  void onDatabaseAddition(QSharedPointer<ChatMessage> &msg);
  void onContacsChanged(std::map<std::string, std::shared_ptr<AbookContact>> contacts);
  void onAvatarChanged(std::string local_uid_str, std::string remote_uid_str);

signals:
  void overviewRowClicked(const QSharedPointer<ChatMessage> &ptr);

protected:
  QHash<int, QByteArray> roleNames() const;

private:
  void preloadPixmaps();
private:
  Telepathy *m_tp = nullptr;
  ConfigState *m_state = nullptr;
  QMap<QString, QPixmap> m_pixmaps;
};
