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
#include "utils.hpp"

#define WIDTH 10u
#define HEIGHT 10u

// #define DO_MASK
// #define DO_R16UI
// #define DO_DUMMY

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

  Shader mainShader("main_r8ui.comp");

  stbi_set_flip_vertically_on_load(false);
  stbi_flip_vertically_on_write(false);

  uvec2 texSize;
  u16 channels;
  void* pixelsDiffuse0 = nullptr;
  void* pixelsDiffuse1 = nullptr;

  #ifdef DO_MASK

  Shader maskShader("mask.comp");
  maskShader.setUniformTextureInt("diffuse", 1);
  // u16 tifDepth, tifSampleFormat;

  {
    int w, h, c;
    pixelsDiffuse0 = stbi_load("pre_mask0.png", &w, &h, &c, 0);
    pixelsDiffuse1 = stbi_load("pre_mask1.png", &w, &h, &c, 0);
    texSize = {w, h};
    channels = c;
  }
  // pixelsDiffuse0 = loadTif("bathymetry0.tif", texSize.x, texSize.y, channels, tifDepth, tifSampleFormat);
  // pixelsDiffuse1 = loadTif("bathymetry1.tif", texSize.x, texSize.y, channels, tifDepth, tifSampleFormat);

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
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R16I, texSize.x, texSize.y, 2);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texSize.x, texSize.y, 1, GL_RED_INTEGER, GL_SHORT, pixelsDiffuse0);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, texSize.x, texSize.y, 1, GL_RED_INTEGER, GL_SHORT, pixelsDiffuse1);

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

  #ifdef DO_DUMMY
    const char* westFile = "dummyMask0.png";
    const char* eastFile = "dummyMask1.png";
  #else
    const char* westFile = "mask0.png";
    const char* eastFile = "mask1.png";
  #endif

  const fspath outputDirPath = "output";

  {
    int w, h, c;
    pixelsDiffuse0 = stbi_load(westFile, &w, &h, &c, 0);
    pixelsDiffuse1 = stbi_load(eastFile, &w, &h, &c, 0);
    texSize = {w, h};
    channels = c;
  }

  #ifdef DO_R16UI
    u16* temp0 = new u16[texSize.x * texSize.y];
    u16* temp1 = new u16[texSize.x * texSize.y];
    scaleU8toU16((byte*)pixelsDiffuse0, temp0, texSize.x * texSize.y);
    scaleU8toU16((byte*)pixelsDiffuse1, temp1, texSize.x * texSize.y);
    stbi_image_free(pixelsDiffuse0);
    stbi_image_free(pixelsDiffuse1);
    pixelsDiffuse0 = temp0;
    pixelsDiffuse1 = temp1;

    constexpr GLuint pixelsType = GL_UNSIGNED_SHORT;
    constexpr GLenum internalFormat = GL_R16UI;

    mainShader = Shader("main_r16ui.comp");

  #else
    constexpr GLuint pixelsType = GL_UNSIGNED_BYTE;
    constexpr GLenum internalFormat = GL_R8UI;

  #endif

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

  constexpr size_t antiInfinityLoop = 1e4;
  constexpr uvec2 localSize(8u);
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
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texSize.x, texSize.y, 1, GL_RED_INTEGER, pixelsType, pixelsDiffuse0);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, texSize.x, texSize.y, 1, GL_RED_INTEGER, pixelsType, pixelsDiffuse1);

  #ifdef DO_R16UI
    delete[] (u16*)pixelsDiffuse0;
    delete[] (u16*)pixelsDiffuse1;
  #else
    stbi_image_free(pixelsDiffuse0);
    stbi_image_free(pixelsDiffuse1);
  #endif

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
    for (size_t k = 0; changed && k < antiInfinityLoop; k++) {
      GLuint zero = 0;
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

      bool swapped = k & 1;
      glBindImageTexture(swapped, texDistField0, 0, GL_TRUE, 0, GL_READ_ONLY, internalFormat);
      glBindImageTexture(1 - swapped, texDistField1, 0, GL_TRUE, 0, GL_WRITE_ONLY, internalFormat);

      printf("%s", std::format("iteration: {}\r", k).c_str());
      mainShader.setUniform1ui("beta", k * 2 + 1);
      glDispatchCompute(numGroups.x, numGroups.y, 2);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
      glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &changed);
    }
  }
  puts("");

  system(std::format("mkdir {}", outputDirPath.string()).c_str());

  #ifdef DO_R16UI
    const size_t bufSize = texSize.x * texSize.y * sizeof(u16);
    u16* pixels = new u16[texSize.x * texSize.y];
    std::string extension= "tif";
  #else
    const size_t bufSize = texSize.x * texSize.y * sizeof(byte);
    byte* pixels = new byte[texSize.x * texSize.y];
    std::string extension= "png";
  #endif

  for (size_t i = 0; i < 2; i++) {
    fspath outputFilePath = outputDirPath / std::format("distanceFieldWater21600_{}.{}", i, extension);
    glGetTextureSubImage(texDistField0, 0, 0, 0, i, texSize.x, texSize.y, 1, GL_RED_INTEGER, pixelsType, bufSize, pixels);

    #ifdef DO_R16UI
      saveTif_R16UI(outputFilePath.string().c_str(), texSize.x, texSize.y, pixels);
    #else
      if (!stbi_write_png(outputFilePath.string().c_str(), texSize.x, texSize.y, 1, pixels, texSize.x))
        puts("stbi write returned 0");
    #endif
  }

  #endif

  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    printf("GL error detected, type 0x%x\n", error);
    exit(1);
  }

  delete[] pixels;

  glfwTerminate();
  puts("Done");

  return 0;
}

