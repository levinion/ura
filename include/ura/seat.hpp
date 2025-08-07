#pragma once

#include <cstdint>
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
  bool locked = false;

  UraToplevel* focused_toplevel();
  std::optional<UraClient> focused_client();
  void init();
  void unfocus();
  void focus(UraClient client);
  void focus(UraToplevel* toplevel);
  void focus(UraLayerShell* layer_shell);
  void notify_idle_activity();
};

} // namespace ura
