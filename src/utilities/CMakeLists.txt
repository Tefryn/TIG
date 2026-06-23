find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(utilities
  hex.cpp
  sha1.cpp
  zlib.cpp
)

target_include_directories(utilities PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(utilities PUBLIC
  OpenSSL::Crypto
  ZLIB::ZLIB
)
