#pragma once

#include <absl/container/inlined_vector.h>
#include <algorithm>
#include <cstddef>
#include "ura/util/flexible.hpp"
#include "wlr/util/box.h"

namespace ura {

template<typename T>
class Vec2 {
public:
  T x = 0;
  T y = 0;

  bool operator==(const Vec2<T>& box) {
    return this->x == box.x && this->y == box.y;
  }

  flexible::object to_table() {
    auto table = flexible::create_table();
    table.set("x", this->x);
    table.set("y", this->y);
    return table;
  }
};

template<typename T>
class Vec4 {
public:
  T x = 0;
  T y = 0;
  T width = 0;
  T height = 0;

  bool operator==(const Vec4<T>& box) const = default;

  wlr_box to_wlr_box() {
    return { this->x, this->y, this->width, this->height };
  }

  std::array<T, 4> to_array() {
    return { x, y, width, height };
  }

  flexible::object to_table() {
    flexible::table obj = flexible::create_table();
    obj.set("x", x);
    obj.set("y", y);
    obj.set("width", width);
    obj.set("height", height);
    return obj;
  }

  static Vec4<T> from(wlr_box& box) {
    return { box.x, box.y, box.width, box.height };
  }

  bool empty() {
    return this->x == 0 && this->y == 0 && this->width == 0
      && this->height == 0;
  }

  void center(Vec4<T>& geo) {
    this->x = geo.x + (geo.width - this->width) / 2;
    this->y = geo.y + (geo.height - this->height) / 2;
  }
};

template<typename T, std::size_t N = 128>
struct Vec: public absl::InlinedVector<T, N> {
  using absl::InlinedVector<T, N>::InlinedVector;

  std::optional<T> get(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= this->size()) {
      return {};
    }
    return (*this)[index];
  }

  void remove(T v) {
    this->erase(std::remove(this->begin(), this->end(), v), this->end());
  }

  bool contains(T v) {
    return std::find(this->begin(), this->end(), v) != this->end();
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
