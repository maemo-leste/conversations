#include <QObject>
#include <QDebug>

#include "lib/abook/abook_public.h"
#include "lib/abook/abook_roster.h"
#include "lib/logger_std/logger_std.h"
#include "overview/OverviewModel.h"
#include "conversations.h"

ServiceAccount::ServiceAccount() = default;
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

OverviewProxyModel::OverviewProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  setSortRole(static_cast<int>(OverviewModel::OverviewModelRoles::TimeRole));
  QSortFilterProxyModel::sort(0, Qt::DescendingOrder); // newest first
}

bool OverviewProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
  const QVariant data_l  = sourceModel()->data(source_left, static_cast<int>(OverviewModel::OverviewModelRoles::TimeRole));
  const QVariant data_r = sourceModel()->data(source_right, static_cast<int>(OverviewModel::OverviewModelRoles::TimeRole));
  const QDateTime time_l  = data_l.toDateTime();
  const QDateTime time_r = data_r.toDateTime();
  return time_l < time_r;
}

void OverviewProxyModel::setNameFilter(QString name) {
  if(name.toLower() == m_nameFilter) return;
  qDebug() << "name filter" << name;

  this->m_nameFilter = name.toLower();
  if (this->sourceModel() != nullptr) {
    auto mdl = dynamic_cast<OverviewModel*>(this->sourceModel());
    this->invalidateFilter();
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
    this->invalidateFilter();
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
  if(m_protocolFilter.isEmpty() && m_nameFilter.isEmpty())
    return true;
  bool show = false;

  if(!m_protocolFilter.isEmpty()) {
    QSharedPointer<ChatMessage> msg = mdl->messages[index.row()];
    if(msg->protocol().contains(m_protocolFilter))
      show = true;
  }

  if(!m_nameFilter.isEmpty()) {
    QSharedPointer<ChatMessage> msg = mdl->messages[index.row()];
    if(msg->matchesName(m_nameFilter))
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

  connect(Conversations::instance(), &Conversations::contactsChanged, this, &OverviewModel::onContacsChanged);
  connect(Conversations::instance(), &Conversations::avatarChanged, this, &OverviewModel::onAvatarChanged);
}

int OverviewModel::rowCount(const QModelIndex & parent) const {
  Q_UNUSED(parent);
  return messages.count();
}

int OverviewModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(Columns::COUNT);
}

void OverviewModel::onDatabaseAddition(QSharedPointer<ChatMessage> &msg) {
  // try to modify an existing row
  // if it's a new row, loadOverviewMessages() as usual
  const auto uid = msg->group_uid();
  bool found = false;
  for (int row = 0; row < messages.count(); row++) {
    if (const auto _msg = messages[row]; _msg->group_uid() == uid) {
      if (row < 0 || row >= messages.size())
        return;
      messages[row] = msg;

      const int lastColumn = this->columnCount() - 1;
      const QModelIndex topLeft = this->index(row, 0);
      const QModelIndex bottomRight = this->index(row, lastColumn);
      emit dataChanged(topLeft, bottomRight);

      found = true;
      break;
    }
  }

  if (!found) {
    this->loadOverviewMessages();
  }
}

// slot: abook contact avatar changed, update table row entry
void OverviewModel::onAvatarChanged(const std::string &abook_uid) {
  for (size_t row = 0; row < messages.size(); ++row) {
    const auto &m = messages[row];
    const auto *raw = m->raw();
    auto row_abook_uid = abook_qt::get_abook_uid(raw->protocol, raw->remote_uid);

    if (row_abook_uid == abook_uid) {
      const int lastColumn = this->columnCount() - 1;
      const QModelIndex topLeft = this->index(row, 0);
      const QModelIndex bottomRight = this->index(row, lastColumn);
      emit dataChanged(topLeft, bottomRight);
    }
  }
}

// slot: abook contact attributes changed, update table row entry
void OverviewModel::onContacsChanged(std::vector<std::shared_ptr<abook_qt::AbookContact>> contacts) {
  for (const auto &contact: contacts) {
    for (size_t row = 0; row < messages.size(); ++row) {
      const auto &m = messages[row];
      auto row_remote_uid = m->remote_uid().toStdString();

      if (row_remote_uid == contact->remote_uid) {
        const int lastColumn = this->columnCount() - 1;
        const QModelIndex topLeft = this->index(row, 0);
        const QModelIndex bottomRight = this->index(row, lastColumn);
        emit dataChanged(topLeft, bottomRight);
      }
    }
  }
}

void OverviewModel::updateMessage(int row, QSharedPointer<ChatMessage> &msg) {
  if (row < 0 || row >= messages.size())
    return;

  messages[row] = msg;

  QModelIndex index = this->index(row);
  emit dataChanged(index, index);
}

QVariant OverviewModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= messages.size())
    return {};

  const auto &msg = messages[index.row()];
  const int col = index.column();

  // column based display
  if (role == Qt::DisplayRole || role == Qt::DecorationRole) {
    switch (col) {
      case static_cast<int>(Columns::ContentColumn):
        return msg->generateOverviewItemDelegateRichText();

      case static_cast<int>(Columns::MsgStatusColumn): {
        const QString icon = msg->icon_name();
        return (!icon.isEmpty() && m_icons.contains(icon)) ? m_icons[icon] : m_icons.value("general_chat");
      }

      case static_cast<int>(Columns::PresenceColumn): {
        auto *raw = msg->raw();
        const auto presence = abook_qt::get_presence(raw->protocol, raw->remote_uid);
        if (!presence.isNull()) {
          QString icon_name = QString::fromStdString(presence.icon_name);
          return m_icons.contains(icon_name) ? m_icons[icon_name] : QVariant();
        }
        return {};
      }

      case static_cast<int>(Columns::ChatTypeColumn): {
        const QString icon = msg->channel().isEmpty() ? "general_default_avatar" : "general_conference_avatar";
        return m_icons.value(icon, {});
      }

      case static_cast<int>(Columns::AvatarColumn): {
        const auto protocol = msg->protocol().toStdString();
        const auto remote_uid = msg->remote_uid().toStdString();
        const auto avatar_token = abook_qt::get_avatar_token(protocol, remote_uid);
        if (!avatar_token.empty() && avatar_token != "0") {
          QPixmap pixmap;
          if (Utils::get_avatar(protocol, remote_uid, avatar_token, pixmap))
            return pixmap;
        }
        return {};
      }

      case static_cast<int>(Columns::TimeColumn):
        return msg->date();

      default:
        return {};
    }
  }

  if (role == Qt::TextAlignmentRole) {
    if (col == static_cast<int>(Columns::MsgStatusColumn) ||
        col == static_cast<int>(Columns::PresenceColumn) ||
        col == static_cast<int>(Columns::ChatTypeColumn)) {
      return Qt::AlignHCenter;
    }
    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
  }

  if (role == Qt::SizeHintRole) {
    if (col == static_cast<int>(Columns::MsgStatusColumn) ||
        col == static_cast<int>(Columns::ChatTypeColumn))
      return QSize(58, 54);
    if (col == static_cast<int>(Columns::PresenceColumn))
      return QSize(24, 24);
    return {};
  }

  // optional: sorting/filtering roles
  // switch (role) {}
  return {};
}

void OverviewModel::loadSearchMessages(const QString& needle, const QString& group_uid) {
  this->onClear();
  const auto needle_str = needle.toStdString();
  const auto group_uid_str = group_uid.toStdString();
  auto results = rtcom_qt::search_messages(needle_str, group_uid_str);

  beginInsertRows(QModelIndex(), 0, results.size());
  for(auto const &message: results)
    messages << QSharedPointer<ChatMessage>(new ChatMessage(message));
  endInsertRows();
}

QList<ChatMessage*> OverviewModel::getOverviewMessages() {
  CLOCK_MEASURE_START(start_total);
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
  CLOCK_MEASURE_START(start_rtcom);
  constexpr unsigned int limit = 50000;
  constexpr unsigned int offset = 0;

  for(auto rtcom_results = rtcom_qt::get_overview_messages(limit, offset); auto *msg: rtcom_results) {
    group_uids << QString::fromStdString(msg->group_uid);
    results << new ChatMessage(msg);
  }

  CLOCK_MEASURE_END(start_rtcom, "OverviewModel::loadOverviewMessages rtcom");
  CLOCK_MEASURE_START(start_tp);

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

      results << new ChatMessage(new rtcom_qt::ChatMessageEntry(
        -1,
        account->getServiceName().toStdString(),
        group_uid.toStdString(),
        account->local_uid.toStdString(),
        account->protocolName().toStdString(),
        "", "", "", "", "", date_created, 0,
        remote_uid.toStdString(),
        "-1",
        false,
        true,
        0
      ));

      group_uids << group_uid;
    }
  }

  CLOCK_MEASURE_END(start_tp, "OverviewModel::loadOverviewMessages TP");
  CLOCK_MEASURE_START(start_config_state);

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

    results << new ChatMessage(new rtcom_qt::ChatMessageEntry(
        -1,
        "RTCOM_EL_SERVICE_CHAT",
        configItem->group_uid.toStdString(),
        configItem->local_uid.toStdString(),
        configItem->protocol().toStdString(),
        remote_uid.toStdString(),
        "", "", "", "", configItem->date_created / 1000, 0,
        channel_str.toStdString(),
        "-1",
        false,
        true,
        0
      ));

    group_uids << configItem->group_uid;
  }

  CLOCK_MEASURE_END(start_config_state, "OverviewModel::loadOverviewMessages ConfigState");
  CLOCK_MEASURE_END(start_total, "OverviewModel::loadOverviewMessages total");

  return results;
}

void OverviewModel::loadOverviewMessages() {
  auto results = getOverviewMessages();
  setMessages(results);
}

void OverviewModel::setMessages(QList<ChatMessage*> lst) {
  beginInsertRows(QModelIndex(), 0, lst.size());
  for (const auto &message: lst) {
    messages << QSharedPointer<ChatMessage>(message);
  }
  endInsertRows();
}

QHash<int, QByteArray> OverviewModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[static_cast<int>(OverviewModelRoles::OverviewNameRole)] = "overview_name";
  roles[static_cast<int>(OverviewModelRoles::ProtocolRole)]     = "protocol";
  roles[static_cast<int>(OverviewModelRoles::MsgStatusIcon)]    = "msg_status_icon";
  roles[static_cast<int>(OverviewModelRoles::ChatTypeIcon)]     = "chat_type_icon";
  roles[static_cast<int>(OverviewModelRoles::AvatarIcon)]       = "avatar_icon";
  roles[static_cast<int>(OverviewModelRoles::PresenceIcon)]     = "presence_icon";
  roles[static_cast<int>(OverviewModelRoles::ContentRole)]      = "content";
  roles[static_cast<int>(OverviewModelRoles::TimeRole)]         = "time";
  return roles;
}

// cached, because used inside a potentially busy table
void OverviewModel::preloadPixmaps() {
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
    "general_presence_sports",
    "general_presence_home",
    "general_presence_out",
    "general_presence_travel",
    "statusarea_presence_away_error",
    "general_presence_cultural_activities",
    "general_presence_online",
    "general_presence_invisible",
    "statusarea_presence_busy_error",
    "general_presence_work_error",
    "general_presence_busy",
    "general_presence_offline",
    "general_presence_work",
    "general_presence_home_error",
    "general_presence_away",
    "control_presence",
    "general_presence_out_error",
    "general_presence_travel_error",
    "statusarea_presence_online_error",
    "general_presence_cultural_activities_error",
    "general_presence_sports_error"
  };

  for(const auto &icon: icons) {
    const auto fn = QString("%1/%2.png").arg(basepath, icon);
    const auto fn_qrc = QString(":/%1.png").arg(icon);

    QPixmap pixmap;
    if (Utils::fileExists(fn_qrc)) {
      pixmap = QPixmap(fn_qrc);
    } else if (Utils::fileExists(fn)) {
      pixmap = QPixmap(fn);
    }

    if (!pixmap.isNull()) {
      QPixmap scaledPixmap;
      if (fn.contains("presence")) {
        scaledPixmap = pixmap.scaled(26, 26, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      } else {
        scaledPixmap = pixmap;
      }

      m_icons[icon] = QIcon(scaledPixmap);
    }
  }
}

void OverviewModel::onClear() {
  beginResetModel();
  this->messages.clear();
  endResetModel();
}
