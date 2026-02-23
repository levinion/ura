#include "ura/api.hpp"
#include <chrono>
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
#include "ura/util/keybinding.hpp"
#include "ura/seat/seat.hpp"
#include <absl/strings/str_split.h>
#include <absl/strings/str_join.h>
#include <fcntl.h>
#include <libinput.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/prctl.h>
#include "ura/util/flexible.hpp"
#include <fnmatch.h>

namespace ura::api::core {

void terminate() {
  UraServer::get_instance()->terminate();
}

void close_window(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (toplevel) {
    toplevel->close();
  }
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

void set_cursor_theme(std::string theme, int size) {
  auto server = UraServer::get_instance();
  server->seat->cursor->set_theme(theme, size);
}

void set_cursor_visible(bool flag) {
  auto server = UraServer::get_instance();
  server->seat->cursor->set_visible(flag);
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
  std::vector<std::string> paths = absl::StrSplit(package_path.value(), ';');
  auto set = std::unordered_set(paths.begin(), paths.end());
  if (set.contains(path))
    return;
  paths.push_back(path);
  std::string result = absl::StrJoin(paths, ";");
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
  std::vector<std::string> paths = absl::StrSplit(package_path.value(), ';');
  auto set = std::unordered_set(paths.begin(), paths.end());
  if (set.contains(path))
    return;
  paths.insert(paths.begin(), path);
  std::string result = absl::StrJoin(paths, ";");
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

void set_window_z_index(uint64_t id, int z) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_z_index(z);
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

std::optional<int> set_timeout(flexible::function f, int64_t timeout) {
  auto server = UraServer::get_instance();
  if (timeout <= 0) {
    f();
    return {};
  }
  return server->dispatcher->set_timeout(
    [=]() { f(); },
    std::chrono::milliseconds(timeout)
  );
}

void clear_timeout(int fd) {
  auto server = UraServer::get_instance();
  server->dispatcher->clear_timeout(fd);
}

std::optional<int> get_window_z_index(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->z_index;
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

flexible::object get_output_logical_geometry(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->logical_geometry().to_table();
}

flexible::object get_output_usable_geometry(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->usable_area.to_table();
}

std::optional<float> get_output_scale(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->scale();
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
  return toplevel->geometry.to_table();
}

std::optional<uint64_t> get_window_output(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  auto output = toplevel->output();
  if (!output)
    return {};
  return output->id();
}

void set_userdata(uint64_t id, flexible::object obj) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id))
    server->globals[id].userdata = obj;
}

flexible::object get_userdata(uint64_t id) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id))
    return server->globals[id].userdata;
  return {};
}

std::string to_json(flexible::object obj) {
  return flexible::to_json(obj)
    .dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore);
}

flexible::object parse_json(std::string str) {
  return flexible::from_str(str);
}

void set_output_tags(uint64_t id, std::vector<std::string> tags) {
  auto output = UraOutput::from(id);
  if (!output)
    return;
  Vec<std::string> v(tags.begin(), tags.end());
  output->set_tags(std::move(v));
}

void set_window_tags(uint64_t id, std::vector<std::string> tags) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  Vec<std::string> v(tags.begin(), tags.end());
  toplevel->set_tags(std::move(v));
}

flexible::object get_output_tags(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->tags.to_table();
}

flexible::object get_window_tags(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->tags.to_table();
}

flexible::object get_all_windows() {
  auto server = UraServer::get_instance();
  auto table = flexible::create_table();
  for (auto toplevel : server->view->toplevels) {
    table.add(toplevel->id());
  }
  return table;
}

std::optional<std::string> get_output_name(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->name;
}

flexible::object get_all_outputs() {
  auto server = UraServer::get_instance();
  auto table = flexible::create_table();
  for (auto [_, output] : server->view->outputs) {
    table.add(output->id());
  }
  return table;
}

std::optional<bool> is_window_mapped(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->mapped();
}

std::optional<bool> is_window_focused(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->is_focused();
}

std::optional<uint64_t> get_keybinding_id(std::string pattern) {
  return util::get_keybinding_id(pattern);
}

std::optional<uint64_t> get_window_lru(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->lru;
}

flexible::object get_cursor_pos() {
  auto server = UraServer::get_instance();
  return server->seat->cursor->position().to_table();
}

long time_since_epoch() {
  return std::chrono::steady_clock::now().time_since_epoch().count();
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
