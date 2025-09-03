#pragma once
#include <string>
#include <utility>

namespace abook_qt {
  struct PresenceInfo {
    std::string icon_name;
    std::string presence;

    [[nodiscard]] bool isNull() const {
      return presence.empty();
    }
  };

  struct AbookContactAvatar {
    int width;
    int height;
    int rowstride;
    int n_channels;
    bool has_alpha;
    unsigned char* buf;
    unsigned int buf_len;
  };

  struct AbookContact {
    explicit AbookContact(std::string remote_uid) : remote_uid(std::move(remote_uid)) {}

    std::string abook_uid;
    std::string display_name;
    std::string local_uid;
    std::string remote_uid;

    std::string subscribed;
    std::string published;
    std::string avatar_token;
    PresenceInfo presence;

    bool is_blocked = false;
    bool can_block = false;
    bool can_auth = false;
  };
}