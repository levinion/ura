#include "ura/lua.hpp"
#include "ura/api.hpp"
#include "ura/server.hpp"
#include <filesystem>
#include <memory>
#include <string>

namespace ura {

std::unique_ptr<Lua> Lua::init() {
  auto lua = std::make_unique<Lua>();
  lua->state.open_libraries(sol::lib::base, sol::lib::os, sol::lib::package);
  lua->table = lua->state.create_named_table("_ura");
  lua->register_function();
  return lua;
};

void Lua::register_function() {
  this->table.set_function("map", map);
  this->table.set_function("terminate", terminate);
  this->table.set_function("close_window", close_window);
  this->table.set_function("fullscreen", fullscreen);
  this->table.set_function("reload", reload);
  this->table.set_function("set_keyboard_repeat", set_keyboard_repeat);
  this->table.set_function("focus_follow_mouse", focus_follow_mouse);
  this->table.set_function("env", env);
  this->table.set_function("switch_workspace", switch_workspace);
  this->table.set_function("move_to_workspace", move_to_workspace);
  this->table.set_function("current_workspace", current_workspace);
  this->table.set_function("hook", hook);
  this->table.set_function("tiling_gap", tiling_gap);
  this->table.set_function("cursor_theme", cursor_theme);
  this->table.set_function("focus", focus);
  this->table.set_function("current_toplevel", current_toplevel);
}

// TODO: unsafe lua scripts

void Lua::execute(std::string script) {
  this->state.script(script);
}

void Lua::execute_file(std::filesystem::path path) {
  if (std::filesystem::is_regular_file(path))
    this->state.script_file(path);
}

void Lua::try_execute_hook(std::string name) {
  auto server = UraServer::get_instance();
  auto& config = server->config;
  if (config->hooks.contains(name)) {
    config->hooks[name]();
  }
}
} // namespace ura
