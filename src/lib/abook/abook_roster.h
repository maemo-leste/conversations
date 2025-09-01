#pragma once

#include <map>
#include <memory>
#include <string>

#include "abook_contact.h"

namespace abook_qt {
  inline std::map<std::string, std::shared_ptr<AbookContact>> ROSTER;
  inline std::map<std::string, std::string> REMOTE_UID_TO_ABOOK_UID_LOOKUP;
  inline std::map<std::string, std::string> REMOTE_UID_TO_DISPLAY_NAME;
}