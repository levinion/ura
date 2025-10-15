#include "ura/view/workspace.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include <algorithm>
#include <memory>
#include <utility>
#include "ura/core/server.hpp"
#include "ura/seat/seat.hpp"

namespace ura {

std::unique_ptr<UraWorkSpace> UraWorkSpace::init() {
  auto workspace = std::make_unique<UraWorkSpace>();
  UraServer::get_instance()->globals.insert(workspace->id());
  return workspace;
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
  if (!output)
    return -1;
  int i = 0;
  auto workspaces = output->get_workspaces();
  for (auto& workspace : workspaces) {
    if (this == workspace)
      return i;
    i++;
  }
  std::unreachable();
}

UraToplevel* UraWorkSpace::get_toplevel_at(int index) {
  auto toplevel = this->toplevels.get(index);
  return toplevel ? *toplevel : nullptr;
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

UraWorkSpace* UraWorkSpace::from(uint64_t id) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id))
    return reinterpret_cast<UraWorkSpace*>(id);
  return nullptr;
}

uint64_t UraWorkSpace::id() {
  return reinterpret_cast<uint64_t>(this);
}

UraWorkSpace::~UraWorkSpace() {
  UraServer::get_instance()->globals.erase(this->id());
}
} // namespace ura
