#include "ura/lua/lua.hpp"
#include "ura/core/log.hpp"
#include "ura/lua/api.hpp"
#include "ura/view/layout.hpp"
#include "ura/view/view.hpp"
#include "ura/core/server.hpp"
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <sol/forward.hpp>
#include <sol/property.hpp>
#include <sol/state_handling.hpp>
#include <string>

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
  this->set("api.terminate", api::terminate);
  this->set("api.reload", api::reload);
  this->set("api.spawn", api::spawn);
  this->set("api.notify_idle_activity", api::notify_idle_activity);
  this->set("api.set_idle_inhibitor", api::set_idle_inhibitor);
  this->set("api.notify", api::notify);
  // window
  this->set("win.focus", api::focus_window);
  this->set("win.close", api::close_window);
  this->set("win.move_to_workspace", api::move_window_to_workspace);
  this->set(
    "win.move_to_workspace_or_create",
    api::move_window_to_workspace_or_create
  );
  this->set("win.size", api::get_window_number);
  this->set("win.get_current", api::get_current_window);
  this->set("win.get", api::get_window);
  this->set("win.activate", api::activate_window);
  this->set("win.set_layout", api::set_window_layout);
  this->set("win.set_z_index", api::set_window_z_index);
  this->set("win.move", api::move_window);
  this->set("win.resize", api::resize_window);
  this->set("win.center", api::center_window);
  this->set("win.set_draggable", api::set_window_draggable);
  this->set("win.swap", api::swap_window);
  this->set("win.redraw", api::redraw_window);
  // input
  this->set("input.keyboard.set_repeat", api::set_keyboard_repeat);
  this->set("input.cursor.set_theme", api::set_cursor_theme);
  this->set("input.cursor.set_visible", api::set_cursor_visible);
  this->set("input.cursor.is_visible", api::is_cursor_visible);
  this->set("input.cursor.set_shape", api::set_cursor_shape);
  // workspace
  this->set("ws.create", api::create_workspace);
  this->set("ws.switch", api::switch_workspace);
  this->set("ws.switch_or_create", api::switch_or_create_workspace);
  this->set("ws.destroy", api::destroy_workspace);
  this->set("ws.size", api::get_workspace_number);
  this->set("ws.get_current", api::get_current_workspace);
  this->set("ws.get", api::get_workspace);
  this->set("ws.list", api::list_workspaces);
  this->set("ws.redraw", api::redraw_current_workspace);
  // output
  this->set("output.get_current", api::get_current_output);
  this->set("output.get", api::get_output);
  this->set("output.set_dpms", api::set_output_dpms);
  this->set("output.set_mode", api::set_output_mode);
  // layout
  this->set("layout.set", api::set_layout);
  this->set("layout.unset", api::unset_layout);
  // keymap
  this->set("keymap.set", api::keymap_set);
  this->set("keymap.set_mode", api::keymap_set_mode);
  this->set("keymap.unset", api::keymap_unset);
  this->set("keymap.unset_mode", api::keymap_unset_mode);
  this->set("keymap.enter_mode", api::keymap_enter_mode);
  this->set("keymap.get_current_mode", api::keymap_get_current_mode);
  // hook
  this->set("hook.set", api::set_hook);
  // fn
  this->set("fn.set_env", api::set_env);
  this->set("fn.unset_env", api::unset_env);
  this->set("fn.append_package_path", api::append_lua_package_path);
  this->set("fn.prepend_package_path", api::prepend_lua_package_path);
  this->set("fn.expanduser", api::expanduser);
  this->set("fn.expandvars", api::expandvars);
  this->set("fn.expand", api::expand);
  // opt
  this->set("opt.border_width", 1);
  this->set("opt.active_border_color", "#89b4fa");
  this->set("opt.inactive_border_color", "#00000000");
  this->set("opt.focus_follow_mouse", true);
  this->set("opt.tilling.gap.outer.top", 10);
  this->set("opt.tilling.gap.outer.left", 10);
  this->set("opt.tilling.gap.outer.bottom", 10);
  this->set("opt.tilling.gap.outer.right", 10);
  this->set("opt.tilling.gap.inner", 10);
  this->set("opt.mouse_scroll_factor", 1.);
  this->set("opt.mouse_move_factor", 1.);
  this->set("opt.output", this->state.create_table());
  // override
  this->state.set("print", api::lua_print);
  // g
  this->set("g", this->state.create_table());

  this->layouts["tiling"] = this->create_protected_function(layout::tiling);
  this->layouts["fullscreen"] =
    this->create_protected_function(layout::fullscreen);
  this->layouts["floating"] = this->create_protected_function(layout::floating);
}

std::expected<std::string, std::string> Lua::execute(std::string script) {
  this->lua_stdout.clear();
  auto result = this->state.safe_script(script, sol::script_pass_on_error);
  if (result.valid())
    return this->lua_stdout;
  sol::error err = result;
  return std::unexpected(err.what());
}

// this is only used to run init.lua by now
std::expected<std::string, std::string>
Lua::execute_file(std::filesystem::path path) {
  this->lua_stdout.clear();
  this->cache.clear();
  if (!std::filesystem::is_regular_file(path))
    return std::unexpected(
      std::format("[ura] path not exists or invalid: {}", path.string())
    );
  auto result = this->state.safe_script_file(path, sol::script_pass_on_error);
  if (result.valid())
    return this->lua_stdout;
  sol::error err = result;
  return std::unexpected(err.what());
}

bool Lua::try_execute_keybinding(uint64_t id) {
  auto server = UraServer::get_instance();
  if (!this->contains_keybinding(id))
    return false;
  this->keymaps[mode][id]();
  return true;
}

std::optional<std::filesystem::path> Lua::find_init_path() {
  auto server = UraServer::get_instance();
  auto root = std::getenv("XDG_CONFIG_HOME")
    ? std::filesystem::path(std::getenv("XDG_CONFIG_HOME"))
    : std::filesystem::path(std::getenv("HOME")) / ".config";
  auto dotfile = std::filesystem::path(root) / "ura/init.lua";
  if (std::filesystem::is_regular_file(dotfile))
    return dotfile;
  auto global_dotfile = std::filesystem::path("/etc/ura/init.lua");
  if (std::filesystem::is_regular_file(global_dotfile))
    return global_dotfile;
  return {};
}

void Lua::try_execute_init() {
  auto path = this->find_init_path();
  if (!path)
    return;
  auto result = this->execute_file(path.value());
  if (!result) {
    log::error("{}", result.error().c_str());
    UraServer::get_instance()->terminate();
  }
}

bool Lua::contains_keybinding(uint64_t id) {
  auto server = UraServer::get_instance();
  if (!this->keymaps.contains(this->mode))
    return false;
  if (!this->keymaps[mode].contains(id))
    return false;
  return true;
}

} // namespace ura
