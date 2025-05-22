#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <functional>

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libosso-abook/osso-abook.h>
#include <libosso-abook/osso-abook-contact.h>
#include <telepathy-glib/account.h>
#include <libebook/libebook.h>

#include "abook_contact.h"
#include "abook_roster.h"

#define FREE(x) free((void*)x)

namespace abookqt {
  // globals
  static bool CONV_ABOOK_INITED = false;
  static OssoABookRoster *CONV_ABOOK_ROSTER = nullptr;
  static OssoABookAggregator *CONV_ABOOK_AGGREGATOR = nullptr;

  // methods
  bool init();

  OssoABookContact* get_tel_contact(const char* telno);
  OssoABookContact* get_sip_contact(const char* sipno);
  OssoABookContact* get_im_contact(const char* local_uid, const char* userid);

  std::string get_avatar_token(const std::string& local_uid, const std::string& remote_uid);
  std::string get_display_name(const std::string& local_uid, const std::string& remote_uid);
  AbookContactAvatar* get_avatar(const std::string& local_uid, const std::string& remote_uid);
  void contacts_changed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer user_data);
  void contacts_added_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data);
  void contacts_removed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data);
  void parse_vcard(OssoABookContact *contact);

  void notify_avatar_image_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data);

  void init_contact_roster();

  /* Utility */
  std::string presence_to_string(OssoABookPresenceState presence);
  std::string presence_type_to_string(TpConnectionPresenceType presenceType);
  std::shared_ptr<AbookContact> upsert_abook_roster_cache(OssoABookContact *contact, bool &dirty);
  bool upsert_abook_roster_avatar(OssoABookContact* contact);
}