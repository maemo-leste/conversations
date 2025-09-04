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

class TpContactsModel;
class TpContactsProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
  explicit TpContactsProxyModel(QObject *parent = nullptr);
  void setNameFilter(const QString& name);

public slots:
  void onTpContactsRowClicked(uint32_t idx);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
  int rowCount(const QModelIndex &parent) const override;
  QHash<int, QByteArray> roleNames() const {
     return sourceModel()->roleNames();
  }

private:
  QString m_nameFilter = "";
};


class TpContactsModel : public QAbstractListModel
{
Q_OBJECT

public:
  enum TpContactsModelRoles {
    MsgStatusIcon = 0,
    ContentRole,
    TpContactsNameRole,
    ChatTypeIcon,
    COUNT
  };

  explicit TpContactsModel(const TelepathyChannelPtr &channel, QObject *parent = nullptr);
  ~TpContactsModel() override {}

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QString generateTpContactsItemDelegateRichText(QString title, QString subtitle) const;

  TelepathyChannelPtr tpchannel;

public slots:
  void onReload();

signals:
  void contactClicked(Tp::ContactPtr ptr);

protected:
  QHash<int, QByteArray> roleNames() const;
private:
  QString m_local_uid;
  QString m_channel;
};
