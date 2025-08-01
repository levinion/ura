#include "ura/lua.hpp"
#include "ura/api.hpp"
#include "ura/server.hpp"
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <sol/forward.hpp>
#include <sol/property.hpp>
#include <sol/state_handling.hpp>
#include <string>

namespace ura {

void check_reset() {
  auto server = UraServer::get_instance();
  if (server->lua->reset) {
    server->lua = Lua::init();
    server->lua->setup();
    server->lua->try_execute_init();
    server->lua->try_execute_hook("reload");
  }
}

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
  // window
  this->set("win.focus", api::focus_window);
  this->set("win.close", api::close_window);
  this->set("win.set_floating", api::set_window_floating);
  this->set("win.is_floating", api::is_window_floating);
  this->set("win.set_fullscreen", api::set_window_fullscreen);
  this->set("win.is_fullscreen", api::is_window_fullscreen);
  this->set("win.move_to_workspace", api::move_window_to_workspace);
  this->set("win.move_to_scratchpad", api::move_window_to_scratchpad);
  this->set("win.index", api::get_current_window_index);
  this->set("win.size", api::get_window_number);
  this->set("win.border.width", 1);
  this->set("win.border.active_color", "#89b4fa");
  this->set("win.border.inactive_color", "#00000000");
  // input
  this->set("input.keyboard.set_repeat", api::set_keyboard_repeat);
  this->set("input.cursor.focus_follow_mouse", true);
  this->set("input.cursor.set_theme", api::set_cursor_theme);
  this->set("input.cursor.set_visible", api::set_cursor_visible);
  this->set("input.cursor.absolute_move", api::cursor_absolute_move);
  this->set("input.cursor.relative_move", api::cursor_relative_move);
  this->set("input.cursor.set_shape", api::set_cursor_shape);
  // workspace
  this->set("ws.switch", api::switch_workspace);
  this->set("ws.destroy", api::destroy_workspace);
  this->set("ws.index", api::get_current_workspace_index);
  this->set("ws.size", api::get_workspace_number);
  // layout
  this->set("layout.tilling.gap.outer.top", 10);
  this->set("layout.tilling.gap.outer.left", 10);
  this->set("layout.tilling.gap.outer.bottom", 10);
  this->set("layout.tilling.gap.outer.right", 10);
  this->set("layout.tilling.gap.inner", 10);
  this->set("layout.floating.default.width", 800);
  this->set("layout.floating.default.height", 600);
  // keymap
  this->set("keymap.set", api::set_keymap);
  // hook
  this->set("hook.set", api::set_hook);
  // fn
  this->set("fn.set_env", api::set_env);
  // global
  this->set("g.hooks", sol::table {});
  this->set("g.keymaps", sol::table {});

  // override
  this->state.set("print", api::lua_print);
}

std::expected<std::string, std::string> Lua::execute(std::string script) {
  this->lua_stdout.clear();
  auto result = this->state.safe_script(script, sol::script_pass_on_error);
  if (result.valid()) {
    auto out = this->lua_stdout;
    check_reset();
    return out;
  }
  sol::error err = result;
  return std::unexpected(err.what());
}

std::expected<std::string, std::string>
Lua::execute_file(std::filesystem::path path) {
  this->lua_stdout.clear();
  if (!std::filesystem::is_regular_file(path))
    return std::unexpected(
      std::format("[ura] path not exists or invalid: {}", path.string())
    );
  auto result = this->state.safe_script_file(path, sol::script_pass_on_error);
  if (result.valid()) {
    auto out = this->lua_stdout;
    check_reset();
    return out;
  }
  sol::error err = result;
  return std::unexpected(err.what());
}

bool Lua::try_execute_hook(std::string name) {
  auto server = UraServer::get_instance();
  auto hook =
    this->fetch<sol::protected_function>(std::format("g.hooks.{}", name));
  if (hook) {
    wlr_log(WLR_DEBUG, "running hook: %s", name.data());
    hook.value()();
    return true;
  }
  return false;
}

bool Lua::try_execute_keybinding(uint64_t id) {
  auto server = UraServer::get_instance();
  auto keybinding =
    this->fetch<sol::protected_function>(std::format("g.keymaps.{}", id));
  if (keybinding) {
    keybinding.value()();
    return true;
  }
  return false;
}

std::optional<std::filesystem::path> Lua::find_init_path() {
  auto server = UraServer::get_instance();
  auto root = std::getenv("XDG_CONFIG_HOME")
    ? std::filesystem::path(std::getenv("XDG_CONFIG_HOME"))
    : std::filesystem::path(std::getenv("HOME")) / ".config";
  auto dotfile = std::filesystem::path(root) / "ura/init.lua";
  if (std::filesystem::is_regular_file(dotfile))
    return dotfile;
  return {};
}

bool Lua::try_execute_init() {
  auto path = this->find_init_path();
  if (path) {
    // TODO: handle error
    auto _ = this->execute_file(path.value());
    return true;
  }
  return false;
}

} // namespace ura
