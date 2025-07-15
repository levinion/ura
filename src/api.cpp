#include "ura/api.hpp"
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

void set_output_scale(float scale) {
  auto server = UraServer::get_instance();
  // TODO: use UraOutput rather than wlr_output
  auto output = wlr_output_layout_output_at(
    server->output_layout,
    server->cursor->x,
    server->cursor->y
  );
  output->scale = scale;
  server->config->scale = scale;
}

void reload() {
  auto server = UraServer::get_instance();
  server->config->load();
}

void set_keyboard_repeat(int rate, int delay) {
  auto server = UraServer::get_instance();
  // TODO: use UraKeyboard rather than wlr_keyboard
  auto keyboard = server->seat->keyboard_state.keyboard;
  wlr_keyboard_set_repeat_info(keyboard, rate, delay);
}

void focus_follow_mouse(bool flag) {
  auto server = UraServer::get_instance();
  server->config->focus_follow_mouse = flag;
}

void env(std::string name, std::string value) {
  setenv(name.data(), value.data(), true);
}

} // namespace ura
