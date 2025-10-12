#pragma once
#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "ura/ura.hpp"
#include <ranges>

namespace ura {

template<typename T>
constexpr std::vector<std::string_view> split(std::string_view s, T symbol) {
  return s | std::views::split(symbol)
    | std::views::transform([](auto r) { return std::string_view(r); })
    | std::ranges::to<std::vector<std::string_view>>();
}

template<typename T>
constexpr std::string join(std::vector<std::string_view>& v, T symbol) {
  return v | std::views::join_with(symbol) | std::ranges::to<std::string>();
}

constexpr std::string_view trim(std::string_view s) {
  auto view = s | std::views::drop_while(isspace) | std::views::reverse
    | std::views::drop_while(isspace) | std::views::reverse;
  return std::string_view(view.begin().base().base(), view.end().base().base());
}

inline std::optional<std::array<float, 4>> hex2rgba(std::string_view hex_str) {
  if (!hex_str.starts_with('#'))
    return {};
  hex_str = hex_str.substr(1);
  for (auto c : hex_str) {
    if (!std::isxdigit(c))
      return {};
  }
  auto hex = std::stoul(std::string(hex_str), 0, 16);
  auto rgba = std::array<float, 4>();
  if (hex_str.size() == 6) { // RGB
    rgba[0] = static_cast<float>((hex >> 16) & 0xFF) / 255.0f;
    rgba[1] = static_cast<float>((hex >> 8) & 0xFF) / 255.0f;
    rgba[2] = static_cast<float>(hex & 0xFF) / 255.0f;
    rgba[3] = 1.0f;
  } else if (hex_str.size() == 8) { // RGBA
    rgba[0] = static_cast<float>((hex >> 24) & 0xFF) / 255.0f;
    rgba[1] = static_cast<float>((hex >> 16) & 0xFF) / 255.0f;
    rgba[2] = static_cast<float>((hex >> 8) & 0xFF) / 255.0f;
    rgba[3] = static_cast<float>(hex & 0xFF) / 255.0f;
  } else
    return {};
  return rgba;
}

inline std::optional<uint64_t> parse_keymap(std::string_view pattern) {
  // modifiers str to modifiers bit
  auto keys = split(pattern, '+') | std::views::transform(trim)
    | std::ranges::to<std::vector<std::string_view>>();
  if (keys.empty())
    return {};
  uint32_t mod = 0;
  if (keys.size() > 1) {
    for (int i = 0; i < keys.size() - 1; i++) {
      auto m = keys[i];
      if (m == "super" || m == "mod" || m == "cmd" || m == "command"
          || m == "logo" || m == "win") {
        mod |= WLR_MODIFIER_LOGO;
      } else if (m == "alt" || m == "opt" || m == "option") {
        mod |= WLR_MODIFIER_ALT;
      } else if (m == "ctrl" || m == "control") {
        mod |= WLR_MODIFIER_CTRL;
      } else if (m == "shift") {
        mod |= WLR_MODIFIER_SHIFT;
      } else if (m == "caps" || m == "capslock") {
        mod |= WLR_MODIFIER_CAPS;
      } else if (m == "mod2") {
        mod |= WLR_MODIFIER_MOD2;
      } else if (m == "mod3") {
        mod |= WLR_MODIFIER_MOD3;
      } else if (m == "mod5") {
        mod |= WLR_MODIFIER_MOD5;
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
  return (static_cast<uint64_t>(mod) << 32) | sym;
}

inline std::string to_lower_t(const std::string& word) {
  std::string s;
  for (auto& c : word) {
    s.push_back(tolower(c));
  }
  return s;
}

inline std::optional<libinput_config_accel_profile>
accel_profile_from_str(std::string_view profile) {
  if (profile == "flat")
    return LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
  else if (profile == "adaptive")
    return LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
  return {};
}
} // namespace ura
