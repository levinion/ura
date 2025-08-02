#include <algorithm>
#include <optional>
#include "ura/client.hpp"
#include "ura/workspace.hpp"

namespace ura {

std::optional<UraClient> UraFocusStack::top() {
  if (this->stack.size() == 0) {
    return {};
  }
  auto client = this->stack.back();
  return client;
}

int UraFocusStack::size() {
  return this->stack.size();
}

std::optional<UraClient> UraFocusStack::pop() {
  if (this->stack.size() == 0) {
    return {};
  }
  auto client = this->stack.back();
  this->stack.pop_back();
  return client;
}

bool UraFocusStack::contains(UraClient client) {
  auto it = std::find_if(this->stack.begin(), this->stack.end(), [=](auto i) {
    return client.surface == i.surface;
  });
  return it != this->stack.end();
}

std::optional<UraClient> UraFocusStack::find_prev(UraClient client) {
  // find last of item.surface != client.surface
  for (auto it = this->stack.rbegin(); it != this->stack.rend(); it++) {
    if (it->surface != client.surface)
      return *it;
  }
  return {};
}
} // namespace ura
