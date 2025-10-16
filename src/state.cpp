#include "ura/core/state.hpp"
#include "ura/core/server.hpp"

namespace ura {

bool UraState::try_execute_keybinding(uint64_t id) {
  auto server = UraServer::get_instance();
  if (!this->contains_keybinding(id))
    return false;
  this->keymaps[this->keymap_mode][id]({});
  return true;
}

bool UraState::contains_keybinding(uint64_t id) {
  auto server = UraServer::get_instance();
  if (!this->keymaps.contains(this->keymap_mode))
    return false;
  if (!this->keymaps[this->keymap_mode].contains(id))
    return false;
  return true;
}

std::unique_ptr<UraState> UraState::init() {
  return std::make_unique<UraState>();
}

void UraState::try_execute_hook(std::string name, flexible::object args) {
  if (!this->hooks.contains(name))
    return;
  this->hooks[name](args);
}

} // namespace ura
