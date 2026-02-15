#include "ura/core/state.hpp"
#include "ura/core/server.hpp"

namespace ura {

bool UraState::emit_keybinding(uint64_t id) {
  if (!this->contains_keybinding(id))
    return false;
  this->keymaps[this->keymap_mode][id]({});
  return true;
}

bool UraState::contains_keybinding(uint64_t id) {
  if (!this->keymaps.contains(this->keymap_mode))
    return false;
  if (!this->keymaps[this->keymap_mode].contains(id))
    return false;
  return true;
}

std::unique_ptr<UraState> UraState::init() {
  auto state = std::make_unique<UraState>();
  return state;
}

void UraState::emit_hook(std::string name, flexible::object args) {
  if (!this->hooks.contains(name))
    return;
  this->hooks[name](args);
}

void UraState::set_option(std::string_view key, flexible::object value) {
  this->options[std::string(key)] = value;
}

} // namespace ura
