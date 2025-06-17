#pragma once

#include <string>

struct image2D {
  image2D();
  image2D(const std::string& path);
  ~image2D();

  void load(const bool& flipVertically);
  void write(const bool& flipVertically);

  std::string path;
  int w, h, channels;
  void* pixels = nullptr;

private:
  bool stbiLoad = false;
};

