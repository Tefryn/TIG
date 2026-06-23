add_executable(tig_tests
  test_hex.cpp
)

target_link_libraries(tig_tests PRIVATE
  utilities
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tig_tests)
