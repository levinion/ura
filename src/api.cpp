#include <cstdint>
#include <regex>
#include <sol/forward.hpp>
#include <unordered_set>
#include "ura/core/log.hpp"
#include "ura/core/lua.hpp"
#include "ura/core/server.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
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
#include "ura/util/flexible.hpp"
#include "ura/core/state.hpp"
#include <fnmatch.h>

namespace ura::api::core {

void set_keymap(std::string pattern, std::string mode, flexible::function f) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (id) {
    server->state->keymaps[mode][id.value()] = f;
  }
}

void unset_keymap(std::string pattern, std::string mode) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (!id)
    return;
  if (!server->state->keymaps.contains(mode))
    return;
  if (!server->state->keymaps[mode].contains(id.value()))
    return;
  server->state->keymaps[mode].erase(id.value());
}

void set_keymap_mode(std::string mode) {
  auto server = UraServer::get_instance();
  server->state->keymap_mode = mode;
}

std::string get_keymap_mode() {
  auto server = UraServer::get_instance();
  return server->state->keymap_mode;
}

void terminate() {
  UraServer::get_instance()->terminate();
}

void close_window(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
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

// TODO: need a output here
void create_indexed_workspace() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  output->create_workspace();
}

void create_named_workspace(std::string name) {
  auto server = UraServer::get_instance();
  server->view->create_named_workspace(name);
}

void switch_workspace(uint64_t id) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  auto workspace = UraWorkspace::from(id);
  output->switch_workspace(workspace);
}

void move_window_to_workspace(uint64_t id, uint64_t workspace_id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  auto target = UraWorkspace::from(workspace_id);
  if (!target)
    return;
  toplevel->move_to_workspace(target);
}

int get_current_workspace_index() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return -1;
  return output->current_workspace()->index();
}

void set_hook(std::string name, flexible::function f) {
  auto server = UraServer::get_instance();
  server->state->hooks[name] = f;
}

void unset_hook(std::string name) {
  auto server = UraServer::get_instance();
  if (server->state->hooks.contains(name))
    server->state->hooks.erase(name);
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

void set_cursor_shape(std::string name) {
  auto server = UraServer::get_instance();
  server->seat->cursor->set_xcursor(name);
}

void focus_window(uint64_t id) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  server->seat->focus(toplevel);
}

void destroy_workspace(uint64_t id) {
  auto workspace = UraWorkspace::from(id);
  auto output = workspace->output();
  if (workspace)
    output->destroy_workspace(workspace);
}

std::optional<uint64_t> get_current_window() {
  auto server = UraServer::get_instance();
  auto toplevel = server->seat->focused_toplevel();
  if (!toplevel)
    return {};
  return toplevel->id();
}

bool is_cursor_visible() {
  auto server = UraServer::get_instance();
  return server->seat->cursor->visible;
}

std::optional<uint64_t> get_current_workspace() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return {};
  auto workspace = output->current_workspace();
  return workspace->id();
}

std::optional<uint64_t> get_window(uint64_t workspace_id, int index) {
  auto workspace = UraWorkspace::from(workspace_id);
  if (!workspace)
    return {};
  auto toplevel = workspace->get_toplevel_at(index);
  if (!toplevel)
    return {};
  return toplevel->id();
}

std::optional<uint64_t> get_indexed_workspace(uint64_t output_id, int index) {
  auto output = UraOutput::from(output_id);
  if (!output)
    return {};
  auto workspace = output->get_workspace_at(index);
  if (!workspace)
    return {};
  return workspace->id();
}

std::optional<uint64_t> get_named_workspace(std::string name) {
  auto server = UraServer::get_instance();
  auto workspace = server->view->get_named_workspace(name);
  if (!workspace)
    return {};
  return workspace->id();
}

void activate_window(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->activate();
}

void move_window(uint64_t id, int x, int y) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->move(x, y);
}

void resize_window(uint64_t id, int width, int height) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->resize(width, height);
}

std::optional<uint64_t> get_current_output() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return {};
  return output->id();
}

void append_package_path(std::string path) {
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

void prepend_package_path(std::string path) {
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

void set_output_dpms(uint64_t id, bool flag) {
  auto output = UraOutput::from(id);
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

void set_window_draggable(uint64_t id, bool flag) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->draggable = flag;
}

void set_window_z_index(uint64_t id, int z) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_z_index(z);
}

void swap_window(uint64_t id, uint64_t target) {
  auto first = UraToplevel::from(id);
  if (!first)
    return;
  auto second = UraToplevel::from(target);
  if (!second)
    return;
  if (first->workspace != second->workspace)
    return;
  first->workspace->swap_toplevel(first, second);
}

std::optional<uint64_t> get_output(std::string name) {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(name);
  if (!output)
    return {};
  return output->id();
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

void schedule(flexible::function f, int64_t time) {
  auto server = UraServer::get_instance();
  if (time < 0)
    return;
  else if (time == 0) {
    f({});
    return;
  }
  server->dispatcher->schedule_task(
    [=]() {
      f({});
      return true;
    },
    time
  );
}

std::optional<int> get_window_z_index(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->z_index;
}

std::optional<bool> is_window_draggable(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->draggable;
}

std::string get_cursor_theme() {
  return UraServer::get_instance()->seat->cursor->get_theme();
}

int get_cursor_size() {
  return UraServer::get_instance()->seat->cursor->get_size();
}

std::string get_cursor_shape() {
  return UraServer::get_instance()->seat->cursor->xcursor_name;
}

void set_pointer_accel_profile(std::string profile, std::string glob) {
  auto server = UraServer::get_instance();
  for (auto pointer : server->seat->pointers) {
    auto name = pointer->name();
    if (name && !fnmatch(glob.c_str(), name.value().c_str(), 0))
      pointer->set_accel_profile(profile);
  }
}

void set_pointer_move_speed(double speed, std::string glob) {
  auto server = UraServer::get_instance();
  for (auto pointer : server->seat->pointers) {
    auto name = pointer->name();
    if (name && !fnmatch(glob.c_str(), name.value().c_str(), 0))
      pointer->move_speed = speed;
  }
}

void set_pointer_scroll_speed(double speed, std::string glob) {
  auto server = UraServer::get_instance();
  for (auto pointer : server->seat->pointers) {
    auto name = pointer->name();
    if (name && !fnmatch(glob.c_str(), name.value().c_str(), 0))
      pointer->scroll_speed = speed;
  }
}

flexible::object get_output_logical_geometry(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->logical_geometry().to_flexible();
}

flexible::object get_output_usable_geometry(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->usable_area.to_flexible();
}

std::optional<float> get_output_scale(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->scale();
}

std::optional<int> get_window_index(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->index();
}

std::optional<int> get_workspace_index(uint64_t id) {
  auto workspace = UraWorkspace::from(id);
  if (!workspace)
    return {};
  return workspace->index();
}

std::optional<std::string> get_window_app_id(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->app_id();
}

std::optional<std::string> get_window_title(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->title();
}

void set_window_fullscreen(uint64_t id, bool flag) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_fullscreen(flag);
}

std::optional<bool> is_window_fullscreen(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->is_fullscreen();
}

flexible::object get_window_geometry(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->geometry.to_flexible();
}

std::optional<bool> is_workspace_named(uint64_t id) {
  auto workspace = UraWorkspace::from(id);
  if (!workspace)
    return {};
  return workspace->name.has_value();
}

std::optional<std::string> get_workspace_name(uint64_t id) {
  auto workspace = UraWorkspace::from(id);
  if (!workspace)
    return {};
  return workspace->name;
}

std::optional<uint64_t> get_window_workspace(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->workspace->id();
}

std::optional<uint64_t> get_window_output(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  auto output = toplevel->workspace->output();
  if (!output)
    return {};
  return output->id();
}

void set_option(std::string key, flexible::object obj) {
  auto server = UraServer::get_instance();
  server->state->set_option(key, obj);
}

flexible::object get_option(std::string key) {
  auto server = UraServer::get_instance();
  return server->state->get_option<flexible::object>(key).value_or(
    flexible::nil()
  );
}

void set_userdata(uint64_t id, flexible::object obj) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id))
    server->globals[id].userdata = flexible::to_json(obj);
}

flexible::object get_userdata(uint64_t id) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id))
    return flexible::from(server->globals[id].userdata);
  return {};
}

std::string to_json(flexible::object obj) {
  return flexible::to_json(obj)
    .dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore);
}

flexible::object parse_json(std::string str) {
  return flexible::from_str(str);
}

flexible::object get_windows(uint64_t workspace_id) {
  auto workspace = UraWorkspace::from(workspace_id);
  if (!workspace)
    return {};
  auto table = flexible::create_table();
  for (auto toplevel : workspace->toplevels) table.add(toplevel->id());
  return table;
}

flexible::object get_workspaces() {
  auto server = UraServer::get_instance();
  auto table = flexible::create_table();
  for (auto& workspace : server->view->workspaces) table.add(workspace->id());
  return table;
}

flexible::object get_indexed_workspaces(uint64_t output_id) {
  auto output = UraOutput::from(output_id);
  if (!output)
    return {};
  auto table = flexible::create_table();
  for (auto workspace : output->get_workspaces()) table.add(workspace->id());
  return table;
}

flexible::object get_named_workspaces() {
  auto server = UraServer::get_instance();
  auto table = flexible::create_table();
  for (auto& [name, workspace] : server->view->named_workspaces)
    table.set(name, workspace->id());
  return table;
}

void eval(std::string code) {
  auto server = UraServer::get_instance();
  auto result = server->lua->execute(code);
  // TODO: handle result
}

void set_workspace_scale(uint64_t id, double scale) {
  auto workspace = UraWorkspace::from(id);
  if (workspace)
    workspace->set_scale(scale);
}

void insert_window(uint64_t id, uint64_t target) {
  auto first = UraToplevel::from(id);
  if (!first)
    return;
  auto second = UraToplevel::from(target);
  if (!second)
    return;
  if (first->workspace != second->workspace)
    return;
  first->workspace->insert_toplevel(first, second);
}

} // namespace ura::api::core

namespace ura::api::lua {

// redirect print result to buffer
void print(sol::variadic_args args) {
  auto server = UraServer::get_instance();
  auto& state = server->lua->state;
  if (args.size() == 0) {
    server->lua->lua_stdout += '\n';
    return;
  }
  std::string r;
  for (std::size_t i = 0; i < args.size() - 1; i++) {
    r += state["tostring"](args[i]).get<std::string>() + ' ';
  }
  r += state["tostring"](args[args.size() - 1]).get<std::string>();
  r.push_back('\n');
  server->lua->lua_stdout += r;
}

} // namespace ura::api::lua
