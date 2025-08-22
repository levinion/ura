#pragma once

#include "wlr/util/box.h"

namespace ura {

template<typename T>
class Vec2 {
public:
  T x = 0;
  T y = 0;

  inline bool operator==(const Vec2<T>& box) {
    return this->x == box.x && this->y == box.y;
  }
};

template<typename T>
class Vec4 {
public:
  T x = 0;
  T y = 0;
  T width = 0;
  T height = 0;

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

  inline bool empty() {
    return this->x == 0 && this->y == 0 && this->width == 0
      && this->height == 0;
  }

  inline void center(Vec4<T>& geo) {
    this->x = geo.x + (geo.width - this->width) / 2;
    this->y = geo.y + (geo.height - this->height) / 2;
  }
};
} // namespace ura
