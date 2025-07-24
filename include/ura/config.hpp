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
  float outer_gap_left = 10, outer_gap_right = 10, outer_gap_top = 10,
        outer_gap_bottom = 10, inner_gap = 10;
  int default_width = 800, default_height = 600;
  std::unordered_map<std::string, sol::protected_function> hooks;
  static std::unique_ptr<UraConfig> init();
  void load();
};

} // namespace ura
