#include "abook_roster.h"
#include "abook_public.h"

#include "abook.h"

namespace abook_qt {
  std::shared_ptr<AbookContact>& get_or_create_cached_contact_from_remote_uid(
      const std::string& protocol, const std::string& remote_uid) {

    auto [it, inserted] = CONTACTS_CACHE_REMOTE_UID.try_emplace(remote_uid, std::make_shared<AbookContact>(remote_uid));
    if (it->second->abook_uid.empty() && !CONTACTS_NOT_FOUND.contains(remote_uid)) {
      auto uid = abookqt::get_abook_uid(protocol, remote_uid);
      if (!uid.empty()) {
        it->second->abook_uid = std::move(uid);
        CONTACTS_CACHE_ABOOK_UID.try_emplace(it->second->abook_uid, it->second);
      } else if (AGGREGATOR_READY) {
        CONTACTS_NOT_FOUND.insert(remote_uid);
      }
    }
    return it->second;
  }
}