#pragma once

#include <algorithm>
#include <optional>
#include <vector>
#include "flexible/flexible.hpp"
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

  flexible::object to_flexible() {
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

template<typename T>
class Vec {
public:
  template<typename U>
  void push_back(U&& x) {
    this->v.push_back(std::forward<U>(x));
  }

  // Find the first element equals to x. Gernerally elements should be unique.
  constexpr std::optional<size_t> search(T& x) {
    for (int i = 0; i < this->v.size(); i++) {
      if (x == this->v[i]) {
        return i;
      }
    }
    return {};
  }

  constexpr size_t size() {
    return this->v.size();
  }

  void remove(T x) {
    auto it = std::remove(this->v.begin(), this->v.end(), x);
    this->v.erase(it, this->v.end());
  }

  void remove_n(int index) {
    auto it = this->v.begin();
    std::advance(it, index);
    this->v.erase(it);
  }

  void push_front(T x) {
    this->v.insert(this->v.begin(), x);
  }

  void erase(auto it) {
    this->v.erase(it);
  }

  void pop_back() {
    this->v.pop_back();
  }

  std::vector<T>::iterator begin() {
    return this->v.begin();
  }

  std::vector<T>::iterator end() {
    return this->v.end();
  }

  std::vector<T>::reverse_iterator rbegin() {
    return this->v.rbegin();
  }

  std::vector<T>::reverse_iterator rend() {
    return this->v.rend();
  }

  constexpr bool empty() {
    return this->v.empty();
  }

  T& back() {
    return this->v.back();
  }

  T& front() {
    return this->v.front();
  }

  T* get(int index) {
    if (index < 0 || index >= this->v.size())
      return nullptr;
    return &this->v[index];
  }

private:
  std::vector<T> v;
};
} // namespace ura
