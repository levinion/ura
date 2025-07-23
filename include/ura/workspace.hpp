#pragma once
#include "ura/client.hpp"
#include "ura/toplevel.hpp"
#include <list>
#include <memory>
#include <optional>

namespace ura {

class UraOutput;

class UraFocusStack {
public:
  std::optional<UraClient> top();
  int size();
  std::optional<UraClient> pop();

  template<typename T>
  void push(T client) {
    if constexpr (std::is_same_v<T, UraClient>) {
      this->stack.push_back(client);
    } else {
      this->stack.push_back(UraClient::from(client));
    }
  }

  template<typename T>
  void remove(T client) {
    UraClient cl;
    if constexpr (std::is_same_v<T, UraClient>) {
      cl = client;
    } else {
      cl = UraClient::from(client);
    }
    this->stack.remove(cl);
  }

  template<typename T>
  void move_to_top(T client) {
    this->remove(client);
    this->push(client);
  }

  template<typename T>
  bool is_top(T client) {
    UraClient cl;
    if constexpr (std::is_same_v<T, UraClient>) {
      cl = client;
    } else {
      cl = UraClient::from(client);
    }
    return this->size() == 0 ? false : this->top().value() == cl;
  }

private:
  std::list<UraClient> stack;
};

class UraWorkSpace {
public:
  std::list<UraToplevel*> toplevels;
  UraOutput* output;
  UraFocusStack focus_stack;
  static std::unique_ptr<UraWorkSpace> init();
  void enable(bool enabled);
  int index();
};

} // namespace ura
