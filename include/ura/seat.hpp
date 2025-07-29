#pragma once

#include <list>
#include <memory>
#include "ura/cursor.hpp"
#include "ura/keyboard.hpp"
#include "ura/text_input.hpp"

namespace ura {

class UraSeat {
public:
  std::unique_ptr<UraCursor> cursor;
  std::unique_ptr<UraTextInput> text_input;
  wlr_seat* seat;
  std::list<UraKeyboard*> keyboards;
  void init();
};

} // namespace ura
