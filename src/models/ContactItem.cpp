#include "models/ContactItem.h"

ContactItem::ContactItem(QString local_uid, QString remote_uid, QObject *parent) :
    m_local_uid(local_uid),
    m_remote_uid(remote_uid),
    QObject(parent) {
        qDebug() << "new ContactItem" << remote_uid;
}

ContactItem::ContactItem(QString persistent_uid, QObject *parent) : QObject(parent) {
    QStringList persistent_uid_spl = persistent_uid.split("-");
    m_local_uid = persistent_uid_spl.at(0);
    m_remote_uid = persistent_uid_spl.at(1);
    qDebug() << "new ContactItem" << m_remote_uid;
}

bool ContactItem::canSetNewAvatar(void* token) {
    return token != m_avatar_token;
}

bool ContactItem::hasAvatar() {
    return m_avatar_token != nullptr;
}

void ContactItem::setAvatar(void* token, QImage &img) {
    m_avatar_token = token;
    m_avatar_token_hex = QString::asprintf("%p", token);
    if(!m_avatar_token_hex.isEmpty())
        m_avatar_token_hex.remove(0, 2);
    m_avatar = img;
    emit avatarChanged();
}

ContactItem::~ContactItem() {
    //qDebug() << "ContactItem destructor";
}