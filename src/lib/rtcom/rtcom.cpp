#include "rtcom.h"

namespace qtrtcom {
  RTComEl *el = NULL;

  RTComEl* rtcomel() {
    if (!el)
      el = rtcom_el_new();
    return el;
  }

  std::string get_room_name(const char* group_uid) {
    gchar* title = rtcom_el_plugin_chat_get_group_title(rtcomel(), g_strdup(group_uid));
    if (!title) return {};
    return {title};
  }

  std::vector<std::string> get_service_accounts() {
    std::vector<std::string> rtn;
    RTComElQuery *query = startQuery(0, 0, RTCOM_EL_QUERY_GROUP_BY_EVENTS_LOCAL_UID);
    if(!rtcom_el_query_prepare(query, NULL)) {
      rtcom_log("Could not prepare query", true);
      g_object_unref(query);
    } else {
      auto items = iterateRtComEvents(query);
      for(const auto &item: items) {
        rtn.emplace_back(item->local_uid);
        delete item;
      }
      g_object_unref(query);
    }
    return rtn;
  }

  std::vector<rtcom_qt::ChatMessageEntry*> get_messages(const char* service_id_cstr, const char* group_uid_cstr, unsigned int limit, unsigned int offset) {
    RTComElQuery *query = startQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_NONE);
    const gint sid = rtcom_el_get_service_id(rtcomel(), service_id_cstr);

    if(!rtcom_el_query_prepare(
        query,
        "group-uid", group_uid_cstr, RTCOM_EL_OP_EQUAL,
        "service-id", sid, RTCOM_EL_OP_EQUAL,
        NULL)) {
      rtcom_log("Could not prepare query", true);
      g_object_unref(query);
      return {};
    }

    auto results = iterateRtComEvents(query);

    g_object_unref(query);

    return results;
  }

  std::vector<rtcom_qt::ChatMessageEntry*> get_overview_messages(unsigned int limit, unsigned int offset) {
    RTComElQuery *query = startQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_GROUP);
    bool query_prepared = FALSE;

    const gint service_id = rtcom_el_get_service_id(rtcomel(), "RTCOM_EL_SERVICE_CALL");
    if (!service_id) {
      rtcom_log("Could not prepare query", true);
      g_object_unref(query);
      return {};
    }

    query_prepared = rtcom_el_query_prepare(query, "service-id", service_id, RTCOM_EL_OP_NOT_EQUAL,  NULL);
    if(!query_prepared) {
      rtcom_log("Could not prepare query", true);
      g_object_unref(query);
      return {};
    }

    auto results = iterateRtComEvents(query);

    g_object_unref(query);
    return results;
  }

  std::vector<rtcom_qt::ChatMessageEntry*> search_messages(
      const std::string& search,
      const std::string& group_uid,
      const unsigned int limit,
      const unsigned int offset) {
    RTComElQuery *query = startQuery(limit, offset, RTCOM_EL_QUERY_GROUP_BY_NONE);
    // gint rtcom_sms_service_id = rtcom_el_get_service_id(rtcomel(), "RTCOM_EL_SERVICE_SMS");
    bool query_prepared = FALSE;

    if(group_uid.empty()) {
      query_prepared = rtcom_el_query_prepare(
        query,
        "free-text", search.c_str(), RTCOM_EL_OP_STR_LIKE,
  //      "service-id", rtcom_sms_service_id, RTCOM_EL_OP_EQUAL,
        NULL);
    } else {
      query_prepared = rtcom_el_query_prepare(
        query,
        "free-text", search.c_str(), RTCOM_EL_OP_STR_LIKE,
        "group-uid", group_uid.c_str(), RTCOM_EL_OP_EQUAL,
  //      "service-id", rtcom_sms_service_id, RTCOM_EL_OP_EQUAL,
        NULL);
    }

    if(!query_prepared) {
      rtcom_log("Couldn't prepare query", true);
      g_object_unref(query);
      return {};
    }

    std::vector<rtcom_qt::ChatMessageEntry*> results = iterateRtComEvents(query);
    g_object_unref(query);

    return results;
  }

  std::vector<rtcom_qt::ChatMessageEntry*> iterateRtComEvents(RTComElQuery *query) {
    std::vector<rtcom_qt::ChatMessageEntry*> results;
    RTComElIter *it = rtcom_el_get_events(rtcomel(), query);

    if(it && rtcom_el_iter_first(it)) {
      do {
        GHashTable *values = NULL;

        values = rtcom_el_iter_get_value_map(
            it,
            "id",
            "service",
            "group-uid",
            "local-uid",
            "remote-uid",
            "remote-name",
            "remote-ebook-uid",
            "content",
            "icon-name",
            "start-time",
            "event-count",
            "group-title",
            "channel",
            "event-type",
            "outgoing",
            "is-read",
            "flags",
            NULL);

        auto *item = new rtcom_qt::ChatMessageEntry(
          LOOKUP_INT("id"),
          LOOKUP_STR("service"),
          LOOKUP_STR("group-uid"),
          LOOKUP_STR("local-uid"),
          "",
          LOOKUP_STR("remote-uid"),
          LOOKUP_STR("remote-name"),
          LOOKUP_STR("remote-ebook-uid"),
          LOOKUP_STR("content"),
          LOOKUP_STR("icon-name"),
          LOOKUP_INT("start-time"),
          LOOKUP_INT("event-count"),
          LOOKUP_STR("group-title"),
          LOOKUP_STR("channel"),
          LOOKUP_STR("event-type"),
          LOOKUP_BOOL("outgoing"),
          LOOKUP_BOOL("is-read"),
          LOOKUP_INT("flags")
        );

        g_hash_table_destroy(values);
        results.emplace_back(item);
      } while (rtcom_el_iter_next(it));

      g_object_unref(it);
    } else {
      rtcom_log("Failed to init iterator to start", false);
    }

    return results;
  }

  void set_room_name(const char* group_uid, const char* title) {
#ifdef DEBUG
    rtcom_log(group_uid + " set_room_name: " + title, false);
#endif
    const auto el = rtcomel();
    rtcom_el_plugin_chat_set_group_title(el, g_strdup(group_uid), g_strdup(title));
  }

  RTComElQuery* startQuery(const unsigned int limit, const unsigned int offset, const RTComElQueryGroupBy group_by) {
    RTComElQuery* query = rtcom_el_query_new(rtcomel());

    if(group_by != RTCOM_EL_QUERY_GROUP_BY_NONE)
      rtcom_el_query_set_group_by(query, group_by);

    if(limit > 0)
      rtcom_el_query_set_limit(query, limit);
    if(offset > 0)
      rtcom_el_query_set_offset(query, offset);

    return query;
  }

  bool delete_events(const char* group_uid) {
    RTComElQuery *query = startQuery(0, 0, RTCOM_EL_QUERY_GROUP_BY_GROUP);

    if(!rtcom_el_query_prepare(
        query,
        "group-uid", group_uid, RTCOM_EL_OP_EQUAL,
        NULL)) {
      rtcom_log("Could not prepare query", true);
      g_object_unref(query);
      return false;
    }

    if(!rtcom_el_delete_events(el, query, NULL))
      rtcom_log( "Couldn't DELETE by group_uid", true);

    g_object_unref(query);
    return true;
  }

  void set_read(const unsigned int event_id, const gboolean read) {
    rtcom_log("set_read, event_id " + std::to_string(event_id), false);
    /* Ignore error for now by setting GError error to NULL */
    rtcom_el_set_read_event(rtcomel(), event_id, read, NULL);
  }

  rtcom_qt::ChatMessageEntry* register_chat_join(time_t start_time, time_t end_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, const char* protocol, const char* channel, const char* group_uid) {
    const auto rtcom_service = protocol_to_rtcom_service_id(protocol);
    const char* rtcom_service_cstr = rtcom_service.c_str();
    auto *ev = _defaultEvent(start_time, end_time, backend_name, remote_uid, text, protocol, channel, false, group_uid, rtcom_service_cstr);
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_JOIN"));
    RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup(abook_uid));
    GError *err = NULL;

    const gint event_id = rtcom_el_add_event(rtcomel(), ev, &err);
    if(event_id < 0)
      rtcom_log("rtcom_el_add_event failed: " + std::string(err->message), true);

    RTComElQuery* query = rtcom_el_query_new(rtcomel());
    if(!rtcom_el_query_prepare(query, "id", event_id, RTCOM_EL_OP_EQUAL, NULL)) {
      rtcom_log("Could not prepare query", true);
      return {};
    }

    const std::vector<rtcom_qt::ChatMessageEntry*> results = iterateRtComEvents(query);
    if (results.empty()) {
      g_object_unref(query);
      rtcom_log("Could not iterate events", true);
      return {};
    }

    rtcom_el_event_free_contents(ev);
    rtcom_el_event_free(ev);
    g_object_unref(query);

    return results[0];
  }
  
  rtcom_qt::ChatMessageEntry* register_chat_leave(time_t start_time, time_t end_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, const char* protocol, const char* channel, const char* group_uid) {
    const auto rtcom_service = protocol_to_rtcom_service_id(protocol);
    const char* rtcom_service_cstr = rtcom_service.c_str();
    auto *ev = _defaultEvent(start_time, end_time, backend_name, remote_uid, text, protocol, channel, false, group_uid, rtcom_service_cstr);
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_LEAVE"));
    RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup(abook_uid));
    GError *err = NULL;

    const gint event_id = rtcom_el_add_event(rtcomel(), ev, &err);
    if(event_id < 0) {
      rtcom_log("rtcom_el_add_event failed: " + std::string(err->message), true);
      return {};
    }

    RTComElQuery* query = rtcom_el_query_new(rtcomel());
    if(!rtcom_el_query_prepare(query, "id", event_id, RTCOM_EL_OP_EQUAL, NULL)) {
      g_object_unref(query);
      rtcom_log("Could not prepare query", true);
      return {};
    }

    const std::vector<rtcom_qt::ChatMessageEntry*> results = iterateRtComEvents(query);
    if (results.empty()) {
      g_object_unref(query);
      rtcom_log("Could not iterate events", true);
      return {};
    }

    rtcom_el_event_free_contents(ev);
    rtcom_el_event_free(ev);
    g_object_unref(query);

    return results[0];
  }

  rtcom_qt::ChatMessageEntry* register_message(
      time_t start_time, time_t end_time, const char* self_name, const char* backend_name,
      const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, bool is_outgoing, const char* protocol, const char* channel, const char* group_uid) {

    const auto rtcom_service = protocol_to_rtcom_service_id(protocol);
    const char* rtcom_service_cstr = rtcom_service.c_str();
    channel = NULL;
    auto *ev = _defaultEvent(start_time, end_time, backend_name, remote_uid, text, protocol, channel, is_outgoing, group_uid, rtcom_service_cstr);
    if (protocol == "sms" || protocol == "tel" || protocol == "ofono") {
      RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_SMS_MESSAGE"));
    } else {
      RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"));
    }

    RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup(abook_uid));
    const auto el = rtcomel();
    GError *err = NULL;
    const gint event_id = rtcom_el_add_event(el, ev, &err);
    if(event_id < 0) {
      rtcom_log("rtcom_el_add_event failed: " + std::string(err->message), true);
      rtcom_el_event_free_contents(ev);
      rtcom_el_event_free(ev);
      return nullptr;
    }

    RTComElQuery* query = rtcom_el_query_new(rtcomel());
    if(!rtcom_el_query_prepare(query, "id", event_id, RTCOM_EL_OP_EQUAL, NULL)) {
      rtcom_log("Could not prepare query", true);
      return {};
    }

    const std::vector<rtcom_qt::ChatMessageEntry*> results = iterateRtComEvents(query);
    if (results.empty()) {
      g_object_unref(query);
      rtcom_log("Could not iterate events", true);
      return {};
    }

    rtcom_el_event_free_contents(ev);
    rtcom_el_event_free(ev);
    g_object_unref(query);

    return results[0];
  }

  RTComElEvent* _defaultEvent(
      time_t start_time, time_t end_time, const char* local_uid, const char *remote_uid,
      const char* text, const char* protocol, const char* channel, bool is_outgoing,
      const char* group_uid, const char* rtcom_service) {
    RTComElEvent *ev = rtcom_el_event_new();

    RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup(rtcom_service));

    if (channel)
      RTCOM_EL_EVENT_SET_FIELD (ev, channel, g_strdup(channel));
    if (group_uid)
      RTCOM_EL_EVENT_SET_FIELD (ev, group_uid, g_strdup(group_uid));

    RTCOM_EL_EVENT_SET_FIELD(ev, storage_time, std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    RTCOM_EL_EVENT_SET_FIELD(ev, start_time, start_time);
    RTCOM_EL_EVENT_SET_FIELD(ev, end_time, end_time);
    RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strdup(local_uid));
    RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup(remote_uid));
    RTCOM_EL_EVENT_SET_FIELD (ev, free_text, g_strdup(text));
    RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, is_outgoing);
    RTCOM_EL_EVENT_SET_FIELD (ev, flags, 0);  // @TODO: flags
    return ev;
  }

  std::string protocol_to_rtcom_service_id(const std::string& protocol) {
    if (protocol == "sms" || protocol == "tel" || protocol == "ofono")
      return "RTCOM_EL_SERVICE_SMS";
    return "RTCOM_EL_SERVICE_CHAT";
  }

  void rtcom_log(const std::string &msg, const bool error) {
    const auto stream = error ? stderr : stdout;
    fputs(msg.c_str(), stream);
    fputc('\n', stream);
    fflush(stream);
  }
}