#include "sha1.hpp"

#include <openssl/evp.h>
#include <stdexcept>

std::array<uint8_t, 20> sha1(std::span<const uint8_t> data) {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    throw std::runtime_error("EVP_MD_CTX_new failed");

  if (EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr) != 1 ||
      EVP_DigestUpdate(ctx, data.data(), data.size()) != 1) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("SHA-1 digest update failed");
  }

  std::array<uint8_t, 20> hash;
  unsigned int len = 0;
  if (EVP_DigestFinal_ex(ctx, hash.data(), &len) != 1 || len != 20) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("SHA-1 digest finalization failed");
  }

  EVP_MD_CTX_free(ctx);
  return hash;
}
