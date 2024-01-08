#ifndef CONV_RTCOM_H
#define CONV_RTCOM_H

#include <rtcom-eventlogger/eventlogger.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "models/ChatMessage.h"

RTComEl *rtcomel();

#define LOOKUP_INT(x) \
      g_value_get_int((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_BOOL(x) \
      g_value_get_boolean((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_STR(x) \
      g_value_get_string((const GValue*)g_hash_table_lookup(values, x))

RTComElQuery *rtcomStartQuery(int limit, int offset, RTComElQueryGroupBy group_by);

QList<ChatMessage *> rtcomIterateResults(RTComElQuery *query_struct);

void rtcomCreateEvent(time_t start_time, time_t end_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, bool is_outgoing, const char* protocol, const char* channel, const char* group_uid, int flags);
QList<QString> rtcomGetLocalUids();

#endif
