project(base64_benchmark
  DESCRIPTION "base64 encoding/decoding benchmark"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.15)

include(../../CMakeLists.include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -std=c++11")

add_executable(base64_bench base64_bench.cpp)
if(PLATFORM STREQUAL "native")
  target_link_libraries(base64_bench wasm_perf)
elseif(PLATFORM STREQUAL "wasm")
  target_compile_options(base64_bench PRIVATE -msimd128 --js-library "${JS_LIBRARY}")
  target_link_options(base64_bench PRIVATE -msimd128 --js-library "${JS_LIBRARY}")
endif()
