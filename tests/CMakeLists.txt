add_executable(tig_tests
  test_hex.cpp
  test_zlib.cpp
  test_sha1.cpp
  test_types.cpp
  test_object_store.cpp
  test_blob.cpp
)

target_link_libraries(tig_tests PRIVATE
  objects
  utilities
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tig_tests)
