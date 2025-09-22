cmake_minimum_required(VERSION 3.25)

project(FRMetadata)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build unit tests" ON)
# This will use nanobind to build a shared library that you
# can load into Python so you can interact with the library
# objects from the Python interpreter
option(BUILD_PYTHON_API "Build Python API" ON)
option(BUILD_REACT_FRONTEND "Build React Frontend" ON)

if (BUILD_PYTHON_API)
  find_package(PkgConfig)
  pkg_check_modules(Pistache REQUIRED IMPORTED_TARGET libpistache)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(nanobind CONFIG REQUIRED)
endif()

set(HEADER_DIR "include/fr/metadata")
set(INTERFACE_HEADERS
  "${HEADER_DIR}/metadata.h"
)

add_library(metadata INTERFACE)

set_target_properties(metadata PROPERTIES
  FRAMEWORK TRUE
  PUBLIC_HEADER "${INTERFACE_HEADERS}"
)

target_include_directories(metadata
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_library(FR::metadata ALIAS metadata)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()

if (BUILD_PYTHON_API)
  nanobind_add_module(
    FRMetadata
    NB_STATIC STABLE_ABI FREE_THREADED
    ${CMAKE_CURRENT_SOURCE_DIR}/python_api/FRMetadata.cpp
  )
  target_link_libraries(FRMetadata PUBLIC
    FR::metadata
    PkgConfig::Pistache
    # Wait a minute! WTF is *magic*? I hear you cry. Magic is the
    # library that file uses to identify the type of a file. I'm using it
    # to determine mime types. They don't seem to have a package config
    # or a find-file, so you may need to apt install libmagic-dev or some
    # such. TODO: File a CMake integration PR with libmagic maintainer
    # at some point in the future.
    #
    # "man 5 magic" and "man 3 libmagic" for details.
    magic
  )
endif()

if (BUILD_REACT_FRONTEND)
  target_compile_definitions(metadata INTERFACE EXPOSE_UI)
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/MetadataUi/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ui"
  )
  add_custom_target(npm-install-ui ALL COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/ui && npm install && npm run build)
endif()

