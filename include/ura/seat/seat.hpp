#pragma once

#include <memory>
#include "ura/seat/cursor.hpp"
#include "ura/seat/keyboard.hpp"
#include "ura/seat/pointer.hpp"
#include "ura/seat/tablet.hpp"
#include "ura/seat/text_input.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/ura.hpp"

namespace ura {

class UraSeat {
public:
  std::unique_ptr<UraCursor> cursor;
  std::unique_ptr<UraTextInput> text_input;
  wlr_seat* seat;
  Vec<UraKeyboard*> keyboards;
  Vec<UraPointer*> pointers;
  Vec<UraTablet*> tablets;
  bool locked = false;
  bool keyboard_shortcuts_inhibited = false;

  UraToplevel* focused_toplevel();
  std::optional<UraClient> focused_client();
  void init();
  void attach_new_input(wlr_input_device* device);
  void unfocus();
  void focus(UraClient client);
  void focus(UraToplevel* toplevel);
  void focus(UraLayerShell* layer_shell);
  void notify_idle_activity();
  void set_idle_inhibitor(bool flag);
  uint32_t get_modifiers();

private:
  void update_capability();
};

} // namespace ura
