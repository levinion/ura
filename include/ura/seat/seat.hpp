#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include "ura/seat/cursor.hpp"
#include "ura/seat/keyboard.hpp"
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
  bool locked = false;
  bool keyboard_shortcuts_inhibited = false;
  Vec<wlr_input_device*> devices;

  UraToplevel* focused_toplevel();
  std::optional<UraClient> focused_client();
  void init();
  void unfocus();
  void focus(UraClient client);
  void focus(UraToplevel* toplevel);
  void focus(UraLayerShell* layer_shell);
  void notify_idle_activity();
  void set_idle_inhibitor(bool flag);
  void try_apply_pointer_rules(wlr_input_device* device);
  void
  set_pointer_accel_profile(wlr_input_device* device, std::string& profile);
  void set_pointer_accel_profile(std::string& pattern, std::string& profile);
};

} // namespace ura
