#include <algorithm>
#include <optional>
#include "ura/toplevel.hpp"
#include "ura/workspace.hpp"

namespace ura {

void UraFocusStack::push(UraToplevel* toplevel) {
  this->stack.push_back(toplevel);
}

void UraFocusStack::move_to_top(UraToplevel* toplevel) {
  this->remove(toplevel);
  this->push(toplevel);
}

bool UraFocusStack::is_top(UraToplevel* toplevel) {
  return this->size() == 0 ? false : this->top().value() == toplevel;
}

void UraFocusStack::remove(UraToplevel* toplevel) {
  this->stack.remove(toplevel);
}

std::optional<UraToplevel*> UraFocusStack::top() {
  if (this->stack.size() == 0) {
    return {};
  }
  auto client = this->stack.back();
  return client;
}

int UraFocusStack::size() {
  return this->stack.size();
}

std::optional<UraToplevel*> UraFocusStack::pop() {
  if (this->stack.size() == 0) {
    return {};
  }
  auto toplevel = this->stack.back();
  this->stack.pop_back();
  return toplevel;
}

bool UraFocusStack::contains(UraToplevel* client) {
  auto it = std::find_if(this->stack.begin(), this->stack.end(), [=](auto i) {
    return client == i;
  });
  return it != this->stack.end();
}

std::optional<UraToplevel*> UraFocusStack::find_active() {
  // find last of item.surface != client.surface
  for (auto it = this->stack.rbegin(); it != this->stack.rend(); it++) {
    if ((*it)->is_active())
      return *it;
  }
  return {};
}

void UraFocusStack::unfocus_all() {
  // find last of item.surface != client.surface
  for (auto it = this->stack.rbegin(); it != this->stack.rend(); it++) {
    if ((*it)->is_active())
      (*it)->unfocus();
  }
}
} // namespace ura
