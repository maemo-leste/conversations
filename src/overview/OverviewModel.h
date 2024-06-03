#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QPixmap>
#include <QStringList>
#include <QDir>

#ifdef RTCOM
#include <glib.h>
#include <glib/gstdio.h>
#include "lib/rtcom.h"
#endif

#include "lib/utils.h"
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


class OverviewProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
  explicit OverviewProxyModel(QObject *parent = nullptr);
  void setProtocolFilter(QString protocol);
  void removeProtocolFilter();

public slots:
  void onOverviewRowClicked(uint32_t idx);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
  int rowCount(const QModelIndex &parent) const override;

private:
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
    ChatTypeIcon,
    TimeRole,
    COUNT
  };

  explicit OverviewModel(QObject *parent = nullptr);
  ~OverviewModel() override {
    this->onClear();
  }

  QList<QSharedPointer<ChatMessage>> messages;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  uint32_t itemHeight = 80;

public slots:
  void onLoad();
  void onClear();

signals:
  void overviewRowClicked(const QSharedPointer<ChatMessage> &ptr);

protected:
  QHash<int, QByteArray> roleNames() const;

private:
  void preloadPixmaps();
  QMap<QString, QPixmap> m_pixmaps;
};
