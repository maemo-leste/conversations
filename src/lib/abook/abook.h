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
#include <telepathy-glib/connection.h>
#include <libebook/libebook.h>

#include "abook_contact.h"
#include "abook_roster.h"

#define FREE(x) free((void*)x)

namespace abookqt {
  // globals
  static bool CONV_ABOOK_INITED = false;
  static OssoABookRoster *CONV_ABOOK_ROSTER = nullptr;
  static OssoABookAggregator *CONV_ABOOK_AGGREGATOR = nullptr;
  static GtkWidget *CONV_ABOOK_DIALOG_CONTACT_CHOOSER = nullptr;
  static std::function<void(std::string)> CONV_ABOOK_DIALOG_CONTACT_CHOOSER_CB = nullptr;

  // methods
  bool init();
  void init_cb(OssoABookWaitable *waitable, const GError *error, gpointer data);
  void new_dialog_contact_chooser(const std::function<void(std::string)> &cb);
  void dialog_contact_chooser_cb(OssoABookContactChooser *chooser, gint response_id, const uintptr_t *data);

  OssoABookContact* get_tel_contact(const char* remote_uid);
  OssoABookContact* get_sip_contact(const char* remote_uid);
  OssoABookContact* get_im_contact(const char* remote_uid);

  std::string get_avatar_token(const std::string& remote_uid);
  std::string get_display_name(const std::string& remote_uid);
  AbookContactAvatar* get_avatar(const std::string& remote_uid);
  std::string get_abook_uid(const std::string& protocol, const std::string& remote_uid);
  void contacts_changed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer user_data);
  void contacts_added_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data);
  void contacts_removed_cb(OssoABookRoster *roster, const char **uids, gpointer data);
  void parse_vcard(OssoABookContact *contact);

  void notify_avatar_image_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data);

  void init_contact_roster();

  /* Utility */
  std::string presence_to_string(OssoABookPresenceState presence);
  std::string presence_type_to_string(TpConnectionPresenceType presenceType);
  std::shared_ptr<AbookContact> upsert_abook_roster_cache(OssoABookContact *contact, bool &dirty);
  bool upsert_abook_roster_avatar(OssoABookContact* contact);

  static inline bool ensure_aggregator_rdy(const char* func_name) {
    if (!CONV_ABOOK_INITED) {
#ifdef DEBUG
      const char* caller = func_name ? func_name : "unknown";
      fputs("abookqt: ", stderr);
      fputs(caller, stderr);
      fputs("(): error, aggregator not ready\n", stderr);
#endif
      return false;
    }
    return true;
  }
}