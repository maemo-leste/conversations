#include <QObject>
#include <QDebug>

#ifdef RTCOM
#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>
#endif

#include "models/OverviewServiceModel.h"
#include "conversations.h"


OverviewServiceItem::OverviewServiceItem(const QString &name, const QString &uid)
    : m_name(name), m_uid(uid) {
  if(uid == "irc") m_name = QString("IRC");
  else if(uid == "tel") m_name = QString("SMS");
  else if(uid == "xmpp") m_name = QString("XMPP");
  if(m_name.isEmpty()) m_name = uid;
  m_name = m_name.left(1).toUpper() + m_name.mid(1);
}
QString OverviewServiceItem::name() const { return m_name; }
QString OverviewServiceItem::uid() const { return m_uid; }

OverviewServiceModel::OverviewServiceModel(Conversations *ctx, QObject *parent)
    : m_ctx(ctx), QAbstractListModel(parent) {

  m_rtcomProtocols = rtcomGetLocalUids();
  this->load();

  connect(m_ctx->telepathy, &Telepathy::accountManagerReady, this, &OverviewServiceModel::onTPAccountManagerReady);
}

int OverviewServiceModel::rowCount(const QModelIndex & parent) const {
  Q_UNUSED(parent);
  return items.count();
}

void OverviewServiceModel::changeActiveIndex(unsigned int idx) {
  m_activeIndex = idx;
  QString protocol = items[idx]->uid();

  emit protocolFilterChanged(protocol);
  emit activeIndexChanged();
}

void OverviewServiceModel::onTPAccountManagerReady() {
  this->load();
}

void OverviewServiceModel::load() {
  this->clear();

  beginInsertRows(QModelIndex(), 0, m_ctx->telepathy->accounts.count() + m_rtcomProtocols.count() + 1);

  OverviewServiceItem *item;
  item = new OverviewServiceItem("All", "");
  this->appendItem(item);

  for(const auto &acc: m_rtcomProtocols) {
    item = new OverviewServiceItem("", acc);
    this->appendItem(item);
  }

  for(const auto &acc: m_ctx->telepathy->accounts) {
    item = new OverviewServiceItem("", acc->protocolName());
    this->appendItem(item);
  }

  endInsertRows();
}

void OverviewServiceModel::appendItem(OverviewServiceItem *item) {
  for(const auto &_item: items)
    if(_item->uid() == item->uid())
      return;

  items << item;
}

QVariant OverviewServiceModel::data(const QModelIndex &index, int role) const {
  if (index.row() < 0 || index.row() >= items.count())
    return QVariant();

  const OverviewServiceItem *acc = items[index.row()];
  if (role == NameRole)
    return acc->name();
  else if (role == UidRole)
    return acc->uid();
  return QVariant();
}

QHash<int, QByteArray> OverviewServiceModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NameRole] = "name";
  roles[UidRole] = "local_uid";
  return roles;
}

void OverviewServiceModel::clear() {
  qDebug() << "Clearing OverviewServiceModel";
  beginResetModel();
  qDeleteAll(items.begin(), items.end());
  items.clear();
  endResetModel();
}
