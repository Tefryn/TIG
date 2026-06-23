find_package(ZLIB REQUIRED)

add_library(utilities
  hex.cpp
  zlib_utils.cpp
)

target_include_directories(utilities PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(utilities PUBLIC
  ZLIB::ZLIB
)
