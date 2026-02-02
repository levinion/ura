#pragma once

#include "ura/ura.hpp"
#include "sol/sol.hpp" // IWYU pragma: keep

namespace ura {

class UraPointer {
public:
  static UraPointer* from(wlr_pointer* pointer);
  void init(wlr_input_device* device);
  void set_accel_profile(std::string_view _profile);
  std::optional<std::string> name();

  float move_speed = 1.;
  float scroll_speed = 1.;
  bool virt = false;

private:
  wlr_pointer* base;
  libinput_device* device();
};

} // namespace ura
