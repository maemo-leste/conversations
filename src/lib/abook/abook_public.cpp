#include "abook_public.h"

#include "abook.h"

namespace abook_qt {
  std::function<void(std::string, std::string)> func_avatarChangedSignal = nullptr;
  std::function<void(std::map<std::string, std::shared_ptr<AbookContact>>)> func_contactsChangedSignal = nullptr;
  std::function<void()> func_initReadySignal = nullptr;

  bool abook_init() {
    return abookqt::init();
  }

  std::string get_abook_uid(const std::string& protocol, const std::string& remote_uid) {
    if (REMOTE_UID_TO_ABOOK_UID_LOOKUP.contains(remote_uid))
      return REMOTE_UID_TO_ABOOK_UID_LOOKUP.at(remote_uid);

    auto uid = abookqt::get_abook_uid(protocol, remote_uid);
    if (!uid.empty())
      REMOTE_UID_TO_ABOOK_UID_LOOKUP[remote_uid] = uid;
    return uid;
  }

  void abook_init_contact_roster() {
    // fills singleton `abook_roster.h`
    return abookqt::init_contact_roster();
  }

  void new_dialog_contact_chooser(const std::function<void(std::string)> &cb) {
    abookqt::new_dialog_contact_chooser(cb);
  }

  AbookContactAvatar* abook_get_avatar(const std::string& remote_uid) {
    return abookqt::get_avatar(remote_uid);
  }

  std::string get_avatar_token(const std::string& remote_uid) {
    return abookqt::get_avatar_token(remote_uid);
  }

  std::string get_display_name(const std::string& remote_uid) {
    if (REMOTE_UID_TO_DISPLAY_NAME.contains(remote_uid))
      return REMOTE_UID_TO_DISPLAY_NAME[remote_uid];

    auto name = abookqt::get_display_name(remote_uid);
    if (!name.empty())
      REMOTE_UID_TO_DISPLAY_NAME[remote_uid] = name;
    return name;
  }
}