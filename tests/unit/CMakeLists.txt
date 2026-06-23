add_executable(tig_tests
  hex_test.cpp
  zlib_test.cpp
  sha1_test.cpp
  types_test.cpp
  object_store_test.cpp
  blob_test.cpp
  tree_test.cpp
  commit_test.cpp
)

target_link_libraries(tig_tests PRIVATE
  objects
  utilities
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tig_tests)
