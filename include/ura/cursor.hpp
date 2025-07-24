#pragma once

#include "ura/ura.hpp"
#include <string>

namespace ura {
template<typename T>
class UraPosition {
public:
  T x;
  T y;
};

class UraCursor {
public:
  bool visible = true;
  std::string xcursor_name = "left_ptr";
  void init();
  void attach_device(wlr_input_device* device);
  void relative_move(double delta_x, double delta_y);
  void absolute_move(double x, double y);
  void process_motion(uint32_t time_msec);
  void set_xcursor(std::string name);
  void hide();
  void show();
  void toggle();
  void destroy();
  void set_theme(std::string theme, int size);
  UraPosition<double> position();

private:
  wlr_cursor* cursor;
  wlr_xcursor_manager* cursor_mgr;
  wlr_input_device* device = nullptr;
};

} // namespace ura
