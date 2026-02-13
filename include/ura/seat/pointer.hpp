#pragma once

#include "ura/ura.hpp"
#include "sol/sol.hpp" // IWYU pragma: keep

namespace ura {

class UraPointer {
public:
  static UraPointer* from(wlr_pointer* pointer);
  void init(wlr_input_device* device);
  std::optional<std::string> name();

  bool virt = false;

private:
  wlr_pointer* base;
  libinput_device* device();
};

} // namespace ura
