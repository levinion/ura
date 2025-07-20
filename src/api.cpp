#include "ura/api.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"
#include "ura/keyboard.hpp"
#include "ura/workspace.hpp"

namespace ura {

std::vector<std::string> split(std::string& s) {
  std::vector<std::string> v;
  std::string t;
  for (int i = 0; i < s.size(); i++) {
    auto c = s[i];
    if (std::isalpha(c)) {
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
    } else if (m == "shift") {
      mod |= WLR_MODIFIER_SHIFT;
    }
  }
  xkb_keysym_t sym =
    xkb_keysym_from_name(key_str.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);

  // if shift is pressed, then upper sym should be binded
  if (mod & WLR_MODIFIER_SHIFT && sym >= XKB_KEY_a && sym <= XKB_KEY_z) {
    sym -= (XKB_KEY_a - XKB_KEY_A);
  }

  wlr_log(WLR_DEBUG, "bind: modifiers: %d; keysym: %d", mod, sym);

  return (static_cast<uint64_t>(mod) << 32) | sym;
}

void map(std::string modifiers, std::string key, sol::protected_function func) {
  auto server = UraServer::get_instance();
  auto keypair_id = keypair_id_from_string(modifiers, key);
  server->config->keybinding[keypair_id] = func;
}

void terminate() {
  UraServer::get_instance()->terminate();
}

void close_window() {
  auto server = UraServer::get_instance();
  auto toplevel = server->focused_toplevel;
  if (!toplevel)
    return;
  toplevel->close();
}

void fullscreen() {
  auto server = UraServer::get_instance();
  auto toplevel = server->focused_toplevel;
  if (!toplevel)
    return;
  toplevel->toggle_fullscreen();
}

void reload() {
  auto server = UraServer::get_instance();
  // reset config
  server->config = UraConfig::init();
  server->config->load();
  server->lua->try_execute_hook("reload");
}

void set_keyboard_repeat(int rate, int delay) {
  auto server = UraServer::get_instance();
  auto keyboard = server->current_keyboard();
  keyboard->set_repeat(rate, delay);
}

void focus_follow_mouse(bool flag) {
  auto server = UraServer::get_instance();
  server->config->focus_follow_mouse = flag;
}

void env(std::string name, std::string value) {
  setenv(name.data(), value.data(), true);
}

int switch_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  index = output->switch_workspace(index);
  return index;
}

int move_to_workspace(int index) {
  auto server = UraServer::get_instance();
  auto toplevel = server->focused_toplevel;
  auto output = toplevel->output;
  if (index >= output->workspaces.size())
    output->create_workspace();
  index = toplevel->move_to_workspace(index);
  index = output->switch_workspace(index);
  return index;
}

int current_workspace() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  return output->current_workspace->index();
}

void hook(std::string name, sol::protected_function f) {
  auto& config = UraServer::get_instance()->config;
  config->hooks[name] = f;
}

void tiling_gap(
  float inner,
  float outer_l,
  float outer_r,
  float outer_t,
  float outer_b
) {
  auto& config = UraServer::get_instance()->config;
  config->outer_gap_bottom = outer_b;
  config->outer_gap_top = outer_t;
  config->outer_gap_left = outer_l;
  config->outer_gap_right = outer_r;
  config->inner_gap = inner;
}

void cursor_theme(std::string theme, int size) {
  auto server = UraServer::get_instance();
  // recreate xcursor manager with given values
  wlr_xcursor_manager_destroy(server->cursor_mgr);
  server->cursor_mgr =
    wlr_xcursor_manager_create(theme.empty() ? NULL : theme.data(), size);
}

int current_toplevel() {
  auto server = UraServer::get_instance();
  return server->focused_toplevel->index();
}

bool focus(int index) {
  auto server = UraServer::get_instance();
  if (index < 0
      || index >= server->current_output()->current_workspace->toplevels.size())
    return false;
  auto it = server->current_output()->current_workspace->toplevels.begin();
  std::advance(it, index);
  (*it)->focus();
  return true;
}

} // namespace ura
