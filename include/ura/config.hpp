#pragma once

#include <xkbcommon/xkbcommon.h>
#include <memory>
// #include <unordered_map>
#include <sol/sol.hpp>
#include <string>

namespace ura {

uint64_t keypair_id_from_string(std::string& modifiers, std::string& key);

class UraServer;

class UraConfigManager {
public:
  sol::state lua;
  sol::table ura;
  std::unordered_map<uint64_t, sol::protected_function> keybinding;
  bool focus_follow_mouse = true;
  float scale = 1;

  static std::unique_ptr<UraConfigManager> init();
  void register_function();
  void load_config();
};

} // namespace ura
