#include "ura/util/image.hpp"

#include <string>

#include <raylib.h>

namespace ura {

std::optional<ImageData> load_image_rgba(std::string_view path) {
  auto fileName = std::string(path);
  Image image = LoadImage(fileName.c_str());
  if (!image.data || image.width <= 0 || image.height <= 0) {
    if (image.data)
      UnloadImage(image);
    return {};
  }
  ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  ImageData result;
  result.width = image.width;
  result.height = image.height;
  auto size = static_cast<std::size_t>(image.width) * image.height * 4;
  result.pixels.assign(
    static_cast<uint8_t*>(image.data),
    static_cast<uint8_t*>(image.data) + size
  );
  UnloadImage(image);
  return result;
}

} // namespace ura
