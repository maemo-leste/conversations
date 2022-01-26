#include "lib/rtcom.h"

rtcom_query* rtcomStartQuery(const int limit, const int offset, const RTComElQueryGroupBy group_by) {
  RTComElQuery *query = NULL;
  RTComElIter *it = NULL;
  RTComEl *el = NULL;

  el = rtcom_el_new();
  query = rtcom_el_query_new(el);

  if(group_by != RTCOM_EL_QUERY_GROUP_BY_NONE)
    rtcom_el_query_set_group_by(query, group_by);

  rtcom_el_query_set_limit(query, limit);
  rtcom_el_query_set_offset(query, offset);

  return new rtcom_query{query, it , el};
}

QList<ChatMessage*> rtcomIterateResults(rtcom_query *query_struct) {
  QList<ChatMessage*> results;
  query_struct->it = rtcom_el_get_events(query_struct->el, query_struct->query);

  if(query_struct->it && rtcom_el_iter_first(query_struct->it)) {
    do {
      GHashTable *values = NULL;
      values = rtcom_el_iter_get_value_map(
          query_struct->it,
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
          LOOKUP_STR("event-type"),
          LOOKUP_BOOL("outgoing"),
          LOOKUP_INT("flags"));
      results << item;
    } while (rtcom_el_iter_next(query_struct->it));
    g_object_unref(query_struct->it);
  } else {
    qCritical() << "Failed to init iterator to start";
  }

  g_object_unref(query_struct->query);
  return results;
}


void create_event(time_t start_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* text, bool is_outgoing, bool is_sms) {
  qDebug() << "create_event";

  RTComElEvent *ev = rtcom_el_event_new();

  if (is_sms) {
    RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup("RTCOM_EL_SERVICE_SMS"));
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_SMS_MESSAGE"));
  } else {
    RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup("RTCOM_EL_SERVICE_CHAT"));
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"));
  }

  RTCOM_EL_EVENT_SET_FIELD(ev, start_time, start_time);
  RTCOM_EL_EVENT_SET_FIELD(ev, end_time, 0);
  RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strdup(backend_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup(remote_uid));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
  RTCOM_EL_EVENT_SET_FIELD (ev, free_text, g_strdup(text));

  RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, is_outgoing);

  if(rtcom_el_add_event(evlog, ev, NULL) < 0) {
    qDebug() << "Failed to add event to RTCom";
    // todo: log failure
    int wegw = 1;
  } else {
    qDebug() << "create_event SUCCESS";
  }
}