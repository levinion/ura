#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <sol/sol.hpp>
#include <vector>
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
  UraToplevel* logical_focused = nullptr;
  bool idle = false;
  bool idle_inhibited = false;

  UraToplevel* focused_toplevel();
  std::optional<UraClient> focused_client();
  void init();
  void attach_new_input(wlr_input_device* device);
  void unfocus();
  void focus(UraClient client);
  void focus(UraToplevel* toplevel);
  void focus(UraLayerShell* layer_shell);
  void focus_lru();
  void notify_idle_activity();
  void set_idle_inhibitor(bool flag);
  void add_idle_timeout(
    std::string name,
    std::chrono::milliseconds timeout,
    std::optional<sol::protected_function> on_idle = {},
    std::optional<sol::protected_function> on_resume = {}
  );
  void remove_idle_timeout(const std::string& name);
  void clear_idle_timeouts();
  bool is_idle() const;
  uint32_t get_modifiers();

private:
  struct IdleTimeout {
    std::string name;
    std::chrono::milliseconds timeout { 0 };
    int timer = -1;
    bool fired = false;
    std::optional<sol::protected_function> on_idle;
    std::optional<sol::protected_function> on_resume;
  };

  std::vector<IdleTimeout> idle_timeouts;
  std::chrono::steady_clock::time_point idle_since;

  void update_capability();
  void cancel_idle_timers();
  void restart_idle_timers();
  void on_idle_stage(const std::string& name);
};

} // namespace ura
