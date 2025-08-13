#pragma once

#include "ura/ura.hpp"
#include "ura/util/vec.hpp"
#include <string>

namespace ura {

enum class UraCursorMode { Passthrough, Move, Resize };

class UraCursor {
public:
  bool visible = true;
  std::string xcursor_name = "left_ptr";
  UraCursorMode mode = UraCursorMode::Passthrough;
  void init();
  void attach_device(wlr_input_device* device);
  void relative_move(double delta_x, double delta_y);
  void absolute_move(double x, double y);
  void process_motion(uint32_t time_msec);
  void process_button(wlr_pointer_button_event* event);
  void set_xcursor(std::string name);
  void hide();
  void show();
  void toggle();
  void destroy();
  void set_theme(std::string theme, int size);
  Vec2<double> position();
  void reset_mode();

private:
  wlr_cursor* cursor;
  wlr_xcursor_manager* cursor_mgr;
  wlr_input_device* device = nullptr;
  Vec2<double> grab; // old cursor postion
  Vec4<int> anchor; // old toplevel geometry
  void process_cursor_mode_move();
  void process_cursor_mode_resize();
};

} // namespace ura
