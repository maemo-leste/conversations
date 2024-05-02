#include "lib/rtcom.h"

RTComEl * qtrtcom::rtcomel() {
  RTComEl *el = NULL;

  if (!el)
    el = rtcom_el_new();

  return el;
}

RTComElQuery *qtrtcom::startQuery(const int limit, const int offset, const RTComElQueryGroupBy group_by) {
  RTComElQuery *query = rtcom_el_query_new(rtcomel());

  if(group_by != RTCOM_EL_QUERY_GROUP_BY_NONE)
    rtcom_el_query_set_group_by(query, group_by);

  if(limit > 0)
    rtcom_el_query_set_limit(query, limit);
  if(offset > 0)
    rtcom_el_query_set_offset(query, offset);

  return query;
}

void qtrtcom::registerChatJoin(time_t start_time, time_t end_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, const char* protocol, const char* channel, const char* group_uid) {
  qDebug() << __FUNCTION__;

  auto *ev = qtrtcom::_defaultEvent(start_time, end_time, backend_name, remote_uid, text, protocol, channel, false, group_uid);
  RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_JOIN"));

  RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup(abook_uid));
  GError *err = NULL;

  if(rtcom_el_add_event(qtrtcom::rtcomel(), ev, &err) < 0) {
    qWarning() << "Failed to add event to RTCom" << "registerChatJoin" << err->message;
  } else {
    qDebug() << "registerChatJoin SUCCESS";
  }
}

void qtrtcom::registerChatLeave(time_t start_time, time_t end_time, const char* self_name, const char* backend_name, const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, const char* protocol, const char* channel, const char* group_uid) {
  qDebug() << __FUNCTION__;

  auto *ev = qtrtcom::_defaultEvent(start_time, end_time, backend_name, remote_uid, text, protocol, channel, false, group_uid);
  RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_LEAVE"));

  RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup(abook_uid));
  GError *err = NULL;

  if(rtcom_el_add_event(qtrtcom::rtcomel(), ev, &err) < 0) {
    qWarning() << "Failed to add event to RTCom" << "registerChatLeave" << err->message;
  } else {
    qDebug() << "registerChatLeave SUCCESS";
  }
}

void qtrtcom::registerMessage(time_t start_time, time_t end_time, const char* self_name, const char* backend_name,
                              const char *remote_uid, const char *remote_name, const char* abook_uid, const char* text, bool is_outgoing, const char* protocol, const char* channel, const char* group_uid) {
  qDebug() << __FUNCTION__;

  auto *ev = qtrtcom::_defaultEvent(start_time, end_time, backend_name, remote_uid, text, protocol, channel, is_outgoing, group_uid);
  if (Utils::protocolIsTelephone(protocol)) {
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_SMS_MESSAGE"));
  } else {
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"));
  }

  RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(self_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_name, g_strdup(remote_name));
  RTCOM_EL_EVENT_SET_FIELD(ev, remote_ebook_uid, g_strdup(abook_uid));
  auto el = rtcom_el_new();
  GError *err = NULL;
  if(rtcom_el_add_event(el, ev, &err) < 0) {
    qWarning() << "Failed to add event to RTCom" << "registerMessage" << err->message;
  } else {
    qDebug() << "registerMessage SUCCESS";
  }
}

void qtrtcom::setRead(const unsigned int event_id, const gboolean read) {
  qDebug() << "setRead" << event_id;
  /* Ignore error for now by setting GError error to NULL */
  rtcom_el_set_read_event(rtcomel(), event_id, read, NULL);
}


RTComElEvent* qtrtcom::_defaultEvent(time_t start_time, time_t end_time, const char* local_uid, const char *remote_uid,
                                     const char* text, const char* protocol, const char* channel, bool is_outgoing,
                                     const char* group_uid) {
  qDebug() << __FUNCTION__;
  RTComElEvent *ev = rtcom_el_event_new();
  auto rtcom_service = Utils::protocolToRTCOMServiceID(protocol);
  RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup(rtcom_service.toLocal8Bit().data()));

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

