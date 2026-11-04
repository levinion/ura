#include "ura/util/tag.hpp"

#include <charconv>
#include <cctype>
#include <string>

namespace ura {

namespace {

  bool natural_less(std::string_view lhs, std::string_view rhs) {
    auto is_digit = [](char c) {
      return std::isdigit(static_cast<unsigned char>(c));
    };
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < lhs.size() && j < rhs.size()) {
      if (is_digit(lhs[i]) && is_digit(rhs[j])) {
        auto i_start = i;
        auto j_start = j;
        while (i < lhs.size() && is_digit(lhs[i])) ++i;
        while (j < rhs.size() && is_digit(rhs[j])) ++j;
        auto strip = [](std::string_view value) {
          std::size_t offset = 0;
          while (offset + 1 < value.size() && value[offset] == '0') ++offset;
          return value.substr(offset);
        };
        auto i_number = strip(lhs.substr(i_start, i - i_start));
        auto j_number = strip(rhs.substr(j_start, j - j_start));
        if (i_number.size() != j_number.size())
          return i_number.size() < j_number.size();
        if (i_number != j_number)
          return i_number < j_number;
        continue;
      }
      if (lhs[i] != rhs[j])
        return static_cast<unsigned char>(lhs[i])
          < static_cast<unsigned char>(rhs[j]);
      ++i;
      ++j;
    }
    return lhs.size() - i < rhs.size() - j;
  }

} // namespace

std::string Tag::name() const {
  return this->name_space + ":" + std::to_string(this->index);
}

std::optional<Tag> Tag::parse(std::string_view value) {
  auto colon = value.rfind(':');
  if (colon == std::string_view::npos)
    return {};

  auto suffix = value.substr(colon + 1);
  if (suffix.empty())
    return {};

  int index = 0;
  auto [ptr, error] =
    std::from_chars(suffix.data(), suffix.data() + suffix.size(), index);
  if (
    error != std::errc {} || ptr != suffix.data() + suffix.size() || index < 0
  )
    return {};

  Tag tag;
  tag.name_space = std::string(value.substr(0, colon));
  tag.index = index;
  return tag;
}

std::optional<Tag> Tag::parse_or_default(std::string_view value) {
  if (value.empty())
    return {};
  if (auto tag = Tag::parse(value))
    return tag;
  auto fallback = std::string(value) + ":1";
  return Tag::parse(fallback);
}

bool TagLess::operator()(const Tag& lhs, const Tag& rhs) const {
  if (lhs.name_space != rhs.name_space)
    return natural_less(lhs.name_space, rhs.name_space);
  return lhs.index < rhs.index;
}

} // namespace ura
