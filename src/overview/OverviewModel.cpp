#include <QObject>
#include <QDebug>

#ifdef RTCOM
#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>
#endif

#include "overview/OverviewModel.h"

ServiceAccount::ServiceAccount() {}
ServiceAccount* ServiceAccount::fromTpProtocol(const QString &local_uid, const QString &protocol) {
  auto *rtn = new ServiceAccount();
  rtn->protocol = protocol;
  rtn->uid = local_uid;
  rtn->setName(protocol);
  return rtn;
}

ServiceAccount* ServiceAccount::fromRtComUID(const QString &local_uid) {
  auto *rtn = new ServiceAccount();
  rtn->uid = local_uid;
  if(local_uid.contains("/") && local_uid.count("/") == 2) {
    rtn->protocol = local_uid.split("/").at(1);
  }
  rtn->setName(rtn->protocol);
  return rtn;
}
void ServiceAccount::setName(const QString &protocol) {
  if(protocol == "irc") 
    this->title = QString("IRC");
  else if(protocol == "tel") 
    this->title = QString("SMS");
  else if(protocol == "xmpp" || protocol == "jabber")
    this->title = QString("XMPP");
  else if(protocol == "telegram" || protocol == "telegram_tdlib")
    this->title = QString("Telegram");
  else
    this->title = "unknown";

  this->title = this->title.left(1).toUpper() + this->title.mid(1);
}

OverviewProxyModel::OverviewProxyModel(QObject *parent) : 
    QSortFilterProxyModel(parent),
    m_visibleColumns({OverviewModel::MsgStatusIcon, 
                      OverviewModel::ContentRole,
                      OverviewModel::ChatTypeIcon}) {
}

void OverviewProxyModel::setProtocolFilter(QString protocol) {
  qDebug() << "proxy filter" << protocol;
  if(protocol == "*")
    protocol = "";

  this->m_protocolFilter = protocol.toLower();
  if (this->sourceModel() != nullptr) {
    auto mdl = dynamic_cast<OverviewModel*>(this->sourceModel());
    this->invalidate();
  }
}

// show only specific columns
bool OverviewProxyModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const {
  return m_visibleColumns.contains(source_column);
}

// optionally filter on protocol (SMS, Telegram, etc.)
bool OverviewProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
  if(this->sourceModel() == nullptr)
    return false;

  auto mdl = dynamic_cast<OverviewModel*>(this->sourceModel());
  auto index = this->sourceModel()->index(source_row, 0, source_parent);

  if(!index.isValid())
    return false;

  if(m_protocolFilter.isEmpty())
    return true;

  // filtering
  QSharedPointer<ChatMessage> msg = mdl->messages[index.row()];
  if(msg->protocol.contains(m_protocolFilter))
    return true;

  return false;
}

int OverviewProxyModel::rowCount(const QModelIndex &parent) const {
  return QSortFilterProxyModel::rowCount(parent);
}

void OverviewProxyModel::onOverviewRowClicked(uint32_t idx) {
  QModelIndex source_idx = this->mapToSource(this->index(idx, 0));
  uint32_t row_idx = source_idx.row();
  auto mdl = dynamic_cast<OverviewModel*>(this->sourceModel());
  QSharedPointer<ChatMessage> msg = mdl->messages[row_idx];
  emit mdl->overviewRowClicked(msg);
}

OverviewModel::OverviewModel(QObject *parent) : 
    QAbstractListModel(parent) {
  this->preloadPixmaps();
}

int OverviewModel::rowCount(const QModelIndex & parent) const {
  Q_UNUSED(parent);
  return messages.count();
}

int OverviewModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return OverviewModelRoles::COUNT;
}

QVariant OverviewModel::data(const QModelIndex &index, int role) const {
  const auto row = index.row();
  if (row < 0 || row >= messages.count())
    return QVariant();
  const auto message = messages[row];

  if(role == Qt::DisplayRole) {
    switch(index.column()) {
      case OverviewModel::ContentRole: {
        if(message->overviewItemDelegateRichText.isEmpty())
          message->generateOverviewItemDelegateRichText();
        return message->overviewItemDelegateRichText;
      }
      case OverviewModel::ProtocolRole: {
        return message->protocol;
      }
      default:
        return QVariant();
    }
  }
  else if(role == Qt::DecorationRole) {
    switch (index.column()) {
      case OverviewModel::MsgStatusIcon: {
        const auto icon = message->icon_name();
        if(!icon.isEmpty()) {
          if(m_pixmaps.contains(icon))
            return m_pixmaps[icon];
          else {
            qWarning() << "icon" << icon << "does not exist";
            return {};
          }
        }
      }
      case OverviewModel::ChatTypeIcon: {
        const auto icon_default = "general_default_avatar";
        const auto icon_conference = "general_conference_avatar";

        if(message->channel().isEmpty()) {
          if(m_pixmaps.contains(icon_default)) {
            return m_pixmaps[icon_default];
          }
        } else {
          if(m_pixmaps.contains(icon_conference)) {
            return m_pixmaps[icon_conference];
          }
        }
      }
      default: {
        return QVariant();
      }
    }
  } else if(role == Qt::SizeHintRole) {
    switch (index.column()) {
      case OverviewModel::MsgStatusIcon: {
        return QSize(58, 54);
      }
      case OverviewModel::ChatTypeIcon: {
        return QSize(58, 54);
      }
      default: {
        return QVariant();
      }
    }
  }

  return {};
}

void OverviewModel::onLoad() {
  this->onClear();

  const uint32_t limit = 50000;
  const uint32_t offset = 0;
  RTComElQuery *query = qtrtcom::startQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_GROUP);
  bool query_prepared = FALSE;

  const gint service_id = rtcom_el_get_service_id(qtrtcom::rtcomel(), "RTCOM_EL_SERVICE_CALL");
  query_prepared = rtcom_el_query_prepare(query, "service-id", service_id, RTCOM_EL_OP_NOT_EQUAL,  NULL);

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
    g_object_unref(query);
    return;
  }

  auto results = iterateRtComEvents(query);

  g_object_unref(query);

  beginInsertRows(QModelIndex(), 0, results.size());
  for (const auto &message: results)
    messages << QSharedPointer<ChatMessage>(message);
  endInsertRows();
}

QHash<int, QByteArray> OverviewModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[OverviewNameRole] = "overview_name";
  roles[ProtocolRole] = "protocol";
  roles[MsgStatusIcon] = "msg_status_icon";
  roles[ChatTypeIcon] = "chat_type_icon";
  return roles;
}

void OverviewModel::preloadPixmaps() {
  // const auto basepath = QDir::currentPath() + "/src/assets/icons/";
  const auto basepath = "/usr/share/icons/hicolor/48x48/hildon/";
  const QStringList icons = {
    "chat_enter",
    "chat_failed_sms",
    "chat_pending_sms",
    "chat_replied_chat",
    "chat_replied_sms",
    "chat_unread_chat",
    "general_conference_avatar",
    "general_default_avatar",
    "chat_unread_sms",
    "general_chat"
  };

  for(const auto &icon: icons) {
    const auto fn = QString("%1/%2.png").arg(basepath, icon);
    if(!Utils::fileExists(fn))
      continue;
    m_pixmaps[icon] = QPixmap(fn);
  }
}

void OverviewModel::onClear() {
  beginResetModel();
  this->messages.clear();
  endResetModel();
}
