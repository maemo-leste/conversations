#include "abook_public.h"

#include "abook.h"

namespace abook_qt {
  std::function<void(std::string, std::string)> func_avatarChangedSignal = nullptr;
  std::function<void(std::map<std::string, std::shared_ptr<AbookContact>>)> func_contactsChangedSignal = nullptr;
  std::function<void()> func_initReadySignal = nullptr;

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

  AbookContactAvatar* abook_get_avatar(const std::string& local_uid, const std::string& remote_uid) {
    return abookqt::get_avatar(local_uid, remote_uid);
  }

  std::string get_avatar_token(const std::string& local_uid, const std::string& remote_uid) {
    return abookqt::get_avatar_token(local_uid, remote_uid);
  }

  std::string get_display_name(const std::string& local_uid, const std::string& remote_uid, std::string& group_uid) {
    auto it = ROSTER.find(group_uid);
    if (it != ROSTER.end()) {
      return it->second->display_name;
    }

    return abookqt::get_display_name(local_uid, remote_uid);
  }
}