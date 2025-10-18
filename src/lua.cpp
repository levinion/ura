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
#include <filesystem>

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

#define LUAAPI(name, f) flexible::set(this->ura, name, f)

void Lua::setup() {
  auto server = UraServer::get_instance();

  // api
  LUAAPI("api.terminate", api::core::terminate);
  LUAAPI("api.reload", api::core::reload);
  LUAAPI("api.spawn", api::core::spawn);
  LUAAPI("api.notify_idle_activity", api::core::notify_idle_activity);
  LUAAPI("api.set_idle_inhibitor", api::core::set_idle_inhibitor);
  LUAAPI("api.notify", api::core::notify);
  LUAAPI("api.schedule", api::core::schedule);
  // window
  LUAAPI("api.focus_window", api::core::focus_window);
  LUAAPI("api.close_window", api::core::close_window);
  LUAAPI("api.move_window_to_workspace", api::core::move_window_to_workspace);
  LUAAPI("api.get_current_window", api::core::get_current_window);
  LUAAPI("api.get_window", api::core::get_window);
  LUAAPI("api.get_window_index", api::core::get_window_index);
  LUAAPI("api.activate_window", api::core::activate_window);
  LUAAPI("api.set_window_z_index", api::core::set_window_z_index);
  LUAAPI("api.get_window_z_index", api::core::get_window_z_index);
  LUAAPI("api.move_window", api::core::move_window);
  LUAAPI("api.resize_window", api::core::resize_window);
  LUAAPI("api.set_window_draggable", api::core::set_window_draggable);
  LUAAPI("api.is_window_draggable", api::core::is_window_draggable);
  LUAAPI("api.swap_window", api::core::swap_window);
  LUAAPI("api.get_window_app_id", api::core::get_window_app_id);
  LUAAPI("api.get_window_title", api::core::get_window_title);
  LUAAPI("api.set_window_fullscreen", api::core::set_window_fullscreen);
  LUAAPI("api.is_window_fullscreen", api::core::is_window_fullscreen);
  LUAAPI("api.get_window_geometry", api::core::get_window_geometry);
  LUAAPI("api.get_window_workspace", api::core::get_window_workspace);
  LUAAPI("api.get_window_output", api::core::get_window_output);
  LUAAPI("api.get_windows", api::core::get_windows);
  // input
  LUAAPI("api.set_keyboard_repeat", api::core::set_keyboard_repeat);
  LUAAPI("api.set_cursor_theme", api::core::set_cursor_theme);
  LUAAPI("api.set_cursor_visible", api::core::set_cursor_visible);
  LUAAPI("api.get_cursor_visible", api::core::is_cursor_visible);
  LUAAPI("api.set_cursor_shape", api::core::set_cursor_shape);
  LUAAPI("api.set_pointer_accel_profile", api::core::set_pointer_accel_profile);
  LUAAPI("api.set_pointer_move_speed", api::core::set_pointer_move_speed);
  LUAAPI("api.set_pointer_scroll_speed", api::core::set_pointer_scroll_speed);
  // workspace
  LUAAPI("api.get_current_workspace", api::core::get_current_workspace);
  LUAAPI("api.switch_workspace", api::core::switch_workspace);
  LUAAPI("api.destroy_workspace", api::core::destroy_workspace);
  LUAAPI("api.create_indexed_workspace", api::core::create_indexed_workspace);
  LUAAPI("api.get_indexed_workspace", api::core::get_indexed_workspace);
  LUAAPI("api.get_workspace_index", api::core::get_workspace_index);
  LUAAPI("api.create_named_workspace", api::core::create_named_workspace);
  LUAAPI("api.get_named_workspace", api::core::get_named_workspace);
  LUAAPI("api.is_workspace_named", api::core::is_workspace_named);
  LUAAPI("api.get_workspace_name", api::core::get_workspace_name);
  LUAAPI("api.get_workspaces", api::core::get_workspaces);
  LUAAPI("api.get_named_workspaces", api::core::get_named_workspaces);
  LUAAPI("api.get_indexed_workspaces", api::core::get_indexed_workspaces);
  // output
  LUAAPI("api.get_current_output", api::core::get_current_output);
  LUAAPI("api.get_output", api::core::get_output);
  LUAAPI("api.set_output_dpms", api::core::set_output_dpms);
  LUAAPI(
    "api.get_output_logical_geometry",
    api::core::get_output_logical_geometry
  );
  LUAAPI(
    "api.get_output_usable_geometry",
    api::core::get_output_usable_geometry
  );
  LUAAPI("api.get_output_scale", api::core::get_output_scale);
  // keymap
  LUAAPI("api.set_keymap", api::core::set_keymap);
  LUAAPI("api.unset_keymap", api::core::unset_keymap);
  LUAAPI("api.set_keymap_mode", api::core::set_keymap_mode);
  LUAAPI("api.get_keymap_mode", api::core::get_keymap_mode);
  // hook
  LUAAPI("api.set_hook", api::core::set_hook);
  LUAAPI("api.unset_hook", api::core::unset_hook);
  // fn
  LUAAPI("api.set_env", api::core::set_env);
  LUAAPI("api.unset_env", api::core::unset_env);
  LUAAPI("api.append_package_path", api::core::append_package_path);
  LUAAPI("api.prepend_package_path", api::core::prepend_package_path);
  LUAAPI("api.expanduser", api::core::expanduser);
  LUAAPI("api.expandvars", api::core::expandvars);
  LUAAPI("api.expand", api::core::expand);
  LUAAPI("api.to_json", api::core::to_json);
  LUAAPI("api.parse_json", api::core::parse_json);
  // opt
  LUAAPI("api.set_option", api::core::set_option);
  LUAAPI("api.get_option", api::core::get_option);
  LUAAPI("api.set_userdata", api::core::set_userdata);
  LUAAPI("api.get_userdata", api::core::get_userdata);
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
    return dotfile;
  }
  auto global_dotfile = std::filesystem::path("/etc/ura/init.lua");
  if (std::filesystem::is_regular_file(global_dotfile)) {
    return global_dotfile;
  }
  return {};
}

std::expected<void, std::string> Lua::load_config() {
  auto runtime = std::filesystem::path("/usr/share/ura/_runtime");

  if (!std::filesystem::is_directory(runtime))
    return std::unexpected("runtime not found");

  api::core::prepend_package_path("/usr/share/ura/?/init.lua");
  api::core::prepend_package_path("/usr/share/ura/?.lua");
  auto result = this->execute("require('_runtime')");
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
