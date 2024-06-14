#pragma once

#include <QObject>

#include "lib/utils.h"

class ConfigStateItem;
typedef QSharedPointer<ConfigStateItem> ConfigStateItemPtr;

enum ConfigStateItemType {
  ConfigStateContact = 0,
  ConfigStateRoom,
  ConfigStateAny
};

class ConfigStateItem {
public:
  explicit ConfigStateItem() {}

  QString remote_uid;
  QString local_uid;
  ConfigStateItemType type;
  bool auto_join = false;
  qint64 date_created;           // in msecs since epoch
  qint64 date_last_message = 0;  // in msecs since epoch

  QString group_uid() const {
    return QString("%1-%2").arg(local_uid, remote_uid);
  }
};

class ConfigState : public QObject {
Q_OBJECT

public:
  explicit ConfigState(const QString &path_statefile, QObject *parent = nullptr);
  QList<ConfigStateItemPtr> items;
  void save();

public:
  bool setAutoJoin(const QString &local_uid, const QString &remote_id, bool auto_join);
  bool getAutoJoin(const QString &local_uid, const QString &remote_id);

  void setLastMessageTimestamp(const QString &local_uid, const QString &remote_id, qint64 timestamp);
  qint64 getLastMessageTimestamp(const QString &local_uid, const QString &remote_id);

public slots:
  ConfigStateItemPtr addItem(const QString &local_uid, const QString &remote_id, ConfigStateItemType type);
  ConfigStateItemPtr getItem(const QString &local_uid, const QString &remote_id, ConfigStateItemType type = ConfigStateItemType::ConfigStateAny);
  void deleteItem(const QString &local_uid, const QString &remote_id);

private slots:
  void onSaveTimerTimeout();

signals:
  void updated();
  void autoJoinChanged(const QString &local_uid, const QString &remote_id, bool auto_join);

private:
  void load();

private:
  QString m_path_statefile;

  bool m_dirty = false;
  QTimer *m_saveTimer;
};

extern ConfigState* configState;