#include "ura/core/server.hpp"
#include "ura/view/view.hpp" // IWYU pragma: keep
#include "ura/view/output.hpp" // IWYU pragma: keep
#include "ura/core/runtime.hpp" // IWYU pragma: keep
#include "ura/seat/keyboard.hpp" // IWYU pragma: keep
#include "ura/view/toplevel.hpp" // IWYU pragma: keep
#include "ura/ura.hpp" // IWYU pragma: keep
#include "ura/view/layer_shell.hpp" // IWYU pragma: keep
#include "ura/seat/seat.hpp" // IWYU pragma: keep
#include "ura/core/lua.hpp" // IWYU pragma: keep
#include "ura/core/state.hpp" // IWYU pragma: keep

namespace ura {

UraServer* UraServer::instance = nullptr;

UraServer* UraServer::get_instance() {
  if (UraServer::instance == nullptr) {
    UraServer::instance = new UraServer {};
  }
  return UraServer::instance;
}

// this should never be called besides through lua api
void UraServer::terminate() {
  this->quit = true;
}

} // namespace ura
