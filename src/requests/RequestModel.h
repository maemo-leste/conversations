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
#include "lib/state.h"
#include "lib/QRichItemDelegate.h"
#include "models/ContactItem.h"

class Telepathy;
class RequestModel : public QAbstractListModel
{
Q_OBJECT

public:
  enum RequestModelRoles {
    MsgStatusIcon = 0,
    ContentRole,
    ButtonRole,
    COUNT
  };

  explicit RequestModel(Telepathy *tp, QObject *parent = nullptr);
  ~RequestModel() override {
    this->onClear();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  unsigned int size() { return requests.size(); }

  void addEntry(QSharedPointer<ContactItem> roster_item);
  void removeEntry(QSharedPointer<ContactItem> roster_item);

  QList<QSharedPointer<ContactItem>> requests;

public slots:
  void onClear();
  void onLoad();

signals:
  void changed();

protected:
  QHash<int, QByteArray> roleNames() const;

private:
  void preloadPixmaps();
  QString generateOverviewItemDelegateRichText(QSharedPointer<ContactItem> roster_item) const;

private:
  Telepathy *m_tp = nullptr;
  ConfigState *m_state = nullptr;
  QMap<QString, QPixmap> m_pixmaps;
};
