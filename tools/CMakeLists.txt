project(wasm_benchmark_tools
  DESCRIPTION "Tools for benchmarking any Wasm engine or natively"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.16)

include(../CMakeLists.include)

include_directories(include)

if(PLATFORM STREQUAL "native")
  add_library(wasm_perf STATIC src/pipe_out.cc)
  add_executable(recorder src/native_recorder.cc src/benchmark.cc)
elseif(PLATFORM STREQUAL "wasm")
  add_executable(recorder src/wasm_recorder.cc src/benchmark.cc)
endif()
