project(zlib_benchmark
  DESCRIPTION "zlib compression/decompression benchmark"
  LANGUAGES C)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

add_executable(zlib_bench zlib_bench.c)

add_subdirectory("${THIRD_PARTY_DIR}/zlib" zlib EXCLUDE_FROM_ALL)
target_include_directories(zlib_bench PRIVATE "${THIRD_PARTY_DIR}/zlib" "${PROJECT_BINARY_DIR}/zlib")
target_link_libraries(zlib_bench PRIVATE zlib)
