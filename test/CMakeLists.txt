cmake_minimum_required(VERSION 3.25)

project(MetadataTests CXX)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
include(GoogleTest)

#if (NOT TARGET FR::Metadata)
#  find_package(FR REQUIRED COMPONENT metadata)
#endif()

set(TEST_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/MetadataTest.cpp
)

add_executable(MetadataTests
  ${TEST_SRC}
)

TARGET_LINK_LIBRARIES(MetadataTests PUBLIC
  GTest::GTest
  GTest::Main
  FR::metadata
)
