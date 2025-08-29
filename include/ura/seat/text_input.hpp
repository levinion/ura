#pragma once
#include <list>
#include "ura/ura.hpp"
#include "ura/util/vec.hpp"

namespace ura {

class UraInputMethodPopup {
public:
  wlr_input_popup_surface_v2* popup_surface;
  wlr_scene_tree* scene_tree;
  Vec4<int> geometry = { 0, 0, 0, 0 };
  void constrain(wlr_text_input_v3* text_input);
};

class UraTextInput {
public:
  Vec<wlr_text_input_v3*> text_inputs;
  wlr_input_method_v2* input_method = nullptr;
  Vec<UraInputMethodPopup*> popups;

  void focus_text_input(wlr_surface* surface);
  wlr_text_input_v3* get_active_text_input();
  void send_state(wlr_text_input_v3* text_input);
  void unfocus_active_text_input();
};

} // namespace ura
