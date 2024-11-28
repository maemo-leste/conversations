#include <QObject>
#include <QDebug>

#ifdef RTCOM
#include "lib/rtcom.h"
#include <rtcom-eventlogger/eventlogger.h>
#endif

#include "lib/abook_roster.h"

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
  else if(protocol == "tel" || protocol == "ofono")
    this->title = QString("SMS");
  else if(protocol == "sip")
    this->title = QString("SIP");
  else if(protocol == "xmpp" || protocol == "jabber")
    this->title = QString("XMPP");
  else if(protocol == "telegram" || protocol == "telegram_tdlib")
    this->title = QString("Telegram");
  else if(protocol == "skype")
    this->title = QString("Skype");
  else if(protocol == "slack")
    this->title = QString("Slack");
  else
    this->title = protocol;

  this->title = this->title.left(1).toUpper() + this->title.mid(1);
}

OverviewProxyModel::OverviewProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void OverviewProxyModel::setNameFilter(QString name) {
  if(name.toLower() == m_nameFilter) return;
  qDebug() << "name filter" << name;

  this->m_nameFilter = name.toLower();
  if (this->sourceModel() != nullptr) {
    auto mdl = dynamic_cast<OverviewModel*>(this->sourceModel());
    this->invalidate();
  }
}

void OverviewProxyModel::setProtocolFilter(QString protocol) {
  if(protocol.toLower() == m_protocolFilter) return;
  qDebug() << "proxy filter" << protocol;
  if(protocol == "*")
    protocol = "";

  this->m_protocolFilter = protocol.toLower();
  if (this->sourceModel() != nullptr) {
    auto mdl = dynamic_cast<OverviewModel*>(this->sourceModel());
    this->invalidate();
  }
}

// optionally filter on:
// 1. protocol (SMS, Telegram, ..)
// 2. remote name
bool OverviewProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
  if(this->sourceModel() == nullptr)
    return false;

  auto mdl = dynamic_cast<OverviewModel*>(this->sourceModel());
  auto index = this->sourceModel()->index(source_row, 0, source_parent);

  if(!index.isValid())
    return false;

  // filtering
  if(m_protocolFilter.isEmpty() && m_nameFilter.isEmpty()) return true;
  bool show = false;

  if(!m_protocolFilter.isEmpty()) {
    QSharedPointer<ChatMessage> msg = mdl->messages[index.row()];
    if(msg->protocol.contains(m_protocolFilter))
      show = true;
  }

  if(!m_nameFilter.isEmpty()) {
    QSharedPointer<ChatMessage> msg = mdl->messages[index.row()];
    if(msg->name().contains(m_nameFilter))
      show = true;
  }

  return show;
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

OverviewModel::OverviewModel(Telepathy *tp, ConfigState *state, QObject *parent) :
    m_tp(tp),
    m_state(state),
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
      case OverviewModel::TimeRole: {
        return message->date();
      }
      default:
        return QVariant();
    }
  }
  else if(role == Qt::DecorationRole) {
    switch (index.column()) {
      case OverviewModel::MsgStatusIcon: {
        const auto icon_default = "general_chat";
        const auto icon = message->icon_name();
        if(!icon.isEmpty()) {
          if(m_pixmaps.contains(icon))
            return m_pixmaps[icon];
          else {
            qWarning() << "icon" << icon << "does not exist";
            return m_pixmaps[icon_default];
          }
        } else {
          return m_pixmaps[icon_default];
        }
      }
      case OverviewModel::PresenceIcon: {
        auto uid = message->local_remote_uid();
        if(abook_roster_cache.contains(uid)) {
          QSharedPointer<ContactItem> contact = abook_roster_cache[uid];
          QString presence = contact->presence();
          // @TODO: use get icon instead @ abook
          if(presence == "available") {
            return m_pixmaps["presence_online"];
          } else if(presence == "unset") {
            return m_pixmaps["presence_unset"];
          } else if(presence == "offline") {
            return m_pixmaps["presence_offline"];
          } else {
            return m_pixmaps["presence_unset"];
          }
        } else {
          return QVariant();
        }
      }
      case OverviewModel::AvatarIcon: {
        if(message->hasAvatar())
          return message->avatarImage();
        return QVariant();
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
  }
  else if(role == Qt::SizeHintRole) {
    switch (index.column()) {
      case OverviewModel::MsgStatusIcon: {
        return QSize(58, 54);
      }
      case OverviewModel::AvatarIcon: {
        return QSize(58, 54);
        // if(message->hasAvatar())
        //   return QSize(58, 54);
        // else
        //   return QSize(0, 54);
      }
      case OverviewModel::AvatarPadding: {
        return QSize(4, 54);
      }
      case OverviewModel::PresenceIcon: {
        return QSize(18, 54);
      }
      case OverviewModel::ChatTypeIcon: {
        return QSize(58, 54);
      }
      default: {
        return QVariant();
      }
    }
  }
  else if(role == OverviewModel::TimeRole) {  // for sort
    return message->date();
  }

  return {};
}

void OverviewModel::onLoad() {
  // The overview screen has 3 sources:
  // 1. rtcom-db
  // - all messages are registered in rtcom, we group by
  //   group_uid and order by timestamp.
  // 2. Telepathy
  // - in the case you just joined a groupchat but there
  //   no messages registered yet in rtcom-db, Tp can give us
  //   more channels than the first option, so that we can
  //   render them on the overview regardless.
  // 3. ConfigState
  // - In the event Telepathy is offline, or otherwise not able
  //   to provide us channels, they are cached in "ConfigState"
  //   see ~/.config/conversations/state.json
  this->onClear();

  QList<ChatMessage*> results;
  QStringList group_uids;
  ConfigStateItemPtr configItem;

  // =====
  // rtcom
  // =====
  const uint32_t limit = 50000;
  const uint32_t offset = 0;
  RTComElQuery *query = qtrtcom::startQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_GROUP);
  bool query_prepared = FALSE;

  const gint service_id = rtcom_el_get_service_id(qtrtcom::rtcomel(), "RTCOM_EL_SERVICE_CALL");
  query_prepared = rtcom_el_query_prepare(query, "service-id", service_id, RTCOM_EL_OP_NOT_EQUAL,  NULL);

  if(!query_prepared) {
    qCritical() << "Couldn't prepare query";
  } else {
    results = iterateRtComEvents(query);
    for(const auto *msg: results) {
      group_uids << msg->group_uid();
    }
  }

  g_object_unref(query);

  // ==
  // tp
  // ==
  for(const auto &account: m_tp->accounts) {
    for(const auto &remote_uid: account->channels.keys()) {
      auto channel = account->channels[remote_uid];
      auto group_uid = account->getGroupUid(channel);
      auto room_name = account->getRoomName(channel);

      if(group_uids.contains(group_uid))
        continue;

      // fetching extra info from ConfigState
      qint64 date_created = 0;
      configItem = m_state->getItem(account->local_uid, remote_uid);
      if(configItem)
        date_created = configItem->date_created / 1000;

      results << new ChatMessage({
        .event_id = -1,
        .service = account->getServiceName(),
        .group_uid = group_uid,
        .local_uid = account->local_uid,
        .remote_uid = "",
        .remote_name = "",
        .remote_ebook_uid = "",
        .text = "",
        .icon_name = "",
        .timestamp = date_created,
        .count = 0,
        .group_title = "",
        .channel = remote_uid,
        .event_type = "-1",
        .outgoing = false,
        .is_read = true,
        .flags = 0
      });

      group_uids << group_uid;
    }
  }

  // ===========
  // ConfigState
  // ===========
  for(const auto &configItem: m_state->items) {
    QString channel_str;
    QString remote_uid;
    if(configItem->type == ConfigStateItemType::ConfigStateRoom)
      channel_str = configItem->remote_uid;
    else if(configItem->type == ConfigStateItemType::ConfigStateContact)
      remote_uid = configItem->remote_uid;
    else {
      qWarning() << "ConfigStateItemType" << configItem->type << "not implemented";
      continue;
    }

    if(group_uids.contains(configItem->group_uid))
      continue;

    if(configItem->group_uid.isEmpty())
      continue;

    results << new ChatMessage({
      .event_id = -1,
      .service = "RTCOM_EL_SERVICE_CHAT",
      .group_uid = configItem->group_uid,
      .local_uid = configItem->local_uid,
      .remote_uid = remote_uid,
      .remote_name = "",
      .remote_ebook_uid = "",
      .text = "",
      .icon_name = "",
      .timestamp = configItem->date_created / 1000,
      .count = 0,
      .group_title = "",
      .channel = channel_str,
      .event_type = "-1",
      .outgoing = false,
      .is_read = true,
      .flags = 0
    });

    group_uids << configItem->group_uid;
  }

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
  roles[AvatarIcon] = "avatar_icon";
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
    "general_chat",
    "general_sms",
    "presence_online",
    "presence_offline",
    "presence_away",
    "presence_empty",
    "presence_unset"
  };

  for(const auto &icon: icons) {
    const auto fn = QString("%1/%2.png").arg(basepath, icon);
    const auto fn_qrc = QString(":/%1.png").arg(icon);

    if(Utils::fileExists(fn_qrc)) {
      m_pixmaps[icon] = QPixmap(fn_qrc);
    } else if(Utils::fileExists(fn)) {
      m_pixmaps[icon] = QPixmap(fn);
    } else {
    }
  }
}

void OverviewModel::onClear() {
  beginResetModel();
  this->messages.clear();
  endResetModel();
}
