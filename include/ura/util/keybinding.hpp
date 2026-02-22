#pragma once
#include <absl/strings/ascii.h>
#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "ura/ura.hpp"
#include <absl/strings/str_split.h>
#include <linux/input-event-codes.h>

namespace ura::util {

enum UraKeybindingDeviceType : uint32_t {
  Keyboard = 0u,
  Mouse = 1u << 31,
};

enum UraKeyCodeExtra : uint32_t {
  WheelUp = 0x1001u,
  WheelDown = 0x1002u,
  WheelLeft = 0x1003u,
  WheelRight = 0x1004u,
};

// mod: 8bit | sym: 32bit (3+29)
inline uint64_t construct_keybinding_id(uint8_t mod, uint32_t sym) {
  return (static_cast<uint64_t>(mod) << 32) | sym;
}

inline std::optional<uint64_t> get_keybinding_id(std::string_view pattern) {
  // modifiers str to modifiers bit
  std::vector<std::string> keys = absl::StrSplit(pattern, '+');
  if (keys.empty())
    return {};
  for (auto& key : keys) {
    key = absl::AsciiStrToLower(absl::StripAsciiWhitespace(key));
  }
  uint8_t mod = 0;
  if (keys.size() > 1) {
    for (std::size_t i = 0; i < keys.size() - 1; i++) {
      auto m = keys[i];
      if (m == "super") {
        mod |= WLR_MODIFIER_LOGO;
      } else if (m == "alt") {
        mod |= WLR_MODIFIER_ALT;
      } else if (m == "ctrl") {
        mod |= WLR_MODIFIER_CTRL;
      } else if (m == "shift") {
        mod |= WLR_MODIFIER_SHIFT;
      } else if (m == "caps") {
        mod |= WLR_MODIFIER_CAPS;
      } else if (m == "mod2") {
        mod |= WLR_MODIFIER_MOD2;
      } else if (m == "mod3") {
        mod |= WLR_MODIFIER_MOD3;
      } else if (m == "mod5") {
        mod |= WLR_MODIFIER_MOD5;
      } else {
        return {};
      }
    }
  }
  xkb_keysym_t sym = xkb_keysym_from_name(
    std::string(keys.back()).c_str(),
    XKB_KEYSYM_CASE_INSENSITIVE
  );

  // if shift is pressed, then upper sym should be binded
  if (mod & WLR_MODIFIER_SHIFT && sym >= XKB_KEY_a && sym <= XKB_KEY_z) {
    sym -= (XKB_KEY_a - XKB_KEY_A);
  }

  if (sym == XKB_KEY_NoSymbol) {
    auto key = keys.back();
    // TODO: add more key event
    if (key == "mouseleft")
      sym = UraKeybindingDeviceType::Mouse | BTN_LEFT;
    else if (key == "mouseright")
      sym = UraKeybindingDeviceType::Mouse | BTN_RIGHT;
    else if (key == "mousemiddle")
      sym = UraKeybindingDeviceType::Mouse | BTN_MIDDLE;
    else if (key == "mouseside")
      sym = UraKeybindingDeviceType::Mouse | BTN_SIDE;
    else if (key == "mouseextra")
      sym = UraKeybindingDeviceType::Mouse | BTN_EXTRA;
    else if (key == "wheelup")
      sym = UraKeybindingDeviceType::Mouse | (uint32_t)UraKeyCodeExtra::WheelUp;
    else if (key == "wheeldown")
      sym =
        UraKeybindingDeviceType::Mouse | (uint32_t)UraKeyCodeExtra::WheelDown;
    else if (key == "wheelleft")
      sym =
        UraKeybindingDeviceType::Mouse | (uint32_t)UraKeyCodeExtra::WheelLeft;
    else if (key == "wheelright")
      sym =
        UraKeybindingDeviceType::Mouse | (uint32_t)UraKeyCodeExtra::WheelRight;
    else
      return {};
  }

  return construct_keybinding_id(mod, sym);
}

} // namespace ura::util
