#include "ura/core/server.hpp"
#include "ura/view/popup.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/runtime.hpp"

namespace ura {
void on_new_popup(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto xdg_popup = static_cast<wlr_xdg_popup*>(data);
  auto popup = new UraPopup {};
  if (!popup->init(xdg_popup)) {
    delete popup;
  }
}

void on_popup_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto popup = server->runtime->fetch<UraPopup*>(listener);
  popup->commit();
}

void on_popup_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto popup = server->runtime->fetch<UraPopup*>(listener);
  popup->destroy();
  delete popup;
}
} // namespace ura
