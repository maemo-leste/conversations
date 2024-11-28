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

void ContactItem::setPresence(QString presence, bool &dirty) {
#ifdef DEBUG
    if(remote_uid() == "dsc@xmpp.is") 
        qDebug() << "setPresence" << presence;
#endif
    presence = presence.toLower();

    if(m_presence != presence) {
        m_presence = presence;
        dirty = true;
    }
}

void ContactItem::setSubscribed(QString subscribed, bool &dirty) {
#ifdef DEBUG    
    if(remote_uid() == "dsc@xmpp.is") 
        qDebug() << "setSubscribed" << subscribed;
#endif

    if(m_subscribed != subscribed) {
        m_subscribed = subscribed;
        dirty = true;
    }
}

void ContactItem::setPublished(QString published, bool &dirty) {
#ifdef DEBUG
    if(remote_uid() == "dsc@xmpp.is") 
        qDebug() << "setPublished" << published;
#endif

    if(m_published != published) {
        m_published = published;
        dirty = true;
    }
}

void ContactItem::setIsBlocked(bool is_blocked, bool &dirty) {
#ifdef DEBUG
    if(remote_uid() == "dsc@xmpp.is") 
        qDebug() << "setIsBlocked" << is_blocked;
#endif

    if(m_is_blocked != is_blocked) {
        m_is_blocked = is_blocked;
        dirty = true;
    }
}

void ContactItem::setCanBlock(bool can_block, bool &dirty) {
#ifdef DEBUG
    if(remote_uid() == "dsc@xmpp.is") 
        qDebug() << "setCanBlock" << can_block;
#endif

    if(m_can_block != can_block) {
        m_can_block = can_block;
        dirty = true;
    }
}

bool ContactItem::canSetNewAvatar(void* token) {
    return token != m_avatar_token;
}

bool ContactItem::hasAvatar() {
    return m_avatar_token != nullptr;
}

void ContactItem::setAvatar(void* token, QImage &img) {
    qDebug() << "== ContactItem::setAvatar";
    m_avatar_token = token;
    m_avatar_token_hex = QString::asprintf("%p", token);
    if(!m_avatar_token_hex.isEmpty())
        m_avatar_token_hex.remove(0, 2);
    m_avatar = img;
    emit avatarChanged();
}

void ContactItem::setCanAuth(bool can_auth, bool &dirty) {
#ifdef DEBUG
    if(remote_uid() == "dsc@xmpp.is") 
        qDebug() << "setCanAuth" << can_auth;
#endif

    if(m_can_auth != can_auth) {
        m_can_auth = can_auth;
        dirty = true;
    }
}

void ContactItem::print() {
#ifdef DEBUG
    if(remote_uid() != "dsc@xmpp.is") 
        return;
#endif
    qDebug() << "======================== ContactItem";
    qDebug() << "group_uid" << group_uid();
    qDebug() << "local_uid" << local_uid();
    qDebug() << "remote_uid" << remote_uid();
    qDebug() << "presence" << presence();
    qDebug() << "subscribed" << subscribed();
    qDebug() << "published" << published();
    qDebug() << "is_blocked" << is_blocked();
    qDebug() << "can_block" << can_block();
    qDebug() << "can_auth" << can_auth();
    qDebug() << "========================";
}

ContactItem::~ContactItem() {
    //qDebug() << "ContactItem destructor";
}