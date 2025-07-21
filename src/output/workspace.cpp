#include "ura/server.hpp"
#include "ura/output.hpp"
#include <iterator>
#include <list>
#include <utility>
#include "ura/workspace.hpp"

namespace ura {

UraWorkSpace* UraOutput::get_workspace_at(int index) {
  if (index < 0 || index >= this->workspaces.size())
    return nullptr;
  auto it = this->workspaces.begin();
  std::advance(it, index);
  return it->get();
}

UraWorkSpace* UraOutput::create_workspace() {
  auto workspace = UraWorkSpace::init();
  workspace->output = this;
  this->workspaces.push_back(std::move(workspace));
  return this->workspaces.back().get();
}

int UraOutput::switch_workspace(int index) {
  if (index < 0)
    return -1;
  auto target = get_workspace_at(index);
  if (target == this->current_workspace)
    return index;
  // if there is no such workspace, then create one
  if (!target) {
    this->create_workspace();
    target = this->workspaces.back().get();
  }
  return this->switch_workspace(target);
}

int UraOutput::switch_workspace(UraWorkSpace* workspace) {
  if (!workspace)
    return -1;
  if (workspace == this->current_workspace)
    return workspace->index();

  if (this->current_workspace)
    this->current_workspace->enable(false);
  this->current_workspace = workspace;
  this->current_workspace->enable(true);

  // focus toplevel
  if (!this->current_workspace->toplevels.empty())
    this->current_workspace->toplevels.front()->focus();

  return this->current_workspace->index();
}

} // namespace ura
