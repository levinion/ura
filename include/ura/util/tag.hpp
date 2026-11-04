#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ura {

// Canonical tag identity. `name_space` may be empty; `index` is the numeric
// suffix. The display tag is always generated as "namespace:index".
struct Tag {
  std::string name_space;
  int index = 1;

  std::string name() const;

  static std::optional<Tag> parse(std::string_view value);
  // Lua-facing compatibility: tags without a numeric suffix get ":1".
  static std::optional<Tag> parse_or_default(std::string_view value);

  bool operator==(const Tag&) const = default;
};

struct TagLess {
  bool operator()(const Tag& lhs, const Tag& rhs) const;
};

} // namespace ura
