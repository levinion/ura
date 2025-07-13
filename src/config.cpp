#include <filesystem>
#include "ura/ura.hpp"
#define SOL_ALL_SAFETIES_ON 1

#include "ura/config.hpp"
#include <sol/sol.hpp>
#include <vector>
#include <string>
#include "ura/server.hpp"

namespace ura {

std::vector<std::string> split(std::string& s) {
  std::vector<std::string> v;
  std::string t;
  for (int i = 0; i < s.size(); i++) {
    auto c = s[i];
    if (c >= 'A' && c <= 'z') {
      t.push_back(c);
    } else if (c == '+') {
      v.push_back(t);
      t.clear();
    }
  }
  if (!t.empty())
    v.push_back(t);
  return v;
}

uint64_t
keypair_id_from_string(std::string& modifiers_str, std::string& key_str) {
  // modifiers str to modifiers bit
  auto modifiers = split(modifiers_str);
  uint32_t mod = 0;
  for (auto m : modifiers) {
    if (m == "super" || m == "mod") {
      mod |= WLR_MODIFIER_LOGO;
    } else if (m == "alt" || m == "opt") {
      mod |= WLR_MODIFIER_ALT;
    } else if (m == "ctrl") {
      mod |= WLR_MODIFIER_CTRL;
    }
  }
  // key_str to keysym: k to XKB_KEY_k
  xkb_keysym_t sym =
    xkb_keysym_from_name(key_str.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);

  return (static_cast<uint64_t>(mod) << 32) | sym;
}

std::unique_ptr<UraConfigManager> UraConfigManager::init() {
  auto mgr = std::make_unique<UraConfigManager>();
  mgr->lua.open_libraries(sol::lib::base, sol::lib::os, sol::lib::package);
  mgr->ura = mgr->lua.create_named_table("ura");
  mgr->register_function();

  return mgr;
}

void UraConfigManager::register_function() {
  // map key
  this->ura.set_function(
    "map",
    [&](std::string modifiers, std::string key, sol::protected_function func) {
      auto keypair_id = keypair_id_from_string(modifiers, key);
      this->keybinding[keypair_id] = func;
    }
  );

  this->ura.set_function("terminate", [&]() {
    auto server = UraServer::get_instance();
    wl_display_terminate(server->display);
  });

  this->ura.set_function("close_window", [&]() {
    auto server = UraServer::get_instance();
    auto focused_surface = server->seat->keyboard_state.focused_surface;
    if (!focused_surface)
      return;
    auto toplevel = wlr_xdg_toplevel_try_from_wlr_surface(focused_surface);
    if (!toplevel)
      return;
    wlr_xdg_toplevel_send_close(toplevel);
  });

  this->ura.set_function("fullscreen", [&]() {
    auto server = UraServer::get_instance();
    auto focused_surface = server->seat->keyboard_state.focused_surface;
    if (!focused_surface)
      return;
    auto toplevel = wlr_xdg_toplevel_try_from_wlr_surface(focused_surface);
    if (!toplevel)
      return;
    wlr_xdg_toplevel_set_fullscreen(toplevel, !toplevel->current.fullscreen);
    wlr_xdg_surface_schedule_configure(toplevel->base);
  });

  this->ura.set_function("set_output_scale", [&](float scale) {
    auto server = UraServer::get_instance();
    auto output = wlr_output_layout_output_at(
      server->output_layout,
      server->cursor->x,
      server->cursor->y
    );
    output->scale = scale;
  });

  this->ura.set_function("reload", [&]() { this->load_config(); });
  this->ura.set_function("set_keyboard_repeat", [&](int rate, int delay) {
    auto server = UraServer::get_instance();
    auto keyboard = server->seat->keyboard_state.keyboard;
    wlr_keyboard_set_repeat_info(keyboard, rate, delay);
  });
}

void UraConfigManager::load_config() {
  std::string _home_dir = getenv("HOME");
  auto home_dir = std::filesystem::path(_home_dir);
  auto config_path = home_dir / ".config/ura/init.lua";
  lua.script_file(config_path);
}

} // namespace ura
