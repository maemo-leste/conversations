#include "abook.h"
#include "abook_public.h"
#include "../logger_std/logger_std.h"

#include <chrono>

using namespace abook_qt;

namespace abookqt {
  bool init() {
    CLOCK_MEASURE_START(start_total);
    GError *err = nullptr;
    if (!CONV_ABOOK_INITED) {
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
    CONV_ABOOK_INITED = true;
    if (func_initReadySignal != nullptr)
      func_initReadySignal();  // let Qt know
  }

  OssoABookContact* get_sip_contact(const char *address) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    CLOCK_MEASURE_START(start);
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
    CLOCK_MEASURE_END(start, "abookqt::get_sip_contact()");
    return res;
  }

  OssoABookContact* get_im_contact(const char* local_uid, const char* userid) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    CLOCK_MEASURE_START(start);
    OssoABookContact *res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_im_contact(CONV_ABOOK_AGGREGATOR, userid, NULL);

    const GList *v = l;
    while (v) {
      OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);

      std::string persistent_uid = osso_abook_contact_get_persistent_uid(contact);
      if (std::string(local_uid) + "-" + std::string(userid) == persistent_uid) {
        res = contact;
        break;
      }

      v = v->next;
    }

    g_list_free(l);
    CLOCK_MEASURE_END(start, "abookqt::get_im_contact()");
    return res;
  }

  OssoABookContact* get_tel_contact(const char *telno) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    CLOCK_MEASURE_START(start);
    OssoABookContact *res = NULL;
    GList *l = NULL;

    l = osso_abook_aggregator_find_contacts_for_phone_number(CONV_ABOOK_AGGREGATOR, telno, TRUE);

    GList *v = l;
    while (v) {
      OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
      res = contact;
      break;
    }

    g_list_free(l);
    CLOCK_MEASURE_END(start, "abookqt::get_tel_contact()");
    return res;
  }

  std::string get_display_name(const std::string& local_uid, const std::string& remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return {};

    CLOCK_MEASURE_START(start);
    OssoABookContact* contact = get_im_contact(local_uid.c_str(), remote_uid.c_str());
    if (contact == NULL)
      return {};

    const char* name_cstr = osso_abook_contact_get_name(contact);
    CLOCK_MEASURE_END(start, "abookqt::get_display_name()");
    return name_cstr ? std::string(name_cstr) : std::string();
  }

  std::string get_avatar_token(const std::string& local_uid, const std::string& remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return {};

    CLOCK_MEASURE_START(start);
    OssoABookContact* contact = get_im_contact(local_uid.c_str(), remote_uid.c_str());
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

  AbookContactAvatar* get_avatar(const std::string& local_uid, const std::string &remote_uid) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    CLOCK_MEASURE_START(start);
    OssoABookContact* contact = get_im_contact(local_uid.c_str(), remote_uid.c_str());
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

    // CLOCK_MEASURE_START(start);
    std::map<std::string, std::shared_ptr<AbookContact>> updated_contacts;

    while (*contacts) {
      OssoABookContact *contact = *contacts;

      bool updated = false;
      GList *rc = osso_abook_contact_get_roster_contacts(contact);
      std::string persistent_uid = osso_abook_contact_get_persistent_uid(contact);

      for (const GList *l = rc; l; l = l->next) {
        OssoABookPresence *abook_presence = OSSO_ABOOK_PRESENCE(l->data);
        const TpConnectionPresenceType presenceType = osso_abook_presence_get_presence_type(abook_presence);
        const OssoABookPresenceState published = osso_abook_presence_get_published(abook_presence);
        const OssoABookPresenceState subscribed = osso_abook_presence_get_subscribed(abook_presence);
        const char* display_name_cstr = osso_abook_contact_get_name(contact);
        std::string display_name = display_name_cstr ? std::string(display_name_cstr) : std::string();

        if (!ROSTER.contains(persistent_uid))
          ROSTER[persistent_uid] = std::make_shared<AbookContact>(persistent_uid);

        if (ROSTER[persistent_uid]->display_name != display_name) {
          ROSTER[persistent_uid]->display_name = display_name;
          updated = true;
        }

        if (ROSTER[persistent_uid]->published != presence_to_string(published)) {
          ROSTER[persistent_uid]->published = presence_to_string(published);
          updated = true;
        }

        if (ROSTER[persistent_uid]->subscribed != presence_to_string(subscribed)) {
          ROSTER[persistent_uid]->subscribed = presence_to_string(subscribed);
          updated = true;
        }

        if (ROSTER[persistent_uid]->presence != presence_type_to_string(presenceType)) {
          ROSTER[persistent_uid]->presence = presence_type_to_string(presenceType);
          updated = true;
        }

        if (updated) {
          updated_contacts[persistent_uid] = ROSTER[persistent_uid];
        }

        break;
      }

      g_list_free(rc);
      contacts++;
    }

    // propagate contacts that were changed
    if (!updated_contacts.empty() && func_contactsChangedSignal != nullptr)
      func_contactsChangedSignal(updated_contacts);

    // CLOCK_MEASURE_END(start, "abookqt::contacts_changed_cb()");
  }

  void contacts_added_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data) {
    if (!ensure_aggregator_rdy(__func__)) return;

    bool dirty = false;
    while (*contacts) {
      OssoABookContact *contact = *contacts;
      upsert_abook_roster_cache(contact, dirty);
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

  void contacts_removed_cb(OssoABookRoster *roster, OssoABookContact **contacts, gpointer data) {
    if (!ensure_aggregator_rdy(__func__)) return;

    while (*contacts) {
      OssoABookContact *contact = *contacts;
      if (!contact) {
        fprintf(stderr, "contacts_removed_cb: null contact\n");
        break;
      }

      std::string persistent_uid = osso_abook_contact_get_persistent_uid(contact);
      if (!persistent_uid.empty())
        ROSTER.erase(persistent_uid);

      contacts++;
    }
  }

  void notify_avatar_image_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    if (!ensure_aggregator_rdy(__func__)) return;

    OssoABookContact *contact = OSSO_ABOOK_CONTACT(gobject);
    if (!contact)
      return;

    std::string persistent_uid = osso_abook_contact_get_persistent_uid(contact);
    const size_t pos = persistent_uid.rfind('-');
    auto local_uid = persistent_uid.substr(0, pos);
    auto remote_uid = persistent_uid.substr(pos + 1);

    if (func_avatarChangedSignal != nullptr)
      func_avatarChangedSignal(local_uid, remote_uid);
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

      // @TODO: connect avatar change
      g_signal_connect(contact, "notify::avatar-image", G_CALLBACK(abookqt::notify_avatar_image_cb), nullptr);

      upsert_abook_roster_cache(contact, dirty);

      // avatar updated
      // if(upsert_abook_roster_avatar(contact))
      //     emit contact_item->avatarChanged();
    }
    CLOCK_MEASURE_END(start_loop, "abookqt: loop done");

    CLOCK_MEASURE_END(start_total, "abookqt: total done");
    g_list_free(contacts);

    // if (dirty)
    //   conv_abook_func_roster_updated();
  }

  std::shared_ptr<AbookContact> upsert_abook_roster_cache(OssoABookContact *contact, bool &dirty) {
    if (!ensure_aggregator_rdy(__func__)) return nullptr;

    std::string persistent_uid = osso_abook_contact_get_persistent_uid(contact);

    const bool is_blocked = osso_abook_contact_get_blocked(contact);
    const bool can_block = osso_abook_contact_can_block(contact, NULL);
    const bool can_auth = osso_abook_contact_can_request_auth(contact, NULL);

    OssoABookPresence *abook_presence = OSSO_ABOOK_PRESENCE(contact);
    const TpConnectionPresenceType presenceType = osso_abook_presence_get_presence_type(abook_presence);

    // current presence: [detailed-name;]{available,away,...}[;custom-message]
    // @TODO: this gives warning: 'e_vcard_attribute_get_value called on multivalued attribute'
    const std::string presence = presence_type_to_string(presenceType);

    // values: yes, no, local-pending, remote-pending
    const std::string subscribed = presence_to_string(osso_abook_presence_get_subscribed(abook_presence));
    const std::string published = presence_to_string(osso_abook_presence_get_published(abook_presence));

    if (!ROSTER.contains(persistent_uid))
      ROSTER[persistent_uid] = std::make_shared<AbookContact>(persistent_uid);

    const char* display_name_cstr = osso_abook_contact_get_name(contact);
    std::string display_name = display_name_cstr ? std::string(display_name_cstr) : std::string();

    ROSTER[persistent_uid]->display_name = display_name;
    ROSTER[persistent_uid]->published = published;
    ROSTER[persistent_uid]->subscribed = subscribed;
    ROSTER[persistent_uid]->presence = presence;
    ROSTER[persistent_uid]->is_blocked = is_blocked;
    ROSTER[persistent_uid]->can_block = can_block;
    ROSTER[persistent_uid]->can_auth = can_auth;
    return ROSTER[persistent_uid];
  }

  void new_dialog_contact_chooser(const std::function<void(std::string)> &cb) {
    if (!CONV_ABOOK_INITED) {
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
      case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_YES:
        return "yes";
      case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_NO:
        return "no";
      case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_LOCAL_PENDING:
        return "local-pending";
      case OssoABookPresenceState::OSSO_ABOOK_PRESENCE_STATE_REMOTE_PENDING:
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
}