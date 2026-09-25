#pragma once
// Minimal, dependency-light PNG writer (24-bit RGB, no filtering) for the
// sim's headless screenshot mode. Uses zlib for the IDAT deflate stream and
// CRC32 — both already on every macOS/Linux dev box, no vendoring needed.

#include <cstdint>
#include <cstdio>
#include <vector>
#include <zlib.h>
#include "Framebuffer.h"

inline void png_put_u32(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back((v >> 24) & 0xFF);
  out.push_back((v >> 16) & 0xFF);
  out.push_back((v >> 8) & 0xFF);
  out.push_back(v & 0xFF);
}

inline void png_chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
  png_put_u32(out, (uint32_t)data.size());
  std::vector<uint8_t> typeAndData(type, type + 4);
  typeAndData.insert(typeAndData.end(), data.begin(), data.end());
  out.insert(out.end(), typeAndData.begin(), typeAndData.end());
  uint32_t crc = (uint32_t)crc32(0L, typeAndData.data(), (uInt)typeAndData.size());
  png_put_u32(out, crc);
}

inline bool write_png(const char* path, const Framebuffer& fb) {
  // Raw scanlines: each row prefixed with filter type 0 (None).
  std::vector<uint8_t> raw;
  raw.reserve((size_t)(fb.w * 3 + 1) * fb.h);
  for (int y = 0; y < fb.h; y++) {
    raw.push_back(0);
    const uint8_t* row = &fb.rgb[(size_t)y * fb.w * 3];
    raw.insert(raw.end(), row, row + fb.w * 3);
  }

  uLongf boundSz = compressBound((uLong)raw.size());
  std::vector<uint8_t> compressed(boundSz);
  if (compress2(compressed.data(), &boundSz, raw.data(), (uLong)raw.size(), 6) != Z_OK)
    return false;
  compressed.resize(boundSz);

  std::vector<uint8_t> out;
  static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  out.insert(out.end(), sig, sig + 8);

  std::vector<uint8_t> ihdr;
  png_put_u32(ihdr, (uint32_t)fb.w);
  png_put_u32(ihdr, (uint32_t)fb.h);
  ihdr.push_back(8);  // bit depth
  ihdr.push_back(2);  // color type: RGB
  ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
  png_chunk(out, "IHDR", ihdr);
  png_chunk(out, "IDAT", compressed);
  png_chunk(out, "IEND", {});

  FILE* f = std::fopen(path, "wb");
  if (!f) return false;
  std::fwrite(out.data(), 1, out.size(), f);
  std::fclose(f);
  return true;
}
