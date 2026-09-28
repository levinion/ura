#pragma once

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace ura::util {

inline char ascii_lower(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}

inline std::string ascii_lower(std::string_view text) {
  std::string result(text);
  std::ranges::transform(text, result.begin(), [](char c) {
    return ascii_lower(c);
  });
  return result;
}

inline bool is_ascii_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f'
    || c == '\r';
}

inline std::string_view strip_ascii_whitespace(std::string_view text) {
  auto first = std::ranges::find_if_not(text, is_ascii_whitespace);
  if (first == text.end())
    return {};
  auto last =
    std::ranges::find_if_not(text | std::views::reverse, is_ascii_whitespace)
      .base();
  return { first, static_cast<std::size_t>(last - first) };
}

inline std::vector<std::string> split(std::string_view text, char delimiter) {
  if (text.empty())
    return { "" };
  const char delimiter_char = delimiter;
  auto pattern = std::string_view(&delimiter_char, 1);
  std::vector<std::string> parts;
  for (auto part : text | std::views::split(pattern))
    parts.emplace_back(part.begin(), part.end());
  return parts;
}

template<std::ranges::input_range Range>
inline std::string join(const Range& parts, std::string_view delimiter) {
  auto text = parts
    | std::views::transform([](const auto& part) -> std::string_view {
                return part;
              })
    | std::views::join_with(delimiter);
  std::string result;
  for (char c : text) result.push_back(c);
  return result;
}

} // namespace ura::util
