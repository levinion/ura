#include "ura/api/lua.hpp"
#include "ura/core/server.hpp"
#include "ura/core/lua.hpp"

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
