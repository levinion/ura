#include "ura/lua/api.hpp"
#include <cstdint>
#include <regex>
#include <sol/forward.hpp>
#include <unordered_set>
#include <variant>
#include "ura/core/log.hpp"
#include "ura/lua/lua.hpp"
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
#include "flexible/flexible.hpp"

namespace ura::api {

void set_keymap(std::string pattern, std::string f, std::string mode) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (id) {
    auto result = server->lua->load_as_function(f);
    if (result)
      server->lua->keymaps[mode][id.value()] = result.value();
  }
}

void unset_keymap(std::string pattern, std::string mode) {
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

void set_keymap_mode(std::string mode) {
  auto server = UraServer::get_instance();
  server->lua->mode = mode;
}

std::string get_keymap_mode() {
  auto server = UraServer::get_instance();
  return server->lua->mode;
}

void terminate() {
  UraServer::get_instance()->terminate();
}

void close_window(uint64_t id) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  auto workspace = output->current_workspace;
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

void create_workspace() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  output->create_workspace();
}

void switch_workspace(uint64_t id) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  auto workspace = UraWorkSpace::from(id);
  output->switch_workspace(workspace);
}

void move_window_to_workspace(uint64_t id, uint64_t workspace_id) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  auto workspace = output->current_workspace;
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  auto target = UraWorkSpace::from(workspace_id);
  if (!target)
    return;
  toplevel->move_to_workspace(workspace);
}

int get_current_workspace_index() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return -1;
  return output->current_workspace->index();
}

void set_hook(std::string name, std::string f) {
  auto server = UraServer::get_instance();
  auto func = server->lua->load_as_function(f);
  if (func)
    server->lua->hooks[name] = func.value();
}

void unset_hook(std::string name) {
  auto server = UraServer::get_instance();
  if (server->lua->hooks.contains(name))
    server->lua->hooks.erase(name);
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

void set_window_layout(uint64_t id, std::string layout) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_layout(layout);
  toplevel->redraw_all_others();
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
  if (!output)
    return -1;
  auto workspaces = output->get_workspaces();
  return workspaces.size();
}

int get_window_number() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return -1;
  auto workspace = output->current_workspace;
  return workspace->toplevels.size();
}

void destroy_workspace(uint64_t id) {
  auto server = UraServer::get_instance();
  auto workspace = UraWorkSpace::from(id);
  auto output = UraOutput::from(workspace->output);
  if (workspace)
    output->destroy_workspace(workspace);
}

uint64_t get_current_window() {
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

uint64_t get_current_workspace() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return {};
  auto workspace = output->current_workspace;
  return workspace->id();
}

uint64_t get_window(int index) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return {};
  auto toplevel = output->current_workspace->get_toplevel_at(index);
  if (!toplevel)
    return {};
  return toplevel->id();
}

uint64_t get_workspace(std::variant<int, std::string> obj) {
  auto server = UraServer::get_instance();
  UraWorkSpace* workspace = nullptr;
  if (std::holds_alternative<int>(obj)) {
    auto output = server->view->current_output();
    if (!output)
      return {};
    workspace = output->get_workspace_at(std::get<int>(obj));
  } else if (std::holds_alternative<std::string>(obj))
    workspace = server->view->get_named_workspace(std::get<std::string>(obj));
  if (!workspace)
    return {};
  return workspace->id();
}

void activate_window(uint64_t id) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->activate();
}

void move_window(uint64_t id, int x, int y) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->move(x, y);
}

void resize_window(uint64_t id, int width, int height) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->resize(width, height);
}

uint64_t get_current_output() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return {};
  return output->id();
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

void set_output_dpms(uint64_t id, bool flag) {
  auto server = UraServer::get_instance();
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
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->draggable = flag;
}

void set_layout(std::string name, std::string f) {
  auto server = UraServer::get_instance();
  auto func = server->lua->load_as_function(f);
  if (func)
    server->lua->layouts[name] = func.value();
}

void unset_layout(std::string name) {
  auto server = UraServer::get_instance();
  if (server->lua->layouts.contains(name)) {
    server->lua->layouts.erase(name);
  }
}

void set_window_z_index(uint64_t id, int z) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_z_index(z);
}

void swap_window(uint64_t id, uint64_t target) {
  auto server = UraServer::get_instance();
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

void redraw_window(uint64_t id) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->redraw(false);
}

void redraw_current_workspace() {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (!output)
    return;
  auto workspace = output->current_workspace;
  workspace->redraw();
}

uint64_t get_output(std::string name) {
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

void set_pointer_properties(std::string pattern, flexible::object obj) {
  if (!obj.is<flexible::table>())
    return;
  auto server = UraServer::get_instance();
  for (auto pointer : server->seat->match_pointers(pattern)) {
    pointer->set_properties(obj);
  }
}

void schedule(std::string f, int64_t time) {
  auto server = UraServer::get_instance();
  auto func = server->lua->load_as_function(f);
  if (!func)
    return;
  if (time < 0)
    return;
  else if (time == 0) {
    func.value()();
    return;
  }
  server->dispatcher->schedule_task(
    [=]() {
      func.value()();
      return true;
    },
    time
  );
}
} // namespace ura::api
