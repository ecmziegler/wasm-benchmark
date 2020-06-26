project(lzma_benchmark
  DESCRIPTION "lzma compression/decompression benchmark"
  LANGUAGES C)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

add_executable(lzma_bench lzma_bench.c)

add_custom_target(lzma
  COMMAND ${MAKE_COMMAND} WORKING_DIRECTORY "${THIRD_PARTY_DIR}/lzma")
add_custom_command(TARGET lzma POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E rename lzma.a "${PROJECT_BINARY_DIR}/liblzma.a"
  WORKING_DIRECTORY "${THIRD_PARTY_DIR}/lzma")
add_dependencies(lzma_bench lzma)
target_include_directories(lzma_bench PRIVATE "${THIRD_PARTY_DIR}/lzma")
target_link_libraries(lzma_bench PRIVATE "${PROJECT_BINARY_DIR}/liblzma.a")
