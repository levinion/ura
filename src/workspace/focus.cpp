#include <optional>
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
} // namespace ura
