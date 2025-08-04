#pragma once

#include <list>
#include <memory>
#include "ura/cursor.hpp"
#include "ura/keyboard.hpp"
#include "ura/text_input.hpp"
#include "ura/toplevel.hpp"

namespace ura {

class UraSeat {
public:
  std::unique_ptr<UraCursor> cursor;
  std::unique_ptr<UraTextInput> text_input;
  wlr_seat* seat;
  std::list<UraKeyboard*> keyboards;
  UraToplevel* focused();
  void init();
  void unfocus();
  void focus(UraClient client);
  void focus(UraToplevel* toplevel);
  void focus(UraLayerShell* layer_shell);
};

} // namespace ura
