#pragma once

#include <map>
#include <memory>
#include <string>

#include "abook_contact.h"

namespace abook_qt {
  inline std::map<std::string, std::shared_ptr<AbookContact>> ROSTER;
}