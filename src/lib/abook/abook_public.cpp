#include "abook_public.h"

#include "abook.h"

namespace abook_qt {
  std::function<void(std::string)> func_avatarChangedSignal = nullptr;
  std::function<void(std::vector<std::shared_ptr<AbookContact>>)> func_contactsChangedSignal = nullptr;
  std::function<void()> func_initReadySignal = nullptr;
  bool AGGREGATOR_READY = false;

  bool abook_init() {
    return abookqt::init();
  }

  void abook_init_contact_roster() {
    // fills singleton `abook_roster.h`
    return abookqt::init_contact_roster();
  }

  void new_dialog_contact_chooser(const std::function<void(std::string)> &cb) {
    abookqt::new_dialog_contact_chooser(cb);
  }

  AbookContactAvatar* abook_get_avatar(const std::string& protocol, const std::string& remote_uid) {
    return abookqt::get_avatar(protocol, remote_uid);
  }

  std::string get_avatar_token(const std::string& protocol, const std::string& remote_uid) {
    return abookqt::get_avatar_token(protocol, remote_uid);
  }

  const PresenceInfo& get_presence(const std::string& protocol, const std::string& remote_uid) {
    const auto& cached_contact = get_or_create_cached_contact_from_remote_uid(protocol, remote_uid);

    if (cached_contact->presence.isNull()) {
      auto info = abookqt::get_presence(protocol, remote_uid);
      cached_contact->presence = std::move(info);
    }

    return cached_contact->presence;
  }

  const std::string& get_display_name(const std::string& protocol, const std::string& remote_uid) {
    const auto& cached_contact = get_or_create_cached_contact_from_remote_uid(protocol, remote_uid);

    if (cached_contact->display_name.empty()) {
      auto display_name = abookqt::get_display_name(protocol, remote_uid);
      if (!display_name.empty())
        cached_contact->display_name = std::move(display_name);
    }

    return cached_contact->display_name;
  }

  const std::string& get_abook_uid(const std::string& protocol, const std::string& remote_uid) {
    const auto& cached_contact = get_or_create_cached_contact_from_remote_uid(protocol, remote_uid);
    return cached_contact->abook_uid;
  }
}