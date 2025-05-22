#include "rtcom_public.h"

#include "rtcom.h"

namespace rtcom_qt {
  std::unordered_map<std::string, std::string> cache_room_name;

  std::vector<rtcom_qt::ChatMessageEntry*> search_messages(std::string search, std::string group_uid, unsigned int limit, unsigned int offset) {
	return qtrtcom::search_messages(search, group_uid);
  }

  std::vector<std::string> get_service_accounts() {
    return qtrtcom::get_service_accounts();
  }

  std::string get_room_name(std::string group_uid) {
    auto it = cache_room_name.find(group_uid);
    if (it != cache_room_name.end()) {
      return it->second;
    }

    auto _group_uid = group_uid.c_str();
    std::string result = qtrtcom::get_room_name(_group_uid);
    if (!result.empty())
      cache_room_name[group_uid] = result;

    return result;
  }

  void set_room_name(const std::string& group_uid, const std::string& title) {
    auto it = cache_room_name.find(group_uid);
    if (it != cache_room_name.end())
      if (title == it->second)
        return;  // already in cache

    const auto group_uid_str = group_uid.c_str();
    const auto title_str = title.c_str();
    qtrtcom::set_room_name(group_uid_str, title_str);
    cache_room_name[group_uid] = title;
  }

  bool set_event_header(const unsigned int event_id, const std::string& key, const std::string& value) {
    return qtrtcom::set_event_header(event_id, key, value);
  }

  std::vector<unsigned int> get_events_by_header(std::string key, std::string value) {
    return qtrtcom::get_events_by_header(key, value);
  }

  std::unordered_map<std::string, std::string> get_event_headers(const unsigned int event_id) {
    return qtrtcom::get_event_headers(event_id);
  }

  void toggle_event_flags(const unsigned int event_id, const unsigned int flags, bool unset) {
    return qtrtcom::toggle_event_flags(event_id, flags, unset);
  }

  void set_read(const unsigned int event_id, const bool read) {
    return qtrtcom::set_read(event_id, read);
  }

  rtcom_qt::ChatMessageEntry* register_message(
      time_t start_time, 
      time_t end_time, 
      std::string self_name, std::string backend_name,
      std::string remote_uid, std::string remote_name, std::string abook_uid, std::string text,
      bool is_outgoing, std::string protocol, std::string channel, std::string group_uid, unsigned int flags) {
    auto self_name_cstr = self_name.c_str();
    auto backend_name_cstr = backend_name.c_str();
    auto remote_uid_cstr = remote_uid.c_str();
    auto remote_name_cstr = remote_name.c_str();
    auto abook_uid_cstr = abook_uid.c_str();
    auto text_cstr = text.c_str();
    auto protocol_cstr = protocol.c_str();
    auto channel_cstr = channel.empty() || channel == "" ? NULL : channel.c_str();
    auto group_uid_cstr = group_uid.c_str();

    return qtrtcom::register_message(
      start_time, end_time, self_name_cstr, backend_name_cstr, remote_uid_cstr,
      remote_name_cstr, abook_uid_cstr, text_cstr, is_outgoing, protocol_cstr,
      channel_cstr, group_uid_cstr, flags);
  }

  bool delete_events(std::string group_uid) {
    auto _group_uid = group_uid.c_str();
    return qtrtcom::delete_events(_group_uid);
  }

  rtcom_qt::ChatMessageEntry* register_chat_join(time_t start_time, time_t end_time, const std::string& self_name, const std::string& backend_name,
                        const std::string& remote_uid, const std::string& remote_name, const std::string& abook_uid, const std::string& text,
                        const std::string& protocol, const std::string& channel, const std::string& group_uid) {
    auto self_name_cstr = self_name.c_str();
    auto backend_name_cstr = backend_name.c_str();
    auto remote_uid_cstr = remote_uid.c_str();
    auto remote_name_cstr = remote_name.c_str();
    auto abook_uid_cstr = abook_uid.c_str();
    auto text_cstr = text.c_str();
    auto protocol_cstr = protocol.c_str();
    auto channel_cstr = channel.c_str();
    auto group_uid_cstr = group_uid.c_str();

    return qtrtcom::register_chat_join(
        start_time, end_time,
        self_name_cstr, backend_name_cstr, remote_uid_cstr,
        remote_name_cstr, abook_uid_cstr, text_cstr, protocol_cstr,
        channel_cstr, group_uid_cstr);
  }

  rtcom_qt::ChatMessageEntry* register_chat_leave(time_t start_time, time_t end_time, const std::string& self_name, const std::string& backend_name,
                        const std::string& remote_uid, const std::string& remote_name, const std::string& abook_uid, const std::string& text,
                        const std::string& protocol, const std::string& channel, const std::string& group_uid) {
    auto self_name_cstr = self_name.c_str();
    auto backend_name_cstr = backend_name.c_str();
    auto remote_uid_cstr = remote_uid.c_str();
    auto remote_name_cstr = remote_name.c_str();
    auto abook_uid_cstr = abook_uid.c_str();
    auto text_cstr = text.c_str();
    auto protocol_cstr = protocol.c_str();
    auto channel_cstr = channel.c_str();
    auto group_uid_cstr = group_uid.c_str();
    return qtrtcom::register_chat_leave(
        start_time, end_time,
        self_name_cstr, backend_name_cstr, remote_uid_cstr,
        remote_name_cstr, abook_uid_cstr, text_cstr, protocol_cstr,
        channel_cstr, group_uid_cstr);
  }

  std::vector<rtcom_qt::ChatMessageEntry*> get_messages(const std::string& service_id, const std::string& group_uid, int limit, int offset) {
    auto service_id_cstr = service_id.c_str();
    auto group_uid_cstr = group_uid.c_str();
    return qtrtcom::get_messages(service_id_cstr, group_uid_cstr, limit, offset);
  }

  std::vector<rtcom_qt::ChatMessageEntry*> get_overview_messages(unsigned int limit, unsigned int offset) {
    return qtrtcom::get_overview_messages(limit, offset);
  }
}