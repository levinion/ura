#include "ura/core/lua.hpp"
#include "ura/api/core.hpp"
#include "ura/api/lua.hpp"
#include "ura/core/server.hpp"
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <sol/forward.hpp>
#include <sol/property.hpp>
#include <sol/state_handling.hpp>
#include <string>
#include "ura/core/state.hpp"

namespace ura {

std::unique_ptr<Lua> Lua::init() {
  auto lua = std::make_unique<Lua>();
  lua->state.open_libraries(
    sol::lib::base,
    sol::lib::os,
    sol::lib::package,
    sol::lib::bit32,
    sol::lib::coroutine,
    sol::lib::count,
    sol::lib::debug,
    sol::lib::ffi,
    sol::lib::io,
    sol::lib::jit,
    sol::lib::math,
    sol::lib::string,
    sol::lib::table,
    sol::lib::utf8
  );
  lua->ura = lua->state.create_named_table("ura");
  lua->setup();
  return lua;
};

void Lua::setup() {
  auto server = UraServer::get_instance();

  // api
  this->set("api.terminate", api::core::terminate);
  this->set("api.reload", api::core::reload);
  this->set("api.spawn", api::core::spawn);
  this->set("api.notify_idle_activity", api::core::notify_idle_activity);
  this->set("api.set_idle_inhibitor", api::core::set_idle_inhibitor);
  this->set("api.notify", api::core::notify);
  this->set("api.schedule", api::lua::schedule);
  // window
  this->set("api.focus_window", api::core::focus_window);
  this->set("api.close_window", api::core::close_window);
  this->set(
    "api.move_window_to_workspace",
    api::core::move_window_to_workspace
  );
  this->set("api.get_window_number", api::core::get_window_number);
  this->set("api.get_current_window", api::core::get_current_window);
  this->set("api.get_window", api::core::get_window);
  this->set("api.activate_window", api::core::activate_window);
  this->set("api.set_window_z_index", api::core::set_window_z_index);
  this->set("api.move_window", api::core::move_window);
  this->set("api.resize_window", api::core::resize_window);
  this->set("api.set_window_draggable", api::core::set_window_draggable);
  this->set("api.swap_window", api::core::swap_window);
  // input
  this->set("api.set_keyboard_repeat", api::core::set_keyboard_repeat);
  this->set("api.set_cursor_theme", api::core::set_cursor_theme);
  this->set("api.set_cursor_visible", api::core::set_cursor_visible);
  this->set("api.get_cursor_visible", api::core::is_cursor_visible);
  this->set("api.set_cursor_shape", api::core::set_cursor_shape);
  this->set(
    "api.set_pointer_accel_profile",
    api::core::set_pointer_accel_profile
  );
  this->set("api.set_pointer_move_speed", api::core::set_pointer_move_speed);
  this->set(
    "api.set_pointer_scroll_speed",
    api::core::set_pointer_scroll_speed
  );
  // workspace
  this->set("api.create_workspace", api::core::create_workspace);
  this->set("api.switch_workspace", api::core::switch_workspace);
  this->set("api.destroy_workspace", api::core::destroy_workspace);
  this->set("api.get_workspace_number", api::core::get_workspace_number);
  this->set("api.get_current_workspace", api::core::get_current_workspace);
  this->set("api.get_workspace", api::core::get_workspace);
  // output
  this->set("api.get_current_output", api::core::get_current_output);
  this->set("api.get_output", api::core::get_output);
  this->set("api.set_output_dpms", api::core::set_output_dpms);
  this->set(
    "api.get_output_logical_geometry",
    api::lua::get_output_logical_geometry
  );
  // keymap
  this->set("api.set_keymap", api::lua::set_keymap);
  this->set("api.unset_keymap", api::core::unset_keymap);
  this->set("api.set_keymap_mode", api::core::set_keymap_mode);
  this->set("api.get_keymap_mode", api::core::get_keymap_mode);
  // hook
  this->set("api.set_hook", api::lua::set_hook);
  this->set("api.unset_hook", api::core::unset_hook);
  // fn
  this->set("api.set_env", api::core::set_env);
  this->set("api.unset_env", api::core::unset_env);
  this->set("api.append_package_path", api::core::append_package_path);
  this->set("api.prepend_package_path", api::core::prepend_package_path);
  this->set("api.expanduser", api::core::expanduser);
  this->set("api.expandvars", api::core::expandvars);
  this->set("api.expand", api::core::expand);
  // override
  this->state.set("print", api::lua::print);
}

std::expected<std::string, std::string> Lua::execute(std::string_view script) {
  this->lua_stdout.clear();
  auto result = this->state.safe_script(script, sol::script_pass_on_error);
  if (result.valid())
    return this->lua_stdout;
  sol::error err = result;
  return std::unexpected(err.what());
}

// this is only used to run init.lua by now
std::expected<std::string, std::string> Lua::execute_file(std::string_view p) {
  this->lua_stdout.clear();
  auto path = std::filesystem::path(p);
  if (!std::filesystem::is_regular_file(path))
    return std::unexpected(
      std::format("[ura] path not exists or invalid: {}", path.string())
    );
  auto result = this->state.safe_script_file(path, sol::script_pass_on_error);
  if (result.valid())
    return this->lua_stdout;
  sol::error err = result;
  return std::unexpected(std::string(path) + ": " + std::string(err.what()));
}

std::optional<std::string> Lua::find_config_path() {
  auto server = UraServer::get_instance();
  if (server->state->config_path) {
    return server->state->config_path;
  }
  auto root = std::getenv("XDG_CONFIG_HOME")
    ? std::filesystem::path(std::getenv("XDG_CONFIG_HOME"))
    : std::filesystem::path(std::getenv("HOME")) / ".config";
  auto dotfile = std::filesystem::path(root) / "ura/init.lua";
  if (std::filesystem::is_regular_file(dotfile)) {
    server->state->config_path = dotfile;
    return dotfile;
  }
  auto global_dotfile = std::filesystem::path("/etc/ura/init.lua");
  if (std::filesystem::is_regular_file(global_dotfile)) {
    server->state->config_path = global_dotfile;
    return global_dotfile;
  }
  return {};
}

std::expected<void, std::string> Lua::load_config() {
  auto result = this->execute(
    R"*(
    ura.api.prepend_package_path("/usr/share/ura/runtime/?.lua")
    ura.win = require("_win")
  )*"
  );

  if (!result) {
    return std::unexpected(result.error());
  }

  auto path = this->find_config_path();
  if (!path) {
    return std::unexpected("could not found any config files, exiting...");
  }

  result = this->execute_file(path.value());
  if (!result) {
    return std::unexpected(result.error());
  }

  return {};
}

std::optional<sol::protected_function>
Lua::load_as_function(std::string_view f) {
  auto server = UraServer::get_instance();
  auto result = server->lua->state.load(f);
  if (!result.valid())
    return {};
  return result;
}

} // namespace ura
