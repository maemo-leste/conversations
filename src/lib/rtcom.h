#ifndef CONV_RTCOM_H
#define CONV_RTCOM_H

#include <rtcom-eventlogger/eventlogger.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "models/ChatMessage.h"

static RTComEl *evlog;
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

rtcom_query* rtcomStartQuery(int limit, int offset, RTComElQueryGroupBy group_by);
QList<ChatMessage*> rtcomIterateResults(rtcom_query *query_struct);
void create_event(time_t start_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* text, bool is_outgoing, bool is_sms);

#endif