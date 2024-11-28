#include "lib/abook.h"
#include "lib/abook_roster.h"

std::function<void()> conv_abook_func_roster_updated = nullptr;

bool conv_abook_init() {
    GError *err = NULL;
    if (!conv_abook_inited) {
        osso_abook_init_with_name("conversations", NULL);

        conv_abook_roster = osso_abook_aggregator_get_default(&err);
        if (err != NULL) {
            fprintf(stderr, "Failed to get a default aggregator\n");
            g_error_free(err);
            return FALSE;
        }
        conv_abook_aggregator = OSSO_ABOOK_AGGREGATOR(conv_abook_roster);
        printf("conv_abook_aggregator: %p\n", conv_abook_aggregator);

        osso_abook_waitable_run(OSSO_ABOOK_WAITABLE(conv_abook_aggregator),
                    g_main_context_default(), &err);
        if (err != NULL) {
            fprintf(stderr, "Failed to wait for the aggregator\n");
            /* Free roster (not aggregator I think) */
            g_error_free(err);
            return FALSE;
        }

        g_signal_connect(conv_abook_roster, "contacts-added", G_CALLBACK(contacts_added_cb), nullptr);
        g_signal_connect(conv_abook_roster, "contacts-removed", G_CALLBACK(contacts_removed_cb), nullptr);
        g_signal_connect(conv_abook_roster, "contacts-changed", G_CALLBACK(contacts_changed_cb), nullptr);
        g_signal_connect(conv_abook_aggregator, "contacts-added", G_CALLBACK(contacts_added_cb), nullptr);
        g_signal_connect(conv_abook_aggregator, "contacts-removed", G_CALLBACK(contacts_removed_cb), nullptr);
        g_signal_connect(conv_abook_aggregator, "contacts-changed", G_CALLBACK(contacts_changed_cb), nullptr);

        conv_abook_inited = TRUE;
    }

    return TRUE;
}

void contacts_changed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer user_data) {
    bool dirty = false;
    while (*contacts) {
        OssoABookContact *contact = *contacts;

        GList *rc = osso_abook_contact_get_roster_contacts(contact);
        QString persistent_uid = QString::fromUtf8(osso_abook_contact_get_persistent_uid(contact));

        for (GList *l = rc; l; l = l->next) {
            OssoABookPresence *abook_presence = OSSO_ABOOK_PRESENCE(l->data);
            TpConnectionPresenceType presenceType = osso_abook_presence_get_presence_type(abook_presence);
            OssoABookPresenceState published = osso_abook_presence_get_published(abook_presence);
            OssoABookPresenceState subscribed = osso_abook_presence_get_subscribed(abook_presence);

            //OSSO_ABOOK_DUMP_VCARD(EDS, l->data, "roster");

            if(!abook_roster_cache.contains(persistent_uid))
                abook_roster_cache[persistent_uid] = QSharedPointer<ContactItem>(new ContactItem(persistent_uid));

            abook_roster_cache[persistent_uid]->setPublished(presence_to_string(published), dirty);
            abook_roster_cache[persistent_uid]->setSubscribed(presence_to_string(subscribed), dirty);
            abook_roster_cache[persistent_uid]->setPresence(presence_type_to_string(presenceType), dirty);
            break;
        }

        g_list_free(rc);
        contacts++;
    }

    if(dirty)
        conv_abook_func_roster_updated();
}

void contacts_added_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data) {
    Q_UNUSED(roster);
    bool dirty = false;
    while (*contacts) {
        OssoABookContact *contact = *contacts;
        upsert_abook_roster_cache(contact, dirty);
        contacts++;
    }

    conv_abook_func_roster_updated();
}

void contacts_removed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data) {
    Q_UNUSED(roster);
    while (*contacts) {
        OssoABookContact *contact = *contacts;
        QString persistent_uid = QString::fromUtf8(osso_abook_contact_get_persistent_uid(contact));
        abook_roster_cache.remove(persistent_uid);
        contacts++;
    }

    conv_abook_func_roster_updated();
}

void notify_avatar_image_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    OssoABookContact *contact = OSSO_ABOOK_CONTACT(gobject);
    if(!contact)
        return;

    upsert_abook_roster_avatar(contact);
    conv_abook_func_roster_updated();
}

void get_contact_roster() {
    GError *err = NULL;
    GList *contacts = osso_abook_aggregator_list_roster_contacts(conv_abook_aggregator);
    if (!contacts) {
        qWarning() << "No contacts found";
        return;
    }

    osso_abook_waitable_run(OSSO_ABOOK_WAITABLE(conv_abook_aggregator),
        g_main_context_default(), &err);

    if (err != NULL) {
        qWarning() << "Failed to wait for the aggregator";
        g_list_free(contacts);
        g_error_free(err);
        return;
    }

    bool dirty = false;
    for (GList *l = contacts; l != nullptr; l = l->next) {
        OssoABookContact *contact = OSSO_ABOOK_CONTACT(l->data);
        EContact* e_contact = E_CONTACT(contact);

        g_signal_connect(contact, "notify::avatar-image", G_CALLBACK(notify_avatar_image_cb), nullptr);

        // roster cache
        QSharedPointer<ContactItem> contact_item = upsert_abook_roster_cache(contact, dirty);

        // avatar
        if(upsert_abook_roster_avatar(contact))
            emit contact_item->avatarChanged();
    }

    g_list_free(contacts);

    if(dirty)
        conv_abook_func_roster_updated();
}

QSharedPointer<ContactItem> upsert_abook_roster_cache(OssoABookContact *contact, bool &dirty) {
    QString persistent_uid = QString::fromUtf8(osso_abook_contact_get_persistent_uid(contact));

    bool is_blocked = osso_abook_contact_get_blocked(contact);
    bool can_block = osso_abook_contact_can_block(contact, NULL);
    bool can_auth = osso_abook_contact_can_request_auth(contact, NULL);

    OssoABookPresence *abook_presence = OSSO_ABOOK_PRESENCE(contact);
    TpConnectionPresenceType presenceType = osso_abook_presence_get_presence_type(abook_presence);

    // current presence: [detailed-name;]{available,away,...}[;custom-message]
    // @TODO: this gives warning: 'e_vcard_attribute_get_value called on multivalued attribute'
    QString presence = presence_type_to_string(presenceType);

    // values: yes, no, local-pending, remote-pending
    QString subscribed = presence_to_string(osso_abook_presence_get_subscribed(abook_presence));
    QString published = presence_to_string(osso_abook_presence_get_published(abook_presence));

    if(!abook_roster_cache.contains(persistent_uid))
        abook_roster_cache[persistent_uid] = QSharedPointer<ContactItem>(new ContactItem(persistent_uid));

    abook_roster_cache[persistent_uid]->setPublished(published, dirty);
    abook_roster_cache[persistent_uid]->setSubscribed(subscribed, dirty);
    abook_roster_cache[persistent_uid]->setPresence(presence, dirty);
    abook_roster_cache[persistent_uid]->setIsBlocked(is_blocked, dirty);
    abook_roster_cache[persistent_uid]->setCanBlock(can_block, dirty);
    abook_roster_cache[persistent_uid]->setCanAuth(can_auth, dirty);
#ifdef DEBUG
    abook_roster_cache[persistent_uid]->print();
#endif
    return abook_roster_cache[persistent_uid];
}

OssoABookContact* conv_abook_lookup_tel(const char* telno) {
    OssoABookContact* res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_phone_number(conv_abook_aggregator, telno, TRUE);

    GList *v = l;
    while (v) {
        OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
        res = contact;
        break;
    }

    g_list_free(l);
    return res;
}

OssoABookContact* conv_abook_lookup_sip(const char* address) {
    OssoABookContact* res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_sip_address(conv_abook_aggregator, address);

    GList *v = l;
    while (v) {
        OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
        res = contact;
        break;
    }

    g_list_free(l);
    return res;
}

OssoABookContact* conv_abook_lookup_im(const char* userid) {
    OssoABookContact* res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_im_contact(conv_abook_aggregator, userid, NULL); /* TODO: provide TpAccount* */

    GList *v = l;
    while (v) {
        OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
        res = contact;
        break;
    }

    g_list_free(l);

    return res;
}

bool upsert_abook_roster_avatar(OssoABookContact *contact) {
    OssoABookAvatar* avatar = OSSO_ABOOK_AVATAR(contact);
    GdkPixbuf* img = osso_abook_avatar_get_image(avatar);

    QString persistent_uid = QString::fromUtf8(osso_abook_contact_get_persistent_uid(contact));
    if(!abook_roster_cache.contains(persistent_uid))
        return false;

    void* avatar_token = static_cast<void*>(osso_abook_avatar_get_image_token(avatar));
    QImage qimg = imageFromGdkPixbuf(img);

    if(abook_roster_cache[persistent_uid]->canSetNewAvatar(avatar_token)) {
        abook_roster_cache[persistent_uid]->setAvatar(avatar_token, qimg);
        return true;
    }

    return false;
}

QString presence_to_string(OssoABookPresenceState presence) {
    switch(presence) {
        case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_YES: return "yes"; break;
        case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_NO: return "no"; break;
        case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_LOCAL_PENDING: return "local-pending"; break;
        case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_REMOTE_PENDING: return "remote-pending"; break;
        default: {
            return "";
        }
    }
}

QString presence_type_to_string(TpConnectionPresenceType presenceType) {
    switch (presenceType) {
        case TP_CONNECTION_PRESENCE_TYPE_UNSET: return "Unset"; break;
        case TP_CONNECTION_PRESENCE_TYPE_OFFLINE: return "Offline"; break;
        case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE: return "Available";  break;
        case TP_CONNECTION_PRESENCE_TYPE_AWAY: return "Away"; break;
        case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY: return "Extended Away"; break;
        case TP_CONNECTION_PRESENCE_TYPE_HIDDEN: return "Hidden"; break;
        case TP_CONNECTION_PRESENCE_TYPE_BUSY: return "Busy"; break;
        case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN: return "Unknown"; break;
        case TP_CONNECTION_PRESENCE_TYPE_ERROR: return "Error"; break;
        default: {
            return "Unknown";
        }
    }
}

QImage imageFromGdkPixbuf(gpointer buf) {
    GdkPixbuf* pixbuf = static_cast<GdkPixbuf*>(buf);
    if(!pixbuf)
        return QImage();

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    bool has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
    const guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);

    QImage::Format format;
    if (n_channels == 3 && !has_alpha) {
        format = QImage::Format_RGB888;
    } else if (n_channels == 4 && has_alpha) {
        format = QImage::Format_RGBA8888;
    } else {
        qWarning() << "Unsupported format";
        return QImage();
    }

    QImage image(pixels, width, height, rowstride, format);
    return image.copy();
}