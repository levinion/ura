#pragma once

#include <memory>
#include <vector>
#include "ura/core/server.hpp"

namespace ura {

class UraRuntime {
public:
  template<typename F>
  void register_callback(wl_signal* signal, F f, void* data) {
    auto listener = std::make_unique<wl_listener>();
    listener->notify = f;
    wl_signal_add(signal, listener.get());

    this->storage[listener.get()] = data;
    if (!this->listeners.contains(data)) {
      this->listeners[data] = std::vector<std::unique_ptr<wl_listener>> {};
    }
    this->listeners[data].push_back(std::move(listener));
  }

  inline static std::unique_ptr<UraRuntime> init() {
    return std::make_unique<UraRuntime>();
  }

  // fetch resource by listener
  template<typename T>
  inline T fetch(wl_listener* listener) {
    return static_cast<T>(this->storage[listener]);
  }

  // remove all items linked with data, this will also delete data itself
  template<typename T>
  void remove(T data) {
    for (auto& listener : this->listeners[data]) {
      wl_list_remove(&listener->link);
      this->storage.erase(listener.get());
    }
    this->listeners.erase(data);
  }

private:
  // listener to data
  absl::flat_hash_map<wl_listener*, void*> storage;
  // data to listeners
  absl::flat_hash_map<void*, std::vector<std::unique_ptr<wl_listener>>>
    listeners;
};
} // namespace ura
