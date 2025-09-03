#include "abook.h"
#include "abook_public.h"
#include "../logger_std/logger_std.h"

#include "abook_roster.h"

#include <chrono>

using namespace abook_qt;

namespace abookqt {
  OssoABookContact* get_contact(const std::string& protocol, const char *remote_uid);

  bool init() {
    CLOCK_MEASURE_START(start_total);
    GError *err = nullptr;
    if (!AGGREGATOR_READY) {
      osso_abook_init_with_name("conversations", nullptr);

      CLOCK_MEASURE_START(start_aggregator_get);
      CONV_ABOOK_ROSTER = osso_abook_aggregator_get_default(&err);
      CLOCK_MEASURE_END(start_aggregator_get, "abookqt::init() osso_abook_aggregator_get_default()");

      if (err != nullptr) {
        fprintf(stderr, "Failed to get a default aggregator\n");
        g_error_free(err);
        return false;
      }

      // await our aggregator
      CONV_ABOOK_AGGREGATOR = OSSO_ABOOK_AGGREGATOR(CONV_ABOOK_ROSTER);
      osso_abook_waitable_call_when_ready(OSSO_ABOOK_WAITABLE(CONV_ABOOK_AGGREGATOR), init_cb, g_main_context_default(), NULL);
    }

    return true;
  }

  void init_cb(OssoABookWaitable *waitable, const GError *error, gpointer data) {
    g_signal_connect(CONV_ABOOK_ROSTER, "contacts-added", G_CALLBACK(contacts_added_cb), nullptr);
    g_signal_connect(CONV_ABOOK_ROSTER, "contacts-removed", G_CALLBACK(contacts_removed_cb), nullptr);
    g_signal_connect(CONV_ABOOK_ROSTER, "contacts-changed", G_CALLBACK(contacts_changed_cb), nullptr);
    g_signal_connect(CONV_ABOOK_AGGREGATOR, "contacts-added", G_CALLBACK(contacts_added_cb), nullptr);
    g_signal_connect(CONV_ABOOK_AGGREGATOR, "contacts-removed", G_CALLBACK(contacts_removed_cb), nullptr);
    g_signal_connect(CONV_ABOOK_AGGREGATOR, "contacts-changed", G_CALLBACK(contacts_changed_cb), nullptr);
    AGGREGATOR_READY = true;
    if (func_initReadySignal != nullptr)
      func_initReadySignal();  // let Qt know
  }

  bool ensure_aggregator_rdy(const char* func_name) {
    if (!AGGREGATOR_READY) {
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

  PresenceInfo get_presence(const std::string& protocol, const std::string& remote_uid) {
    if (!ensure_aggregator_rdy(__func__))
      return {};

    CLOCK_MEASURE_START(start);
    OssoABookContact* contact = get_contact(protocol, remote_uid.c_str());
    if (contact == NULL)
      return {};

    OssoABookPresence *abook_presence = OSSO_ABOOK_PRESENCE(contact);
    PresenceInfo abook_presence_info = get_presence_info(abook_presence);

    CLOCK_MEASURE_END(start, "abookqt::get_presence()");
    return abook_presence_info;
  }

  PresenceInfo get_presence_info(OssoABookPresence *abook_presence) {
    if (!ensure_aggregator_rdy(__func__))
      return {};

    const char *icon_name = osso_abook_presence_get_icon_name(abook_presence);

    const TpConnectionPresenceType presenceType = osso_abook_presence_get_presence_type(abook_presence);
    const std::string presence = presence_type_to_string(presenceType);

    PresenceInfo result;
    if (icon_name != NULL)
      result.icon_name = icon_name;

    // current presence: [detailed-name;]{available,away,...}[;custom-message]
    result.presence = presence;
    return result;
  }

  std::string get_display_name(const std::string& protocol, const std::string& remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return {};

    CLOCK_MEASURE_START(start);
    OssoABookContact* contact = get_contact(protocol, remote_uid.c_str());
    if (contact == NULL)
      return {};

    const char* name_cstr = osso_abook_contact_get_name(contact);
    CLOCK_MEASURE_END(start, "abookqt::get_display_name()");
    return name_cstr ? std::string(name_cstr) : std::string();
  }

  std::string get_avatar_token(const std::string& protocol, const std::string& remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return {};

    CLOCK_MEASURE_START(start);
    OssoABookContact* contact = get_contact(protocol, remote_uid.c_str());
    if (contact == NULL) {
      CLOCK_MEASURE_END(start, "abookqt::get_avatar_token()");
      return {};
    }

    const char* uid_cstr = osso_abook_contact_get_persistent_uid(contact);
    std::string persistent_uid = uid_cstr ? std::string(uid_cstr) : std::string();

    const auto avatar = OSSO_ABOOK_AVATAR(contact);
    if (avatar == NULL) {
      CLOCK_MEASURE_END(start, "abookqt::get_avatar_token()");
      return {};
    }

    void* avatar_token = static_cast<void*>(osso_abook_avatar_get_image_token(avatar));

    std::stringstream ss;
    ss << std::hex << reinterpret_cast<uintptr_t>(avatar_token);  // Convert pointer to hex
    std::string tokenHex = ss.str();
    CLOCK_MEASURE_END(start, "abookqt::get_avatar_token()");
    return tokenHex;
  }

  std::string get_abook_uid(const std::string& protocol, const std::string& remote_uid) {
    OssoABookContact* contact = get_contact(protocol, remote_uid.c_str());
    if (!contact) {
      fprintf(stderr, "abookqt::get_abook_uid(): could not find contact for protocol %s, remote_uid %s\n", protocol.c_str(), remote_uid.c_str());
      return {};
    }

    const char* abook_uid = osso_abook_contact_get_uid(contact);
    return {abook_uid};
  }

  AbookContactAvatar* get_avatar(const std::string& protocol, const std::string &remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    CLOCK_MEASURE_START(start);
    OssoABookContact* contact = get_contact(protocol, remote_uid.c_str());
    if (contact == NULL) {
      CLOCK_MEASURE_END(start, "abookqt::get_avatar()");
      return NULL;
    }

    const auto avatar = OSSO_ABOOK_AVATAR(contact);
    if (avatar == NULL) {
      CLOCK_MEASURE_END(start, "abookqt::get_avatar()");
      return NULL;
    }

    // avatar data is owned by abook, no need to free
    const GdkPixbuf *img = osso_abook_avatar_get_image(avatar);
    const auto rtn = new AbookContactAvatar();
    rtn->width = gdk_pixbuf_get_width(img);
    rtn->height = gdk_pixbuf_get_height(img);
    rtn->rowstride = gdk_pixbuf_get_rowstride(img);
    rtn->n_channels = gdk_pixbuf_get_n_channels(img);
    rtn->has_alpha = gdk_pixbuf_get_has_alpha(img);
    rtn->buf = gdk_pixbuf_get_pixels(img);

    int rowstride = gdk_pixbuf_get_rowstride(img);
    int height = gdk_pixbuf_get_height(img);
    rtn->buf_len = rowstride * height;

    CLOCK_MEASURE_END(start, "abookqt::get_avatar()");
    return rtn;
  }

  void contacts_changed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer user_data) {
    if (!ensure_aggregator_rdy(__func__)) return;

    CLOCK_MEASURE_START(start);
    std::vector<std::shared_ptr<AbookContact>> updated_contacts;

    while (*contacts) {
      OssoABookContact *contact = *contacts;
      contact = try_ensure_master_contact(contact);

      bool updated = false;
      auto cached_contact = update_roster_cache(contact, updated);

      if (updated) {
        updated_contacts.emplace_back(cached_contact);
      }

      contacts++;
    }

    // propagate contacts that were changed
    if (!updated_contacts.empty() && func_contactsChangedSignal != nullptr)
      func_contactsChangedSignal(updated_contacts);

    CLOCK_MEASURE_END(start, "abookqt::contacts_changed_cb()");
  }

  OssoABookContact* try_ensure_master_contact(OssoABookContact* contact) {
    if (!ensure_aggregator_rdy(__func__)) return contact;
    const bool is_roster_contact = osso_abook_contact_is_roster_contact(contact);
    if (!is_roster_contact) return contact;

    GList *it = osso_abook_aggregator_resolve_master_contacts(CONV_ABOOK_AGGREGATOR, contact);
    for (const GList *l = it; l;) {
      contact = static_cast<OssoABookContact *>(l->data);
      break;
    }

    g_list_free(it);
    return contact;
  }

  void contacts_added_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data) {
    if (!ensure_aggregator_rdy(__func__)) return;

    bool dirty = false;
    while (*contacts) {
      OssoABookContact *contact = *contacts;
      contact = try_ensure_master_contact(contact);

      update_roster_cache(contact, dirty);
      contacts++;
    }
  }

  void parse_vcard(OssoABookContact *contact) {
    if (!ensure_aggregator_rdy(__func__)) return;

    CLOCK_MEASURE_START(start);
    GList* _l;
    GList* vcard_attrs = e_vcard_get_attributes(E_VCARD(contact));
    for (_l = g_list_last(vcard_attrs); _l; _l = _l->prev) {

      EVCardAttribute *attribute = (EVCardAttribute *)vcard_attrs->data;
      std::string attr_name = e_vcard_attribute_get_name (attribute);
      printf("%s\n", attr_name.c_str());

      GList *params = e_vcard_attribute_get_params (attribute);
      for (GList *param_ptr = params; param_ptr != NULL; param_ptr = g_list_next (param_ptr)) {
        EVCardAttributeParam *param = (EVCardAttributeParam *)param_ptr->data;
        const gchar *param_name_raw = NULL;
        gchar *param_name_cased = NULL;
        std::string param_name;

        param_name_raw = e_vcard_attribute_param_get_name (param);
        param_name_cased = g_utf8_strup (param_name_raw, -1);
        param_name = param_name_cased;
        g_free (param_name_cased);
        printf("param_name: %s\n", param_name.c_str());

        if (param_name == "TYPE") {
          for (GList *type_ptr = e_vcard_attribute_param_get_values (param);
               type_ptr != NULL;
               type_ptr = g_list_next (type_ptr)) {
            const gchar *type_name_raw = NULL;
            gchar *type_name_cased = NULL;
            std::string type_name;

            type_name_raw = (const gchar *)type_ptr->data;
            type_name_cased = g_utf8_strup (type_name_raw, -1);
            type_name = type_name_cased;
            g_free (type_name_cased);

            printf("%s\n", type_name.c_str());
               }
        }
      }

      // if (!g_strcmp0(e_vcard_attribute_get_name(_l->data), "tel")) {
      //   GList *v = e_vcard_attribute_get_values(_l->data);
      // }
    }

    OSSO_ABOOK_DUMP_VCARD(VCARD, l->data, "roster");
    CLOCK_MEASURE_END(start, "abookqt::parse_vcard()");
  }

  void contacts_removed_cb(OssoABookRoster *roster, const char **uids, gpointer data) {
    if (!ensure_aggregator_rdy(__func__)) return;

    // this returns tmp osso uids, but our cache
    // may have contacts whose uids changed
    // nuke the cache

    CONTACTS_NOT_FOUND.clear();
    CONTACTS_CACHE_ABOOK_UID.clear();
    CONTACTS_CACHE_REMOTE_UID.clear();

    // for (const char **p = uids; p && *p; p++) {
    //   std::string abook_uid = *p;
    // }
  }

  void notify_avatar_image_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    if (!ensure_aggregator_rdy(__func__)) return;

    OssoABookContact *contact = OSSO_ABOOK_CONTACT(gobject);
    if (!contact)
      return;

    const std::string uid = osso_abook_contact_get_persistent_uid(contact);
    if (func_avatarChangedSignal != nullptr)
      func_avatarChangedSignal(uid);
  }

  void init_contact_roster() {
    if (!ensure_aggregator_rdy(__func__)) return;

    CLOCK_MEASURE_START(start_total);
    CLOCK_MEASURE_START(start_list_roster_contacts);
    GError *err = NULL;
    GList *contacts = osso_abook_aggregator_list_roster_contacts(CONV_ABOOK_AGGREGATOR);
    CLOCK_MEASURE_END(start_list_roster_contacts, "abookqt::init_contact_roster(): osso_abook_aggregator_list_roster_contacts");

    if (!contacts) {
      return;
    }

    CLOCK_MEASURE_START(start_await_aggr);
    osso_abook_waitable_run(OSSO_ABOOK_WAITABLE(CONV_ABOOK_AGGREGATOR),
                            g_main_context_default(), &err);
    CLOCK_MEASURE_END(start_await_aggr, "abookqt::init_contact_roster(): osso_abook_waitable_run(OSSO_ABOOK_WAITABLE(CONV_ABOOK_AGGREGATOR)()");

    if (err != NULL) {
      fprintf(stderr, "Failed to wait for the aggregator\n");
      g_list_free(contacts);
      g_error_free(err);
      return;
    }

    bool dirty = false;
    CLOCK_MEASURE_START(start_loop);
    for (GList *l = contacts; l != nullptr; l = l->next) {
      OssoABookContact *contact = OSSO_ABOOK_CONTACT(l->data);
      EContact *e_contact = E_CONTACT(contact);

      contact = try_ensure_master_contact(contact);

      g_signal_connect(contact, "notify::avatar-image", G_CALLBACK(abookqt::notify_avatar_image_cb), nullptr);

      update_roster_cache(contact, dirty);
    }
    CLOCK_MEASURE_END(start_loop, "abookqt: loop done");

    CLOCK_MEASURE_END(start_total, "abookqt: total done");
    g_list_free(contacts);
  }

  std::shared_ptr<AbookContact> update_roster_cache(OssoABookContact *contact, bool &dirty) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    std::string persistent_uid = osso_abook_contact_get_persistent_uid(contact);
    std::string abook_uid = osso_abook_contact_get_uid(contact);

    if (!CONTACTS_CACHE_ABOOK_UID.contains(abook_uid)) {
      return nullptr;
    }

    const auto ptr = CONTACTS_CACHE_ABOOK_UID[abook_uid];
    CONTACTS_NOT_FOUND.erase(ptr->remote_uid);

    const bool is_blocked = osso_abook_contact_get_blocked(contact);
    const bool can_block = osso_abook_contact_can_block(contact, NULL);
    const bool can_auth = osso_abook_contact_can_request_auth(contact, NULL);

    if (ptr->is_blocked != is_blocked) {
      ptr->is_blocked = is_blocked;
      dirty = true;
    }

    if (ptr->can_block != can_block) {
      ptr->can_block = can_block;
      dirty = true;
    }

    if (ptr->can_auth != can_auth) {
      ptr->can_auth = can_auth;
      dirty = true;
    }

    OssoABookPresence *abook_presence = OSSO_ABOOK_PRESENCE(contact);
    PresenceInfo abook_presence_info = get_presence_info(abook_presence);

    if (ptr->presence.icon_name != abook_presence_info.icon_name) {
      ptr->presence.icon_name = abook_presence_info.icon_name;
      dirty = true;
    }

    if (ptr->presence.presence != abook_presence_info.presence) {
      ptr->presence.presence = abook_presence_info.presence;
      dirty = true;
    }

    // values: yes, no, local-pending, remote-pending
    const std::string subscribed = presence_to_string(osso_abook_presence_get_subscribed(abook_presence));
    const std::string published = presence_to_string(osso_abook_presence_get_published(abook_presence));

    if (ptr->subscribed != subscribed) {
      ptr->subscribed = subscribed;
      dirty = true;
    }

    if (ptr->published != published) {
      ptr->published = published;
      dirty = true;
    }

    const char* display_name_cstr = osso_abook_contact_get_name(contact);
    const std::string display_name = display_name_cstr ? std::string(display_name_cstr) : std::string();
    if (ptr->display_name != display_name) {
      ptr->display_name = display_name;
      dirty = true;
    }

    return ptr;
  }

  void new_dialog_contact_chooser(const std::function<void(std::string)> &cb) {
    if (!AGGREGATOR_READY) {
      fprintf(stderr, "abook not initialized yet\n");
      return;
    }
    CONV_ABOOK_DIALOG_CONTACT_CHOOSER_CB = cb;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    CONV_ABOOK_DIALOG_CONTACT_CHOOSER =
      osso_abook_contact_chooser_new(GTK_WINDOW(window), g_dgettext("maemo-af-desktop", "home_ti_select_contact"));
    g_signal_connect(G_OBJECT(CONV_ABOOK_DIALOG_CONTACT_CHOOSER), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(CONV_ABOOK_DIALOG_CONTACT_CHOOSER, "response", G_CALLBACK(dialog_contact_chooser_cb), NULL);
    gtk_widget_show(CONV_ABOOK_DIALOG_CONTACT_CHOOSER);
  }

  void dialog_contact_chooser_cb(OssoABookContactChooser *chooser, const gint response_id, const uintptr_t *data) {
    if (response_id == GTK_RESPONSE_OK) {
      GList *contacts = osso_abook_contact_chooser_get_selection(chooser);

      if (contacts) {
        auto *contact = static_cast<OssoABookContact*>(contacts->data);
        std::string persistent_uid = osso_abook_contact_get_persistent_uid(contact);
        if (CONV_ABOOK_DIALOG_CONTACT_CHOOSER_CB != nullptr)
          CONV_ABOOK_DIALOG_CONTACT_CHOOSER_CB(persistent_uid);
      }

      g_list_free(contacts);
    }

    gtk_widget_destroy(CONV_ABOOK_DIALOG_CONTACT_CHOOSER);
    CONV_ABOOK_DIALOG_CONTACT_CHOOSER = nullptr;
    CONV_ABOOK_DIALOG_CONTACT_CHOOSER_CB = nullptr;
  }

  std::string presence_to_string(OssoABookPresenceState presence) {
    switch (presence) {
      case OSSO_ABOOK_PRESENCE_STATE_YES:
        return "yes";
      case OSSO_ABOOK_PRESENCE_STATE_NO:
        return "no";
      case OSSO_ABOOK_PRESENCE_STATE_LOCAL_PENDING:
        return "local-pending";
      case OSSO_ABOOK_PRESENCE_STATE_REMOTE_PENDING:
        return "remote-pending";
      default: {
        return "";
      }
    }
  }

  std::string presence_type_to_string(TpConnectionPresenceType presenceType) {
    switch (presenceType) {
      case TP_CONNECTION_PRESENCE_TYPE_UNSET:
        return "Unset";
      case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
        return "Offline";
      case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
        return "Available";
      case TP_CONNECTION_PRESENCE_TYPE_AWAY:
        return "Away";
      case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
        return "Extended Away";
      case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
        return "Hidden";
      case TP_CONNECTION_PRESENCE_TYPE_BUSY:
        return "Busy";
      case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN:
        return "Unknown";
      case TP_CONNECTION_PRESENCE_TYPE_ERROR:
        return "Error";
      default: {
        return "Unknown";
      }
    }
  }

  OssoABookContact* get_contact(const std::string& protocol, const char *remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;
    OssoABookContact* contact;

    CLOCK_MEASURE_START(start);
    if (protocol == "tel") {
      contact = get_tel_contact(remote_uid);
    } else if (protocol == "sip") {
      contact = get_sip_contact(remote_uid);
    } else {
      contact = get_im_contact(remote_uid);
    }

    if (contact)
      contact = try_ensure_master_contact(contact);

    CLOCK_MEASURE_END(start, "abookqt::get_contact()");

    return contact;
  }

  OssoABookContact* get_sip_contact(const char *address) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    OssoABookContact *res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_sip_address(CONV_ABOOK_AGGREGATOR, address);

    GList *v = l;
    while (v) {
      OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
      res = contact;
      break;
    }

    g_list_free(l);
    return res;
  }

  OssoABookContact* get_im_contact(const char* remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    OssoABookContact *res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_im_contact(CONV_ABOOK_AGGREGATOR, remote_uid, NULL);

    const GList *v = l;
    while (v) {
      OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
      if (contact) {
        res = contact;
        break;
      }
      v = v->next;
    }

    g_list_free(l);
    return res;
  }

  OssoABookContact* get_tel_contact(const char *remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    OssoABookContact *res = NULL;
    GList *l = NULL;

    l = osso_abook_aggregator_find_contacts_for_phone_number(CONV_ABOOK_AGGREGATOR, remote_uid, TRUE);

    GList *v = l;
    while (v) {
      OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
      res = contact;
      break;
    }

    g_list_free(l);
    return res;
  }
}