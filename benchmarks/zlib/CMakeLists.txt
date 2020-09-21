project(zlib_benchmark
  DESCRIPTION "zlib compression/decompression benchmark"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

set_source_files_properties(zlib_bench.c PROPERTIES LANGUAGE CXX)
add_executable(zlib_bench zlib_bench.c)

add_subdirectory("${THIRD_PARTY_DIR}/zlib" zlib EXCLUDE_FROM_ALL)
target_include_directories(zlib_bench PRIVATE "${THIRD_PARTY_DIR}/zlib" "${PROJECT_BINARY_DIR}/zlib")
if(PLATFORM STREQUAL "native")
  target_link_libraries(zlib_bench PRIVATE wasm_perf zlib)
elseif(PLATFORM STREQUAL "wasm")
  target_link_libraries(zlib_bench PRIVATE zlib)
  target_compile_options(zlib_bench PRIVATE --js-library "${JS_LIBRARY}")
  target_link_options(zlib_bench PRIVATE --js-library "${JS_LIBRARY}")
endif()
