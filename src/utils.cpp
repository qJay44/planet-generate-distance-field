#include "utils.hpp"

#include "tiffio.h"

s16* loadTif(
  const char* path,
  u32& width,
  u32& height,
  u16& channels,
  u16& depth,
  u16& sampleFormat
) {
  TIFF* tif = TIFFOpen(path, "r");
  if (!tif) {
    printf("tif can't open file [%s]\n", path);
    exit(1);
  }

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &channels);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &depth);
  TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

  s16* buf = new s16[width * height];

  for (u32 row = 0; row < height; row++) {
    s16* bufRow = buf + row * width;

    if (TIFFReadScanline(tif, bufRow, row) < 0) {
      puts("tif scanline read error");
      exit(1);
    }
  }

  // for (u32 y = 0; y < height / 2; ++y) {
  //   short* rowTop = buf + y * width;
  //   short* rowBottom = buf + (height - y - 1) * width;
  //   std::swap_ranges(rowTop, rowTop + width, rowBottom);
  // }

  return buf;

  TIFFClose(tif);
}

void saveTif_R16UI(
  const char* path,
  const u32& width,
  const u32& height,
  const u16* pixels
) {
  TIFF* tif = TIFFOpen(path, "w");
  if (!tif) {
    printf("tif can't open file [%s]\n", path);
    exit(1);
  }

  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
  TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

  for (u32 row = 0; row < height; row++) {
    const u16* rowPtr = pixels + row * width;

    if (TIFFWriteScanline(tif, (void*)rowPtr, row) < 0) {
      printf("Failed to write scanline, row: [%i]\n", row);
      TIFFClose(tif);
      exit(1);
    }
  }

  TIFFClose(tif);
}
