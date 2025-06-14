#pragma once

s16* loadTif_R16UI(
  const char* path,
  u32& width,
  u32& height,
  u16& channels,
  u16& depth,
  u16& sampleFormat
);

void saveTif_R16UI(
  const char* path,
  const u32& width,
  const u32& height,
  const u16* pixels
);

inline void scaleU8toU16(const uint8_t* src, uint16_t* dst, size_t count) {
  for (size_t i = 0; i < count; i++)
    dst[i] = static_cast<uint16_t>(src[i]) * 257; // 255 * 257 ≈ 65535
}

