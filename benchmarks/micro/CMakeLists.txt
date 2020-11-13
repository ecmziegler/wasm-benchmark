project(micro_benchmark
  DESCRIPTION "micro benchmarks with relevant use cases"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.15)

include(../../CMakeLists.include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -std=c++11")

add_executable(string_bench string_bench.cpp)
if(PLATFORM STREQUAL "native")
  target_link_libraries(string_bench wasm_perf)
elseif(PLATFORM STREQUAL "wasm")
  target_compile_options(string_bench PRIVATE --js-library "${JS_LIBRARY}")
  target_link_options(string_bench PRIVATE --js-library "${JS_LIBRARY}")
endif()
