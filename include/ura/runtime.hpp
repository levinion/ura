#include "ura/ura.hpp"
#include <memory>
#include <unordered_map>

namespace ura {

class UraContext {
public:
  wl_listener listener;
  void* data = nullptr;
};

class UraRuntime {
public:
  template<typename F>
  void register_callback(wl_signal* signal, F f, void* data) {
    auto ctx = std::make_unique<UraContext>();
    ctx->listener.notify = f;
    wl_signal_add(signal, &ctx->listener);

    ctx->data = data;
    this->storage[&ctx->listener] = std::move(ctx);
  }

  static std::unique_ptr<UraRuntime> init();

  template<typename T>
  T fetch(wl_listener* listener) {
    return static_cast<T>(this->storage[listener]->data);
  }

  void remove(wl_listener* listener) {
    wl_list_remove(&listener->link);
    this->storage.erase(listener);
  }

private:
  std::unordered_map<wl_listener*, std::unique_ptr<UraContext>> storage;
};
} // namespace ura
