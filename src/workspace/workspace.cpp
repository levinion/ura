#include "ura/view/workspace.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include <algorithm>
#include <memory>
#include <utility>
#include "ura/core/server.hpp"
#include "ura/seat/seat.hpp"
#include "ura/lua/lua.hpp"

namespace ura {

std::unique_ptr<UraWorkSpace> UraWorkSpace::init() {
  return std::make_unique<UraWorkSpace>();
}

void UraWorkSpace::enable() {
  auto server = UraServer::get_instance();
  for (auto toplevel : this->toplevels) toplevel->map();
  if (this->focus_stack.size() != 0)
    server->seat->focus(this->focus_stack.top().value());
}

void UraWorkSpace::disable() {
  auto server = UraServer::get_instance();
  for (auto toplevel : this->toplevels) toplevel->unmap();
  server->seat->unfocus();
}

int UraWorkSpace::index() {
  auto server = UraServer::get_instance();
  if (this->name)
    return -1;

  auto output = server->view->get_output_by_name(this->output);
  int i = 0;
  for (auto& workspace : output->get_workspaces()) {
    if (this == workspace)
      return i;
    i++;
  }
  std::unreachable();
}

std::optional<UraToplevel*> UraWorkSpace::get_toplevel_at(int index) {
  auto toplevel = this->toplevels.get(index);
  return toplevel ? *toplevel : nullptr;
}

sol::table UraWorkSpace::to_lua_table() {
  auto server = UraServer::get_instance();
  auto table = server->lua->state.create_table();
  auto toplevels = server->lua->state.create_table();
  for (auto toplevel : this->toplevels) {
    toplevels.add(toplevel->to_lua_table());
  }
  table["index"] = this->index();
  table["name"] = this->name;
  table["windows"] = toplevels;
  return table;
}

void UraWorkSpace::add(UraToplevel* toplevel) {
  this->toplevels.push_back(toplevel);
  this->focus_stack.push(toplevel);
}

void UraWorkSpace::remove(UraToplevel* toplevel) {
  this->toplevels.remove(toplevel);
  this->focus_stack.remove(toplevel);
}

void UraWorkSpace::swap_toplevel(UraToplevel* src, UraToplevel* dst) {
  auto it1 = std::find(this->toplevels.begin(), this->toplevels.end(), src);
  if (it1 == this->toplevels.end())
    return;
  auto it2 = std::find(this->toplevels.begin(), this->toplevels.end(), dst);
  if (it2 == this->toplevels.end())
    return;
  *it1 = dst;
  *it2 = src;
}

void UraWorkSpace::redraw() {
  for (auto toplevel : this->toplevels) toplevel->redraw();
}
} // namespace ura
