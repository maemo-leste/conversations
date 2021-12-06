#include <rtcom-eventlogger/eventlogger.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "models/ChatMessage.h"

struct rtcom_query {
    RTComElQuery *query = NULL;
    RTComElIter *it = NULL;
    RTComEl *el = NULL;
};

#define LOOKUP_INT(x) \
      g_value_get_int((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_BOOL(x) \
      g_value_get_boolean((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_STR(x) \
      g_value_get_string((const GValue*)g_hash_table_lookup(values, x))

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
