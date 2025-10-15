#include "ura/lua/api/lua.hpp"
#include "flexible/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/lua/api/core.hpp"
#include "ura/lua/lua.hpp"

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
  for (int i = 0; i < args.size() - 1; i++) {
    r += state["tostring"](args[i]).get<std::string>() + ' ';
  }
  r += state["tostring"](args[args.size() - 1]).get<std::string>();
  r.push_back('\n');
  server->lua->lua_stdout += r;
}

void set_pointer_properties(std::string pattern, sol::object object) {
  return core::set_pointer_properties(pattern, flexible::object::from(object));
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

void set_hook(std::string name, sol::protected_function f) {
  auto server = UraServer::get_instance();
  server->lua->hooks[name] = f;
}

void set_keymap(
  std::string pattern,
  std::string mode,
  sol::protected_function f
) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (id) {
    server->lua->keymaps[mode][id.value()] = f();
  }
}

void set_layout(std::string name, sol::protected_function f) {
  auto server = UraServer::get_instance();
  server->lua->layouts[name] = f();
}
} // namespace ura::api::lua
