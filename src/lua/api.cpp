#include "ura/api.hpp"
#include <format>
#include "ura/cursor.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"
#include "ura/keyboard.hpp"
#include "ura/workspace.hpp"
#include "ura/util.hpp"

namespace ura::api {

uint64_t
keypair_id_from_string(std::string& modifiers_str, std::string& key_str) {
  // modifiers str to modifiers bit
  auto modifiers = split(modifiers_str, '+');
  uint32_t mod = 0;
  for (auto m : modifiers) {
    if (m == "super" || m == "mod" || m == "cmd" || m == "command") {
      mod |= WLR_MODIFIER_LOGO;
    } else if (m == "alt" || m == "opt") {
      mod |= WLR_MODIFIER_ALT;
    } else if (m == "ctrl" || m == "control") {
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
  return (static_cast<uint64_t>(mod) << 32) | sym;
}

void set_keymap(
  std::string modifiers,
  std::string key,
  sol::protected_function f
) {
  auto server = UraServer::get_instance();
  auto id = keypair_id_from_string(modifiers, key);
  server->lua->set(std::format("g.keymaps.{}", id), f);
}

void terminate() {
  UraServer::get_instance()->terminate();
}

void close_window() {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  auto client = workspace->focus_stack.top();
  if (client) {
    auto toplevel = client.value().transform<UraToplevel>();
    if (toplevel)
      toplevel->close();
  }
}

void set_window_fullscreen(bool flag) {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  auto client = workspace->focus_stack.top();
  if (client) {
    auto toplevel = client.value().transform<UraToplevel>();
    if (toplevel && toplevel->fullscreen() != flag)
      toplevel->set_fullscreen(flag);
  }
}

void reload() {
  auto server = UraServer::get_instance();
  // TODO: reset lua VM
  server->lua->try_execute_hook("reload");
}

void set_keyboard_repeat(int rate, int delay) {
  auto server = UraServer::get_instance();
  auto keyboard = server->current_keyboard();
  keyboard->set_repeat(rate, delay);
}

void set_env(std::string name, std::string value) {
  setenv(name.data(), value.data(), true);
}

int switch_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  index = output->switch_workspace(index);
  return index;
}

int move_window_to_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto workspace = output->current_workspace;
  auto client = workspace->focus_stack.top();
  if (client) {
    auto toplevel = client.value().transform<UraToplevel>();
    if (!toplevel)
      return -1;
    if (index >= output->workspaces.size())
      output->create_workspace();
    index = toplevel->move_to_workspace(index);
    index = output->switch_workspace(index);
    return index;
  }
  return -1;
}

int get_current_workspace_index() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  return output->current_workspace->index();
}

void set_hook(std::string name, sol::protected_function f) {
  auto server = UraServer::get_instance();
  server->lua->set(std::format("g.hooks.{}", name), f);
}

void set_cursor_theme(std::string theme, int size) {
  auto server = UraServer::get_instance();
  server->cursor->set_theme(theme, size);
}

void set_cursor_visible(bool flag) {
  auto server = UraServer::get_instance();
  if (!flag)
    server->cursor->hide();
  else
    server->cursor->show();
}

void cursor_absolute_move(double x, double y) {
  auto server = UraServer::get_instance();
  server->cursor->absolute_move(x, y);
}

void cursor_relative_move(double delta_x, double delta_y) {
  auto server = UraServer::get_instance();
  server->cursor->relative_move(delta_x, delta_y);
}

void set_cursor_shape(std::string name) {
  auto server = UraServer::get_instance();
  server->cursor->set_xcursor(name);
}

int get_current_window_index() {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  auto client = workspace->focus_stack.top();
  if (client) {
    auto toplevel = client.value().transform<UraToplevel>();
    if (!toplevel)
      return -1;
    return toplevel->index();
  }
  return -1;
}

bool focus_window(int index) {
  auto server = UraServer::get_instance();
  if (index < 0
      || index >= server->current_output()->current_workspace->toplevels.size())
    return false;
  auto it = server->current_output()->current_workspace->toplevels.begin();
  std::advance(it, index);
  (*it)->focus();
  return true;
}

void set_window_floating(bool flag) {
  auto server = UraServer::get_instance();
  auto focused = server->current_output()->current_workspace->focus_stack.top();
  if (focused) {
    auto toplevel = focused->transform<UraToplevel>();
    if (toplevel) {
      toplevel->set_float(flag);
    }
  }
}

bool is_window_fullscreen() {
  auto server = UraServer::get_instance();
  auto focused = server->current_output()->current_workspace->focus_stack.top();
  if (focused) {
    auto toplevel = focused->transform<UraToplevel>();
    if (toplevel) {
      return toplevel->fullscreen();
    }
  }
  return false;
}

bool is_window_floating() {
  auto server = UraServer::get_instance();
  auto focused = server->current_output()->current_workspace->focus_stack.top();
  if (focused) {
    auto toplevel = focused->transform<UraToplevel>();
    if (toplevel) {
      return toplevel->floating;
    }
  }
  return false;
}

} // namespace ura::api
