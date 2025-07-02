#pragma once

#include <map>
#include <memory>
#include <functional>
#include <iostream>

#include "abook_contact.h"

namespace abook_qt {
  extern std::function<void(std::string, std::string)> func_avatarChangedSignal;
  extern std::function<void(std::map<std::string, std::shared_ptr<AbookContact>>)> func_contactsChangedSignal;
  extern std::function<void()> func_initReadySignal;

  bool abook_init();
  void abook_init_contact_roster();
  void new_dialog_contact_chooser(const std::function<void(std::string)> &cb);
  void abook_set_func_contact_updated(const std::function<void(std::map<std::string, std::shared_ptr<AbookContact>>)> &func);
  AbookContactAvatar* abook_get_avatar(const std::string& local_uid, const std::string& remote_uid);
  std::string get_avatar_token(const std::string& local_uid, const std::string& remote_uid);
  std::string get_display_name(const std::string& local_uid, const std::string& remote_uid, std::string& group_uid);
}
