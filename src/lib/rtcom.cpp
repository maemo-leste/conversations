#include "lib/rtcom.h"

RTComEl *rtcomel() {
  static RTComEl *el = NULL;

  if (!el)
    el = rtcom_el_new();

  return el;
}

RTComElQuery *rtcomStartQuery(const int limit, const int offset, const RTComElQueryGroupBy group_by) {
  RTComElQuery *query = rtcom_el_query_new(rtcomel());

  if(group_by != RTCOM_EL_QUERY_GROUP_BY_NONE)
    rtcom_el_query_set_group_by(query, group_by);

  if(limit > 0)
    rtcom_el_query_set_limit(query, limit);
  if(offset > 0)
    rtcom_el_query_set_offset(query, offset);

  return query;
}

QList<ChatMessage*> rtcomIterateResults(RTComElQuery *query) {
  QList<ChatMessage *> results;
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
          "flags",
          NULL);

      auto *item = new ChatMessage(
          LOOKUP_INT("id"),
          LOOKUP_STR("service"),
          LOOKUP_STR("group-uid"),
          LOOKUP_STR("local-uid"),
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
          LOOKUP_INT("flags"));

      g_hash_table_destroy(values);
      results << item;
    } while (rtcom_el_iter_next(it));

    g_object_unref(it);
  } else {
    qCritical() << "Failed to init iterator to start";
  }

  return results;
}

void rtcomCreateEvent(time_t start_time, time_t end_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, bool is_outgoing, const char* protocol, const char* channel, const char* group_uid, int flags) {
  qDebug() << "create_event";

  RTComElEvent *ev = rtcom_el_event_new();

  if (strcmp(protocol, "sms") == 0 || strcmp(protocol, "tel") == 0 || strcmp(protocol, "ofono") == 0) {
    RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup("RTCOM_EL_SERVICE_SMS"));
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_SMS_MESSAGE"));
  } else {
    RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup("RTCOM_EL_SERVICE_CHAT"));
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"));
  }

  RTCOM_EL_EVENT_SET_FIELD(ev, storage_time, std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

  RTCOM_EL_EVENT_SET_FIELD(ev, start_time, start_time);
  RTCOM_EL_EVENT_SET_FIELD(ev, end_time, end_time);
  RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strdup(backend_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup(remote_uid));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
  RTCOM_EL_EVENT_SET_FIELD (ev, free_text, g_strdup(text));
  RTCOM_EL_EVENT_SET_FIELD (ev, flags, flags);

  RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup(abook_uid));

  if (channel) {
    RTCOM_EL_EVENT_SET_FIELD (ev, channel, g_strdup(channel));
  }

  if (group_uid) {
    RTCOM_EL_EVENT_SET_FIELD (ev, group_uid, g_strdup(group_uid));
  }

  RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, is_outgoing);

  if(rtcom_el_add_event(rtcomel(), ev, NULL) < 0) {
    qDebug() << "Failed to add event to RTCom";
    // todo: log failure
    int wegw = 1;
  } else {
    qDebug() << "create_event SUCCESS";
  }
}

QList<QString> rtcomGetLocalUids() {
  QList<QString> protocols;
  RTComElQuery *query = rtcomStartQuery(0, 0, RTCOM_EL_QUERY_GROUP_BY_EVENTS_LOCAL_UID);
  if(!rtcom_el_query_prepare(query, NULL)) {
    qCritical() << __FUNCTION__ << "Could not prepare query";
    g_object_unref(query);
    return protocols;
  }

  auto items = rtcomIterateResults(query);
  for(auto &item: items) {
    auto local_uid = item->local_uid();
    if(local_uid.count("/") != 2) continue;
    auto protocol = local_uid.split("/").at(1);
    protocols << protocol;
  }
  qDeleteAll(items);

  g_object_unref(query);

  return protocols;
}
