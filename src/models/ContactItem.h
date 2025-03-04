#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QPixmap>
#include <QStringList>
#include <QDir>
#include <QDebug>

#include "lib/abook/abook_contact.h"

class ContactItem : public QObject {
Q_OBJECT

public:
  explicit ContactItem(QString local_uid, QString remote_uid, QObject *parent = nullptr);
  explicit ContactItem(QString persistent_uid, QObject *parent = nullptr);
  ~ContactItem() override;

  QString id() const { return m_local_uid + "-" + m_remote_uid; }

  // AbookContactAvatar

  QString group_uid() const { return m_group_uid; }
  QString local_uid() const { return m_local_uid; }
  QString remote_uid() const { return m_remote_uid; }

  QImage& avatar() { return m_avatar; }
  bool canSetNewAvatar(void* token);
  void setAvatar(void* token, QImage &img);
  bool hasAvatar();
  QString avatar_token_hex() { return m_avatar_token_hex; }

signals:
  void avatarChanged();

private:
  QString m_group_uid;
  QString m_local_uid;
  QString m_remote_uid;

  void* m_avatar_token = nullptr;
  QString m_avatar_token_hex;
  QImage m_avatar;
};
