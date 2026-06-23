#include "zlib.hpp"

#include <stdexcept>
#include <zlib.h>

std::vector<uint8_t> zlibCompress(std::span<const uint8_t> data) {
  uLongf bound = compressBound(static_cast<uLong>(data.size()));
  std::vector<uint8_t> out(bound);

  int ret = compress(
      out.data(), &bound, data.data(), static_cast<uLong>(data.size()));
  if (ret != Z_OK)
    throw std::runtime_error("zlib compression failed (code " +
                             std::to_string(ret) + ")");

  out.resize(bound);
  return out;
}

std::vector<uint8_t> zlibDecompress(std::span<const uint8_t> data) {
  // Start at 4x input size and double on buffer exhaustion.
  std::vector<uint8_t> out(data.size() * 4 + 64);

  while (true) {
    uLongf size = static_cast<uLongf>(out.size());
    int ret = uncompress(
        out.data(), &size, data.data(), static_cast<uLong>(data.size()));
    if (ret == Z_OK) {
      out.resize(size);
      return out;
    }
    if (ret == Z_BUF_ERROR) {
      out.resize(out.size() * 2);
      continue;
    }
    throw std::runtime_error("zlib decompression failed (code " +
                             std::to_string(ret) + ")");
  }
}
