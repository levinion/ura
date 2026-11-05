#pragma once

#include <libinput.h>
#include "ura/ura.hpp"
#include <chrono>
#include <string>
#include <utility>

namespace ura {

class UraCursor {
public:
  bool visible = true;
  bool idle_hidden = false;
  std::string xcursor_name = "left_ptr";
  void init();
  void attach_device(wlr_input_device* device);
  void relative_move(wlr_pointer_motion_event* event);
  void absolute_move(wlr_pointer_motion_absolute_event* event);
  void process_button(wlr_pointer_button_event* event);
  void process_axis(wlr_pointer_axis_event* event);
  void set_xcursor(std::string name);
  void set_visible(bool flag, bool from_idle = false);
  void notify_activity();
  void destroy();
  void set_theme(std::string theme, int size);
  std::string get_theme();
  int get_size();
  std::pair<double, double> position();

private:
  wlr_cursor* cursor;
  wlr_xcursor_manager* cursor_mgr;
  std::chrono::steady_clock::time_point last_activity =
    std::chrono::steady_clock::now();
  void process_motion(
    uint32_t time_msec,
    double dx,
    double dy,
    double dx_unaccel,
    double dy_unaccel
  );
};

} // namespace ura
