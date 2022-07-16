#ifndef OVERVIEWSERVICEMODEL_H
#define OVERVIEWSERVICEMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QStringList>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountSet>

#include "lib/tp.h"

#include <glib.h>
#include <glib/gstdio.h>

class Conversations;
class OverviewServiceItem
{
public:
  OverviewServiceItem(const QString &name, const QString &uid);
  QString name() const;
  QString uid() const;

private:
  QString m_name;
  QString m_uid;
};

class OverviewServiceModel : public QAbstractListModel
{
  Q_OBJECT
public:
  enum OverviewServiceRoles {
    NameRole = Qt::UserRole + 1,
    UidRole,
    ProtocolRole,
    ProtocolTitleRole
  };

  QList<OverviewServiceItem*> items;

  explicit OverviewServiceModel(Conversations *ctx, QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  void clear();

  Q_INVOKABLE void changeActiveIndex(unsigned int idx);
  Q_PROPERTY(int activeIndex READ activeIndex NOTIFY activeIndexChanged);
  int activeIndex() const { return m_activeIndex; }

private slots:
  void onTPAccountManagerReady();

signals:
  void activeIndexChanged();
  void protocolFilterChanged(QString protocol);

protected:
  QHash<int, QByteArray> roleNames() const;

private:
  void load();
  void appendItem(OverviewServiceItem *item);

  Conversations *m_ctx;
  int m_activeIndex = 0;
  QList<QString> m_rtcomProtocols;
};

#endif