#pragma once
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

namespace ura {

inline std::vector<std::string> split(std::string& s, char symbol) {
  std::vector<std::string> v;
  std::string t;
  for (int i = 0; i < s.size(); i++) {
    auto c = tolower(s[i]);
    if (c == symbol) {
      v.push_back(t);
      t.clear();
    } else {
      t.push_back(c);
    }
  }
  if (!t.empty())
    v.push_back(t);
  return v;
}

inline std::optional<std::array<float, 4>> hex2rgba(std::string hex_str) {
  if (!hex_str.starts_with('#'))
    return {};
  hex_str = hex_str.substr(1);
  for (auto c : hex_str) {
    if (!std::isxdigit(c))
      return {};
  }
  auto hex = std::stoul(hex_str, 0, 16);
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
} // namespace ura
