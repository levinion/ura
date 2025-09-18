#pragma once

#include "ura/ura.hpp"

namespace ura {

class UraTablet {
public:
  void init(wlr_input_device* device);

private:
  wlr_tablet_v2_tablet* tablet;
};

} // namespace ura
