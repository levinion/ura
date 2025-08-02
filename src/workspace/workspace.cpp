#include "ura/workspace.hpp"
#include "ura/output.hpp"
#include <memory>
#include <utility>
#include "ura/server.hpp"

namespace ura {

std::unique_ptr<UraWorkSpace> UraWorkSpace::init() {
  return std::make_unique<UraWorkSpace>();
}

void UraWorkSpace::enable(bool enabled) {
  for (auto toplevel : this->toplevels) {
    if (enabled)
      toplevel->map();
    else
      toplevel->unmap();
  }
}

int UraWorkSpace::index() {
  auto server = UraServer::get_instance();
  if (this == server->scratchpad.get())
    return -1;
  int i = 0;
  for (auto& workspace : this->output->workspaces) {
    if (this == workspace.get()) {
      return i;
    }
    i++;
  }
  std::unreachable();
}

std::optional<UraToplevel*> UraWorkSpace::get_toplevel_at(int index) {
  if (index < 0 || index >= this->toplevels.size())
    return {};
  auto it = this->toplevels.begin();
  std::advance(it, index);
  return *it;
}

sol::table UraWorkSpace::to_lua_table() {
  auto server = UraServer::get_instance();
  auto table = server->lua->state.create_table();
  auto toplevels = server->lua->state.create_table();
  for (auto toplevel : this->toplevels) {
    toplevels.add(toplevel->to_lua_table());
  }
  table["index"] = this->index();
  table["windows"] = toplevels;
  return table;
}

} // namespace ura
