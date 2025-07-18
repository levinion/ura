#pragma once

#include <xkbcommon/xkbcommon.h>
#include <sol/sol.hpp>
#include <memory>
#include <unordered_map>
#include <string>

namespace ura {

uint64_t keypair_id_from_string(std::string& modifiers, std::string& key);

class UraServer;

class UraConfig {
public:
  std::unordered_map<uint64_t, sol::protected_function> keybinding;
  bool focus_follow_mouse = true;
  float scale = 1;
  float outer_gap = 10, inner_gap = 10;
  std::unordered_map<std::string, sol::protected_function> hooks;
  static std::unique_ptr<UraConfig> init();
  void load();
};

} // namespace ura
