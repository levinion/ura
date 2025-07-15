#pragma once

#include <xkbcommon/xkbcommon.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <sol/sol.hpp>

namespace ura {

uint64_t keypair_id_from_string(std::string& modifiers, std::string& key);

class UraServer;

class UraConfig {
public:
  std::unordered_map<uint64_t, sol::protected_function> keybinding;
  bool focus_follow_mouse = true;
  float scale = 1;

  static std::unique_ptr<UraConfig> init();
  void load();
};

} // namespace ura
