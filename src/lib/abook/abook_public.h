#pragma once

#include <map>
#include <memory>
#include <functional>
#include <iostream>

#include "abook_contact.h"

namespace abook_qt {
  inline std::function<void(std::string, std::string)> func_avatarChangedSignal = nullptr;
  inline std::function<void(std::map<std::string, std::shared_ptr<AbookContact>>)> func_contactsChangedSignal = nullptr;

  bool abook_init();
  void abook_init_contact_roster();
  void abook_set_func_contact_updated(const std::function<void(std::map<std::string, std::shared_ptr<AbookContact>>)> &func);
  AbookContactAvatar* abook_get_avatar(const std::string& local_uid, const std::string& remote_uid);
  std::string get_avatar_token(const std::string& local_uid, const std::string& remote_uid);
}
