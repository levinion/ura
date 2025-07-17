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
  int i = 0;
  for (auto& workspace : this->output->workspaces) {
    if (this == workspace.get()) {
      return i;
    }
    i++;
  }
  std::unreachable();
}

void UraWorkSpace::add(UraToplevel* toplevel) {
  this->toplevels.push_back(toplevel);
}

} // namespace ura
