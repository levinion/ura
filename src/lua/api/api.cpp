#include "ura/lua/api.hpp"
#include <regex>
#include <sol/forward.hpp>
#include <unordered_set>
#include "ura/core/log.hpp"
#include "ura/lua/hook.hpp"
#include "ura/lua/lua.hpp"
#include "ura/core/server.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/seat/keyboard.hpp"
#include "ura/view/workspace.hpp"
#include "ura/util/util.hpp"
#include "ura/seat/seat.hpp"
#include <fcntl.h>
#include <libinput.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/prctl.h>

namespace ura::api {

void keymap_set(std::string pattern, sol::protected_function f) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (id)
    server->lua->keymaps["normal"][id.value()] = f;
}

void keymap_set_mode(
  std::string mode,
  std::string pattern,
  sol::protected_function f
) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (id)
    server->lua->keymaps[mode][id.value()] = f;
}

void keymap_unset(std::string pattern) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (!id)
    return;
  if (!server->lua->keymaps.contains("normal"))
    return;
  if (!server->lua->keymaps["normal"].contains(id.value()))
    return;
  server->lua->keymaps["normal"].erase(id.value());
}

void keymap_unset_mode(std::string mode, std::string pattern) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (!id)
    return;
  if (!server->lua->keymaps.contains(mode))
    return;
  if (!server->lua->keymaps[mode].contains(id.value()))
    return;
  server->lua->keymaps[mode].erase(id.value());
}

void keymap_enter_mode(std::string mode) {
  auto server = UraServer::get_instance();
  server->lua->mode = mode;
}

std::string keymap_get_current_mode() {
  auto server = UraServer::get_instance();
  return server->lua->mode;
}

void terminate() {
  UraServer::get_instance()->terminate();
}

void close_window(int index) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (toplevel) {
    toplevel->close();
  }
}

void reload() {
  auto server = UraServer::get_instance();
  server->lua->reset = true;
}

void set_keyboard_repeat(int rate, int delay) {
  auto server = UraServer::get_instance();
  for (auto keyboard : server->seat->keyboards)
    keyboard->set_repeat(rate, delay);
}

void set_env(std::string name, std::string value) {
  setenv(name.data(), value.data(), true);
}

void unset_env(std::string name) {
  unsetenv(name.data());
}

void create_workspace() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  output->create_workspace();
}

void switch_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  output->switch_workspace(index);
}

void switch_or_create_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  for (int i = output->get_workspaces().size(); i <= index; i++)
    output->create_workspace();
  output->switch_workspace(index);
}

void move_window_to_workspace(int window_index, sol::object workspace_id) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  auto workspace = output->current_workspace;
  auto toplevel = workspace->get_toplevel_at(window_index);
  if (!toplevel)
    return;
  if (workspace_id.is<int>()) {
    auto workspace_index = workspace_id.as<int>();
    toplevel->move_to_workspace(workspace_index);
  } else if (workspace_id.is<std::string>()) {
    auto workspace_name = workspace_id.as<std::string>();
    toplevel->move_to_workspace(workspace_name);
  }
}

void move_window_to_workspace_or_create(int window_index, int workspace_index) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  auto workspace = output->current_workspace;
  auto toplevel = workspace->get_toplevel_at(window_index);
  if (!toplevel)
    return;
  for (int i = output->get_workspaces().size(); i <= workspace_index; i++) {
    output->create_workspace();
  }
  toplevel->move_to_workspace(workspace_index);
}

int get_current_workspace_index() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  return output->current_workspace->index();
}

void set_hook(std::string name, sol::protected_function f, sol::object obj) {
  UraPluginHook p;
  if (obj.is<sol::table>()) {
    auto table = obj.as<sol::table>();
    p.group =
      table["group"].get<std::optional<std::string>>().value_or("group");
    p.priority = table["priority"].get<std::optional<int>>().value_or(1);
  }
  p.callback = f;
  auto server = UraServer::get_instance();
  server->lua->hooks[name].insert(std::move(p));
}

void set_cursor_theme(sol::object obj) {
  if (!obj.is<sol::table>())
    return;
  auto table = obj.as<sol::table>();
  auto server = UraServer::get_instance();
  auto theme =
    server->lua->fetch<std::string>(table, "theme").value_or("default");
  auto size = server->lua->fetch<int>(table, "size");
  if (!size)
    return;
  server->seat->cursor->set_theme(theme, size.value());
}

void set_cursor_visible(bool flag) {
  auto server = UraServer::get_instance();
  if (!flag)
    server->seat->cursor->hide();
  else
    server->seat->cursor->show();
}

void set_cursor_shape(std::string name) {
  auto server = UraServer::get_instance();
  server->seat->cursor->set_xcursor(name);
}

void focus_window(int index) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  if (index < 0 || index >= workspace->toplevels.size())
    return;
  auto toplevel = workspace->toplevels.get(index);
  if (!toplevel)
    return;
  server->seat->focus(*toplevel);
}

void set_window_layout(int index, std::string layout) {
  auto server = UraServer::get_instance();
  auto toplevel =
    server->view->current_output()->current_workspace->get_toplevel_at(index);
  if (toplevel) {
    toplevel->set_layout(layout);
    toplevel->redraw_all_others();
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
  auto output = server->view->current_output();
  return output->get_workspaces().size();
}

int get_window_number() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  auto workspace = output->current_workspace;
  return workspace->toplevels.size();
}

void destroy_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  output->destroy_workspace(index);
}

std::optional<sol::table> get_current_window() {
  auto server = UraServer::get_instance();
  auto toplevel = server->seat->focused_toplevel();
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
  auto workspace = server->view->current_output()->current_workspace;
  return workspace->to_lua_table();
}

std::optional<sol::table> get_window(int index) {
  auto server = UraServer::get_instance();
  auto toplevel =
    server->view->current_output()->current_workspace->get_toplevel_at(index);
  if (!toplevel)
    return {};
  return toplevel->to_lua_table();
}

std::optional<sol::table> get_workspace(sol::object id) {
  auto server = UraServer::get_instance();
  UraWorkSpace* workspace = nullptr;
  if (id.is<int>())
    workspace = server->view->current_output()->get_workspace_at(id.as<int>());
  else if (id.is<std::string>())
    workspace = server->view->get_named_workspace(id.as<std::string>());
  if (!workspace)
    return {};
  return workspace->to_lua_table();
}

void activate_window(sol::object workspace_id, int window_index) {
  auto server = UraServer::get_instance();
  UraWorkSpace* workspace = nullptr;
  if (workspace_id.is<int>())
    workspace =
      server->view->current_output()->get_workspace_at(workspace_id.as<int>());
  else if (workspace_id.is<std::string>())
    workspace = server->view->get_named_workspace_or_create(
      workspace_id.as<std::string>()
    );
  if (!workspace)
    return;
  auto toplevel = workspace->get_toplevel_at(window_index);
  if (!toplevel)
    return;
  toplevel->activate();
}

sol::table list_workspaces() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  auto table = server->lua->state.create_table();
  for (auto& workspace : output->get_workspaces())
    table.add(workspace->to_lua_table());
  for (auto& [_, workspace] : server->view->named_workspaces)
    table.add(workspace->to_lua_table());
  return table;
}

void move_window(int index, int x, int y) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel->move(x, y);
}

void resize_window(int index, int width, int height) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel->resize(width, height);
  toplevel->redraw(false);
}

void center_window(int index) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel->center();
}

std::optional<sol::table> get_current_output() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (output)
    return output->to_lua_table();
  return {};
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
  auto set = std::unordered_set(paths.begin(), paths.end());
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
  auto set = std::unordered_set(paths.begin(), paths.end());
  if (set.contains(path))
    return;
  paths.insert(paths.begin(), path);
  auto result = join(paths, ';');
  package.value().set("path", result);
}

std::string expanduser(std::string path) {
  if (!path.starts_with("~"))
    return path;
  std::string userhome;
  auto i = path.find('/', 1);
  if (i == std::string::npos) {
    i = path.size();
  }
  if (i == 1) {
    auto home = getenv("HOME");
    if (home) {
      userhome = home;
    } else {
      auto pwent = getpwuid(getuid());
      if (!pwent)
        return path;
      userhome = pwent->pw_dir;
    }
  } else {
    auto username = path.substr(1, i - 1);
    auto pwent = getpwnam(username.data());
    if (!pwent)
      return path;
    userhome = pwent->pw_dir;
  }
  // rstrip '/'
  while (!userhome.empty() && userhome.back() == '/') {
    userhome.pop_back();
  }
  path = userhome + path.substr(i);
  return path.empty() ? "/" : path;
}

std::string expandvars(std::string path) {
  std::regex re(
    "\\$([a-zA-Z_][a-zA-Z0-9_]*)|\\$\\{([a-zA-Z_][a-zA-Z0-9_]*)\\}"
  );
  std::string result = "";
  std::smatch match;
  auto search_start = path.cbegin();

  while (std::regex_search(search_start, path.cend(), match, re)) {
    result += match.prefix().str();
    auto var_name = match[1].matched ? match[1].str() : match[2].str();
    auto var_value = getenv(var_name.c_str());
    if (var_value) {
      result += var_value;
    }
    search_start = match.suffix().first;
  }
  result += std::string(search_start, path.cend());
  return result;
}

std::string expand(std::string path) {
  return expanduser(expandvars(path));
}

void set_output_dpms(std::string name, bool flag) {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(name);
  if (output)
    output->set_dpms_mode(flag);
}

void notify_idle_activity() {
  auto server = UraServer::get_instance();
  server->seat->notify_idle_activity();
}

void set_idle_inhibitor(bool flag) {
  auto server = UraServer::get_instance();
  server->seat->set_idle_inhibitor(flag);
}

void set_window_draggable(int index, bool flag) {
  auto server = UraServer::get_instance();
  auto toplevel =
    server->view->current_output()->current_workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel->draggable = flag;
}

void set_layout(std::string name, sol::protected_function f) {
  auto server = UraServer::get_instance();
  server->lua->layouts[name] = f;
}

void unset_layout(std::string name) {
  auto server = UraServer::get_instance();
  if (server->lua->layouts.contains(name)) {
    server->lua->layouts.erase(name);
  }
}

void set_window_z_index(int index, int z) {
  auto server = UraServer::get_instance();
  auto toplevel =
    server->view->current_output()->current_workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel->set_z_index(z);
}

void swap_window(int index, int target) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  auto src = workspace->get_toplevel_at(index);
  auto dst = workspace->get_toplevel_at(target);
  if (!src || !dst)
    return;
  workspace->swap_toplevel(src, dst);
  src->redraw(false);
  dst->redraw(false);
}

void redraw_window(int index) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return;
  toplevel->redraw(false);
}

void redraw_current_workspace() {
  auto server = UraServer::get_instance();
  auto workspace = server->view->current_output()->current_workspace;
  workspace->redraw();
}

std::optional<sol::table> get_output(std::string name) {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(name);
  if (output)
    return output->to_lua_table();
  return {};
}

void set_output_mode(std::string name, sol::object obj) {
  if (!obj.is<sol::table>())
    return;
  auto mode = obj.as<sol::table>();
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(name);
  if (output)
    output->set_mode(mode);
}

void spawn(std::string cmd) {
  pid_t pid = fork();

  if (pid < 0)
    return;

  if (pid == 0) {
    // try redirecting stdout and stderr to /dev/null to avoid log pollution
    int dev_null = open("/dev/null", O_WRONLY);
    if (dev_null != -1) {
      dup2(dev_null, STDOUT_FILENO);
      dup2(dev_null, STDERR_FILENO);
      close(dev_null);
    }
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    if (getppid() == 1)
      exit(1);
    execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
  }
}

void notify(std::string summary, std::string body) {
  log::notify(summary, body);
}

void set_pointer_properties(std::string pattern, sol::object obj) {
  if (!obj.is<sol::table>())
    return;
  auto properties = obj.as<sol::table>();
  auto server = UraServer::get_instance();
  for (auto pointer : server->seat->match_pointers(pattern)) {
    pointer->set_properties(properties);
  }
}

void schedule(sol::protected_function f, int64_t time) {
  if (time < 0)
    return;
  else if (time == 0) {
    f();
    return;
  }
  auto server = UraServer::get_instance();
  server->dispatcher->schedule_task(
    [=]() {
      f();
      return true;
    },
    time
  );
}

} // namespace ura::api
