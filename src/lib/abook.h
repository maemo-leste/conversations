#pragma once
#undef signals

#include <functional>

/*Work around issues with signals being defined earlier */
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libosso-abook/osso-abook.h>
#include <libosso-abook/osso-abook-contact.h>
#include <telepathy-glib/account.h>
#include <libebook/libebook.h>

#include <QObject>
#include "models/ContactItem.h"

#define FREE(x) free((void*)x)
extern std::function<void()> conv_abook_func_roster_updated;

void test(const auto& remote_uid);

bool conv_abook_init();
static bool conv_abook_inited = FALSE;
static OssoABookRoster *conv_abook_roster = NULL;
static OssoABookAggregator *conv_abook_aggregator = NULL;

void contacts_changed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer user_data);
void contacts_added_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data);
void contacts_removed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data);

void notify_avatar_image_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data);

void get_contact_roster();

/* Look up user data given a phone number */
OssoABookContact* conv_abook_lookup_tel(const char* telno);
OssoABookContact* conv_abook_lookup_sip(const char* sipno);
OssoABookContact* conv_abook_lookup_im(const char* userid);

/* Utility */
QString presence_to_string(OssoABookPresenceState presence);
QString presence_type_to_string(TpConnectionPresenceType presenceType);
QImage imageFromGdkPixbuf(gpointer pixbufPointer);
QSharedPointer<ContactItem> upsert_abook_roster_cache(OssoABookContact *contact, bool &dirty);
bool upsert_abook_roster_avatar(OssoABookContact* contact);
