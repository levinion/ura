#pragma once

#include <libinput.h>
#include "ura/ura.hpp"
#include "ura/util/vec.hpp"
#include <string>

namespace ura {

class UraCursor {
public:
  bool visible = true;
  std::string xcursor_name = "left_ptr";
  void init();
  void attach_device(wlr_input_device* device);
  void relative_move(wlr_pointer_motion_event* event);
  void absolute_move(wlr_pointer_motion_absolute_event* event);
  void process_button(wlr_pointer_button_event* event);
  void process_axis(wlr_pointer_axis_event* event);
  void set_xcursor(std::string name);
  void set_visible(bool flag);
  void destroy();
  void set_theme(std::string theme, int size);
  std::string get_theme();
  int get_size();
  Vec2<double> position();

private:
  wlr_cursor* cursor;
  wlr_xcursor_manager* cursor_mgr;
  void process_motion(
    uint32_t time_msec,
    double dx,
    double dy,
    double dx_unaccel,
    double dy_unaccel
  );
};

} // namespace ura
