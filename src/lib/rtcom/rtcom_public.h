#pragma once
//#include <QObject>
#include <vector>
#include <string>

#include "rtcom_models.h"

namespace rtcom_qt {
  std::vector<std::string>  get_service_accounts();
  std::string               get_room_name(std::string channel);
  void                      set_room_name(const std::string& group_uid, const std::string& title);
  void                      set_read(const unsigned int event_id, const bool read);
  bool                      delete_events(std::string group_uid);

  rtcom_qt::ChatMessageEntry* register_message(
      time_t start_time,
      time_t end_time,
      std::string self_name, std::string backend_name,
      std::string remote_uid, std::string remote_name, std::string abook_uid, std::string text,
      bool is_outgoing, std::string protocol, std::string channel, std::string group_uid);

  std::vector<rtcom_qt::ChatMessageEntry*> get_messages(const std::string& service_id, const std::string& group_uid, int limit, int offset);
  std::vector<rtcom_qt::ChatMessageEntry*> get_overview_messages(unsigned int limit, unsigned int offset);

  std::vector<rtcom_qt::ChatMessageEntry*> search_messages(std::string search, std::string group_uid, unsigned int limit = 20, unsigned int offset = 0);

  rtcom_qt::ChatMessageEntry* register_chat_join(time_t start_time, time_t end_time, const std::string& self_name, const std::string& backend_name,
                        const std::string& remote_uid, const std::string& remote_name, const std::string& abook_uid, const std::string& text,
                        const std::string& protocol, const std::string& channel, const std::string& group_uid);

  rtcom_qt::ChatMessageEntry* register_chat_leave(time_t start_time, time_t end_time, const std::string& self_name, const std::string& backend_name,
                        const std::string& remote_uid, const std::string& remote_name, const std::string& abook_uid, const std::string& text,
                        const std::string& protocol, const std::string& channel, const std::string& group_uid);
}