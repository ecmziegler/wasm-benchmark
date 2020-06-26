project(base64_benchmark
  DESCRIPTION "base64 encoding/decoding benchmark"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

add_executable(base64_bench base64_bench.cpp)
if(PLATFORM STREQUAL "native")
  target_link_libraries(base64_bench wasm_perf)
elseif(PLATFORM STREQUAL "wasm")
  target_compile_options(base64_bench PRIVATE --js-library "${JS_LIBRARY}")
  target_link_options(base64_bench PRIVATE --js-library "${JS_LIBRARY}")
endif()
