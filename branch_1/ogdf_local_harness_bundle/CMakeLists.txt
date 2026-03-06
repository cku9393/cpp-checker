cmake_minimum_required(VERSION 3.20)
project(rewrite_r_harness LANGUAGES CXX)

option(USE_OGDF "Enable OGDF-backed raw SPQR backend" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(USE_OGDF)
  if(NOT OGDF_ROOT STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH "${OGDF_ROOT}")
  endif()

  find_path(OGDF_INCLUDE_DIR
    NAMES ogdf/decomposition/StaticSPQRTree.h
    HINTS
      ${OGDF_ROOT}
      $ENV{OGDF_ROOT}
    PATH_SUFFIXES include)

  find_library(OGDF_LIBRARY
    NAMES OGDF ogdf
    HINTS
      ${OGDF_ROOT}
      $ENV{OGDF_ROOT}
    PATH_SUFFIXES lib lib64 build)

  if(NOT OGDF_INCLUDE_DIR OR NOT OGDF_LIBRARY)
    message(FATAL_ERROR "USE_OGDF=ON but OGDF headers/libs were not found. Set OGDF_ROOT or OGDF_INCLUDE_DIR/OGDF_LIBRARY.")
  endif()

  include(cmake/DetectOGDFFeatures.cmake)
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ogdf_feature_config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ogdf_feature_config.hpp
  @ONLY
)

add_executable(rewrite_r_harness
  src/rewrite_r_harness_main.cpp
)

target_include_directories(rewrite_r_harness
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(USE_OGDF)
  target_include_directories(rewrite_r_harness PRIVATE ${OGDF_INCLUDE_DIR})
  target_link_libraries(rewrite_r_harness PRIVATE ${OGDF_LIBRARY})
  target_compile_definitions(rewrite_r_harness PRIVATE USE_OGDF=1)
endif()
