#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <set>

#include "abook_contact.h"

namespace abook_qt {
  // conversations -> abook is generally supplied via remote_uid
  inline std::unordered_map<std::string, std::shared_ptr<AbookContact>> CONTACTS_CACHE_REMOTE_UID;
  // abook -> conversations is generally supplied via abook_uid
  inline std::unordered_map<std::string, std::shared_ptr<AbookContact>> CONTACTS_CACHE_ABOOK_UID;
  // throttle unknown entries
  inline std::set<std::string> CONTACTS_NOT_FOUND;

  std::shared_ptr<AbookContact>& get_or_create_cached_contact_from_remote_uid(const std::string& protocol, const std::string& remote_uid);
}