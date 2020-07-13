project(wasm_benchmark_tools
  DESCRIPTION "Tools for benchmarking any Wasm engine or natively"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.15)

include(../CMakeLists.include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories(include)

if(PLATFORM STREQUAL "native")
  add_library(wasm_perf STATIC src/pipe_out.cc)
  add_executable(recorder src/native_recorder.cc src/benchmark.cc)
elseif(PLATFORM STREQUAL "wasm")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS=['ccall']")
  add_executable(recorder src/wasm_recorder.cc src/benchmark.cc)
endif()
