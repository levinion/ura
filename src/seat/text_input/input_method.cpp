#include "ura/core/server.hpp"
#include "ura/seat/text_input.hpp"
#include "ura/view/client.hpp"
#include "ura/view/view.hpp"

namespace ura {

void UraInputMethodPopup::constrain(wlr_text_input_v3* text_input) {
  auto client = UraClient::from(text_input->focused_surface);
  if (client.type != UraSurfaceType::Toplevel)
    return;
  auto parent = client.transform<UraToplevel>();
  auto cursor_rect = text_input->current.cursor_rectangle;

  auto popup_rect = Vec4<int>();

  // transform local coordination to global coordination
  popup_rect.x = cursor_rect.x;
  popup_rect.y = cursor_rect.y + cursor_rect.height;
  popup_rect.width = this->popup_surface->surface->current.width;
  popup_rect.height = this->popup_surface->surface->current.height;

  // constrain to current screen
  auto output = parent->output();
  if (!output)
    return;
  auto geo = output->logical_geometry();
  auto parent_transform = parent->transform();
  auto avaliable_right =
    geo.x + geo.width - (parent_transform.x + popup_rect.x + popup_rect.width);
  auto avaliable_bottom = geo.y + geo.height
    - (parent_transform.y + popup_rect.y + popup_rect.height);
  if (avaliable_right < 0)
    popup_rect.x = geo.x + geo.width - popup_rect.width - parent_transform.x;
  if (avaliable_bottom < 0)
    popup_rect.y -= cursor_rect.height + popup_rect.height;

  wlr_input_popup_surface_v2_send_text_input_rectangle(
    this->popup_surface,
    &cursor_rect
  );

  if (this->geometry != popup_rect) {
    this->geometry = popup_rect;
    UraServer::get_instance()->view->schedule_frames();
  }
}
} // namespace ura
