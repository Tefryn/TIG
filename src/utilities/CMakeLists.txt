add_library(utilities
  hex.cpp
)

target_include_directories(utilities PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)
