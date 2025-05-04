#pragma once

#include <string>
#include <algorithm>

namespace rtcom_qt {
  enum {
    RTCOM_EL_FLAG_SMS_PENDING         = 1 << 0,
    RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR = 1 << 1,
    RTCOM_EL_FLAG_SMS_PERMANENT_ERROR = 1 << 2
  };

  struct ChatMessageEntry {
    int event_id = -1;
    std::string service;
    std::string group_uid;

    std::string local_uid;
    std::string protocol;
    std::string remote_uid;
    std::string remote_name;
    std::string remote_ebook_uid;
    std::string text;
    std::string icon_name;
    time_t timestamp = -1;
    unsigned int count = -1;
    std::string group_title;
    std::string channel;
    std::string event_type;
    bool outgoing = true;
    bool is_read = false;
    unsigned int flags = 0;

    ChatMessageEntry(
      int event_id, const std::string& service, const std::string& group_uid,
      const std::string& local_uid, const std::string& _protocol,
      const std::string& remote_uid, const std::string& remote_name,
      const std::string& remote_ebook_uid, const std::string& text,
      const std::string& icon_name, time_t timestamp, unsigned int count,
      const std::string& group_title, const std::string& channel,
      const std::string& event_type, bool outgoing, bool is_read,
      unsigned int flags) : event_id(event_id), service(service), group_uid(group_uid),
        local_uid(local_uid), protocol(_protocol), remote_uid(remote_uid),
        remote_name(remote_name), remote_ebook_uid(remote_ebook_uid), text(text),
        icon_name(icon_name), timestamp(timestamp), count(count),
        group_title(group_title), channel(channel), event_type(event_type),
        outgoing(outgoing), is_read(is_read), flags(flags) {

      // parse protocol
      if(std::ranges::count(local_uid, '/') == 2) {
        const size_t first = local_uid.find('/');
        const size_t second = local_uid.find('/', first + 1);

        if (first != std::string::npos && second != std::string::npos) {
          protocol = local_uid.substr(first + 1, second - first - 1);
        }
      }
    }
  };
}