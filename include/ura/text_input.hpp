#pragma once
#include <list>
#include "ura/ura.hpp"

namespace ura {

class UraInputPopup {
public:
  wlr_input_popup_surface_v2* popup_surface;
  wlr_scene_tree* scene_tree;
};

class UraTextInput {
public:
  std::list<wlr_text_input_v3*> text_inputs;
  wlr_input_method_v2* input_method = nullptr;
  std::list<UraInputPopup*> popups;

  void focus_text_input(wlr_surface* surface);
  wlr_text_input_v3* get_active_text_input();
  void send_state(wlr_text_input_v3* text_input);

private:
  void unfocus_active_text_input();
};

} // namespace ura
