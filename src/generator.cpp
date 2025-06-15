#include "generator.hpp"

#include <cstdlib>
#include <format>
#include <map>
#include <string>

#include "utils/utils.hpp"
#include "utils/loadTif.hpp"
#include "utils/saveTif.hpp"
#include "utils/status.hpp"
#include "ComputeShader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const std::map<std::string, u8> extensionsMap{
  {".jpeg", 0},
  {".jpg" , 1},
  {".png" , 2},
  {".tga" , 3},
  {".bmp" , 4},
  {".psd" , 5},
  {".gif" , 6},
  {".hdr" , 7},
  {".pic" , 8},
  {".tif" , 9},
};

namespace generator {

void genMask(const fspath &path0, const fspath &path1) {
  const std::string path0String = path0.string();
  const std::string path1String = path1.string();
  const char* path0Cstr = path0String.c_str();
  const char* path1Cstr = path1String.c_str();

  const std::string ext0 = path0.extension().string();
  const std::string ext1 = path1.extension().string();

  if (ext0 != ext1)
    error("The files extension are not the same [{}] and [{}]\n", ext0, ext1);

  Shader shader;
  uvec2 texSize;
  GLenum internalFormat;
  GLenum format;
  GLenum pixelsType;
  void* pixelsDiffuse0 = nullptr;
  void* pixelsDiffuse1 = nullptr;

  const auto& it = extensionsMap.find(ext0);
  if (it == extensionsMap.end())
    error("Unsupported extension [{}]", ext0);

  const bool isTif = it->second == 9;

  if (isTif) {
    pixelsDiffuse0 = loadTif_R16I(path0Cstr, &texSize.x, &texSize.y);
    pixelsDiffuse1 = loadTif_R16I(path1Cstr, &texSize.x, &texSize.y);
    internalFormat = GL_R16I;
    format = GL_RED_INTEGER;
    pixelsType = GL_SHORT;
    shader = Shader("mask_tif.comp");

  } else {
    stbi_set_flip_vertically_on_load(false);
    stbi_flip_vertically_on_write(false);

    int w, h, channels;
    pixelsDiffuse0 = stbi_load(path0Cstr, &w, &h, &channels, 0);
    pixelsDiffuse1 = stbi_load(path1Cstr, &w, &h, &channels, 0);
    texSize = {w, h};
    internalFormat = GL_R8;
    format = GL_RED;
    pixelsType = GL_UNSIGNED_BYTE;
    shader = Shader("mask.comp");

    if (pixelsDiffuse0 == nullptr) error("stbi can't open [{}]\n", path0Cstr);
    if (pixelsDiffuse1 == nullptr) error("stbi can't open [{}]\n", path1Cstr);
    if (channels != 1)             error("Unexpected number of channels");
  }

  shader.setUniformTextureInt("diffuse", 1);

  GLuint texPreMask;
  glGenTextures(1, &texPreMask);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texPreMask);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, texSize.x, texSize.y, 2);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texSize.x, texSize.y, 1, format, pixelsType, pixelsDiffuse0);
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, texSize.x, texSize.y, 1, format, pixelsType, pixelsDiffuse1);

  if (isTif) {
    delete[] (u16*)pixelsDiffuse0;
    delete[] (u16*)pixelsDiffuse1;
  } else {
    stbi_image_free(pixelsDiffuse0);
    stbi_image_free(pixelsDiffuse1);
  }

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

  status::start("Generating", "mask");
  shader.use();
  glDispatchCompute(texSize.x, texSize.y, 2);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  status::end(true);

  byte* pixels = new byte[texSize.x * texSize.y];
  for (size_t i = 0; i < 2; i++) {
    std::string name =std::format("mask{}.png", i);
    status::start("Saving", name);
    glGetTextureSubImage(texMask, 0, 0, 0, i, texSize.x, texSize.y, 1, GL_RED, GL_UNSIGNED_BYTE, texSize.x * texSize.y, pixels);
    stbi_write_png(name.c_str(), texSize.x, texSize.y, 1, pixels, texSize.x);
    status::end(true);
  }
}

void genDistanceField(const fspath& path0, const fspath& path1, const GLenum& internalFormat) {
  const std::string path0String = path0.string();
  const std::string path1String = path1.string();
  const char* path0Cstr = path0String.c_str();
  const char* path1Cstr = path1String.c_str();

  const std::string ext0 = path0.extension().string();
  const std::string ext1 = path1.extension().string();

  if (ext0 != ext1)
    error("The files extension are not the same [{}] and [{}]\n", ext0, ext1);

  uvec2 texSize;
  void* pixelsDiffuse0 = nullptr;
  void* pixelsDiffuse1 = nullptr;

  {
    int w, h, channels;
    pixelsDiffuse0 = stbi_load(path0Cstr, &w, &h, &channels, 0);
    pixelsDiffuse1 = stbi_load(path1Cstr, &w, &h, &channels, 0);
    texSize = {w, h};

    if (pixelsDiffuse0 == nullptr) error(std::format("stbi can't open [%s]\n", path0Cstr));
    if (pixelsDiffuse1 == nullptr) error(std::format("stbi can't open [%s]\n", path1Cstr));
    if (channels != 1)             error("Unexpected number of channels");
  }

  Shader shader;
  GLuint pixelsType = 0;
  bool isTif = false;
  size_t bufSize = 0;
  void* pixels = nullptr;

  switch (internalFormat) {
    case GL_R8UI: {
      pixelsType = GL_UNSIGNED_BYTE;
      shader = Shader("main_r8ui.comp");
      isTif = false;
      bufSize = texSize.x * texSize.y * sizeof(byte);
      pixels = new byte[texSize.x * texSize.y];
      break;
    }
    case GL_R16UI: {
      u16* temp0 = new u16[texSize.x * texSize.y];
      u16* temp1 = new u16[texSize.x * texSize.y];
      scaleU8toU16((byte*)pixelsDiffuse0, temp0, texSize.x * texSize.y);
      scaleU8toU16((byte*)pixelsDiffuse1, temp1, texSize.x * texSize.y);
      stbi_image_free(pixelsDiffuse0);
      stbi_image_free(pixelsDiffuse1);

      pixelsDiffuse0 = temp0;
      pixelsDiffuse1 = temp1;
      pixelsType = GL_UNSIGNED_SHORT;
      shader = Shader("main_r16ui.comp");
      isTif = true;
      bufSize = texSize.x * texSize.y * sizeof(u16);
      pixels = new u16[texSize.x * texSize.y];

      break;
    }
    default:
      error("[genDistanceField] Unexpected format ({})", internalFormat);
  }

  constexpr size_t antiInfinityLoop = 1e4;
  constexpr uvec2 localSize(8u); // NOTE: Must match in the shader
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

  GLuint texDistField1;
  glGenTextures(1, &texDistField1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texDistField1);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, texSize.x, texSize.y, 2);

  if (isTif) {
    delete[] (u16*)pixelsDiffuse0;
    delete[] (u16*)pixelsDiffuse1;
  } else {
    stbi_image_free(pixelsDiffuse0);
    stbi_image_free(pixelsDiffuse1);
  }

  GLuint changeFlagBuffer;
  glGenBuffers(1, &changeFlagBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_COPY);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, changeFlagBuffer); // binding point 2

  for (size_t j = 0; j < 2; j++) {
    if (j)
      shader.setUniform2ui("passOffset", {1, 0});
    else
      shader.setUniform2ui("passOffset", {0, 1});

    GLuint changed = 1;
    for (size_t k = 0; changed && k < antiInfinityLoop; k++) {
      GLuint zero = 0;
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

      bool swapped = k & 1;
      glBindImageTexture(swapped, texDistField0, 0, GL_TRUE, 0, GL_READ_ONLY, internalFormat);
      glBindImageTexture(1 - swapped, texDistField1, 0, GL_TRUE, 0, GL_WRITE_ONLY, internalFormat);

      printf("%s", std::format("Processing j: {}, k: {}\r", j, k).c_str());
      shader.setUniform1ui("beta", k * 2 + 1);
      glDispatchCompute(numGroups.x, numGroups.y, 2);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, changeFlagBuffer);
      glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &changed);
    }
  }
  puts("");

  for (size_t i = 0; i < 2; i++) {
    std::string outputFileName = std::format("distanceFieldWater{}_{}{}", (u16)texSize.x * 2, i, ext0);

    status::start("Saving", outputFileName);
    glGetTextureSubImage(texDistField0, 0, 0, 0, i, texSize.x, texSize.y, 1, GL_RED_INTEGER, pixelsType, bufSize, pixels);
    if (isTif)
      saveTif_R16UI(outputFileName.c_str(), texSize.x, texSize.y, (u16*)pixels);
    else
      if (!stbi_write_png(outputFileName.c_str(), texSize.x, texSize.y, 1, pixels, texSize.x)) {
        status::end(false);
        error("stbi write returned 0");
      }

    status::end(true);
  }

  if (isTif)
    delete[] (u16*)pixels;
  else
    stbi_image_free(pixels);
}

} // namespace generator

