#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>
#include "ura/util/flexible.hpp"

namespace ura {

template<typename Value>
struct Vec: public std::vector<Value> {
  using std::vector<Value>::vector;

  std::optional<Value> get(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= this->size()) {
      return {};
    }
    return (*this)[index];
  }

  void remove(const Value& value) {
    auto new_end = std::ranges::remove(*this, value).begin();
    this->erase(new_end, this->end());
  }

  bool contains(const Value& value) const {
    return std::ranges::find(*this, value) != this->end();
  }

  flexible::object to_table() {
    auto table = flexible::create_table();
    for (auto v : *this) {
      table.add(v);
    }
    return table;
  }
};
} // namespace ura
