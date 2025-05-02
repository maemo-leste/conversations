#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <rtcom-eventlogger/eventlogger.h>
#include "rtcom-eventlogger/eventlogger-query.h"

extern "C" {
#include <rtcom-eventlogger-plugins/chat.h>
}

#include <glib.h>
#include <glib/gstdio.h>

#include "rtcom_models.h"

// #include "lib/config.h"
// #include "lib/utils.h"

#define LOOKUP_INT(x) \
g_value_get_int((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_BOOL(x) \
g_value_get_boolean((const GValue*)g_hash_table_lookup(values, x))
// #define LOOKUP_STR(x) g_value_get_string((const GValue*)g_hash_table_lookup(values, x))
#define LOOKUP_STR(x) std::string(g_value_get_string((const GValue*)g_hash_table_lookup(values, x)) ? g_value_get_string((const GValue*)g_hash_table_lookup(values, x)) : "")

namespace qtrtcom {
  extern RTComEl *el;
  RTComEl *rtcomel();
  //
  std::vector<std::string> get_service_accounts();
  std::string              get_room_name(const char* group_uid);
  void                     set_room_name(const char* group_uid, const char* title);
  bool                     delete_events(const char* group_uid);

  std::vector<rtcom_qt::ChatMessageEntry*> get_messages(const char* service_id_cstr, const char* group_uid_cstr, unsigned int limit, unsigned int offset);
  std::vector<rtcom_qt::ChatMessageEntry*> get_overview_messages(unsigned int limit, unsigned int offset);

  std::vector<rtcom_qt::ChatMessageEntry*> search_messages(const std::string& search, const std::string& group_uid, unsigned int limit = 20, unsigned int offset = 0);

  RTComElQuery* startQuery(const unsigned int limit, const unsigned int offset, RTComElQueryGroupBy group_by);

  RTComElEvent * _defaultEvent(time_t start_time, time_t end_time, const char *local_uid, const char *remote_uid,
                               const char *text, const char* protocol, const char *channel, bool is_outgoing, const char *group_uid, const char* rtcom_service,
                               unsigned int flags = 0);

  rtcom_qt::ChatMessageEntry* register_message(time_t start_time, time_t end_time, const char *self_name, const char *backend_name,
                        const char *remote_uid, const char *remote_name, const char *abook_uid, const char *text,
                        bool is_outgoing, const char *protocol, const char *channel, const char *group_uid, unsigned int flags);

  rtcom_qt::ChatMessageEntry* register_chat_join(time_t start_time, time_t end_time, const char *self_name, const char *backend_name,
                        const char *remote_uid, const char *remote_name, const char *abook_uid, const char *text,
                        const char *protocol, const char *channel, const char *group_uid);

  rtcom_qt::ChatMessageEntry* register_chat_leave(time_t start_time, time_t end_time, const char *self_name, const char *backend_name,
                        const char *remote_uid, const char *remote_name, const char *abook_uid, const char *text,
                        const char *protocol, const char *channel, const char *group_uid);

  void set_read(const unsigned int event_id, const gboolean read);
  void toggle_flag(const unsigned int event_id, const unsigned int flags, bool unset = false);

  std::string protocol_to_rtcom_service_id(const std::string& protocol);

  void rtcom_log(const std::string &msg, bool error = false);

  std::vector<rtcom_qt::ChatMessageEntry*> iterateRtComEvents(RTComElQuery *query_struct);
}
