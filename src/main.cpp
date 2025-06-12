// https://stackoverflow.com/a/29681646/17694832

#include "pch.hpp"
#include <cassert>
#include <cstdio>
#include <format>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "ComputeShader.hpp"
#include "tiffio.h"

#define WIDTH 10u
#define HEIGHT 10u

void writeTif_R16UI(const char* fileName, const int& width, const int& height, const u16* pixels);

int main() {
  // Assuming the executable is launching from its own directory
  SetCurrentDirectory("../../../src");

  // GLFW init
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Window init
  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "", NULL, NULL);
  if (!window) {
    printf("Failed to create GFLW window\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);

  // GLAD init
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to initialize GLAD\n");
    return EXIT_FAILURE;
  }

  glViewport(0, 0, WIDTH, HEIGHT);

  Shader mainShader("main.comp");
  mainShader.setUniformTexture("diffuse", 1);

  stbi_set_flip_vertically_on_load(false);
  stbi_flip_vertically_on_write(false);

  ivec2 texSize;
  int channels;
  byte* pixelsDiffuse0 = nullptr;
  byte* pixelsDiffuse1 = nullptr;

  // #define DO_MASK

  #ifdef DO_MASK

  Shader maskShader("mask.comp");
  maskShader.setUniformTexture("diffuse", 1);

  pixelsDiffuse0 = stbi_load("pre_mask0.png", &texSize.x, &texSize.y, &channels, 0);
  pixelsDiffuse1 = stbi_load("pre_mask1.png", &texSize.x, &texSize.y, &channels, 0);

  if (pixelsDiffuse0 == nullptr) {
    printf("stbi can't open [%s]\n", "pre_mask0.png");
    exit(1);
  }

  if (pixelsDiffuse1 == nullptr) {
    printf("stbi can't open [%s]\n", "pre_mask1.png");
    exit(1);
  }

  if (channels != 1) {
    puts("Unexpected number of channels");
    exit(1);
  }

  GLuint texPreMask;
  glGenTextures(1, &texPreMask);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texPreMask);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, texSize.x, texSize.y, 2);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, pixelsDiffuse0);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, pixelsDiffuse1);

  GLuint texMask;
  glGenTextures(1, &texMask);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texMask);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, texSize.x, texSize.y, 2);
  glBindImageTexture(0, texMask, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R8);

  maskShader.use();
  glDispatchCompute(texSize.x, texSize.y, 2);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  byte* pixels = new byte[texSize.x * texSize.y];
  for (size_t i = 0; i < 2; i++) {
    glGetTextureSubImage(texMask, 0, 0, 0, i, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, texSize.x * texSize.y, pixels);
    stbi_write_png(std::format("mask{}.png", i).c_str(), texSize.x, texSize.y, 1, pixels, texSize.x);
  }

  #else

  const char* westFile = "mask0.png";
  const char* eastFile = "mask1.png";
  fspath outputDirPath = "output";

  pixelsDiffuse0 = stbi_load(westFile, &texSize.x, &texSize.y, &channels, 0);
  pixelsDiffuse1 = stbi_load(eastFile, &texSize.x, &texSize.y, &channels, 0);

  if (pixelsDiffuse0 == nullptr) {
    printf("stbi can't open [%s]\n", westFile);
    exit(1);
  }

  if (pixelsDiffuse1 == nullptr) {
    printf("stbi can't open [%s]\n", eastFile);
    exit(1);
  }

  if (channels != 1) {
    printf("Unexpected number of channels [%d]\n", channels);
    exit(1);
  }

  constexpr size_t antiInfinityLoop = 500u;
  constexpr GLenum internalFormat = GL_R8UI;
  constexpr uvec2 localSize{16, 16};
  const uvec2 numGroups = (uvec2(texSize) + localSize - 1u) / localSize;

  GLuint texDistField0;
  glGenTextures(1, &texDistField0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texDistField0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, texSize.x, texSize.y, 2);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texSize.x, texSize.y, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pixelsDiffuse0);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, texSize.x, texSize.y, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pixelsDiffuse1);

  GLuint texDistField1;
  glGenTextures(1, &texDistField1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texDistField1);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, texSize.x, texSize.y, 2);

  GLuint changeFlagBuffer;
  glGenBuffers(1, &changeFlagBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_COPY);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, changeFlagBuffer); // binding point 2

  for (size_t i = 0; i < 2; i++) {
    if (i) {
      puts("Generating horizontal");
      mainShader.setUniform2ui("passOffset", {1, 0});
    } else {
      puts("Generating vertical");
      mainShader.setUniform2ui("passOffset", {0, 1});
    }

    GLuint changed = 1;
    for (size_t j = 0; changed && j < antiInfinityLoop; j++) {
      GLuint zero = 0;
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

      bool swapped = j & 1;
      glBindImageTexture(swapped, texDistField0, 0, GL_TRUE, 0, GL_READ_ONLY, internalFormat);
      glBindImageTexture(1 - swapped, texDistField1, 0, GL_TRUE, 0, GL_WRITE_ONLY, internalFormat);

      printf("%s", std::format("iteration: {}\r", j).c_str());
      mainShader.setUniform1ui("beta", 1);
      glDispatchCompute(numGroups.x, numGroups.y, 2);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
      glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &changed);
    }
  }
  puts("");

  system(std::format("mkdir {}", outputDirPath.string()).c_str());

  byte* pixels = new byte[texSize.x * texSize.y];
  for (size_t i = 0; i < 2; i++) {
    fspath output = outputDirPath / std::format("distanceFieldWater21600_{}.png", i);
    glGetTextureSubImage(texDistField0, 0, 0, 0, i, texSize.x, texSize.y, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, texSize.x * texSize.y, pixels);
    if (!stbi_write_png(output.string().c_str(), texSize.x, texSize.y, 1, pixels, texSize.x))
      puts("stbi write returned 0");
  }

  #endif

  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    printf("GL error detected, type 0x%x\n", error);
    exit(1);
  }

  stbi_image_free(pixelsDiffuse0);
  stbi_image_free(pixelsDiffuse1);
  delete[] pixels;

  glfwTerminate();
  puts("Done");

  return 0;
}

void writeTif_R16UI(const char* fileName, const int& width, const int& height, const u16* pixels) {
  TIFF* tif = TIFFOpen(fileName, "w");
  if (!tif) {
    printf("Tif can't write to [%s]\n", fileName);
    exit(1);
  }

  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  // TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

  for (int row = 0; row < height; ++row)
    TIFFWriteScanline(tif, (void*)&pixels[row * width], row, 0);

  TIFFClose(tif);
}

