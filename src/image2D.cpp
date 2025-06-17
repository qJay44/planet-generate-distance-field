#include "image2D.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "utils/utils.hpp"

image2D::image2D() {}

image2D::image2D(const std::string& path) : path(path) {}

image2D::~image2D() {
  if (stbiLoad) {
    if (pixels)
      stbi_image_free(pixels);
    else
      error("[image2D] Loaded pixels were deleted mannualy [{}]", path);
  } else {
    if (pixels)
      error("[image2D] Pixels were not cleared [{}]", path);
  }
}

void image2D::load(const bool& flipVertically) {
  stbi_set_flip_vertically_on_load(flipVertically);
  pixels = stbi_load(path.c_str(), &w, &h, &channels, 0);
  stbiLoad = true;
  if (!pixels)
    error("[image2D] stbi didn't load [{}]", path);
}

void image2D::write(const bool& flipVertically) {
  stbi_flip_vertically_on_write(flipVertically);
  if (!stbi_write_png(path.c_str(), w, h, channels, pixels, w * channels))
    error("[image2D] stbi didn't write [{}]", path);
}

