#include "ura/view/workspace.hpp"
#include "flexible/flexible.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include <algorithm>
#include <memory>
#include <utility>
#include "ura/core/server.hpp"
#include "ura/core/state.hpp"
#include "ura/seat/seat.hpp"

namespace ura {

std::unique_ptr<UraWorkspace> UraWorkspace::init() {
  auto workspace = std::make_unique<UraWorkspace>();
  auto server = UraServer::get_instance();
  server->globals[workspace->id()] = UraGlobalType::Workspace;
  return workspace;
}

void UraWorkspace::enable() {
  auto server = UraServer::get_instance();
  for (auto toplevel : this->toplevels) toplevel->map();
  if (this->focus_stack.size() != 0)
    server->seat->focus(this->focus_stack.top().value());
}

void UraWorkspace::disable() {
  auto server = UraServer::get_instance();
  for (auto toplevel : this->toplevels) toplevel->unmap();
  server->seat->unfocus();
}

int UraWorkspace::index() {
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

UraToplevel* UraWorkspace::get_toplevel_at(int index) {
  auto toplevel = this->toplevels.get(index);
  return toplevel ? *toplevel : nullptr;
}

void UraWorkspace::add(UraToplevel* toplevel) {
  this->toplevels.push_back(toplevel);
  this->focus_stack.push(toplevel);
}

void UraWorkspace::remove(UraToplevel* toplevel) {
  this->toplevels.remove(toplevel);
  this->focus_stack.remove(toplevel);
}

void UraWorkspace::swap_toplevel(UraToplevel* src, UraToplevel* dst) {
  auto it1 = std::find(this->toplevels.begin(), this->toplevels.end(), src);
  if (it1 == this->toplevels.end())
    return;
  auto it2 = std::find(this->toplevels.begin(), this->toplevels.end(), dst);
  if (it2 == this->toplevels.end())
    return;
  *it1 = dst;
  *it2 = src;

  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("src", src->id());
  args.set("dst", dst->id());
  server->state->try_execute_hook("window-swap", args);
}

UraWorkspace* UraWorkspace::from(uint64_t id) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id)
      && server->globals[id].type == UraGlobalType::Workspace)
    return reinterpret_cast<UraWorkspace*>(id);
  return nullptr;
}

uint64_t UraWorkspace::id() {
  return reinterpret_cast<uint64_t>(this);
}

UraWorkspace::~UraWorkspace() {
  UraServer::get_instance()->globals.erase(this->id());
}
} // namespace ura
