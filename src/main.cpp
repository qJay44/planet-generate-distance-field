// https://stackoverflow.com/a/29681646/17694832

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <format>

#include "ComputeShader.hpp"
#include "pch.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIDTH 10u
#define HEIGHT 10u

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
  byte* pixels = nullptr;

  #define DO_MASK

  #ifdef DO_MASK

  Shader maskShader("mask.comp");
  maskShader.setUniformTexture("diffuse", 1);

  pixelsDiffuse0 = stbi_load("pre_mask0.png", &texSize.x, &texSize.y, &channels, 0);
  pixelsDiffuse1 = stbi_load("pre_mask1.png", &texSize.x, &texSize.y, &channels, 0);

  if (channels != 1) {
    puts("Unexpected number of channels");
    exit(1);
  }

  if (pixelsDiffuse0 == nullptr) {
    printf("stbi can't open [%s]\n", "pre_mask0.png");
    exit(1);
  }

  if (pixelsDiffuse1 == nullptr) {
    printf("stbi can't open [%s]\n", "pre_mask1.png");
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

  pixels = new byte[texSize.x * texSize.y];
  for (size_t i = 0; i < 2; i++) {
    glGetTextureSubImage(texMask, 0, 0, 0, i, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, texSize.x * texSize.y, pixels);
    stbi_write_png(std::format("mask{}.png", i).c_str(), texSize.x, texSize.y, 1, pixels, texSize.x);
  }

  #else

  const char* westFile = "heightmapWater21600_0.png";
  const char* eastFile = "heightmapWater21600_1.png";
  fspath outputDirPath = "output";

  pixelsDiffuse0 = stbi_load(westFile, &texSize.x, &texSize.y, &channels, 0);
  pixelsDiffuse1 = stbi_load(eastFile, &texSize.x, &texSize.y, &channels, 0);

  if (channels != 1) {
    puts("Unexpected number of channels");
    exit(1);
  }

  if (pixelsDiffuse0 == nullptr) {
    printf("stbi can't open [%s]\n", westFile);
    exit(1);
  }

  if (pixelsDiffuse1 == nullptr) {
    printf("stbi can't open [%s]\n", eastFile);
    exit(1);
  }

  GLuint texDistField0;
  glGenTextures(1, &texDistField0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texDistField0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, texSize.x, texSize.y, 2);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, pixelsDiffuse0);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, pixelsDiffuse1);
  glBindImageTexture(0, texDistField0, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R8);

  GLuint texDistField1;
  glGenTextures(1, &texDistField1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texDistField1);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, texSize.x, texSize.y, 2);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, pixelsDiffuse0);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, pixelsDiffuse1);
  glBindImageTexture(1, texDistField1, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R8);


  system(std::format("mkdir {}", outputDirPath.string()).c_str());

  constexpr size_t maxIterations = 1;

  for (size_t i = 0; i < 1; i++) {
    if (i) {
      puts("Generating horizontal");
      mainShader.setUniform2ui("passOffset", {1, 0});
    } else {
      puts("Generating vertical");
      mainShader.setUniform2ui("passOffset", {0, 1});
    }

    for (size_t k = 0; k < maxIterations; k++) {
      printf("%s", std::format("{}/{}\r", k + 1, maxIterations).c_str());
      mainShader.setUniform1f("iterationScale", static_cast<float>(k) / maxIterations);
      glDispatchCompute(texSize.x, texSize.y, 2);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
  }
  puts("");

  byte* pixels = new byte[texSize.x * texSize.y * channels];
  for (size_t i = 0; i < 2; i++) {
    std::string outputName = std::format("distanceFieldWater21600_{}.png", i);
    glGetTextureSubImage(texDistField0, 0, 0, 0, i, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, texSize.x * texSize.y * channels, pixels);
    stbi_write_png((outputDirPath / outputName).string().c_str(), texSize.x, texSize.y, channels, pixels, channels * texSize.x);
  }

  #endif

  stbi_image_free(pixelsDiffuse0);
  stbi_image_free(pixelsDiffuse1);
  delete[] pixels;

  glfwTerminate();
  puts("Done");

  return 0;
}

