#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace ura {

struct ImageData {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels; // RGBA8
};

std::optional<ImageData> load_image_rgba(std::string_view path);

} // namespace ura
