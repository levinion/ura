#include "ura/api/lua.hpp"
#include "flexible/flexible.hpp"
#include "ura/api/core.hpp"
#include "ura/core/server.hpp"
#include "ura/core/lua.hpp"
#include "ura/core/state.hpp"

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
  server->state->hooks[name] = [=](flexible::object obj) {
    auto result = f(obj);
    if (result.valid()) {
      return flexible::object::from(result.get<sol::object>());
    }
    return flexible::object::init(flexible::null {});
  };
}

void set_keymap(
  std::string pattern,
  std::string mode,
  sol::protected_function f
) {
  auto server = UraServer::get_instance();
  auto id = parse_keymap(pattern);
  if (id) {
    server->state->keymaps[mode][id.value()] = [=](flexible::object obj) {
      f(obj);
      return flexible::object::init(flexible::null {});
    };
    ;
  }
}

sol::object get_output_logical_geometry(uint64_t id) {
  auto obj = core::get_output_logical_geometry(id);
  auto state = UraServer::get_instance()->lua->state.lua_state();
  return obj.to_sol(state);
}

} // namespace ura::api::lua
