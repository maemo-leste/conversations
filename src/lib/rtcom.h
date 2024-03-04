#pragma once

#include <rtcom-eventlogger/eventlogger.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "lib/utils.h"
#include "models/ChatMessage.h"

namespace qtrtcom {
    RTComEl *rtcomel();

#define LOOKUP_INT(x) \
      g_value_get_int((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_BOOL(x) \
      g_value_get_boolean((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_STR(x) \
      g_value_get_string((const GValue*)g_hash_table_lookup(values, x))

    RTComElQuery *startQuery(int limit, int offset, RTComElQueryGroupBy group_by);

    QList<ChatMessage *> iterateResults(RTComElQuery *query_struct);

    RTComElEvent * _defaultEvent(time_t start_time, time_t end_time, const char *local_uid, const char *remote_uid,
                                 const char *text, const char* protocol, const char *channel, bool is_outgoing, const char *group_uid);

    void registerMessage(time_t start_time, time_t end_time, const char *self_name, const char *backend_name,
                          const char *remote_uid, const char *remote_name, const char *abook_uid, const char *text,
                          bool is_outgoing, const char *protocol, const char *channel, const char *group_uid);

    QList<QString> localUIDs();

    void registerChatJoin(time_t start_time, time_t end_time, const char *self_name, const char *backend_name,
                          const char *remote_uid, const char *remote_name, const char *abook_uid, const char *text,
                          const char *protocol, const char *channel, const char *group_uid);

    void registerChatLeave(time_t start_time, time_t end_time, const char *self_name, const char *backend_name,
                          const char *remote_uid, const char *remote_name, const char *abook_uid, const char *text,
                          const char *protocol, const char *channel, const char *group_uid);

    void setFlag(const int event_id, const gchar* flag_value);
}