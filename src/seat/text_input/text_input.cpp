#include "ura/seat/text_input.hpp"
#include <algorithm>

namespace ura {

void UraTextInput::focus_text_input(wlr_surface* surface) {
  // unfocus prev text_input if any
  this->unfocus_active_text_input();
  auto it = std::find_if(
    this->text_inputs.begin(),
    this->text_inputs.end(),
    [&](auto text_input) {
      return wl_resource_get_client(text_input->resource)
        == wl_resource_get_client(surface->resource);
    }
  );
  if (it != this->text_inputs.end()) {
    auto text_input = *it;
    wlr_text_input_v3_send_enter(text_input, surface);
  }
}

void UraTextInput::unfocus_active_text_input() {
  auto active_text_input = this->get_active_text_input();
  if (!active_text_input)
    return;
  wlr_text_input_v3_send_leave(active_text_input);
}

wlr_text_input_v3* UraTextInput::get_active_text_input() {
  for (auto& text_input : this->text_inputs)
    if (text_input->focused_surface)
      return text_input;
  return nullptr;
}

void UraTextInput::send_state(wlr_text_input_v3* text_input) {
  if (!this->input_method) {
    wlr_log(WLR_DEBUG, "no input method, return");
    return;
  }
  if (!text_input->focused_surface) {
    wlr_log(WLR_DEBUG, "text_input is not focused, return");
    return;
  }
  wlr_input_method_v2_send_surrounding_text(
    input_method,
    text_input->current.surrounding.text,
    text_input->current.surrounding.cursor,
    text_input->current.surrounding.anchor
  );
  wlr_input_method_v2_send_text_change_cause(
    input_method,
    text_input->current.text_change_cause
  );
  wlr_input_method_v2_send_content_type(
    input_method,
    text_input->current.content_type.hint,
    text_input->current.content_type.purpose
  );
  wlr_input_method_v2_send_done(input_method);
}
} // namespace ura
