project(lzma_benchmark
  DESCRIPTION "lzma compression/decompression benchmark"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

set_source_files_properties(lzma_bench.c PROPERTIES LANGUAGE CXX)
add_executable(lzma_bench lzma_bench.c)

add_custom_target(lzma
  COMMAND ${MAKE_COMMAND} WORKING_DIRECTORY "${THIRD_PARTY_DIR}/lzma")
add_custom_command(TARGET lzma POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E rename lzma.a "${PROJECT_BINARY_DIR}/liblzma.a"
  COMMAND "${CMAKE_COMMAND}" -E remove *.o
  WORKING_DIRECTORY "${THIRD_PARTY_DIR}/lzma")
add_dependencies(lzma_bench lzma)
target_include_directories(lzma_bench PRIVATE "${THIRD_PARTY_DIR}/lzma")
if(PLATFORM STREQUAL "native")
  target_link_libraries(lzma_bench PRIVATE wasm_perf "${PROJECT_BINARY_DIR}/liblzma.a")
elseif(PLATFORM STREQUAL "wasm")
  target_link_libraries(lzma_bench PRIVATE "${PROJECT_BINARY_DIR}/liblzma.a")
  target_compile_options(lzma_bench PRIVATE --js-library "${JS_LIBRARY}")
  target_link_options(lzma_bench PRIVATE --js-library "${JS_LIBRARY}")
endif()
