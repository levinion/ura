#pragma once

#include <algorithm>
#include <array>
#include <type_traits>

#include <wlr/util/box.h>

namespace ura {

// Generic rectangle. Use Rect<double> for layout/animation geometry and
// Rect<int> at integer/wlroots boundaries.
template<typename Scalar>
struct Rect {
  Scalar x = 0;
  Scalar y = 0;
  Scalar width = 0;
  Scalar height = 0;

  bool operator==(const Rect&) const = default;

  bool empty() const {
    return this->width <= Scalar {} || this->height <= Scalar {};
  }

  std::array<Scalar, 4> to_array() const {
    return { this->x, this->y, this->width, this->height };
  }

  void center(const Rect& area) {
    this->x = area.x + (area.width - this->width) / 2;
    this->y = area.y + (area.height - this->height) / 2;
  }

  template<typename Other>
  bool intersects(const Rect<Other>& other) const {
    if (this->empty() || other.empty())
      return false;

    using Common = std::common_type_t<Scalar, Other>;
    auto ax = static_cast<Common>(this->x);
    auto ay = static_cast<Common>(this->y);
    auto aw = static_cast<Common>(this->width);
    auto ah = static_cast<Common>(this->height);
    auto bx = static_cast<Common>(other.x);
    auto by = static_cast<Common>(other.y);
    auto bw = static_cast<Common>(other.width);
    auto bh = static_cast<Common>(other.height);

    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
  }

  template<typename Other>
  auto intersection(const Rect<Other>& other) const
    -> Rect<std::common_type_t<Scalar, Other>> {
    using Common = std::common_type_t<Scalar, Other>;
    if (this->empty() || other.empty())
      return {};

    auto left =
      std::max(static_cast<Common>(this->x), static_cast<Common>(other.x));
    auto top =
      std::max(static_cast<Common>(this->y), static_cast<Common>(other.y));
    auto right = std::min(
      static_cast<Common>(this->x) + static_cast<Common>(this->width),
      static_cast<Common>(other.x) + static_cast<Common>(other.width)
    );
    auto bottom = std::min(
      static_cast<Common>(this->y) + static_cast<Common>(this->height),
      static_cast<Common>(other.y) + static_cast<Common>(other.height)
    );
    if (right <= left || bottom <= top)
      return {};

    return {
      .x = left,
      .y = top,
      .width = right - left,
      .height = bottom - top,
    };
  }

  static Rect from(const wlr_box& box) {
    return {
      .x = static_cast<Scalar>(box.x),
      .y = static_cast<Scalar>(box.y),
      .width = static_cast<Scalar>(box.width),
      .height = static_cast<Scalar>(box.height),
    };
  }

  wlr_box to_wlr_box() const {
    return {
      .x = static_cast<int>(this->x),
      .y = static_cast<int>(this->y),
      .width = static_cast<int>(this->width),
      .height = static_cast<int>(this->height),
    };
  }
};

template<typename Scalar>
inline Rect<Scalar>
lerp(const Rect<Scalar>& from, const Rect<Scalar>& to, Scalar t) {
  return Rect<Scalar> {
    .x = from.x + (to.x - from.x) * t,
    .y = from.y + (to.y - from.y) * t,
    .width = from.width + (to.width - from.width) * t,
    .height = from.height + (to.height - from.height) * t,
  };
}

} // namespace ura
