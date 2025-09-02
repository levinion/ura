#pragma once

#include "ura/ura.hpp"
#include "sol/sol.hpp"
#include <optional>
#include <string>

namespace ura {

class UraPointer {
public:
  static UraPointer* from(wlr_pointer* pointer);
  void init(wlr_input_device* device);
  void set_accel_profile(std::string& _profile);
  bool is_libinput();
  void try_apply_rules();
  void set_properties(sol::table properties);

  std::string name;
  float move_speed = 1.;
  float scroll_speed = 1.;

private:
  wlr_pointer* base;
  std::optional<libinput_device*> libinput_device_;
};

} // namespace ura
