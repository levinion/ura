#include "ura/core/lua.hpp"
#include "ura/api.hpp"
#include "ura/core/log.hpp"
#include "ura/core/server.hpp"
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <sol/forward.hpp>
#include <sol/property.hpp>
#include <sol/state_handling.hpp>
#include <string>
#include "ura/util/util.hpp"
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

#define LUAAPI(name, f) flexible::set(lua->ura, name, f)

  // api
  LUAAPI("api.terminate", api::core::terminate);
  LUAAPI("api.spawn", api::core::spawn);
  LUAAPI("api.notify_idle_activity", api::core::notify_idle_activity);
  LUAAPI("api.set_idle_inhibitor", api::core::set_idle_inhibitor);
  LUAAPI("api.notify", api::core::notify);
  LUAAPI("api.set_timer", api::core::set_timer);
  LUAAPI("api.clear_timer", api::core::clear_timer);
  // window
  LUAAPI("api.focus_window", api::core::focus_window);
  LUAAPI("api.close_window", api::core::close_window);
  LUAAPI("api.get_current_window", api::core::get_current_window);
  LUAAPI("api.activate_window", api::core::activate_window);
  LUAAPI("api.set_window_z_index", api::core::set_window_z_index);
  LUAAPI("api.get_window_z_index", api::core::get_window_z_index);
  LUAAPI("api.move_window", api::core::move_window);
  LUAAPI("api.resize_window", api::core::resize_window);
  LUAAPI("api.set_window_draggable", api::core::set_window_draggable);
  LUAAPI("api.is_window_draggable", api::core::is_window_draggable);
  LUAAPI("api.get_window_app_id", api::core::get_window_app_id);
  LUAAPI("api.get_window_title", api::core::get_window_title);
  LUAAPI("api.set_window_fullscreen", api::core::set_window_fullscreen);
  LUAAPI("api.is_window_fullscreen", api::core::is_window_fullscreen);
  LUAAPI("api.get_window_geometry", api::core::get_window_geometry);
  LUAAPI("api.get_window_output", api::core::get_window_output);
  LUAAPI("api.set_window_tags", api::core::set_window_tags);
  LUAAPI("api.get_window_tags", api::core::get_window_tags);
  LUAAPI("api.get_all_windows", api::core::get_all_windows);
  LUAAPI("api.is_window_mapped", api::core::is_window_mapped);
  LUAAPI("api.is_window_focused", api::core::is_window_focused);
  // input
  LUAAPI("api.set_keyboard_repeat", api::core::set_keyboard_repeat);
  LUAAPI("api.set_cursor_theme", api::core::set_cursor_theme);
  LUAAPI("api.set_cursor_visible", api::core::set_cursor_visible);
  LUAAPI("api.get_cursor_visible", api::core::is_cursor_visible);
  LUAAPI("api.set_cursor_shape", api::core::set_cursor_shape);
  // output
  LUAAPI("api.get_current_output", api::core::get_current_output);
  LUAAPI("api.get_output", api::core::get_output);
  LUAAPI("api.get_all_outputs", api::core::get_all_outputs);
  LUAAPI("api.get_output_name", api::core::get_output_name);
  LUAAPI("api.set_output_dpms", api::core::set_output_dpms);
  LUAAPI("api.set_output_tags", api::core::set_output_tags);
  LUAAPI("api.get_output_tags", api::core::get_output_tags);
  LUAAPI(
    "api.get_output_logical_geometry",
    api::core::get_output_logical_geometry
  );
  LUAAPI(
    "api.get_output_usable_geometry",
    api::core::get_output_usable_geometry
  );
  LUAAPI("api.get_output_scale", api::core::get_output_scale);
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
  // userdata
  LUAAPI("api.set_userdata", api::core::set_userdata);
  LUAAPI("api.get_userdata", api::core::get_userdata);
  // util
  LUAAPI("api.get_keybinding_id", api::core::get_keybinding_id);
  // override
  lua->state.set("print", api::lua::print);

  return lua;
}

void Lua::load_runtime() {
  api::core::prepend_package_path("/usr/share/ura/runtime/lua/?/init.lua");
  api::core::prepend_package_path("/usr/share/ura/runtime/lua/?.lua");
  auto result = this->execute("require('ura')");
  if (!result) {
    log::error("{}", result.error());
    exit(1);
  }
}

std::expected<std::string, std::string> Lua::execute(std::string_view script) {
  this->lua_stdout.clear();
  auto result = this->state.safe_script(script, sol::script_pass_on_error);
  if (result.valid())
    return std::string(trim(this->lua_stdout));
  sol::error err = result;
  return std::unexpected(err.what());
}

std::expected<std::string, std::string> Lua::execute_file(std::string_view p) {
  this->lua_stdout.clear();
  auto path = std::filesystem::path(p);
  if (!std::filesystem::is_regular_file(path))
    return std::unexpected(
      std::format("[ura] path not exists or invalid: {}", path.string())
    );
  auto result = this->state.safe_script_file(path, sol::script_pass_on_error);
  if (result.valid())
    return std::string(trim(this->lua_stdout));
  sol::error err = result;
  return std::unexpected(std::string(path) + ": " + std::string(err.what()));
}

bool Lua::emit_keybinding(uint64_t id) {
  auto f = this->ura["keymap"]["_keymaps"][id]
             .get<std::optional<sol::protected_function>>();
  if (f) {
    f.value()();
    return true;
  }
  return false;
}

bool Lua::contains_keybinding(uint64_t id) {
  return this->ura["keymap"]["_keymaps"][id]
    .get<std::optional<sol::protected_function>>()
    .has_value();
}

void Lua::emit_hook(std::string name, flexible::object args) {
  auto f = this->ura["hook"]["_hooks"][name]
             .get<std::optional<sol::protected_function>>();
  if (f) {
    f.value()(args);
  }
}

void Lua::set_option(std::string_view key, flexible::object value) {
  auto opt = this->ura["opt"].get<std::optional<sol::table>>();
  if (opt) {
    opt.value().set(key, value);
  }
}

} // namespace ura
