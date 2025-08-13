#pragma once

#include "wlr/util/box.h"

namespace ura {

template<typename T>
class Vec2 {
public:
  T x;
  T y;

  inline bool operator==(const Vec2<T>& box) {
    return this->x == box.x && this->y == box.y;
  }
};

template<typename T>
class Vec4 {
public:
  T x;
  T y;
  T width;
  T height;

  inline bool operator==(const Vec4<T>& box) {
    return this->x == box.x && this->y == box.y && this->width == box.width
      && this->height == box.height;
  }

  inline wlr_box to_wlr_box() {
    return { this->x, this->y, this->width, this->height };
  }

  static inline Vec4<T> from(wlr_box& box) {
    return { box.x, box.y, box.width, box.height };
  }
};
} // namespace ura
