#include "ura/api.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
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
#include "ura/util/string.hpp"
#include "ura/seat/seat.hpp"
#include <fcntl.h>
#include <libinput.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/prctl.h>
#include "ura/util/flexible.hpp"
#include "ura/render/effect_registry.hpp"
#include "ura/util/image.hpp"
#include "ura/ext_workspace.hpp"

#include "ura/scene/animation_system.hpp"
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
  server->seat->cursor->set_visible(flag, true);
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

sol::optional<uint64_t> get_current_window() {
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

namespace {

  flexible::object rect_table(const Rect<int>& rect) {
    auto table = flexible::create_table();
    table.set("x", rect.x);
    table.set("y", rect.y);
    table.set("width", rect.width);
    table.set("height", rect.height);
    return table;
  }

  ura::EasingCurve easing_curve(const flexible::object& value) {
    if (value.valid() && value.is<std::string>())
      return ura::EasingCurve::from_name(value.as<std::string>());

    if (value.valid() && value.is<sol::table>()) {
      auto table = value.as<sol::table>();
      std::vector<float> samples;
      samples.reserve(ura::EasingCurve::sample_count);
      for (int i = 1; i <= static_cast<int>(ura::EasingCurve::sample_count);
           ++i) {
        auto sample = table.get<std::optional<float>>(i);
        if (!sample)
          break;
        samples.push_back(*sample);
      }
      if (!samples.empty())
        return ura::EasingCurve::from_samples(samples);
    }

    return ura::EasingCurve::from_name("in_out_cubic");
  }

  float
  table_number(const sol::table& table, std::string_view key, float fallback) {
    auto value = table.get<std::optional<float>>(std::string(key));
    return value.value_or(fallback);
  }

  ura::AnimationOptions animation_options(
    double duration,
    const flexible::object& easing,
    bool animate_opacity = false,
    float opacity = 1.f
  ) {
    ura::AnimationOptions options;
    auto milliseconds = std::llround(std::max(0.0, duration));
    options.duration = std::chrono::milliseconds(milliseconds);
    options.easing = easing_curve(easing);
    options.animate_opacity = animate_opacity;
    options.opacity = opacity;
    return options;
  }

} // namespace

void move_window(uint64_t id, int x, int y) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  auto options = animation_options(0, flexible::object {});
  toplevel->move(x, y, options);
}

void animate_window_move(
  uint64_t id,
  int x,
  int y,
  double duration,
  flexible::object easing
) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->move(x, y, animation_options(duration, easing));
}

void resize_window(uint64_t id, int width, int height) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  auto options = animation_options(0, flexible::object {});
  toplevel->resize(width, height, options);
}

void animate_window_resize(
  uint64_t id,
  int width,
  int height,
  double duration,
  flexible::object easing
) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->resize(width, height, animation_options(duration, easing));
}

void animate_window_rotation(
  uint64_t id,
  double degrees,
  double duration,
  flexible::object easing
) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel || !server->view || !server->view->animations)
    return;

  constexpr double degrees_to_radians = 3.14159265358979323846 / 180.0;
  auto options = animation_options(duration, easing);
  options.animate_rotation = true;
  options.rotation = static_cast<float>(degrees * degrees_to_radians);
  auto bounds = server->view->scene->node_bounds(toplevel->entity);
  server->view->animations
    ->animate_from(toplevel->entity, bounds, bounds, options);
}

sol::optional<double> get_window_rotation(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  constexpr double radians_to_degrees = 180.0 / 3.14159265358979323846;
  auto* node = UraServer::get_instance()->view->scene->try_get<SceneNode>(
    toplevel->entity
  );
  if (!node)
    return {};
  return static_cast<double>(node->rotation) * radians_to_degrees;
}

void animate_window_opacity(
  uint64_t id,
  float opacity,
  double duration,
  flexible::object easing
) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->animate_opacity(
    opacity,
    animation_options(duration, easing, true, opacity)
  );
}

void animate_window_open(
  uint64_t id,
  double duration,
  flexible::object easing,
  flexible::object options
) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;

  float from_scale = 0.95f;
  float from_opacity = 0.f;
  float from_rotation = 0.f;
  float to_scale = 1.f;
  float to_opacity = 1.f;
  float to_rotation = 0.f;
  if (options.valid() && options.is<sol::table>()) {
    auto table = options.as<sol::table>();
    from_scale = table_number(table, "from_scale", from_scale);
    from_opacity = table_number(table, "from_opacity", from_opacity);
    from_rotation = table_number(table, "from_rotation", from_rotation);
    to_scale = table_number(table, "to_scale", to_scale);
    to_opacity = table_number(table, "to_opacity", to_opacity);
    to_rotation = table_number(table, "to_rotation", to_rotation);
  }

  constexpr double degrees_to_radians = 3.14159265358979323846 / 180.0;
  toplevel->animate_open(
    animation_options(duration, easing),
    from_scale,
    from_opacity,
    static_cast<float>(from_rotation * degrees_to_radians),
    to_scale,
    to_opacity,
    static_cast<float>(to_rotation * degrees_to_radians)
  );
}

void animate_window_close(
  uint64_t id,
  double duration,
  flexible::object easing,
  flexible::object options
) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;

  float to_scale = 0.95f;
  float to_opacity = 0.f;
  float to_rotation = 0.f;
  if (options.valid() && options.is<sol::table>()) {
    auto table = options.as<sol::table>();
    to_scale = table_number(table, "scale", to_scale);
    to_scale = table_number(table, "to_scale", to_scale);
    to_opacity = table_number(table, "opacity", to_opacity);
    to_opacity = table_number(table, "to_opacity", to_opacity);
    to_rotation = table_number(table, "rotation", to_rotation);
    to_rotation = table_number(table, "to_rotation", to_rotation);
  }

  constexpr double degrees_to_radians = 3.14159265358979323846 / 180.0;
  auto close_options = animation_options(duration, easing);
  close_options.animate_rotation = std::abs(to_rotation) > 1e-6;
  close_options.rotation = static_cast<float>(to_rotation * degrees_to_radians);
  toplevel->request_close_animation(close_options, to_scale, to_opacity);
}

sol::optional<uint64_t> get_current_output() {
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
  auto paths = util::split(package_path.value(), ';');
  auto set = std::unordered_set(paths.begin(), paths.end());
  if (set.contains(path))
    return;
  paths.push_back(path);
  std::string result = util::join(paths, ";");
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
  auto paths = util::split(package_path.value(), ';');
  auto set = std::unordered_set(paths.begin(), paths.end());
  if (set.contains(path))
    return;
  paths.insert(paths.begin(), path);
  std::string result = util::join(paths, ";");
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

void notify_idle_activity() {
  auto server = UraServer::get_instance();
  server->seat->notify_idle_activity();
}

void set_idle_inhibitor(bool flag) {
  auto server = UraServer::get_instance();
  server->seat->set_idle_inhibitor(flag);
}

void add_idle_timeout(
  std::string name,
  double seconds,
  sol::optional<flexible::function> on_idle,
  sol::optional<flexible::function> on_resume
) {
  auto server = UraServer::get_instance();
  if (seconds <= 0) {
    server->seat->remove_idle_timeout(name);
    return;
  }
  auto ms = static_cast<int64_t>(seconds * 1000.0);
  std::optional<sol::protected_function> idle_callback;
  std::optional<sol::protected_function> resume_callback;
  if (on_idle)
    idle_callback = *on_idle;
  if (on_resume)
    resume_callback = *on_resume;
  server->seat->add_idle_timeout(
    std::move(name),
    std::chrono::milliseconds(ms),
    std::move(idle_callback),
    std::move(resume_callback)
  );
}

void remove_idle_timeout(std::string name) {
  UraServer::get_instance()->seat->remove_idle_timeout(name);
}

void clear_idle_timeouts() {
  UraServer::get_instance()->seat->clear_idle_timeouts();
}

bool is_idle() {
  return UraServer::get_instance()->seat->is_idle();
}

void set_window_z_index(uint64_t id, int z) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_z_index(z);
}

sol::optional<uint64_t> get_output(std::string name) {
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

sol::optional<int> set_timeout(flexible::function f, double timeout) {
  auto server = UraServer::get_instance();
  if (timeout <= 0) {
    f();
    return {};
  }
  std::chrono::duration<double, std::milli> duration(timeout);
  return server->dispatcher->set_timeout(
    [=]() { f(); },
    std::chrono::round<std::chrono::nanoseconds>(duration)
  );
}

void clear_timeout(int fd) {
  auto server = UraServer::get_instance();
  server->dispatcher->clear_timer(fd);
}

sol::optional<int> set_interval(flexible::function f, double interval) {
  auto server = UraServer::get_instance();
  if (interval <= 0) {
    f();
    return {};
  }
  std::chrono::duration<double, std::milli> duration(interval);
  return server->dispatcher->set_interval(
    [=]() { f(); },
    std::chrono::round<std::chrono::nanoseconds>(duration)
  );
}

void clear_interval(int fd) {
  auto server = UraServer::get_instance();
  server->dispatcher->clear_timer(fd);
}

sol::optional<int> get_window_z_index(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->z_order().layer;
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
  return rect_table(output->logical_geometry());
}

flexible::object get_output_usable_geometry(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return rect_table(output->usable_area);
}

sol::optional<float> get_output_scale(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return output->scale();
}

void set_output_dpms(uint64_t id, bool flag) {
  auto output = UraOutput::from(id);
  if (!output)
    return;
  output->set_dpms_mode(flag);
}

sol::optional<std::string> get_window_app_id(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->app_id();
}

sol::optional<std::string> get_window_title(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->title();
}

sol::optional<int> get_window_pid(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->pid();
}

sol::optional<std::string> get_window_icon_name(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  auto name = toplevel->icon_name();
  if (name.empty())
    return {};
  return name;
}

flexible::object get_window_icon(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};

  auto name = toplevel->icon_name();
  auto icon_buffers = toplevel->icon_buffers();
  if (name.empty() && icon_buffers.empty())
    return {};

  auto table = flexible::create_table();
  if (!name.empty())
    table.set("name", name);

  auto buffers = flexible::create_table();
  for (auto& values : icon_buffers) {
    auto buffer = flexible::create_table();
    buffer.set("width", values[0]);
    buffer.set("height", values[1]);
    buffer.set("scale", values[2]);
    buffers.add(buffer);
  }
  table.set("buffers", buffers);
  return table;
}

void set_window_fullscreen(uint64_t id, bool flag) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_fullscreen(flag);
}

sol::optional<bool> is_window_fullscreen(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->is_fullscreen();
}

sol::optional<bool> is_window_fullscreen_requested(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->xdg_toplevel->requested.fullscreen;
}

void set_window_resizing(uint64_t id, bool flag) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_resizing(flag);
}

sol::optional<bool> is_window_resizing(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->is_resizing();
}

void set_window_maximized(uint64_t id, bool flag) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_maximized(flag);
}

sol::optional<bool> is_window_maximized(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->is_maximized();
}

sol::optional<bool> is_window_maximized_requested(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->xdg_toplevel->requested.maximized;
}

flexible::object get_window_geometry(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  auto target = toplevel->layout().target;
  auto box = Rect<int> {
    static_cast<int>(std::lround(target.x)),
    static_cast<int>(std::lround(target.y)),
    static_cast<int>(std::lround(target.width)),
    static_cast<int>(std::lround(target.height)),
  };
  return rect_table(box);
}

flexible::object get_window_visual_geometry(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  auto bounds = toplevel->node().bounds;
  auto table = flexible::create_table();
  table.set("x", bounds.x);
  table.set("y", bounds.y);
  table.set("width", bounds.width);
  table.set("height", bounds.height);
  return table;
}

sol::optional<uint64_t> get_window_output(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  auto output = toplevel->output();
  if (!output)
    return {};
  return output->id();
}

void set_userdata(uint64_t id, flexible::object obj) {
  UraServer::get_instance()->entities->set_userdata(id, obj);
}

flexible::object get_userdata(uint64_t id) {
  return UraServer::get_instance()->entities->userdata(id);
}

std::string to_json(flexible::object obj) {
  return ura::util::stringify_json(flexible::to_json(obj));
}

flexible::object parse_json(std::string str) {
  return flexible::from_str(str);
}

namespace {

  std::vector<Tag> parse_tags(const std::vector<std::string>& values) {
    std::set<Tag, TagLess> unique;
    for (auto& value : values) {
      if (auto tag = Tag::parse_or_default(value))
        unique.insert(std::move(*tag));
    }
    return { unique.begin(), unique.end() };
  }

  flexible::object tag_table(const std::vector<Tag>& tags) {
    auto table = flexible::create_table();
    for (auto& tag : tags) table.add(tag.name());
    return table;
  }

  flexible::object tag_descriptor(const Tag& tag) {
    auto table = flexible::create_table();
    table.set("name", tag.name());
    table.set("namespace", tag.name_space);
    table.set("index", tag.index);
    return table;
  }

  std::vector<Tag> all_tags() {
    std::set<Tag, TagLess> values;
    auto* server = UraServer::get_instance();
    if (server->view) {
      for (auto& [_, output] : server->view->outputs) {
        if (output)
          values.insert(output->tags.begin(), output->tags.end());
      }
      for (auto* toplevel : server->view->toplevels) {
        if (toplevel)
          values.insert(
            toplevel->tags().values.begin(),
            toplevel->tags().values.end()
          );
      }
    }
    return { values.begin(), values.end() };
  }

  std::vector<std::string> namespace_names() {
    std::set<std::string> names;
    for (auto& tag : all_tags()) names.insert(tag.name_space);
    std::vector<std::string> sorted(names.begin(), names.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& lhs, auto& rhs) {
      return TagLess {}(Tag { .name_space = lhs }, Tag { .name_space = rhs });
    });
    return sorted;
  }

  std::optional<Tag> active_tag_for(std::string_view name_space) {
    auto* server = UraServer::get_instance();
    if (!server->view)
      return {};

    std::optional<Tag> best;
    uint64_t best_lru = 0;
    for (auto& tag : all_tags()) {
      if (tag.name_space != name_space)
        continue;
      for (auto* toplevel : server->view->toplevels) {
        if (!toplevel || !toplevel->tags().contains(tag))
          continue;
        auto lru = toplevel->lru;
        if (!best || lru > best_lru) {
          best = tag;
          best_lru = lru;
        }
      }
    }
    return best;
  }

  std::pair<UraOutput*, std::optional<Tag>> current_output_tag() {
    auto* server = UraServer::get_instance();
    if (!server->view)
      return { nullptr, {} };
    auto* output = server->view->current_output();
    if (!output || output->tags.empty())
      return { output, {} };
    return { output, output->tags.front() };
  }

  bool set_output_tag(UraOutput* output, const Tag& tag) {
    if (!output)
      return false;
    output->set_tags({ tag });
    return true;
  }

} // namespace

void set_output_tags(uint64_t id, std::vector<std::string> tags) {
  auto output = UraOutput::from(id);
  if (!output)
    return;
  output->set_tags(parse_tags(tags));
}

void set_window_tags(uint64_t id, std::vector<std::string> tags) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_tags(parse_tags(tags));
}

flexible::object get_output_tags(uint64_t id) {
  auto output = UraOutput::from(id);
  if (!output)
    return {};
  return tag_table(output->tags);
}

flexible::object get_window_tags(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return tag_table(toplevel->tags().values);
}

flexible::object get_tag(std::string name) {
  auto tag = Tag::parse_or_default(name);
  if (!tag)
    return {};
  return tag_descriptor(*tag);
}

flexible::object get_tags() {
  auto table = flexible::create_table();
  for (auto& tag : all_tags()) table.add(tag_descriptor(tag));
  return table;
}

flexible::object get_namespaces() {
  auto table = flexible::create_table();
  for (auto& name : namespace_names()) table.add(name);
  return table;
}

flexible::object get_active_tag(std::string name_space) {
  auto tag = active_tag_for(name_space);
  if (!tag)
    return {};
  return tag_descriptor(*tag);
}

flexible::object get_current_tag() {
  auto [output, tag] = current_output_tag();
  (void)output;
  if (!tag)
    return {};
  return tag_descriptor(*tag);
}

bool set_current_tag(std::string name) {
  auto tag = Tag::parse_or_default(name);
  if (!tag)
    return false;
  auto [output, current] = current_output_tag();
  (void)current;
  return set_output_tag(output, *tag);
}

bool shift_tag_index(int delta) {
  auto [output, tag] = current_output_tag();
  if (!output || !tag)
    return false;
  auto index = tag->index + delta;
  if (index < 1)
    return false;
  return set_output_tag(
    output,
    Tag { .name_space = tag->name_space, .index = index }
  );
}

bool set_tag_index(int index) {
  if (index < 1)
    return false;
  auto [output, tag] = current_output_tag();
  if (!output || !tag)
    return false;
  return set_output_tag(
    output,
    Tag { .name_space = tag->name_space, .index = index }
  );
}

bool shift_window_tag_index(int delta, bool update_output) {
  auto* server = UraServer::get_instance();
  auto* window = server->seat ? server->seat->focused_toplevel() : nullptr;
  auto [output, tag] = current_output_tag();
  if (!window || !output || !tag)
    return false;

  auto index = tag->index + delta;
  if (index < 1)
    return false;
  Tag next { .name_space = tag->name_space, .index = index };
  window->set_tags({ next });
  if (update_output)
    set_output_tag(output, next);
  return true;
}

bool switch_tag_namespace(int delta) {
  auto [output, tag] = current_output_tag();
  if (!output || !tag)
    return false;
  auto namespaces = namespace_names();
  auto it = std::ranges::find(namespaces, tag->name_space);
  if (it == namespaces.end())
    return false;
  auto position = std::distance(namespaces.begin(), it);
  auto target = position + delta;
  if (target < 0 || target >= static_cast<std::ptrdiff_t>(namespaces.size()))
    return false;
  auto active = active_tag_for(namespaces[target]);
  if (!active)
    return false;
  return set_output_tag(output, *active);
}

flexible::object get_all_windows() {
  auto server = UraServer::get_instance();
  auto table = flexible::create_table();
  for (auto toplevel : server->view->toplevels) {
    table.add(toplevel->id());
  }
  return table;
}

sol::optional<std::string> get_output_name(uint64_t id) {
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

sol::optional<bool> is_window_mapped(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->mapped();
}

sol::optional<bool> is_window_focused(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->is_focused();
}

sol::optional<uint64_t> get_keybinding_id(std::string pattern) {
  if (auto id = util::get_keybinding_id(pattern); id.has_value()) {
    return id.value();
  } else {
    return {};
  }
}

sol::optional<uint64_t> get_window_lru(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->lru;
}

flexible::object get_cursor_pos() {
  auto server = UraServer::get_instance();
  auto [x, y] = server->seat->cursor->position();
  auto table = flexible::create_table();
  table.set("x", x);
  table.set("y", y);
  return table;
}

long time_since_epoch() {
  return std::chrono::steady_clock::now().time_since_epoch().count();
}

sol::optional<float> get_window_opacity(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->opacity().value;
}

void set_window_opacity(uint64_t id, float opacity) {
  if (opacity < 0 || opacity > 1)
    return;
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_opacity(opacity);
}

namespace {

  std::optional<EffectUniformType> effect_uniform_type(std::string_view name) {
    if (name == "float")
      return EffectUniformType::Float;
    if (name == "int")
      return EffectUniformType::Int;
    if (name == "bool")
      return EffectUniformType::Bool;
    if (name == "vec2")
      return EffectUniformType::Vec2;
    if (name == "vec3")
      return EffectUniformType::Vec3;
    if (name == "vec4")
      return EffectUniformType::Vec4;
    return {};
  }

  PaintOutset effect_outset(const sol::object& value) {
    PaintOutset outset;
    if (!value.valid() || value.get_type() == sol::type::nil)
      return outset;

    if (value.is<sol::table>()) {
      auto table = value.as<sol::table>();
      if (auto padding = table.get<std::optional<double>>("padding")) {
        auto value = std::max(0, static_cast<int>(std::lround(*padding)));
        outset.left = outset.top = outset.right = outset.bottom = value;
        return outset;
      }
      outset.left =
        std::max(0, table.get<std::optional<int>>("left").value_or(0));
      outset.top =
        std::max(0, table.get<std::optional<int>>("top").value_or(0));
      outset.right =
        std::max(0, table.get<std::optional<int>>("right").value_or(0));
      outset.bottom =
        std::max(0, table.get<std::optional<int>>("bottom").value_or(0));
      return outset;
    }

    if (value.is<int>() || value.is<double>() || value.is<float>()) {
      auto padding =
        std::max(0, static_cast<int>(std::lround(value.as<double>())));
      outset.left = outset.top = outset.right = outset.bottom = padding;
    }
    return outset;
  }

  std::optional<std::string> effect_shader_source(const std::string& spec) {
    if (spec.find('\n') != std::string::npos)
      return spec;

    auto path = api::core::expand(spec);
    std::ifstream file(path);
    if (!file) {
      log::error("EffectInstance shader file cannot be opened: {}", path);
      return {};
    }
    std::ostringstream source;
    source << file.rdbuf();
    if (source.str().empty()) {
      log::error("EffectInstance shader file is empty: {}", path);
      return {};
    }
    return source.str();
  }

  std::optional<std::string> effect_shader_spec(const sol::table& table) {
    if (auto shader = table.get<std::optional<std::string>>("shader"))
      return shader;
    return table.get<std::optional<std::string>>("fragment");
  }

  bool effect_uniform_value(
    const sol::object& value,
    EffectUniformType type,
    EffectUniform& uniform
  ) {
    if (!value.valid() || value.get_type() == sol::type::nil)
      return true;

    switch (type) {
      case EffectUniformType::Float:
        if (!value.is<double>() && !value.is<int>() && !value.is<float>())
          return false;
        uniform.values[0] = static_cast<float>(value.as<double>());
        return true;
      case EffectUniformType::Int:
        if (!value.is<int>() && !value.is<double>() && !value.is<float>())
          return false;
        uniform.values[0] = static_cast<float>(value.as<double>());
        return true;
      case EffectUniformType::Bool:
        if (!value.is<bool>())
          return false;
        uniform.values[0] = value.as<bool>() ? 1.f : 0.f;
        return true;
      case EffectUniformType::Vec2:
      case EffectUniformType::Vec3:
      case EffectUniformType::Vec4: {
        if (!value.is<sol::table>())
          return false;
        auto table = value.as<sol::table>();
        int count = type == EffectUniformType::Vec2 ? 2
          : type == EffectUniformType::Vec3         ? 3
                                                    : 4;
        for (int i = 0; i < count; ++i) {
          auto component = table.get<std::optional<float>>(i + 1);
          if (!component)
            return false;
          uniform.values[i] = *component;
        }
        return true;
      }
    }
    return false;
  }

  std::optional<EffectUniform>
  effect_static_uniform(const std::string& name, const sol::object& value) {
    EffectUniform uniform { .name = name };
    if (value.is<double>() || value.is<int>() || value.is<float>()) {
      uniform.type = EffectUniformType::Float;
      uniform.values[0] = static_cast<float>(value.as<double>());
      return uniform;
    }
    if (value.is<bool>()) {
      uniform.type = EffectUniformType::Bool;
      uniform.values[0] = value.as<bool>() ? 1.f : 0.f;
      return uniform;
    }
    if (!value.is<sol::table>())
      return {};

    auto table = value.as<sol::table>();
    if (auto type = table.get<std::optional<std::string>>("type")) {
      auto parsed_type = effect_uniform_type(*type);
      auto raw = table.get<sol::object>("value");
      if (!parsed_type || !raw.valid())
        return {};
      uniform.type = *parsed_type;
      if (!effect_uniform_value(raw, uniform.type, uniform))
        return {};
      return uniform;
    }

    auto size = table.size();
    if (size < 2 || size > 4)
      return {};
    uniform.type = size == 2 ? EffectUniformType::Vec2
      : size == 3            ? EffectUniformType::Vec3
                             : EffectUniformType::Vec4;
    for (std::size_t i = 0; i < size; ++i) {
      auto component = table.get<std::optional<float>>(i + 1);
      if (!component)
        return {};
      uniform.values[i] = *component;
    }
    return uniform;
  }

} // namespace

flexible::object define_effect(flexible::object spec) {
  auto server = UraServer::get_instance();
  if (!server->effects || !spec.valid() || !spec.is<sol::table>())
    return {};

  auto table = spec.as<sol::table>();
  EffectPipeline pipeline;
  pipeline.name =
    table.get<std::optional<std::string>>("name").value_or("effect");

  std::vector<sol::table> pass_tables;
  bool has_passes = false;
  if (auto passes = table.get<std::optional<sol::table>>("passes")) {
    has_passes = true;
    for (std::size_t i = 1; i <= passes->size(); ++i) {
      if (auto pass = passes->get<std::optional<sol::table>>(i))
        pass_tables.push_back(*pass);
    }
  } else {
    pass_tables.push_back(table);
  }
  if (pass_tables.empty()) {
    log::error("EffectInstance definition is missing a shader pass");
    return {};
  }
  if (pass_tables.size() > 8) {
    log::error("EffectInstance '{}' exceeds the 8-pass limit", pipeline.name);
    return {};
  }

  std::vector<std::string> available_sources { "source", "previous" };
  for (auto& pass_table : pass_tables) {
    auto shader_spec = effect_shader_spec(pass_table);
    if (!shader_spec) {
      log::error("EffectInstance pass is missing a shader");
      return {};
    }
    auto source = effect_shader_source(*shader_spec);
    if (!source)
      return {};

    EffectPass pass;
    pass.id = pass_table.get<std::optional<std::string>>("id").value_or("");
    if (
      !pass.id.empty()
      && (pass.id == "source" || pass.id == "previous"
          || std::ranges::find(available_sources, pass.id)
            != available_sources.end())
    ) {
      log::error(
        "EffectInstance pass id '{}' is reserved or duplicated",
        pass.id
      );
      return {};
    }

    if (auto inputs = pass_table.get<std::optional<sol::table>>("inputs")) {
      for (auto&& [key, value] : *inputs) {
        if (!key.is<std::string>()) {
          log::error("Effect pass input names must be strings");
          return {};
        }
        auto sampler = key.as<std::string>();

        if (value.is<sol::table>()) {
          auto texture = value.as<sol::table>();
          auto id = texture.get<std::optional<uint64_t>>("id");
          auto generation =
            texture.get<std::optional<uint32_t>>("generation").value_or(1);
          auto image = id ? server->effects->get_texture(
                              EffectTextureHandle {
                                .id = *id,
                                .generation = generation,
                              }
                            )
                          : std::shared_ptr<const EffectTexture> {};
          if (!image) {
            log::error("Effect pass references an invalid texture handle");
            return {};
          }
          pass.inputs.push_back(
            EffectPassInput {
              .name = std::move(sampler),
              .source = "texture",
              .texture = std::move(image),
            }
          );
          continue;
        }

        if (!value.is<std::string>()) {
          log::error("Effect pass inputs must map sampler names to sources");
          return {};
        }
        auto source_name = value.as<std::string>();
        if (
          std::ranges::find(available_sources, source_name)
          == available_sources.end()
        ) {
          log::error(
            "Effect pass '{}' references unavailable input '{}'",
            pass.id,
            source_name
          );
          return {};
        }
        pass.inputs.push_back(
          EffectPassInput {
            .name = std::move(sampler),
            .source = std::move(source_name),
            .texture = {},
          }
        );
      }
      std::ranges::sort(pass.inputs, {}, [](const EffectPassInput& input) {
        return input.name;
      });
    }

    pass.fragment_shader = std::move(*source);
    auto uniform_key = has_passes ? "uniforms" : "pass_uniforms";
    if (
      auto uniforms = pass_table.get<std::optional<sol::table>>(uniform_key)
    ) {
      for (auto&& [key, value] : *uniforms) {
        if (!key.is<std::string>())
          continue;
        auto uniform = effect_static_uniform(key.as<std::string>(), value);
        if (!uniform) {
          log::error("Invalid static uniform on effect pass");
          return {};
        }
        pass.uniforms.push_back(std::move(*uniform));
      }
      std::ranges::sort(pass.uniforms, {}, &EffectUniform::name);
    }
    if (!pass.id.empty())
      available_sources.push_back(pass.id);
    pipeline.passes.push_back(std::move(pass));
  }

  auto outset_value = table.get<sol::object>("max_outset");
  if (!outset_value.valid() || outset_value.get_type() == sol::type::nil)
    outset_value = table.get<sol::object>("outset");
  if (!outset_value.valid() || outset_value.get_type() == sol::type::nil)
    outset_value = table.get<sol::object>("padding");
  pipeline.max_outset = effect_outset(outset_value);

  if (auto uniforms = table.get<std::optional<sol::table>>("uniforms")) {
    for (auto&& [key, value] : *uniforms) {
      if (!key.is<std::string>())
        continue;
      std::optional<std::string> type_name;
      if (value.is<std::string>())
        type_name = value.as<std::string>();
      else if (value.is<sol::table>())
        type_name =
          value.as<sol::table>().get<std::optional<std::string>>("type");
      if (!type_name)
        continue;
      auto type = effect_uniform_type(*type_name);
      if (!type) {
        log::error("Unknown effect uniform type: {}", *type_name);
        return {};
      }
      pipeline.uniforms.push_back(
        EffectUniformDef {
          .name = key.as<std::string>(),
          .type = *type,
        }
      );
    }
  }
  std::ranges::sort(pipeline.uniforms, {}, &EffectUniformDef::name);

  auto name = pipeline.name;
  auto handle = server->effects->define(std::move(pipeline));
  auto result = flexible::create_table();
  result.set("id", handle.id);
  result.set("generation", handle.generation);
  result.set("name", name);
  return result;
}

bool set_window_effect_instance(
  uint64_t id,
  uint64_t effect_id,
  uint32_t generation,
  flexible::object uniforms,
  flexible::object options
) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel || !server->effects)
    return false;

  auto pipeline = server->effects->get(
    EffectHandle { .id = effect_id, .generation = generation }
  );
  if (!pipeline)
    return false;

  EffectInstance instance;
  instance.pipeline = pipeline;
  instance.version = server->effects->next_instance_version();
  instance.outset = pipeline->max_outset;

  if (options.valid() && options.is<sol::table>()) {
    auto table = options.as<sol::table>();
    auto outset = table.get<sol::object>("outset");
    if (outset.valid() && outset.get_type() != sol::type::nil)
      instance.outset = effect_outset(outset);
  }

  auto values = uniforms.valid() && uniforms.is<sol::table>()
    ? uniforms.as<sol::table>()
    : sol::table {};
  for (auto& definition : pipeline->uniforms) {
    EffectUniform uniform {
      .name = definition.name,
      .type = definition.type,
    };
    if (values.valid()) {
      auto value = values.get<sol::object>(definition.name);
      if (!effect_uniform_value(value, definition.type, uniform)) {
        log::error("Invalid value for effect uniform {}", definition.name);
        return false;
      }
    }
    instance.uniforms.push_back(std::move(uniform));
  }

  if (auto* animations = server->view->animations.get())
    animations->cancel_effect_uniforms(toplevel->entity);
  toplevel->set_effect(std::move(instance));
  return true;
}

flexible::object define_texture(std::string path) {
  auto server = UraServer::get_instance();
  if (!server->effects)
    return {};

  auto expanded = expand(path);
  auto image = load_image_rgba(expanded);
  if (!image) {
    log::error("Effect texture cannot be loaded: {}", expanded);
    return {};
  }

  EffectTexture texture;
  texture.name = path;
  texture.width = image->width;
  texture.height = image->height;
  texture.pixels = std::move(image->pixels);

  auto handle = server->effects->define_texture(std::move(texture));
  auto result = flexible::create_table();
  result.set("id", handle.id);
  result.set("generation", handle.generation);
  result.set("name", path);
  return result;
}

flexible::object find_texture(std::string name) {
  auto server = UraServer::get_instance();
  if (!server->effects)
    return {};
  auto texture = server->effects->find_texture(name);
  if (!texture)
    return {};
  auto result = flexible::create_table();
  result.set("id", texture->handle.id);
  result.set("generation", texture->handle.generation);
  result.set("name", texture->name);
  return result;
}

bool undefine_texture(uint64_t id, uint32_t generation) {
  auto server = UraServer::get_instance();
  if (!server->effects)
    return false;
  return server->effects->remove_texture(
    EffectTextureHandle { .id = id, .generation = generation }
  );
}

flexible::object find_effect(std::string name) {
  auto server = UraServer::get_instance();
  if (!server->effects)
    return {};
  auto pipeline = server->effects->find(name);
  if (!pipeline)
    return {};
  auto result = flexible::create_table();
  result.set("id", pipeline->handle.id);
  result.set("generation", pipeline->handle.generation);
  result.set("name", pipeline->name);
  return result;
}

bool undefine_effect(uint64_t id, uint32_t generation) {
  auto server = UraServer::get_instance();
  if (!server->effects)
    return false;
  return server->effects->remove(
    EffectHandle { .id = id, .generation = generation }
  );
}

void clear_effects() {
  auto server = UraServer::get_instance();
  if (server->effects)
    server->effects->clear();
}

void migrate_effects() {
  auto server = UraServer::get_instance();
  if (!server->view || !server->view->scene || !server->effects)
    return;

  auto& registry = server->view->scene->raw();
  auto effects = registry.view<ShaderEffect>();

  struct Update {
    entt::entity entity;
    EffectInstance effect;
  };

  std::vector<Update> updates;

  for (auto [entity, shader_effect] : effects.each()) {
    auto& current = shader_effect.effect;
    if (!current.active())
      continue;
    auto replacement = server->effects->find(current.pipeline->name);
    if (!replacement || replacement.get() == current.pipeline.get())
      continue;

    EffectInstance next;
    next.pipeline = replacement;
    next.version = server->effects->next_instance_version();
    next.outset = replacement->max_outset;
    for (auto& definition : replacement->uniforms) {
      EffectUniform uniform {
        .name = definition.name,
        .type = definition.type,
      };
      auto old = std::ranges::find_if(
        current.uniforms,
        [&](const EffectUniform& candidate) {
          return candidate.name == definition.name
            && candidate.type == definition.type;
        }
      );
      if (old != current.uniforms.end())
        uniform.values = old->values;
      next.uniforms.push_back(std::move(uniform));
    }
    updates.push_back(Update { .entity = entity, .effect = std::move(next) });
  }

  for (auto& update : updates) {
    if (auto* animations = server->view->animations.get())
      animations->cancel_effect_uniforms(update.entity);
    server->view->scene->set_effect(update.entity, update.effect);
  }
}

void animate_window_effect_uniform(
  uint64_t id,
  std::string name,
  flexible::object from,
  flexible::object to,
  double duration,
  flexible::object easing
) {
  auto server = UraServer::get_instance();
  auto toplevel = UraToplevel::from(id);
  if (!toplevel || !server->view || !server->view->animations)
    return;

  auto effect = toplevel->effect();
  if (!effect.active())
    return;

  auto definition = std::ranges::find_if(
    effect.pipeline->uniforms,
    [&](const EffectUniformDef& candidate) { return candidate.name == name; }
  );
  if (definition == effect.pipeline->uniforms.end())
    return;
  if (
    definition->type == EffectUniformType::Int
    || definition->type == EffectUniformType::Bool
  )
    return;

  EffectUniform from_value {
    .name = name,
    .type = definition->type,
  };
  EffectUniform to_value {
    .name = name,
    .type = definition->type,
  };
  if (
    !effect_uniform_value(from, definition->type, from_value)
    || !effect_uniform_value(to, definition->type, to_value)
  )
    return;

  auto instance_uniform =
    std::ranges::find_if(effect.uniforms, [&](const EffectUniform& candidate) {
      return candidate.name == name;
    });
  if (instance_uniform == effect.uniforms.end())
    return;

  EffectUniformTrack track {
    .name = name,
    .slot = static_cast<std::size_t>(
      std::distance(effect.uniforms.begin(), instance_uniform)
    ),
    .type = definition->type,
    .from = from_value.values,
    .to = to_value.values,
    .start = {},
    .options = animation_options(duration, easing),
    .running = true,
  };
  server->view->animations->animate_effect_uniform(toplevel->entity, track);
}

void clear_window_effect(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  if (auto* animations = UraServer::get_instance()->view->animations.get())
    animations->cancel_effect_uniforms(toplevel->entity);
  toplevel->set_effect(EffectInstance {});
}

void set_window_border_color(uint64_t id, std::string color) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return;
  toplevel->set_border_color(color);
}

sol::optional<std::string> get_window_border_color(uint64_t id) {
  auto toplevel = UraToplevel::from(id);
  if (!toplevel)
    return {};
  return toplevel->border_color;
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
