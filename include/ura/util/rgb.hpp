#pragma once
#include <absl/strings/ascii.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <absl/strings/str_split.h>

namespace ura::util {

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
} // namespace ura::util
