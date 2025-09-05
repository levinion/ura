#pragma once

#include <set>
#include <sol/sol.hpp>
#include <string>
#include <vector>

namespace ura {

class UraPluginHook {
public:
  std::string group;
  int priority;
  bool final;
  sol::protected_function callback;

  bool operator<(const UraPluginHook& other) const {
    return this->priority > other.priority;
  }
};

class UraHook {
public:
  template<typename T, typename... Args>
  inline std::vector<T> execute(Args&&... args) {
    std::vector<T> v;
    for (auto plugin_hook : this->plugin_hooks) {
      sol::protected_function_result result =
        plugin_hook.callback(std::forward<Args>(args)...);
      if (result.valid()) {
        auto ret = result.get<std::optional<T>>();
        if (ret)
          v.push_back(ret.value());
      }
      if (plugin_hook.final)
        break;
    }
    return v;
  }

  template<typename... Args>
  inline void execute(Args&&... args) {
    for (auto plugin_hook : this->plugin_hooks) {
      sol::protected_function_result result =
        plugin_hook.callback(std::forward<Args>(args)...);
      if (plugin_hook.final)
        break;
    }
  }

  inline void insert(UraPluginHook&& p) {
    this->plugin_hooks.insert(p);
  }

private:
  std::multiset<UraPluginHook> plugin_hooks;
};

} // namespace ura
