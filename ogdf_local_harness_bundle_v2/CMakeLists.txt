cmake_minimum_required(VERSION 3.16)
project(rewrite_r_harness LANGUAGES CXX)

option(USE_OGDF "Enable OGDF-backed raw SPQR backend" OFF)
set(OGDF_ROOT "" CACHE PATH "OGDF install/build prefix")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DetectOGDFFeatures)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ogdf_feature_config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ogdf_feature_config.hpp
  @ONLY
)

add_library(rewrite_r_harness_core
  src/ogdf_wrapper.cpp
  src/dump.cpp
  src/runners.cpp
  src/project_hooks_stub.cpp
)

target_include_directories(rewrite_r_harness_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_compile_definitions(rewrite_r_harness_core
  PUBLIC
    $<$<BOOL:${USE_OGDF}>:USE_OGDF>
)

if(USE_OGDF)
  if(NOT OGDF_INCLUDE_DIRS OR NOT OGDF_LIBRARY_DIRS)
    message(FATAL_ERROR "USE_OGDF=ON but OGDF include/lib dirs were not detected. Set OGDF_ROOT.")
  endif()

  find_library(OGDF_LIB NAMES OGDF PATHS ${OGDF_LIBRARY_DIRS} NO_DEFAULT_PATH)
  find_library(COIN_LIB NAMES COIN PATHS ${OGDF_LIBRARY_DIRS} NO_DEFAULT_PATH)
  if(NOT OGDF_LIB OR NOT COIN_LIB)
    message(FATAL_ERROR "Could not find OGDF/COIN libraries under OGDF_ROOT. Check OGDF_ROOT or run scripts/install_ogdf_local.sh")
  endif()

  target_include_directories(rewrite_r_harness_core PUBLIC ${OGDF_INCLUDE_DIRS})
  target_link_directories(rewrite_r_harness_core PUBLIC ${OGDF_LIBRARY_DIRS})
  target_link_libraries(rewrite_r_harness_core PUBLIC ${OGDF_LIB} ${COIN_LIB})
endif()

add_executable(rewrite_r_harness src/rewrite_r_harness_main.cpp)
target_link_libraries(rewrite_r_harness PRIVATE rewrite_r_harness_core)

message(STATUS "USE_OGDF=${USE_OGDF}")
message(STATUS "OGDF_INCLUDE_DIRS=${OGDF_INCLUDE_DIRS}")
message(STATUS "OGDF_LIBRARY_DIRS=${OGDF_LIBRARY_DIRS}")
message(STATUS "OGDF_HAS_RANGE_GRAPH_ITER=${OGDF_HAS_RANGE_GRAPH_ITER}")
message(STATUS "OGDF_HAS_NODE_ADJENTRIES=${OGDF_HAS_NODE_ADJENTRIES}")
message(STATUS "OGDF_HAS_STATICSKELETON_TREEEDGE=${OGDF_HAS_STATICSKELETON_TREEEDGE}")
