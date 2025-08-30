#include "ura/core/server.hpp"
#include <optional>
#include "ura/view/view.hpp"
#include "ura/view/client.hpp"
#include "ura/view/output.hpp"
#include "ura/core/runtime.hpp"
#include "ura/seat/keyboard.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/ura.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/seat/seat.hpp"
#include "ura/lua/lua.hpp"

namespace ura {

UraServer* UraServer::instance = nullptr;

UraServer* UraServer::get_instance() {
  if (UraServer::instance == nullptr) {
    UraServer::instance = new UraServer {};
  }
  return UraServer::instance;
}

void UraServer::terminate() {
  this->quit = true;
  wl_display_terminate(this->display);
}

} // namespace ura
