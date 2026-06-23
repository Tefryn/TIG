add_library(objects
  types.cpp
  object_store.cpp
  blob.cpp
  tree.cpp
  commit.cpp
)

target_include_directories(objects PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(objects PUBLIC
  utilities
)
