#pragma once

#include <set>
#include <sol/sol.hpp>
#include <string>

namespace ura {

class UraPluginHook {
public:
  std::string group;
  int priority;
  sol::protected_function callback;

  bool operator<(const UraPluginHook& other) const {
    return this->priority > other.priority;
  }
};

class UraHook {
public:
  template<typename... Args>
  inline void execute(Args&&... args) {
    for (auto plugin_hook : this->plugin_hooks) {
      plugin_hook.callback(std::forward<Args>(args)...);
    }
  }

  inline void insert(UraPluginHook&& p) {
    this->plugin_hooks.insert(p);
  }

private:
  std::multiset<UraPluginHook> plugin_hooks;
};

} // namespace ura
