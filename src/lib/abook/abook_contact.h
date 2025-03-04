#pragma once
#include <string>

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
  std::string abook_uid;       // e.g haze/jabber/stevejobs_40xmpp_2etest_2eorg0-test123@xmpp.is
  std::string display_name;    // e.g Some Display Name
  std::string local_uid;       // e.g idle/irc/myself
  std::string remote_uid;      // e.g cool_username (counterparty)
  std::string presence;
  std::string subscribed;
  std::string published;
  std::string avatar_token;

  explicit AbookContact(std::string _abook_uid) {
    const size_t pos = _abook_uid.rfind('-');

    // Extract the substrings
    local_uid = _abook_uid.substr(0, pos);
    remote_uid = _abook_uid.substr(pos + 1);
    abook_uid = _abook_uid;
  }

  [[nodiscard]] std::string id() const { return local_uid + "-" + remote_uid; }

  bool is_blocked = false;
  bool can_block = false;
  bool can_auth = false;
};
