#include <ranges>
#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/ipc.h"
#include "lib/utils.h"
#include "lib/globals.h"
#include "lib/state.h"

ConfigState* configState = nullptr;

ConfigState::ConfigState(const QString &path_statefile, QObject *parent) :
    m_path_statefile(path_statefile),
    m_saveTimer(new QTimer(this)),
    QObject(parent) {
  // throttle writing to disk
  m_saveTimer->setInterval(30000);
  connect(m_saveTimer, &QTimer::timeout, this, &ConfigState::onSaveTimerTimeout);
  m_saveTimer->start();

  this->load();
}

void ConfigState::load() {
  qDebug() << "ConfigState::load()";

  items.clear();
  if(!Utils::fileExists(m_path_statefile))
    return;

  auto data = Utils::fileOpen(m_path_statefile);
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if(doc.isNull() || !doc.isArray()) {
    qWarning() << "invalid json encountered parsing ConfigState";
    return;
  }

  auto arr = doc.array();
  for(const auto &obj: arr) {
    auto item  = ConfigStateItemPtr(new ConfigStateItem());
    auto blob = obj.toObject();
    item->remote_uid = blob["remote_uid"].toString();
    item->local_uid = blob["local_uid"].toString();
    item->group_uid = blob["group_uid"].toString();
    item->auto_join = blob["auto_join"].toBool();
    if (blob.contains("ignore_notifications"))
      item->ignore_notifications = blob["ignore_notifications"].toBool();

    item->type = (ConfigStateItemType) blob["type"].toInt();

    qint64 date_created = 0;
    if(blob.contains("date_created"))
        date_created = blob["date_created"].toString().toLongLong();
    item->date_created = date_created;

    qint64 date_last_message = 0;
    if(blob.contains("date_last_message"))
        date_last_message = blob["date_last_message"].toString().toLongLong();
    item->date_last_message = date_last_message;

    items << item;
  }

  qDebug() << "ConfigState::load()" << QString::number(items.size()) << "items";
  emit updated();
}

ConfigStateItemPtr ConfigState::getItem(const QString &local_uid, const QString &remote_id, ConfigStateItemType type) {
  qDebug() << "ConfigState::getItem()" << local_uid << remote_id;

  QList<ConfigStateItemPtr>::const_iterator it =
    std::find_if(items.cbegin(), items.cend(), [local_uid, remote_id, type](auto &ptr) {
      auto valid = ptr->local_uid == local_uid && ptr->remote_uid == remote_id;
      if(type != ConfigStateItemType::ConfigStateAny) {
        return ptr->type == type && valid;
      } else {
        return valid;
      }
    });

  if(it == items.end())
    return {};
  return *it;
}

ConfigStateItemPtr ConfigState::addItem(const QString &local_uid, const QString &remote_id, const QString &group_uid, ConfigStateItemType type) {
  qDebug() << "ConfigState::addItem()" << local_uid << remote_id << group_uid;
  auto item = getItem(local_uid, remote_id);
  if(item) {
    // handle edge-case when a configStateItem was added via setAutoJoin()
    if(item->group_uid.isEmpty() && !group_uid.isEmpty()) {
      item->group_uid = group_uid;
      m_dirty = true;
      save();
    }
    return {};
  }

  item = ConfigStateItemPtr(new ConfigStateItem());
  item->local_uid = local_uid;
  item->remote_uid = remote_id;
  item->group_uid = group_uid;
  item->type = type;
  item->date_created = QDateTime::currentMSecsSinceEpoch();
  items << item;

  save();
  return item;
}

bool ConfigState::deleteItem(const QString &local_uid, const QString &remote_id) {
  qDebug() << "ConfigState::deleteItem()" << local_uid << remote_id;
  if(const auto item = getItem(local_uid, remote_id); !item)
    return false;

  auto filtered = items | std::views::filter([remote_id](const ConfigStateItemPtr &_item) {
      return _item->remote_uid != remote_id;
  });

  items = QList<ConfigStateItemPtr>(filtered.begin(), filtered.end());

  qDebug() << "ConfigState::deleteItem()" << local_uid << remote_id;
  save();
  return true;
}

bool ConfigState::setNotificationsIgnore(const QString &local_uid, const QString &remote_id, bool ignore) {
  const auto item = getItem(local_uid, remote_id);
  if(!item)
    return false;

  item->ignore_notifications = ignore;

  save(false);
  m_dirty = true;
  return true;
}

bool ConfigState::getNotificationsIgnore(const QString &local_uid, const QString &remote_id) {
  const auto item = getItem(local_uid, remote_id);
  if(!item)
    return false;
  return item->ignore_notifications;
}

bool ConfigState::setAutoJoin(const QString &local_uid, const QString &remote_id, bool auto_join) {
  auto item = getItem(local_uid, remote_id);
  if(!item)
    item = addItem(local_uid, remote_id, "", ConfigStateItemType::ConfigStateRoom);

  if(item->type != ConfigStateItemType::ConfigStateRoom) {
    qWarning() << "setAutoJoin on" << remote_id << "failed: item is not type room";
    return false;
  }

  item->auto_join = auto_join;
  emit autoJoinChanged(local_uid, remote_id, auto_join);

  save(false);
  m_dirty = true;
  return true;
}

bool ConfigState::getAutoJoin(const QString &local_uid, const QString &remote_id) {
  auto item = getItem(local_uid, remote_id);
  if(!item)
    return false;
  return item->auto_join;
}

void ConfigState::setLastMessageTimestamp(const QString &local_uid, const QString &remote_id, qint64 timestamp) {
  auto item = getItem(local_uid, remote_id);
  if(!item) {
    qWarning() << "setLastMessageTimestamp() could not set timestamp on non-existent configItem";
    return;
  }

  item->date_last_message = timestamp;

  save(false);
  m_dirty = true;
}

qint64 ConfigState::getLastMessageTimestamp(const QString &local_uid, const QString &remote_id) {
  auto item = getItem(local_uid, remote_id);
  if(!item)
    return 0;
  return item->date_last_message;
}

void ConfigState::save(const bool emit_update_signal) {
  qDebug() << "ConfigState::save()";

  QJsonArray arr;
  for(const auto &item: items) {
    QJsonObject obj;
    obj["remote_uid"] = item->remote_uid;
    obj["local_uid"] = item->local_uid;
    obj["group_uid"] = item->group_uid;
    obj["auto_join"] = item->auto_join;
    obj["ignore_notifications"] = item->ignore_notifications;
    obj["type"] = double(item->type);

    // @TODO: unfortunately we need to use QString to represent a qint64, but Qt6 has QJsonValue::toInteger()
    obj["date_created"] = QString::number(item->date_created);
    obj["date_last_message"] = QString::number(item->date_last_message);
    arr << obj;
  }

  auto dumps = QJsonDocument(arr).toJson(QJsonDocument::Indented);
  QFile file(m_path_statefile);
  if(file.open(QIODevice::WriteOnly)){
    QTextStream out(&file);
    out << dumps << "\n";
    file.close();
  }

  if (emit_update_signal)
    emit updated();
}

void ConfigState::onSaveTimerTimeout() {
  if(!m_dirty)
    return;

  save(false);
  m_dirty = false;
}
