#include "ura/runtime.hpp"
#include <memory>

namespace ura {

std::unique_ptr<UraRuntime> UraRuntime::init() {
  return std::make_unique<UraRuntime>();
}

} // namespace ura
