#include "ura/api.hpp"
#include <set>
#include "ura/lua.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/toplevel.hpp"
#include "ura/keyboard.hpp"
#include "ura/workspace.hpp"
#include "ura/util.hpp"
#include "ura/seat.hpp"

namespace ura::api {

void set_keymap(std::string pattern, sol::protected_function f) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (id)
    server->lua->keymaps[id.value()] = f;
}

void terminate() {
  UraServer::get_instance()->terminate();
}

void close_window(int index) {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  auto client = workspace->get_toplevel_at(index);
  if (client) {
    auto toplevel = client.value();
    if (toplevel)
      toplevel->close();
  }
}

void reload() {
  auto server = UraServer::get_instance();
  server->lua->reset = true;
}

void set_keyboard_repeat(int rate, int delay) {
  auto server = UraServer::get_instance();
  auto keyboard = server->current_keyboard();
  keyboard->set_repeat(rate, delay);
}

void set_env(std::string name, std::string value) {
  setenv(name.data(), value.data(), true);
}

void unset_env(std::string name) {
  unsetenv(name.data());
}

// switch to certain workspace, if not exists then create one
void switch_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  if (index < 0)
    return;
  if (index >= output->workspaces.size())
    index = output->create_workspace()->index();
  output->switch_workspace(index);
}

void move_window_to_workspace(int window_index, int workspace_index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto workspace = output->current_workspace;
  auto client = workspace->get_toplevel_at(window_index);
  if (!client)
    return;
  auto toplevel = client.value();
  if (workspace_index < -1)
    return;
  // workspace_index -1 always means scratchpad
  if (workspace_index == -1) {
    toplevel->move_to_scratchpad();
    return;
  }
  // create if not exists
  if (workspace_index >= output->workspaces.size())
    workspace_index = output->create_workspace()->index();
  auto result = toplevel->move_to_workspace(workspace_index);
  if (result)
    workspace_index = result.value();
  output->switch_workspace(workspace_index);
}

int get_current_workspace_index() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  return output->current_workspace->index();
}

void set_hook(std::string name, sol::protected_function f) {
  auto server = UraServer::get_instance();
  server->lua->hooks[name] = f;
}

void set_cursor_theme(std::string theme, int size) {
  auto server = UraServer::get_instance();
  server->seat->cursor->set_theme(theme, size);
}

void set_cursor_visible(bool flag) {
  auto server = UraServer::get_instance();
  if (!flag)
    server->seat->cursor->hide();
  else
    server->seat->cursor->show();
}

void cursor_absolute_move(double x, double y) {
  auto server = UraServer::get_instance();
  server->seat->cursor->absolute_move(x, y);
}

void cursor_relative_move(double delta_x, double delta_y) {
  auto server = UraServer::get_instance();
  server->seat->cursor->relative_move(delta_x, delta_y);
}

void set_cursor_shape(std::string name) {
  auto server = UraServer::get_instance();
  server->seat->cursor->set_xcursor(name);
}

bool focus_window(int index) {
  auto server = UraServer::get_instance();
  if (index < 0
      || index >= server->current_output()->current_workspace->toplevels.size())
    return false;
  auto it = server->current_output()->current_workspace->toplevels.begin();
  std::advance(it, index);
  server->seat->focus(*it);
  return true;
}

void set_window_fullscreen(int index, bool flag) {
  auto server = UraServer::get_instance();
  auto client =
    server->current_output()->current_workspace->get_toplevel_at(index);
  if (client) {
    auto toplevel = client.value();
    if (toplevel && toplevel->fullscreen() != flag) {
      toplevel->set_fullscreen(flag);
      toplevel->request_commit();
    }
  }
}

void set_window_floating(int index, bool flag) {
  auto server = UraServer::get_instance();
  auto client =
    server->current_output()->current_workspace->get_toplevel_at(index);
  if (client) {
    auto toplevel = client.value();
    if (toplevel && toplevel->floating != flag) {
      toplevel->set_float(flag);
      toplevel->request_commit();
    }
  }
}

// redirect print result to buffer
void lua_print(sol::variadic_args args) {
  auto server = UraServer::get_instance();
  auto& state = server->lua->state;
  if (args.size() == 0) {
    server->lua->lua_stdout += '\n';
    return;
  }
  std::string r;
  for (int i = 0; i < args.size() - 1; i++) {
    r += state["tostring"](args[i]).get<std::string>() + ' ';
  }
  r += state["tostring"](args[args.size() - 1]).get<std::string>();
  r.push_back('\n');
  server->lua->lua_stdout += r;
}

int get_workspace_number() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  return output->workspaces.size();
}

int get_window_number() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto workspace = output->current_workspace;
  return workspace->toplevels.size();
}

void destroy_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  output->destroy_workspace(index);
}

std::optional<sol::table> get_current_window() {
  auto server = UraServer::get_instance();
  auto toplevel = server->seat->focused();
  if (!toplevel)
    return {};
  return toplevel->to_lua_table();
}

bool is_cursor_visible() {
  auto server = UraServer::get_instance();
  return server->seat->cursor->visible;
}

sol::table get_current_workspace() {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  return workspace->to_lua_table();
}

std::optional<sol::table> get_window(int index) {
  auto server = UraServer::get_instance();
  auto client =
    server->current_output()->current_workspace->get_toplevel_at(index);
  if (!client)
    return {};
  auto toplevel = client.value();
  return toplevel->to_lua_table();
}

// index == -1 means scratchpad
std::optional<sol::table> get_workspace(int index) {
  auto server = UraServer::get_instance();
  if (index == -1)
    return server->scratchpad->to_lua_table();
  if (index < -1)
    return {};
  auto workspace = server->current_output()->get_workspace_at(index);
  if (!workspace)
    return {};
  return workspace->to_lua_table();
}

void activate_window(int workspace_index, int window_index) {
  auto server = UraServer::get_instance();
  UraWorkSpace* workspace = nullptr;
  if (workspace_index == -1)
    workspace = server->scratchpad.get();
  else
    workspace = server->current_output()->get_workspace_at(workspace_index);
  if (!workspace)
    return;
  auto toplevel = workspace->get_toplevel_at(window_index);
  if (!toplevel)
    return;
  toplevel.value()->activate();
}

sol::table list_workspaces() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto table = server->lua->state.create_table();
  for (auto& workspace : output->workspaces) {
    table.add(workspace->to_lua_table());
  }
  return table;
}

void move_floating_window(int index, int x, int y) {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel.value()->floating_geometry.x = x;
  toplevel.value()->floating_geometry.y = y;
}

void resize_floating_window(int index, int width, int height) {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel.value()->floating_geometry.width = width;
  toplevel.value()->floating_geometry.height = height;
}

void center_floating_window(int index) {
  auto server = UraServer::get_instance();
  auto workspace = server->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel.value()->center(
    toplevel.value()->floating_geometry.width,
    toplevel.value()->floating_geometry.height
  );
}

sol::table get_current_output() {
  auto server = UraServer::get_instance();
  return server->current_output()->to_lua_table();
}

void append_lua_package_path(std::string path) {
  auto server = UraServer::get_instance();
  auto package = server->lua->state.get<std::optional<sol::table>>("package");
  if (!package)
    return;
  auto package_path = package.value().get<std::optional<std::string>>("path");
  if (!package_path)
    return;
  auto paths = split(package_path.value(), ';');
  auto set = std::set(paths.begin(), paths.end());
  if (set.contains(path))
    return;
  paths.push_back(path);
  auto result = join(paths, ';');
  package.value().set("path", result);
}

void prepend_lua_package_path(std::string path) {
  auto server = UraServer::get_instance();
  auto package = server->lua->state.get<std::optional<sol::table>>("package");
  if (!package)
    return;
  auto package_path = package.value().get<std::optional<std::string>>("path");
  if (!package_path)
    return;
  auto paths = split(package_path.value(), ';');
  auto set = std::set(paths.begin(), paths.end());
  if (set.contains(path))
    return;
  paths.insert(paths.begin(), path);
  auto result = join(paths, ';');
  package.value().set("path", result);
}

} // namespace ura::api
